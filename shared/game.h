/*
 * cops_and_robbers.h
 *
 * Created: 2015-01-26 17:09:37
 *  Author: Daniel
 */


#ifndef _GAME_H_
#define _GAME_H_

#include "avr/io.h"

typedef enum _roomba_nums {PLAYER0=0,PLAYER1,PLAYER2,PLAYER3} PLAYERS;
extern uint8_t ROOMBA_ADDRESSES[][5];
extern uint8_t ROOMBA_FREQUENCIES[];

typedef struct _game_t{
    uint8_t sender_address[5];  /// The return address of the station sending this packet

    uint8_t vx;
    uint8_t vy;
    uint8_t button;

    // if not zero send out
    uint8_t ir_fire;
    uint8_t ir_data;

    //roomba state
    uint8_t game_player_id; // the id of the roomba
    uint8_t game_team; // 0 for zombie, 1 for human
    uint8_t game_state; // 0 for normal, 1 for stunned
    uint8_t game_hit_flag; // 0 for not hit, 1 for hit
    uint8_t game_enemy_id; // id of the enemy which shot you, only valid when hit_flag is 1
} pf_game_t;


void Game_set_packet(pf_game_t* game);
void Game_fire_ir(pf_game_t* game, uint8_t code);
void Game_sender_addr(pf_game_t* game, uint8_t player_num);
void Game_drive(pf_game_t* game,int16_t velocity, int16_t radius);
void Game_player_state(pf_game_t* game, uint8_t player_id,
        uint8_t team,uint8_t state, uint8_t hit_flag, uint8_t enemy_id);

#endif /* _GAME_H_ */
