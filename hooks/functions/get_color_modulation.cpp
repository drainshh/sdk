#include "../hooks.hpp"
#include "../../menu/config/config.hpp"
#include "../../sdk/structs/materials.hpp"
#include "../../features/visuals/visuals.hpp"
#include <algorithm>

void __fastcall sdk::hooks::get_color_modulation::get_color_modulation(i_material* material, void* edx, float* r, float* g, float* b) {
	if (!material || material->is_error_material() || !g::local || !interfaces::engine->is_connected() || !interfaces::engine->is_in_game()) {
		ofunc(material, edx, r, g, b);
		return;
	}

	ofunc(material, edx, r, g, b);

	const char* group = material->get_texture_group_name();
	if (!group)
		return;

	const bool is_world_texture = strstr(group, "World textures") != nullptr;
	const bool is_prop_texture = strstr(group, "StaticProp textures") != nullptr;
	const bool is_skybox_texture = strstr(group, "SkyBox textures") != nullptr;
	const bool is_world_visual =
		is_world_texture
		|| (c::visuals::world_modulate_props && is_prop_texture)
		|| (c::visuals::world_modulate_skybox && is_skybox_texture);

	if (is_world_visual && c::visuals::custom_shaders && c::visuals::world_modulate) {
		const auto safe_set_float = [&](const char* name, float value) {
			if (auto var = material->find_var(name, nullptr, false))
				var->set_float_value(value);
		};

		const auto safe_set_vec = [&](const char* name, float x, float y, float z) {
			if (auto var = material->find_var(name, nullptr, false))
				var->set_vec_value(x, y, z);
		};

		safe_set_float("$phong", 1.0f);
		safe_set_float("$phongboost", 0.01f);
		safe_set_float("$phongexponent", 0.01f);
		safe_set_float("$envmapfresnel", 0.01f);
		safe_set_vec("$phongfresnelranges", 0.01f, 0.01f, 0.01f);
		safe_set_vec("$envmapfresnelminmaxexp", 0.01f, 0.01f, 0.01f);
		safe_set_vec("$envmaptint", 0.01f, 0.01f, 0.01f);
		safe_set_vec("$selfillumtint", 0.01f, 0.01f, 0.01f);
	}

	// World modulate (123123: fixed multipliers per texture group + menu color)
	if (c::visuals::world_modulate && is_world_visual) {
		const float intensity = std::clamp(c::visuals::world_modulate_intensity, 0.0f, 2.0f);
		if (is_prop_texture) {
			*r *= (0.5f * intensity) * c::visuals::world_color[0];
			*g *= (0.5f * intensity) * c::visuals::world_color[1];
			*b *= (0.5f * intensity) * c::visuals::world_color[2];
		}
		else if (is_skybox_texture) {
			*r *= intensity * c::visuals::world_color[0];
			*g *= intensity * c::visuals::world_color[1];
			*b *= intensity * c::visuals::world_color[2];
		}
		else {
			*r *= (0.23f * intensity) * c::visuals::world_color[0];
			*g *= (0.23f * intensity) * c::visuals::world_color[1];
			*b *= (0.23f * intensity) * c::visuals::world_color[2];
		}
	}

	for (auto& entry : changed_materials) {
		if ((_stricmp(entry.name.c_str(), material->get_name()) == 0)
			|| (entry.group[0] && strstr(group, entry.group.c_str()))) {
			vec3_t col = parse_rgb(entry.value.c_str());
			*r = col.x;
			*g = col.y;
			*b = col.z;
			break;
		}
	}
}


float __fastcall sdk::hooks::get_alpha_modulation::get_alpha_modulation(i_material* material, void* edx) {
	if (!g::local || !interfaces::engine->is_connected() || !interfaces::engine->is_in_game())
		return ofunc(material, edx);

	if (!c::visuals::world_modulate)
		return ofunc(material, edx);

	if (!material || material->is_error_material())
		return ofunc(material, edx);

	float alpha = c::visuals::world_color[3];

	const auto group = material->get_texture_group_name();
	if (group && (strstr(group, ("World textures"))
		|| (c::visuals::world_modulate_props && strstr(group, ("StaticProp textures")))
		|| (c::visuals::world_modulate_skybox && strstr(group, ("SkyBox textures")))))
		return alpha;

	return ofunc(material, edx);
}
