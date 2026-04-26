#pragma once
#include "../../../sdk/sdk.hpp"

namespace prediction_backup {
	inline player_move_data data{};
	inline vec3_t		origin{};
	inline vec3_t		velocity{};
	inline vec3_t      angles{};
	inline c_usercmd* cmd{};
	inline float		forward_move{};
	inline float		side_move{};
	inline float		up_move{};
	inline float		curtime{};
	inline float		frametime{};
	inline bool		in_pred = false;
	inline bool		in_first_time = true;
	inline int		tickcount{};
	inline int       player_tickbase{};
	inline int		movetype{};
	inline int		flags{};
	inline int		buttons{};
	inline float	stamina{};
	inline float	max_speed{};
	inline float	sim_time{};
	inline int* prediction_random_seed{};
	inline int* prediction_player = nullptr;
}

namespace prediction {
	inline void backup_originals(c_usercmd* cmd) {
		//player						   
		prediction_backup::flags = g::local->flags();
		prediction_backup::velocity = g::local->velocity();
		prediction_backup::origin = g::local->origin();
		prediction_backup::player_tickbase = g::local->get_tick_base();
		prediction_backup::movetype = g::local->move_type();
		prediction_backup::stamina = g::local->stamina();
		prediction_backup::max_speed = g::local->max_speed();
		prediction_backup::sim_time = g::local->simulation_time();

		//cmd
		prediction_backup::cmd = cmd;
		prediction_backup::angles = cmd->view_angles;
		prediction_backup::side_move = cmd->side_move;
		prediction_backup::forward_move = cmd->forward_move;
		prediction_backup::up_move = cmd->up_move;
		prediction_backup::buttons = cmd->buttons;
	};

	void begin(c_usercmd* cmd);
	void end();
	void restore_ent_to_predicted_frame(int i);
	void repredict();
	void post_think();

	inline player_move_data data{};
	inline c_usercmd* last_command;
	inline int* prediction_random_seed;
	inline int* prediction_player;
	inline int correct_tickbase;
	inline float correct_simulation_time;
	inline bool in_first;
	inline bool in_pred;
	inline bool in_prediction;

	inline vec3_t origin;
	inline vec3_t velocity;
	inline vec3_t original_angle;
	inline int flags;
	inline int buttons;
	inline float out_going_packet;

	inline float old_cur_time;
	inline float old_frame_time;
	inline int old_tick_count;
}