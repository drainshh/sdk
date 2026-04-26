#include "pixelsurf_db.hpp"

#include "../movement/movement.hpp"
#include "../../menu/config/config.hpp"
#include "../../sdk/sdk.hpp"
#include "../../utils/render/draw.hpp"
#include "../../includes/json/json.h"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cstdlib>
#include <cmath>
#include <deque>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace features::visuals::pixelsurf_db {
	namespace {
		struct pixelsurf_db_entry_t {
			vec3_t pos{};
			vec3_t normal{};
			bool crouch = false;
			bool stand = true;
		};

		struct route_t {
			std::vector<vec3_t> points;
			pixelsurf_db_entry_t entry{};
		};

		std::deque<route_t> g_routes;
		std::vector<vec3_t> g_active_points;
		std::vector<pixelsurf_db_entry_t> g_hits;
		std::vector<vec3_t> g_mate_hits;
		bool g_was_pixelsurfing = false;
		bool g_active_pixelsurf_crouched = false;
		std::string g_loaded_map;
		bool g_has_loaded_for_map = false;

		constexpr std::size_t k_min_points_to_store = 4;
		constexpr float k_entry_merge_xy_dist = 32.f;
		constexpr float k_entry_merge_z_dist = 10.f;
		constexpr float k_entry_normal_dot = 0.8f;
		constexpr float k_wall_trace_distance = 96.f;
		constexpr const char* k_db_folder = "C:/dna";
		constexpr const char* k_db_file = "C:/dna/pxdatabase.px";
		constexpr const char* k_mate_db_file = "C:/dna/csgomate_pixelsurfs.json";

		void merge_hit(std::vector<pixelsurf_db_entry_t>& hits, const pixelsurf_db_entry_t& entry);

		bool world_to_screen(const vec3_t& in, vec3_t& out) {
			return interfaces::debug_overlay->world_to_screen(in, out);
		}

		std::string normalize_map_token(std::string s) {
			while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' '))
				s.pop_back();
			const char prefix[] = "maps/";
			if (s.size() >= sizeof(prefix) - 1) {
				bool is_prefix = true;
				for (size_t i = 0; i < sizeof(prefix) - 1; ++i) {
					if (std::tolower(static_cast<unsigned char>(s[i])) != static_cast<unsigned char>(prefix[i])) {
						is_prefix = false;
						break;
					}
				}
				if (is_prefix)
					s = s.substr(sizeof(prefix) - 1);
			}
			if (s.size() > 4 && s.compare(s.size() - 4, 4, ".bsp") == 0)
				s = s.substr(0, s.size() - 4);
			return s;
		}

		std::string get_map_name() {
			if (!interfaces::engine || !interfaces::engine->is_in_game())
				return {};
			const char* level_name = interfaces::engine->get_level_name();
			return normalize_map_token(level_name ? std::string(level_name) : std::string{});
		}

		// Skip leading samples from fast vertical drop (pre-ledge fall) so the line starts on the surf.
		std::size_t surf_draw_start_index(const std::vector<vec3_t>& pts) {
			if (pts.size() < 2)
				return 0;
			constexpr float k_steep_fall_dz = -11.f;
			std::size_t i = 0;
			while (i + 1 < pts.size()) {
				const float dz = pts[i + 1].z - pts[i].z;
				if (dz > k_steep_fall_dz)
					break;
				++i;
			}
			if (i + 1 >= pts.size())
				return 0;
			return i;
		}

		float origin_dist_sq(const vec3_t& p) {
			const vec3_t o = g::local->origin();
			const float dx = p.x - o.x;
			const float dy = p.y - o.y;
			const float dz = p.z - o.z;
			return dx * dx + dy * dy + dz * dz;
		}

		float dist_sq_vec(const vec3_t& p, const vec3_t& q) {
			const float dx = p.x - q.x;
			const float dy = p.y - q.y;
			const float dz = p.z - q.z;
			return dx * dx + dy * dy + dz * dz;
		}

		float dist_sq_xy(const vec3_t& p, const vec3_t& q) {
			const float dx = p.x - q.x;
			const float dy = p.y - q.y;
			return dx * dx + dy * dy;
		}

		float dot_vec(const vec3_t& a, const vec3_t& b) {
			return a.x * b.x + a.y * b.y + a.z * b.z;
		}

		float length_sq_vec(const vec3_t& v) {
			return dot_vec(v, v);
		}

		vec3_t normalize_vec(const vec3_t& v) {
			const float len_sq = length_sq_vec(v);
			if (len_sq <= 0.0001f)
				return {};
			const float inv_len = 1.0f / std::sqrt(len_sq);
			return vec3_t(v.x * inv_len, v.y * inv_len, v.z * inv_len);
		}

		vec3_t average_point_range(const std::vector<vec3_t>& pts, const std::size_t a, const std::size_t e) {
			vec3_t avg{};
			if (pts.empty())
				return avg;

			const std::size_t end = (std::min)(e, pts.size() - 1);
			int count = 0;
			for (std::size_t i = a; i <= end; ++i) {
				avg.x += pts[i].x;
				avg.y += pts[i].y;
				avg.z += pts[i].z;
				++count;
			}

			if (count <= 0)
				return {};

			const float inv = 1.0f / static_cast<float>(count);
			return vec3_t(avg.x * inv, avg.y * inv, avg.z * inv);
		}

		bool entry_has_normal(const pixelsurf_db_entry_t& entry) {
			return length_sq_vec(entry.normal) > 0.01f;
		}

		void normalize_entry_normal(pixelsurf_db_entry_t& entry) {
			if (!entry_has_normal(entry))
				return;
			entry.normal = normalize_vec(entry.normal);
		}

		bool entries_overlap(const pixelsurf_db_entry_t& lhs, const pixelsurf_db_entry_t& rhs) {
			if (dist_sq_xy(lhs.pos, rhs.pos) > k_entry_merge_xy_dist * k_entry_merge_xy_dist)
				return false;
			if (std::fabs(lhs.pos.z - rhs.pos.z) > k_entry_merge_z_dist)
				return false;
			if (entry_has_normal(lhs) && entry_has_normal(rhs) && dot_vec(lhs.normal, rhs.normal) < k_entry_normal_dot)
				return false;
			return true;
		}

		void merge_entry_into(pixelsurf_db_entry_t& dst, const pixelsurf_db_entry_t& src) {
			dst.pos.x = (dst.pos.x + src.pos.x) * 0.5f;
			dst.pos.y = (dst.pos.y + src.pos.y) * 0.5f;
			dst.pos.z = (dst.pos.z + src.pos.z) * 0.5f;
			dst.crouch = dst.crouch || src.crouch;
			dst.stand = dst.stand || src.stand;

			if (entry_has_normal(src)) {
				if (entry_has_normal(dst))
					dst.normal = normalize_vec(vec3_t(dst.normal.x + src.normal.x, dst.normal.y + src.normal.y, dst.normal.z + src.normal.z));
				else
					dst.normal = src.normal;
			}
		}

		bool scan_pixelsurf_wall(const vec3_t& player_pos, vec3_t& out_normal, vec3_t& out_position) {
			trace_world_only filter;
			constexpr float k_two_pi = 6.28318530718f;
			constexpr int k_sample_count = 32;
			const float angle_step = k_two_pi / static_cast<float>(k_sample_count);

			float closest_wall_dist = FLT_MAX;
			bool found_wall = false;

			for (int sample = 0; sample < k_sample_count; ++sample) {
				const float angle = angle_step * static_cast<float>(sample);
				const vec3_t direction(std::cos(angle), std::sin(angle), 0.f);
				const vec3_t end_pos = player_pos + direction * k_wall_trace_distance;

				ray_t ray;
				ray.initialize(player_pos, end_pos);

				trace_t trace;
				interfaces::trace_ray->trace_ray(ray, MASK_PLAYERSOLID, &filter, &trace);

				if (trace.flFraction >= 1.f || std::fabs(trace.plane.normal.z) >= 0.1f)
					continue;

				const vec3_t delta(
					trace.end.x - player_pos.x,
					trace.end.y - player_pos.y,
					trace.end.z - player_pos.z);
				const float wall_dist = length_sq_vec(delta);
				if (wall_dist >= closest_wall_dist)
					continue;

				closest_wall_dist = wall_dist;
				out_normal = trace.plane.normal;
				out_position = trace.end;
				found_wall = true;
			}

			if (found_wall)
				out_normal = normalize_vec(out_normal);

			return found_wall;
		}

		// Drop trailing samples: falling off / popping up, Z drift off ledge, or sharp plan-view kink (exit wiggle).
		std::size_t surf_draw_end_index(const std::vector<vec3_t>& pts, std::size_t a, std::size_t b) {
			std::size_t e = b;
			if (e <= a + 1)
				return e;

			float z_ref = 0.f;
			int zc = 0;
			const std::size_t z_until = (std::min)(a + 10u, b);
			for (std::size_t i = a; i <= z_until; ++i) {
				z_ref += pts[i].z;
				++zc;
			}
			if (zc > 0)
				z_ref /= static_cast<float>(zc);

			constexpr float k_z_drift = 11.f;
			constexpr float k_dz_fall = -9.f;
			constexpr float k_dz_rise = 7.f;

			while (e > a + 1) {
				const float dz = pts[e].z - pts[e - 1].z;
				if (dz < k_dz_fall || dz > k_dz_rise) {
					--e;
					continue;
				}
				if (std::fabs(pts[e].z - z_ref) > k_z_drift) {
					--e;
					continue;
				}
				if (e >= a + 2) {
					const float dx = pts[e].x - pts[e - 1].x;
					const float dy = pts[e].y - pts[e - 1].y;
					const float dx0 = pts[e - 1].x - pts[e - 2].x;
					const float dy0 = pts[e - 1].y - pts[e - 2].y;
					const float len = std::sqrt(dx * dx + dy * dy);
					const float len0 = std::sqrt(dx0 * dx0 + dy0 * dy0);
					if (len > 0.4f && len0 > 0.4f) {
						const float dot = (dx * dx0 + dy * dy0) / (len * len0);
						if (dot < 0.4f) {
							--e;
							continue;
						}
					}
				}
				break;
			}
			return e;
		}

		bool trimmed_segment(const std::vector<vec3_t>& points, std::size_t& out_a, std::size_t& out_e) {
			if (points.size() < 2)
				return false;
			const std::size_t raw_b = points.size() - 1;
			out_a = surf_draw_start_index(points);
			if (out_a >= raw_b)
				out_a = 0;
			if (out_a >= raw_b)
				return false;
			out_e = surf_draw_end_index(points, out_a, raw_b);
			if (out_e <= out_a)
				return false;
			return true;
		}

		vec3_t segment_mid_world(const std::vector<vec3_t>& pts, std::size_t a, std::size_t e) {
			vec3_t m{};
			m.x = (pts[a].x + pts[e].x) * 0.5f;
			m.y = (pts[a].y + pts[e].y) * 0.5f;
			m.z = (pts[a].z + pts[e].z) * 0.5f;
			return m;
		}

		bool build_clean_route(const std::vector<vec3_t>& raw_points, const bool crouched, route_t& out_route) {
			std::size_t a = 0;
			std::size_t e = 0;
			if (!trimmed_segment(raw_points, a, e))
				return false;

			const std::size_t sample_count = e - a + 1;
			if (sample_count < 2)
				return false;

			const std::size_t edge_window = (std::min<std::size_t>)(3u, sample_count);
			const vec3_t start_anchor = average_point_range(raw_points, a, a + edge_window - 1);
			const vec3_t end_anchor = average_point_range(raw_points, e - edge_window + 1, e);
			vec3_t direction(end_anchor.x - start_anchor.x, end_anchor.y - start_anchor.y, 0.f);
			float line_len = std::sqrt(direction.x * direction.x + direction.y * direction.y);

			if (line_len < 0.001f) {
				direction = vec3_t(
					raw_points[e].x - raw_points[a].x,
					raw_points[e].y - raw_points[a].y,
					0.f);
				line_len = std::sqrt(direction.x * direction.x + direction.y * direction.y);
			}

			if (line_len < 0.001f)
				return false;

			const vec3_t dir_norm(direction.x / line_len, direction.y / line_len, 0.f);
			float flat_z = 0.f;
			for (std::size_t i = a; i <= e; ++i)
				flat_z += raw_points[i].z;
			flat_z /= static_cast<float>(sample_count);

			std::vector<vec3_t> cleaned_points;
			cleaned_points.reserve(sample_count);

			const float min_step = (std::max)(4.0f, c::visuals::pixelsurf_db_point_spacing * 0.6f);
			float last_t = -FLT_MAX;
			for (std::size_t i = a; i <= e; ++i) {
				const vec3_t delta(
					raw_points[i].x - start_anchor.x,
					raw_points[i].y - start_anchor.y,
					0.f);
				float t = delta.x * dir_norm.x + delta.y * dir_norm.y;
				t = std::clamp(t, 0.0f, line_len);
				if (t < last_t)
					t = last_t;

				vec3_t projected(
					start_anchor.x + dir_norm.x * t,
					start_anchor.y + dir_norm.y * t,
					flat_z);

				if (cleaned_points.empty() || cleaned_points.back().distance_to(projected) >= min_step) {
					cleaned_points.push_back(projected);
					last_t = t;
				}
			}

			const vec3_t clean_start(start_anchor.x, start_anchor.y, flat_z);
			const vec3_t clean_end(
				start_anchor.x + dir_norm.x * line_len,
				start_anchor.y + dir_norm.y * line_len,
				flat_z);

			if (cleaned_points.empty() || cleaned_points.front().distance_to(clean_start) > 1.0f)
				cleaned_points.insert(cleaned_points.begin(), clean_start);
			if (cleaned_points.back().distance_to(clean_end) > 1.0f)
				cleaned_points.push_back(clean_end);

			if (cleaned_points.size() < 2)
				return false;

			out_route.points = std::move(cleaned_points);
			out_route.entry.pos = segment_mid_world(out_route.points, 0, out_route.points.size() - 1);
			out_route.entry.pos.z = flat_z;
			out_route.entry.crouch = crouched;
			out_route.entry.stand = !crouched;

			vec3_t wall_normal{};
			vec3_t wall_position{};
			if (scan_pixelsurf_wall(out_route.entry.pos, wall_normal, wall_position))
				out_route.entry.normal = wall_normal;

			normalize_entry_normal(out_route.entry);
			return true;
		}

		void rebuild_hits_from_routes() {
			g_hits.clear();
			for (const auto& route : g_routes)
				merge_hit(g_hits, route.entry);
		}

		int find_matching_route_index(const pixelsurf_db_entry_t& entry) {
			for (int i = 0; i < static_cast<int>(g_routes.size()); ++i) {
				if (entries_overlap(g_routes[i].entry, entry))
					return i;
			}
			return -1;
		}

		void insert_or_merge_route(route_t route) {
			const int existing_index = find_matching_route_index(route.entry);
			if (existing_index >= 0) {
				route_t& existing = g_routes[existing_index];
				merge_entry_into(existing.entry, route.entry);
				if (route.points.size() > existing.points.size())
					existing.points = std::move(route.points);
			}
			else {
				g_routes.push_front(std::move(route));
				while (g_routes.size() > static_cast<std::size_t>(c::visuals::pixelsurf_db_max_paths))
					g_routes.pop_back();
			}

			rebuild_hits_from_routes();
		}

		void draw_world_polyline(const std::vector<vec3_t>& pts, std::size_t a, std::size_t e_last, const color_t& col, float thick) {
			for (std::size_t i = a; i < e_last; ++i) {
				vec3_t s0{};
				vec3_t s1{};
				if (world_to_screen(pts[i], s0) && world_to_screen(pts[i + 1], s1))
					im_render.drawline(s0.x, s0.y, s1.x, s1.y, col, thick);
			}
		}

		void draw_mate_world_segment(const vec3_t& p, const color_t& col, float thick) {
			constexpr float k_half = 20.f;
			const vec3_t q0{ p.x + k_half, p.y, p.z };
			const vec3_t q1{ p.x - k_half, p.y, p.z };
			vec3_t s0{};
			vec3_t s1{};
			if (world_to_screen(q0, s0) && world_to_screen(q1, s1))
				im_render.drawline(s0.x, s0.y, s1.x, s1.y, col, thick);
		}

		void merge_hit(std::vector<pixelsurf_db_entry_t>& hits, const pixelsurf_db_entry_t& entry) {
			for (auto& hit : hits) {
				if (!entries_overlap(hit, entry))
					continue;
				merge_entry_into(hit, entry);
				return;
			}
			hits.push_back(entry);
		}

		float json_num_or(const Json::Value& v, const char* key, float fallback) {
			if (!v.isObject() || !v.isMember(key))
				return fallback;
			const auto& m = v[key];
			if (m.isNumeric())
				return m.asFloat();
			if (m.isString())
				return static_cast<float>(std::atof(m.asString().c_str()));
			return fallback;
		}

		void write_entry_to_json(Json::Value& out, const pixelsurf_db_entry_t& entry) {
			out["x"] = entry.pos.x;
			out["y"] = entry.pos.y;
			out["z"] = entry.pos.z;
			out["normal"]["x"] = entry.normal.x;
			out["normal"]["y"] = entry.normal.y;
			out["normal"]["z"] = entry.normal.z;
			out["crouch"] = entry.crouch;
			out["stand"] = entry.stand;
		}

		pixelsurf_db_entry_t read_entry_from_json(const Json::Value& value) {
			pixelsurf_db_entry_t entry{};
			if (value.isObject()) {
				entry.pos.x = json_num_or(value, "x", json_num_or(value, "X", 0.f));
				entry.pos.y = json_num_or(value, "y", json_num_or(value, "Y", 0.f));
				entry.pos.z = json_num_or(value, "z", json_num_or(value, "Z", 0.f));
				if (value.isMember("normal") && value["normal"].isObject()) {
					entry.normal.x = json_num_or(value["normal"], "x", 0.f);
					entry.normal.y = json_num_or(value["normal"], "y", 0.f);
					entry.normal.z = json_num_or(value["normal"], "z", 0.f);
				}
				entry.crouch = value.get("crouch", false).asBool();
				entry.stand = value.get("stand", true).asBool();
			}

			normalize_entry_normal(entry);
			return entry;
		}

		void append_point_from_json(const Json::Value& v, std::vector<vec3_t>& out) {
			if (v.isArray() && v.size() >= 3 && v[static_cast<Json::ArrayIndex>(0)].isNumeric()) {
				out.emplace_back(
					v[Json::ArrayIndex(0)].asFloat(),
					v[Json::ArrayIndex(1)].asFloat(),
					v[Json::ArrayIndex(2)].asFloat());
				return;
			}
			if (!v.isObject())
				return;
			const float x = json_num_or(v, "x", json_num_or(v, "X", 0.f));
			const float y = json_num_or(v, "y", json_num_or(v, "Y", 0.f));
			const float z = json_num_or(v, "z", json_num_or(v, "Z", 0.f));
			out.emplace_back(x, y, z);
		}

		void parse_json_points_array(const Json::Value& arr, std::vector<vec3_t>& out) {
			if (!arr.isArray())
				return;
			for (Json::ArrayIndex i = 0; i < arr.size(); ++i)
				append_point_from_json(arr[i], out);
		}

		void try_append_points_for_map_key(const Json::Value& root, const std::string& map_name, std::vector<vec3_t>& out) {
			if (!root.isObject())
				return;
			if (root.isMember(map_name) && root[map_name].isArray()) {
				parse_json_points_array(root[map_name], out);
				return;
			}
			if (root.isMember("maps") && root["maps"].isObject() && root["maps"].isMember(map_name))
				parse_json_points_array(root["maps"][map_name], out);
		}

		void load_mate_database(const std::string& map_name) {
			g_mate_hits.clear();
			if (!c::visuals::pixelsurf_db_load_csgomate || map_name.empty())
				return;

			std::ifstream in(k_mate_db_file);
			if (!in.good())
				return;

			Json::Value root;
			in >> root;

			if (root.isArray()) {
				for (Json::ArrayIndex i = 0; i < root.size(); ++i) {
					const auto& row = root[i];
					if (!row.isObject())
						continue;
					std::string row_map;
					if (row.isMember("map"))
						row_map = normalize_map_token(row["map"].asString());
					else if (row.isMember("Map"))
						row_map = normalize_map_token(row["Map"].asString());
					if (!row_map.empty() && row_map != map_name)
						continue;
					if (row.isMember("position") || row.isMember("Position")) {
						append_point_from_json(row.isMember("position") ? row["position"] : row["Position"], g_mate_hits);
						continue;
					}
					append_point_from_json(row, g_mate_hits);
				}
				return;
			}

			if (!root.isObject())
				return;

			std::string file_map_raw;
			if (root.isMember("map"))
				file_map_raw = root["map"].asString();
			else if (root.isMember("Map"))
				file_map_raw = root["Map"].asString();
			const std::string file_map = normalize_map_token(file_map_raw);
			if (!file_map.empty() && file_map != map_name) {
				try_append_points_for_map_key(root, map_name, g_mate_hits);
				return;
			}

			static const char* keys[] = { "points", "Points", "surfs", "Surfs", "pixelsurfs", "PixelSurfs", "hits", "Hits" };
			for (const char* k : keys) {
				if (root.isMember(k) && root[k].isArray()) {
					parse_json_points_array(root[k], g_mate_hits);
					break;
				}
			}

			if (g_mate_hits.empty())
				try_append_points_for_map_key(root, map_name, g_mate_hits);
		}

		void save_database() {
			std::error_code ec;
			std::filesystem::create_directories(k_db_folder, ec);

			Json::Value root;
			root["map"] = g_loaded_map;
			root["routes_count"] = static_cast<int>(g_routes.size());
			root["hits_count"] = static_cast<int>(g_hits.size());

			int route_index = 0;
			for (const auto& route : g_routes) {
				Json::Value route_json;
				route_json["points_count"] = static_cast<int>(route.points.size());

				for (int i = 0; i < static_cast<int>(route.points.size()); ++i) {
					route_json["points"][i]["x"] = route.points[i].x;
					route_json["points"][i]["y"] = route.points[i].y;
					route_json["points"][i]["z"] = route.points[i].z;
				}
				write_entry_to_json(route_json["entry"], route.entry);

				root["routes"][route_index++] = route_json;
			}

			for (int i = 0; i < static_cast<int>(g_hits.size()); ++i) {
				write_entry_to_json(root["hits"][i], g_hits[i]);
			}

			std::ofstream out(k_db_file, std::ios::out | std::ios::trunc);
			if (!out.good())
				return;

			out << root;
		}

		void migrate_hits_from_routes_if_needed() {
			if (!g_hits.empty() || g_routes.empty())
				return;
			for (const auto& route : g_routes)
				merge_hit(g_hits, route.entry);
		}

		void load_database_for_map(const std::string& map_name) {
			g_routes.clear();
			g_active_points.clear();
			g_was_pixelsurfing = false;
			g_active_pixelsurf_crouched = false;
			g_hits.clear();

			std::ifstream in(k_db_file);
			if (!in.good()) {
				g_loaded_map = map_name;
				g_has_loaded_for_map = true;
				load_mate_database(map_name);
				return;
			}

			Json::Value root;
			in >> root;

			if (!root.isObject()) {
				g_loaded_map = map_name;
				g_has_loaded_for_map = true;
				load_mate_database(map_name);
				return;
			}

			std::string dna_map_raw;
			if (root.isMember("map"))
				dna_map_raw = root["map"].asString();
			const std::string file_map = normalize_map_token(dna_map_raw);
			if (!file_map.empty() && file_map != map_name) {
				g_loaded_map = map_name;
				g_has_loaded_for_map = true;
				load_mate_database(map_name);
				return;
			}

			const int route_count = root.get("routes_count", 0).asInt();
			for (int r = 0; r < route_count; ++r) {
				const auto& route_json = root["routes"][r];
				const int points_count = route_json.get("points_count", 0).asInt();
				route_t route;
				route.points.reserve(points_count);

				for (int p = 0; p < points_count; ++p) {
					vec3_t point{};
					point.x = route_json["points"][p].get("x", 0.f).asFloat();
					point.y = route_json["points"][p].get("y", 0.f).asFloat();
					point.z = route_json["points"][p].get("z", 0.f).asFloat();
					route.points.push_back(point);
				}

				if (route_json.isMember("entry"))
					route.entry = read_entry_from_json(route_json["entry"]);
				else if (!route.points.empty()) {
					route.entry.pos = segment_mid_world(route.points, 0, route.points.size() - 1);
					route.entry.stand = true;
				}

				if (route.points.size() >= 2)
					g_routes.push_back(route);
			}

			if (root.isMember("hits") && root["hits"].isArray()) {
				const auto& ha = root["hits"];
				for (Json::ArrayIndex h = 0; h < ha.size(); ++h) {
					merge_hit(g_hits, read_entry_from_json(ha[h]));
				}
			}

			migrate_hits_from_routes_if_needed();

			g_loaded_map = map_name;
			g_has_loaded_for_map = true;
			load_mate_database(map_name);
		}
	}

	void clear() {
		g_routes.clear();
		g_active_points.clear();
		g_hits.clear();
		g_was_pixelsurfing = false;
		g_active_pixelsurf_crouched = false;
		save_database();
	}

	void on_create_move() {
		if (!interfaces::engine->is_in_game() || !interfaces::engine->is_connected() || !g::local || !g::local->is_alive()) {
			g_routes.clear();
			g_active_points.clear();
			g_was_pixelsurfing = false;
			g_active_pixelsurf_crouched = false;
			g_loaded_map.clear();
			g_has_loaded_for_map = false;
			g_hits.clear();
			g_mate_hits.clear();
			return;
		}

		const std::string current_map = get_map_name();
		if (!g_has_loaded_for_map || current_map != g_loaded_map) {
			load_database_for_map(current_map);
		}

		const bool is_pixelsurfing =
			features::movement::should_ps || features::movement::ps_data.pixelsurfing;
		const vec3_t current_origin = g::local->origin();

		if (is_pixelsurfing) {
			if (!g_was_pixelsurfing) {
				g_active_points.clear();
				g_active_points.push_back(current_origin);
				g_active_pixelsurf_crouched = features::movement::ps_data.pixelducking;
			}
			else if (g_active_points.empty() || g_active_points.back().distance_to(current_origin) >= c::visuals::pixelsurf_db_point_spacing) {
				g_active_points.push_back(current_origin);
			}
			g_active_pixelsurf_crouched = g_active_pixelsurf_crouched || features::movement::ps_data.pixelducking;
		}
		else if (g_was_pixelsurfing) {
			if (g_active_points.size() >= k_min_points_to_store) {
				route_t finished;
				if (build_clean_route(g_active_points, g_active_pixelsurf_crouched, finished)) {
					insert_or_merge_route(std::move(finished));
					save_database();
					if (c::movehelp::push_notice && interfaces::chat_element)
						interfaces::chat_element->chatprintf("#dna#_print_ps_db_found");
				}
			}

			g_active_points.clear();
			g_active_pixelsurf_crouched = false;
		}

		g_was_pixelsurfing = is_pixelsurfing;
	}

	void on_paint_traverse() {
		if (!c::visuals::pixelsurf_db_enable || !interfaces::engine->is_in_game() || !g::local || !g::local->is_alive())
			return;

		const color_t route_color(
			static_cast<int>(c::visuals::pixelsurf_db_clr[0] * 255.f),
			static_cast<int>(c::visuals::pixelsurf_db_clr[1] * 255.f),
			static_cast<int>(c::visuals::pixelsurf_db_clr[2] * 255.f),
			static_cast<int>(c::visuals::pixelsurf_db_clr[3] * 255.f)
		);

		if (c::visuals::pixelsurf_db_draw_path) {
			struct route_entry {
				const std::vector<vec3_t>* pts;
				std::size_t a = 0;
				std::size_t e = 0;
				vec3_t mid{};
				float dist_sq = 0.f;
			};

			std::vector<route_entry> routes_sorted;
			routes_sorted.reserve(g_routes.size());
			for (const auto& route : g_routes) {
				std::size_t a = 0;
				std::size_t e = 0;
				if (!trimmed_segment(route.points, a, e))
					continue;
				route_entry entry{};
				entry.pts = &route.points;
				entry.a = a;
				entry.e = e;
				entry.mid = segment_mid_world(route.points, a, e);
				entry.dist_sq = origin_dist_sq(entry.mid);
				routes_sorted.push_back(entry);
			}

			std::sort(routes_sorted.begin(), routes_sorted.end(), [](const route_entry& x, const route_entry& y) {
				return x.dist_sq < y.dist_sq;
			});

			constexpr float k_world_cluster_r = 68.f;
			const float k_world_cluster_sq = k_world_cluster_r * k_world_cluster_r;
			std::vector<vec3_t> accepted_mids;
			accepted_mids.reserve(routes_sorted.size());

			for (const auto& e : routes_sorted) {
				bool crowded = false;
				for (const auto& m : accepted_mids) {
					if (dist_sq_vec(e.mid, m) < k_world_cluster_sq) {
						crowded = true;
						break;
					}
				}
				if (crowded)
					continue;
				accepted_mids.push_back(e.mid);
				draw_world_polyline(*e.pts, e.a, e.e, route_color, c::visuals::pixelsurf_db_line_thickness);
			}

			if (c::visuals::pixelsurf_db_draw_active) {
				std::size_t a = 0;
				std::size_t e = 0;
				if (trimmed_segment(g_active_points, a, e))
					draw_world_polyline(g_active_points, a, e, route_color, c::visuals::pixelsurf_db_line_thickness);
			}

			std::vector<std::pair<vec3_t, float>> mate_pts;
			mate_pts.reserve(g_mate_hits.size());
			for (const auto& h : g_mate_hits)
				mate_pts.emplace_back(h, origin_dist_sq(h));
			std::sort(mate_pts.begin(), mate_pts.end(), [](const std::pair<vec3_t, float>& x, const std::pair<vec3_t, float>& y) {
				return x.second < y.second;
			});

			std::vector<vec3_t> accepted_mate;
			accepted_mate.reserve(mate_pts.size());
			for (const auto& mp : mate_pts) {
				bool crowded = false;
				for (const auto& m : accepted_mate) {
					if (dist_sq_vec(mp.first, m) < k_world_cluster_sq) {
						crowded = true;
						break;
					}
				}
				if (crowded)
					continue;
				accepted_mate.push_back(mp.first);
				draw_mate_world_segment(mp.first, route_color, c::visuals::pixelsurf_db_line_thickness);
			}
		}
	}
}
