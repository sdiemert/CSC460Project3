/*
 * game.h
 *
 * Created: 2015-01-26 17:09:37
 *  Author: Scott and Simon
 */

#ifndef GAME_H_
#define GAME_H_

#include <avr/io.h>

extern uint8_t ROOMBA_ADDRESSES[4][5];
extern uint8_t ROOMBA_FREQUENCIES[4];
extern uint8_t PLAYER_IDS[4];

extern uint8_t base_station_address[5];// = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

typedef enum _roomba_nums {ZOMBIE=0, HUMAN=1} TEAM;
typedef enum _zombie_states {NORMAL=0, STUNNED} ZOMBIE_STATES;
typedef enum _human_states {SHIELDED=0, SHIELDLESS} HUMAN_STATES;


typedef struct _game_t{
    uint8_t sender_address[5];  /// The return address of the station sending this packet

    // roomba command
    uint8_t velocity_x;
    uint8_t velocity_y;
    uint8_t button;

    //roomba state
    uint8_t game_player_id; // the id of the roomba
    uint8_t game_team; // 0 for zombie, 1 for human
    uint8_t game_state; // 0 for normal, 1 for stunned
    uint8_t game_hit_flag; // 0 for not hit, 1 for hit
    uint8_t game_enemy_id; // id of the enemy which shot you, only valid when hit_flag is 1
} pf_game_t;


typedef struct _roomba_command_t {
    uint8_t opcode;
    uint8_t num_args;
    uint8_t args[32];
} roomba_command_t;

void Game_send_command_to_roomba(roomba_command_t* cmd);
int  Game_is_player_id(uint8_t player_id);

#endif /* GAME_H_ */
