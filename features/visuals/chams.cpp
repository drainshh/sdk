#include "../visuals/visuals.hpp"
#include "../../menu/config/config.hpp"
#include "../../menu/menu.hpp"
#include "../../sdk/math/math.hpp"
#include "../../features/aimbot/aimbot.hpp"
#include "../../hooks/hooks.hpp"

i_material* create_material(std::string name, std::string type, std::string material) {
	static auto key_values_system = interfaces::key_values_system;
	if (!key_values_system) return nullptr;

	using alloc_key_values_memory_fn = void* (__thiscall*)(void*, int);
	static auto alloc_key_values_memory = (*reinterpret_cast<alloc_key_values_memory_fn**>(key_values_system))[2];

	auto kv = alloc_key_values_memory(key_values_system, 128); // Using 128 to be safe
	if (!kv) return nullptr;

	auto kv_ptr = static_cast<c_key_values*>(kv);
	kv_ptr->init(type.c_str());
	kv_ptr->load_from_buffer(name.c_str(), material.c_str());
	return interfaces::material_system->create_material(name.c_str(), kv_ptr);
}

// Material definitions
namespace materials {
    // Base materials
    i_material* mat_regular = nullptr;
    i_material* mat_flat = nullptr;

    // Custom materials
    i_material* mat_glow = nullptr;
    i_material* mat_chrome = nullptr;
    i_material* mat_plastic = nullptr;
    i_material* mat_glass = nullptr;
    i_material* mat_crystal = nullptr;
    i_material* mat_animated = nullptr;
    i_material* mat_bubble = nullptr;
    i_material* mat_metallic = nullptr;

    // Overlay materials
    i_material* mat_overlay_glow = nullptr;
    i_material* mat_overlay_chrome = nullptr;
    i_material* mat_overlay_plastic = nullptr;
    i_material* mat_overlay_glass = nullptr;
    i_material* mat_overlay_crystal = nullptr;
    i_material* mat_overlay_animated = nullptr;
    i_material* mat_overlay_bubble = nullptr;
    i_material* mat_overlay_metallic = nullptr;
}

enum material_type {
    MATERIAL_REGULAR = 0,
    MATERIAL_FLAT,
    MATERIAL_GLOW,
    MATERIAL_CHROME,
    MATERIAL_PLASTIC,
    MATERIAL_GLASS,
    MATERIAL_CRYSTAL,
    MATERIAL_ANIMATED,
    MATERIAL_BUBBLE,
    MATERIAL_METALLIC
};

void override_material(bool ignorez, bool wireframe, int type, color_t clr) {
    i_material* material = nullptr;

    switch (type) {
    case MATERIAL_REGULAR: material = materials::mat_regular; break;
    case MATERIAL_FLAT: material = materials::mat_flat; break;
    case MATERIAL_GLOW: material = materials::mat_glow; break;
    case MATERIAL_CHROME: material = materials::mat_chrome; break;
    case MATERIAL_PLASTIC: material = materials::mat_plastic; break;
    case MATERIAL_GLASS: material = materials::mat_glass; break;
    case MATERIAL_CRYSTAL: material = materials::mat_crystal; break;
    case MATERIAL_ANIMATED: material = materials::mat_animated; break;
    case MATERIAL_BUBBLE: material = materials::mat_bubble; break;
    case MATERIAL_METALLIC: material = materials::mat_metallic; break;
    }

    if (!material)
        return;

    material->set_material_var_flag(material_var_ignorez, ignorez);
    material->set_material_var_flag(material_var_wireframe, wireframe);

    auto overlay = material->find_var("$envmaptint", nullptr);
    if (overlay)
        overlay->set_vec_value(clr.r(), clr.g(), clr.b());

    material->color_modulate(clr.r(), clr.g(), clr.b());
    material->alpha_modulate(clr.a());

    interfaces::model_render->override_material(material);
}

void override_overlay_material(bool ignorez, bool wireframe, int type, color_t clr) {
    i_material* material = nullptr;

    switch (type) {
    case MATERIAL_REGULAR: material = materials::mat_regular; break;
    case MATERIAL_FLAT: material = materials::mat_flat; break;
    case MATERIAL_GLOW: material = materials::mat_glow; break;
    case MATERIAL_CHROME: material = materials::mat_chrome; break;
    case MATERIAL_PLASTIC: material = materials::mat_plastic; break;
    case MATERIAL_GLASS: material = materials::mat_glass; break;
    case MATERIAL_CRYSTAL: material = materials::mat_crystal; break;
    case MATERIAL_ANIMATED: material = materials::mat_animated; break;
    case MATERIAL_BUBBLE: material = materials::mat_bubble; break;
    case MATERIAL_METALLIC: material = materials::mat_metallic; break;
    }

    if (!material)
        return;

    material->set_material_var_flag(material_var_ignorez, ignorez);
    material->set_material_var_flag(material_var_wireframe, wireframe);

    // Set overlay tint
    auto overlay = material->find_var("$envmaptint", nullptr);
    if (overlay)
        overlay->set_vec_value(clr.r(), clr.g(), clr.b());

    material->color_modulate(clr.r(), clr.g(), clr.b());
    material->alpha_modulate(clr.a());

    interfaces::model_render->override_material(material);
}

void features::chams::initialize() {
    // Base materials (keep original)
    materials::mat_regular = interfaces::material_system->find_material("debug/debugambientcube", TEXTURE_GROUP_MODEL);
    materials::mat_regular->increment_reference_count();

    materials::mat_flat = interfaces::material_system->find_material("debug/debugdrawflat", TEXTURE_GROUP_MODEL);
    materials::mat_flat->increment_reference_count();

    // Glow material
    materials::mat_glow = create_material("glow.vmt", "VertexLitGeneric", R"#("VertexLitGeneric" {
        "$additive" "1"
        "$envmap" "models/effects/cube_white"
        "$envmaptint" "[1 1 1]"
        "$envmapfresnel" "1"
        "$alpha" "0.8"
    })#");

    materials::mat_overlay_glow = create_material("glow_overlay.vmt", "VertexLitGeneric", R"#("VertexLitGeneric" {
        "$additive" "1"
        "$envmap" "models/effects/cube_white"
        "$envmaptint" "[1 1 1]"
        "$envmapfresnel" "1"
        "$alpha" "0.8"
    })#");

    // Chrome material
    materials::mat_chrome = create_material("chrome.vmt", "VertexLitGeneric", R"#("VertexLitGeneric" {
        "$basetexture" "vgui/white_additive"
        "$envmap" "env_cubemap"
        "$envmaptint" "[1 1 1]"
        "$normalmapalphaenvmapmask" "1"
        "$envmapcontrast" "1"
        "$model" "1"
        "$phong" "1"
        "$phongboost" "20"
        "$phongexponent" "3000"
        "$phongfresnelranges" "[0.5 0.5 1]"
    })#");

    materials::mat_overlay_chrome = create_material("chrome_overlay.vmt", "VertexLitGeneric", R"#("VertexLitGeneric" {
        "$basetexture" "vgui/white_additive"
        "$envmap" "env_cubemap"
        "$envmaptint" "[1 1 1]"
        "$normalmapalphaenvmapmask" "1"
        "$envmapcontrast" "1"
        "$model" "1"
        "$phong" "1"
        "$phongboost" "20"
        "$phongexponent" "3000"
        "$phongfresnelranges" "[0.5 0.5 1]"
    })#");

    // Plastic material
    materials::mat_plastic = create_material("plastic.vmt", "VertexLitGeneric", R"#("VertexLitGeneric" {
        "$basetexture" "vgui/white_additive"
        "$model" "1"
        "$envmap" "env_cubemap"
        "$envmaptint" "[1 1 1]"
        "$phong" "1"
        "$phongboost" "2"
        "$phongexponent" "10"
        "$phongfresnelranges" "[0.5 1.5 3]"
        "$rimlight" "1"
        "$rimlightexponent" "2"
        "$rimlightboost" "1"
    })#");

    materials::mat_overlay_plastic = create_material("plastic_overlay.vmt", "VertexLitGeneric", R"#("VertexLitGeneric" {
        "$basetexture" "vgui/white_additive"
        "$model" "1"
        "$envmap" "env_cubemap"
        "$envmaptint" "[1 1 1]"
        "$phong" "1"
        "$phongboost" "2"
        "$phongexponent" "10"
        "$phongfresnelranges" "[0.5 1.5 3]"
        "$rimlight" "1"
        "$rimlightexponent" "2"
        "$rimlightboost" "1"
    })#");

    // Glass material
    materials::mat_glass = create_material("glass.vmt", "VertexLitGeneric", R"#("VertexLitGeneric" {
        "$basetexture" "vgui/white_additive"
        "$envmap" "env_cubemap"
        "$envmaptint" "[0.6 0.6 0.6]"
        "$envmapfresnel" "1"
        "$envmapfresnelminmaxexp" "[0 1 2]"
        "$alpha" "0.4"
        "$model" "1"
        "$nocull" "1"
        "$translucent" "1"
        "$phong" "1"
        "$phongboost" "0.5"
        "$phongexponent" "2048"
        "$phongfresnelranges" "[0.5 0.5 1]"
    })#");

    materials::mat_overlay_glass = create_material("glass_overlay.vmt", "VertexLitGeneric", R"#("VertexLitGeneric" {
        "$basetexture" "vgui/white_additive"
        "$envmap" "env_cubemap"
        "$envmaptint" "[0.6 0.6 0.6]"
        "$envmapfresnel" "1"
        "$envmapfresnelminmaxexp" "[0 1 2]"
        "$alpha" "0.4"
        "$model" "1"
        "$nocull" "1"
        "$translucent" "1"
        "$phong" "1"
        "$phongboost" "0.5"
        "$phongexponent" "2048"
        "$phongfresnelranges" "[0.5 0.5 1]"
    })#");

    // Crystal material
    materials::mat_crystal = create_material("crystal.vmt", "VertexLitGeneric", R"#("VertexLitGeneric" {
        "$basetexture" "vgui/white_additive"
        "$bumpmap" "de_nuke/hr_nuke/pool_water_normals_002"
        "$bumptransform" "center 0.5 0.5 scale 0.5 0.5 rotate 0 translate 0 0"
        "$envmap" "env_cubemap"
        "$envmaptint" "[0.2 0.2 0.2]"
        "$envmapfresnel" "1"
        "$envmapfresnelminmaxexp" "[0 1 2]"
        "$model" "1"
        "$nocull" "1"
        "$translucent" "1"
        "$alpha" "0.3"
        "$phong" "1"
        "$phongboost" "10"
        "$phongexponent" "5000"
        "$phongfresnelranges" "[0.5 0.5 1]"
        "$rimlight" "1"
        "$rimlightexponent" "2"
        "$rimlightboost" "2"
        "Proxies" {
            "TextureScroll" {
                "textureScrollVar" "$bumptransform"
                "textureScrollRate" "0.05"
                "textureScrollAngle" "90"
            }
        }
    })#");

    materials::mat_overlay_crystal = create_material("crystal_overlay.vmt", "VertexLitGeneric", R"#("VertexLitGeneric" {
        "$basetexture" "vgui/white_additive"
        "$bumpmap" "de_nuke/hr_nuke/pool_water_normals_002"
        "$bumptransform" "center 0.5 0.5 scale 0.5 0.5 rotate 0 translate 0 0"
        "$envmap" "env_cubemap"
        "$envmaptint" "[0.2 0.2 0.2]"
        "$envmapfresnel" "1"
        "$envmapfresnelminmaxexp" "[0 1 2]"
        "$model" "1"
        "$nocull" "1"
        "$translucent" "1"
        "$alpha" "0.3"
        "$phong" "1"
        "$phongboost" "10"
        "$phongexponent" "5000"
        "$phongfresnelranges" "[0.5 0.5 1]"
        "$rimlight" "1"
        "$rimlightexponent" "2"
        "$rimlightboost" "2"
        "Proxies" {
            "TextureScroll" {
                "textureScrollVar" "$bumptransform"
                "textureScrollRate" "0.05"
                "textureScrollAngle" "90"
            }
        }
    })#");

    // Animated material (scrolling texture)
    materials::mat_animated = create_material("animated.vmt", "VertexLitGeneric", R"#("VertexLitGeneric" {
        "$basetexture" "dev/zone_warning"
        "$model" "1"
        "$phong" "1"
        "$phongboost" "5"
        "$phongexponent" "500"
        "$phongfresnelranges" "[0.5 1 2]"
        "$rimlight" "1"
        "$rimlightexponent" "5"
        "$rimlightboost" "1"
        "Proxies" {
            "TextureScroll" {
                "texturescrollvar" "$baseTextureTransform"
                "texturescrollrate" "0.2"
                "texturescrollangle" "45"
            }
        }
    })#");

    materials::mat_overlay_animated = create_material("animated_overlay.vmt", "VertexLitGeneric", R"#("VertexLitGeneric" {
        "$basetexture" "dev/zone_warning"
        "$additive" "1"
        "$envmap" "env_cubemap"
        "$envmaptint" "[1 1 1]"
        "$model" "1"
        "$phong" "1"
        "$phongboost" "5"
        "$phongexponent" "500"
        "$phongfresnelranges" "[0.5 1 2]"
        "$rimlight" "1"
        "$rimlightexponent" "5"
        "$rimlightboost" "1"
        "Proxies" {
            "TextureScroll" {
                "texturescrollvar" "$baseTextureTransform"
                "texturescrollrate" "0.2"
                "texturescrollangle" "45"
            }
        }
    })#");

    // Bubble material
    materials::mat_bubble = create_material("bubble.vmt", "VertexLitGeneric", R"#("VertexLitGeneric" {
        "$basetexture" "vgui/white_additive"
        "$bumpmap" "models/player/shared/shared_normal"
        "$envmap" "env_cubemap"
        "$envmaptint" "[0.5 0.5 0.5]"
        "$envmapfresnel" "1"
        "$envmapfresnelminmaxexp" "[0 0.5 2]"
        "$phong" "1"
        "$phongboost" "15"
        "$phongexponent" "8000"
        "$phongfresnelranges" "[0.25 0.5 1]"
        "$rimlight" "1"
        "$rimlightexponent" "3"
        "$rimlightboost" "3"
        "$model" "1"
        "$translucent" "1"
        "$alpha" "0.5"
    })#");

    materials::mat_overlay_bubble = create_material("bubble_overlay.vmt", "VertexLitGeneric", R"#("VertexLitGeneric" {
        "$basetexture" "vgui/white_additive"
        "$bumpmap" "models/player/shared/shared_normal"
        "$envmap" "env_cubemap"
        "$envmaptint" "[0.5 0.5 0.5]"
        "$envmapfresnel" "1"
        "$envmapfresnelminmaxexp" "[0 0.5 2]"
        "$phong" "1"
        "$phongboost" "15"
        "$phongexponent" "8000"
        "$phongfresnelranges" "[0.25 0.5 1]"
        "$rimlight" "1"
        "$rimlightexponent" "3"
        "$rimlightboost" "3"
        "$model" "1"
        "$translucent" "1"
        "$alpha" "0.5"
    })#");

    // Metallic material
    materials::mat_metallic = create_material("metallic.vmt", "VertexLitGeneric", R"#("VertexLitGeneric" {
        "$basetexture" "vgui/white_additive"
        "$envmap" "env_cubemap"
        "$envmaptint" "[0.8 0.8 0.8]"
        "$envmapfresnel" "0.7"
        "$model" "1"
        "$phong" "1"
        "$phongboost" "8"
        "$phongexponent" "1024"
        "$phongfresnelranges" "[0.7 0.8 1]"
        "$rimlight" "1"
        "$rimlightexponent" "4"
        "$rimlightboost" "2"
        "$bumpmap" "models/player/shared/shared_normal"
    })#");

    materials::mat_overlay_metallic = create_material("metallic_overlay.vmt", "VertexLitGeneric", R"#("VertexLitGeneric" {
        "$basetexture" "vgui/white_additive"
        "$envmap" "env_cubemap"
        "$envmaptint" "[0.8 0.8 0.8]"
        "$envmapfresnel" "0.7"
        "$additive" "1"
        "$model" "1"
        "$phong" "1"
        "$phongboost" "8"
        "$phongexponent" "1024"
        "$phongfresnelranges" "[0.7 0.8 1]"
        "$rimlight" "1"
        "$rimlightexponent" "4"
        "$rimlightboost" "2"
        "$bumpmap" "models/player/shared/shared_normal"
    })#");
}

void add_dlight_for_entity(int index, const vec3_t& origin, const color_t& clr) {
	if (!interfaces::effects)
		return;

	dlight_t* dlight = interfaces::effects->cl_alloc_dlight(index);
	if (!dlight)
		return;

	dlight->origin = origin;
	dlight->direction = origin; // optional: use view angles if needed

	dlight->radius = 100.0f;
	dlight->die_time = interfaces::globals->cur_time + 0.1f;
	dlight->decay = dlight->radius / 2.0f;
	dlight->key = index;

	dlight->color.r = static_cast<int>(clr.r() * 255.0f);
	dlight->color.g = static_cast<int>(clr.g() * 255.0f);
	dlight->color.b = static_cast<int>(clr.b() * 255.0f);
	dlight->color.exponent = 8;
}

void features::chams::run(i_mat_render_context* ctx, const draw_model_state_t& state, const model_render_info_t& info, matrix_t* matrix) {
	if (!c::visuals::enable_chams || !g::local || !interfaces::engine->is_connected() || !interfaces::engine->is_in_game())
		return;

	const model_t* model = info.model;
	bool is_nuke = strstr(model->name, ("nuke_cargo_elevator_arms")) != nullptr;
	bool is_arm = strstr(model->name, ("arms")) != nullptr && !is_nuke;
	bool is_weapon = strstr(model->name, ("weapons/v_")) != nullptr && !is_arm;
	bool is_player = strstr(model->name, ("models/player")) != nullptr;
	bool is_sleeve = strstr(model->name, ("sleeve")) != nullptr;
	auto ent = interfaces::ent_list->get_client_entity(info.entity_index);

	auto material = interfaces::material_system->find_material(info.model->name, TEXTURE_GROUP_MODEL);

	if (!material)
	{
		return;
	}

	if (is_player) {
		player_t* player = reinterpret_cast<player_t*>(ent);

		if (!player || !player->is_alive() || player->dormant() || !g::local || player == g::local || player->team() == g::local->team())
			return;

		if (c::chams::invisible_chams) {
			color_t invisible_clr = color_t(c::chams::invisible_chams_clr[0], c::chams::invisible_chams_clr[1], c::chams::invisible_chams_clr[2], c::chams::invisible_chams_clr[3]);
			if (menu::experimental_femboy_theme_active)
				invisible_clr = color_t(255, 105, 180, 150);
			else if (menu::experimental_fight_club_mode)
				invisible_clr = color_t(80, 0, 0, 150);

			override_material(true, c::chams::invisible_wireframe, c::chams::cham_type, invisible_clr);
			sdk::hooks::draw_model_execute::draw_model_execute_original(interfaces::model_render, 0, ctx, state, info, matrix);
		}
		if (c::chams::visible_chams&& c::visuals::cham_lights && player) {
			vec3_t origin = player->origin(); // Make sure origin() returns a vec3_t
			color_t dlight_color = color_t(
				c::chams::visible_chams_clr[0],
				c::chams::visible_chams_clr[1],
				c::chams::visible_chams_clr[2],
				c::chams::visible_chams_clr[3]
			);

			add_dlight_for_entity(player->index(), origin, dlight_color);
		}
		interfaces::model_render->override_material(nullptr);

		if (c::chams::visible_chams && c::visuals::remove_original_model) {
			material->set_material_var_flag(material_var_no_draw, true);
			interfaces::model_render->override_material(material);
		}

		if (c::chams::visible_chams) {
			sdk::hooks::draw_model_execute::draw_model_execute_original(interfaces::model_render, 0, ctx, state, info, matrix);

			color_t visible_clr = color_t(c::chams::visible_chams_clr[0], c::chams::visible_chams_clr[1], c::chams::visible_chams_clr[2], c::chams::visible_chams_clr[3]);
			if (menu::experimental_femboy_theme_active)
				visible_clr = color_t(255, 182, 193, 200);
			else if (menu::experimental_fight_club_mode)
				visible_clr = color_t(150, 0, 0, 200);

			override_material(false, false, c::chams::cham_type, visible_clr);
		}
		if (c::chams::visible_chams_ov) {
			color_t overlay_clr = color_t(c::chams::visible_chams_clr_ov[0], c::chams::visible_chams_clr_ov[1], c::chams::visible_chams_clr_ov[2], c::chams::visible_chams_clr_ov[3]);
			if (menu::experimental_femboy_theme_active)
				overlay_clr = color_t(255, 255, 255, 100);

			override_overlay_material(false, c::chams::visible_wireframe_ov, c::chams::cham_type_overlay, overlay_clr);
			sdk::hooks::draw_model_execute::draw_model_execute_original(interfaces::model_render, 0, ctx, state, info, matrix);
		}
	}
}

void override_material_bt(bool ignorez, bool wireframe, int type, color_t clr) {
	i_material* material = nullptr;

    switch (type) {
    case MATERIAL_REGULAR: material = materials::mat_regular; break;
    case MATERIAL_FLAT: material = materials::mat_flat; break;
    case MATERIAL_GLOW: material = materials::mat_glow; break;
    case MATERIAL_CHROME: material = materials::mat_chrome; break;
    case MATERIAL_PLASTIC: material = materials::mat_plastic; break;
    case MATERIAL_GLASS: material = materials::mat_glass; break;
    case MATERIAL_CRYSTAL: material = materials::mat_crystal; break;
    case MATERIAL_ANIMATED: material = materials::mat_animated; break;
    case MATERIAL_BUBBLE: material = materials::mat_bubble; break;
    case MATERIAL_METALLIC: material = materials::mat_metallic; break;
    }

	material->set_material_var_flag(material_var_ignorez, ignorez);
	material->set_material_var_flag(material_var_wireframe, wireframe);

    auto overlay = material->find_var("$envmaptint", nullptr);
    if (overlay)
        overlay->set_vec_value(clr.r(), clr.g(), clr.b());

	material->color_modulate(clr.r(), clr.g(), clr.b());
	material->alpha_modulate(clr.a());
	interfaces::model_render->override_material(material);
}

void features::chams::run_bt(i_mat_render_context* ctx, const draw_model_state_t& state, const model_render_info_t& info, matrix_t* matrix) {
	if (!c::visuals::enable_backtrack_chams || !g::local || !interfaces::engine->is_connected() || !interfaces::engine->is_in_game())
		return;

	const auto mdl = info.model;

	if (!mdl)
		return;

	bool is_player = strstr(mdl->name, "models/player") != nullptr;

	if (is_player) {
		player_t* player = reinterpret_cast<player_t*>(interfaces::ent_list->get_client_entity(info.entity_index));
		if (!player || !player->is_alive() || player->dormant())
			return;

		if (player->team() != g::local->team()) {
			if (c::chams::backtrack_chams && c::backtrack::backtrack && player->is_moving() && g::local->is_alive()) {
				auto record = &backtrack.data[player->index()];
				if (c::chams::backtrack_chams_draw_all_ticks) {
					for (int i = 0; i < record->size(); i++) {
						if (record && !record->empty() && record->size() && (record->at(i).sim_time)) {
							float h, s, v, a;
							h = (math::rgbtoh(c::chams::backtrack_chams_clr2[0], c::chams::backtrack_chams_clr2[1], c::chams::backtrack_chams_clr2[2]) - math::rgbtoh(c::chams::backtrack_chams_clr1[0], c::chams::backtrack_chams_clr1[1], c::chams::backtrack_chams_clr1[2])) / backtrack.data[player->index()].size();
							s = (math::rgbtos(c::chams::backtrack_chams_clr2[0], c::chams::backtrack_chams_clr2[1], c::chams::backtrack_chams_clr2[2]) - math::rgbtos(c::chams::backtrack_chams_clr1[0], c::chams::backtrack_chams_clr1[1], c::chams::backtrack_chams_clr1[2])) / backtrack.data[player->index()].size();
							v = (math::rgbtov(c::chams::backtrack_chams_clr2[0], c::chams::backtrack_chams_clr2[1], c::chams::backtrack_chams_clr2[2]) - math::rgbtov(c::chams::backtrack_chams_clr1[0], c::chams::backtrack_chams_clr1[1], c::chams::backtrack_chams_clr1[2])) / backtrack.data[player->index()].size();
							a = (c::chams::backtrack_chams_clr2[3] - c::chams::backtrack_chams_clr1[3]) / backtrack.data[player->index()].size();

							float hh, ss, vv;
							hh = math::rgbtoh(c::chams::backtrack_chams_clr1[0], c::chams::backtrack_chams_clr1[1], c::chams::backtrack_chams_clr1[2]) + (h * i);
							ss = math::rgbtos(c::chams::backtrack_chams_clr1[0], c::chams::backtrack_chams_clr1[1], c::chams::backtrack_chams_clr1[2]) + (s * i);
							vv = math::rgbtov(c::chams::backtrack_chams_clr1[0], c::chams::backtrack_chams_clr1[1], c::chams::backtrack_chams_clr1[2]) + (v * i);
							float doneclr[3] = { math::fromhsvtor(hh, ss, vv), math::fromhsvtog(hh, ss, vv), math::fromhsvtob(hh, ss, vv) };

							color_t gradient = color_t(doneclr[0], doneclr[1], doneclr[2], c::chams::backtrack_chams_clr1[3] + (a * i));
							color_t non_gradient = color_t(c::chams::backtrack_chams_clr2[0], c::chams::backtrack_chams_clr2[1], c::chams::backtrack_chams_clr2[2], c::chams::backtrack_chams_clr2[3]);

							override_material_bt(c::chams::backtrack_chams_invisible, false, c::chams::cham_type_bt, c::chams::backtrack_chams_gradient ? gradient : non_gradient);

							sdk::hooks::draw_model_execute::draw_model_execute_original(interfaces::model_render, 0, ctx, state, info, backtrack.data[player->index()][i].m_matrix);
							if (c::visuals::backtrack_cham_lights && player) {
								vec3_t origin = player->origin(); // Make sure origin() returns a vec3_t
								color_t dlight_color = color_t(
									c::chams::visible_chams_clr[0],
									c::chams::visible_chams_clr[1],
									c::chams::visible_chams_clr[2],
									c::chams::visible_chams_clr[3]
								);

								add_dlight_for_entity(player->index(), origin, dlight_color);
							}
							interfaces::model_render->override_material(nullptr);
						}
					}
				}
				else {
					if (record && !record->empty() && record->size() >= 2) {
						// Lerp target time: current time - interpolation delay
						const float interpolation_time = g::local->get_tick_base() * interfaces::globals->interval_per_tick / 2;

						bt_data::backtrack_data* older = nullptr;
						bt_data::backtrack_data* newer = nullptr;

						// Find two records to interpolate between
						for (int i = 0; i < record->size() - 1; i++) {
							const auto& current = record->at(i);
							const auto& next = record->at(i + 1);

							if (current.sim_time <= interpolation_time && next.sim_time >= interpolation_time) {
								older = &record->at(i);
								newer = &record->at(i + 1);
								break;
							}
						}

						// If we found valid pair
						if (older && newer) {
							// Calculate interpolation factor
							const float total = newer->sim_time - older->sim_time;
							const float frac = total > 0.0f ? (interpolation_time - older->sim_time) / total : 0.0f;

							matrix_t interpolated[MAXSTUDIOBONES];
							for (int i = 0; i < MAXSTUDIOBONES; i++) {
								for (int j = 0; j < 3; j++) {
									interpolated[i].mat_val[j][0] = std::lerp(older->m_matrix[i].mat_val[j][0], newer->m_matrix[i].mat_val[j][0], frac);
									interpolated[i].mat_val[j][1] = std::lerp(older->m_matrix[i].mat_val[j][1], newer->m_matrix[i].mat_val[j][1], frac);
									interpolated[i].mat_val[j][2] = std::lerp(older->m_matrix[i].mat_val[j][2], newer->m_matrix[i].mat_val[j][2], frac);
									interpolated[i].mat_val[j][3] = std::lerp(older->m_matrix[i].mat_val[j][3], newer->m_matrix[i].mat_val[j][3], frac);
								}
							}

							override_material_bt(
								c::chams::backtrack_chams_invisible,
								false,
								c::chams::cham_type_bt,
								color_t(
									c::chams::backtrack_chams_clr2[0],
									c::chams::backtrack_chams_clr2[1],
									c::chams::backtrack_chams_clr2[2],
									c::chams::backtrack_chams_clr2[3]
								)
							);

							sdk::hooks::draw_model_execute::draw_model_execute_original(
								interfaces::model_render, 0, ctx, state, info, interpolated
							);
							interfaces::model_render->override_material(nullptr);
						}
					}
				}
			}
		}
	}
}