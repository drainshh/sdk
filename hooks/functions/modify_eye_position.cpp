#include "../hooks.hpp"
#include "../../menu/config/config.hpp"
#include <cmath>
#include <iostream>

// helper to smooth any float value using critically damped spring
static float smooth_damp(float current, float target, float& velocity, float smooth_time, float delta_time) {
    const float omega = 2.0f / smooth_time;
    const float x = omega * delta_time;
    const float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);

    float change = current - target;
    float temp = (velocity + omega * change) * delta_time;
    velocity = (velocity - omega * temp) * exp;

    return target + (change + temp) * exp;
}

void __fastcall sdk::hooks::modify_eye_position::modify_eye_position(void* this_ptr, void* edx, vec3_t& eye_pos) {
    const auto animstate = static_cast<c_anim_state*>(this_ptr);
    if (!animstate || !animstate->m_player || !animstate->m_player->is_alive())
        return ofunc(this_ptr, edx, eye_pos);

    const auto player = animstate->m_player;

    const float frametime = std::max<float>(interfaces::globals->frame_time, 0.0001f);

    // ============================
    // Smooth eye position
    // ============================
    const int head_bone = player->lookup_bone("head_0");
    if (head_bone != -1) {
        vec3_t head_pos = player->get_hitbox_position(head_bone);
        head_pos.z += 1.7f;

        if (eye_pos.z > head_pos.z) {
            float delta = eye_pos.z - head_pos.z;

            float blend = (delta - 4.0f) * (1.0f / 6.0f);
            blend = std::clamp(blend, 0.0f, 1.0f);

            float eased = blend * blend * (3.0f - 2.0f * blend);
            float target_z = std::lerp(eye_pos.z, head_pos.z, eased);

            static float smoothed_eye = target_z;
            static float eye_vel = 0.0f;

            smoothed_eye = smooth_damp(smoothed_eye, target_z, eye_vel, 0.12f, frametime);
            eye_pos.z = smoothed_eye;
        }
    }

    // ============================
    // Smooth duck amount
    // ============================
    {
        static float smoothed_duck = animstate->m_duck_amount;
        static float duck_vel = 0.0f;

        smoothed_duck = smooth_damp(smoothed_duck, animstate->m_duck_amount, duck_vel, 0.08f, frametime);
        animstate->m_duck_amount = smoothed_duck;
    }

    // ============================
    // Smooth camera height (landing dips / transitions)
    // ============================
    {
        static float smoothed_height = animstate->m_camera_smooth_height;
        static float height_vel = 0.0f;

        if (std::isfinite(animstate->m_camera_smooth_height)) {
            smoothed_height = smooth_damp(smoothed_height, animstate->m_camera_smooth_height, height_vel, 0.15f, frametime);
            animstate->m_camera_smooth_height = smoothed_height;
        }
    }
}
