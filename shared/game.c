/*
 * cops_and_robbers.h
 *
 * Created: 2015-01-26 17:09:37
 *  Author: Daniel
 */

#include "game.h"
#include "avr/io.h"
#include "roomba/roomba_sci.h"

uint8_t ROOMBA_ADDRESSES[4][5] = {
	{0xAA,0xAA,0xAA,0xAA,0xAA},
	{0xBB,0xBB,0xBB,0xBB,0xBB},
	{0xCC,0xCC,0xCC,0xCC,0xCC},
	{0xDD,0xDD,0xDD,0xDD,0xDD}
};
uint8_t ROOMBA_FREQUENCIES [4] = {104, 106, 108, 110};


// each roomba on a differnent freq
// base station

void Game_fire_ir(pf_game_t* game, uint8_t code){
    game->ir_fire = 1;
    game->ir_data = code;
}

void Game_sender_addr(pf_game_t* game, uint8_t player_num){
    for(int i = 0; i < 5; ++i){
        game->sender_address[i] = ROOMBA_ADDRESSES[player_num][i];
    }
}

// void Game_drive(pf_game_t* game,int16_t velocity, int16_t radius){
//     game->roomba_command = DRIVE;
//     game->roomba_num_arg_bytes = 4;

//     game->roomba_args[0] = HIGH_BYTE(velocity);
//     game->roomba_args[1] = LOW_BYTE(velocity);
//     game->roomba_args[2] = HIGH_BYTE(radius);
//     game->roomba_args[3] = LOW_BYTE(radius);
// }

void Game_player_state(pf_game_t* game, uint8_t player_id,
    uint8_t team,uint8_t state, uint8_t hit_flag, uint8_t enemy_id)
{
    game->game_player_id = player_id;
    game->game_team = team;
    game->game_state = state;
    game->game_hit_flag = hit_flag;
    game->game_enemy_id = enemy_id;
}

