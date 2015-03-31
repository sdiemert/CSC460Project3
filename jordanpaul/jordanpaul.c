#include <avr/io.h>
#include "rtos/os.h"
#include "rtos/timer.h"
#include "radio/radio.h"
#include "roomba/roomba.h"
#include "roomba/roomba_sci.h"
#include "roomba/roomba_led_sci.h"
#include "uart/uart.h"
#include "ir/ir.h"
#include "game.h"
#include "profiler.h"

SERVICE* radio_receive_service;
SERVICE* ir_receive_service;
uint8_t roomba_num = 1;
uint8_t ir_count = 0;

struct player_state {
    uint8_t player_id;
    uint8_t team;
    uint8_t state;
    uint8_t hit_flag;
    uint8_t last_ir_code;
};
struct player_state player;

void radio_rxhandler(uint8_t pipenumber) {
	PORTB ^= ( 1<< PB4);
	Service_Publish(radio_receive_service,0);
}

void ir_rxhandler() {
    PORTB ^= ( 1 << PB6);
    int16_t value = IR_getLast();

    for(int i = 0; i< 4; ++i){
        if( value == PLAYER_IDS[i]){
            if( value != PLAYER_IDS[roomba_num]){
                PORTB ^= ( 1 << PB6);
                Service_Publish(ir_receive_service,value);
            }
        }
    }

}

struct roomba_command {
	uint8_t opcode;
	uint8_t num_args;
	uint8_t args[32];
};

void SendCommandToRoomba(struct roomba_command* cmd){
	if (cmd->opcode == START ||
		cmd->opcode == BAUD ||
		cmd->opcode == SAFE ||
		cmd->opcode == FULL ||
		cmd->opcode == SENSORS)
	{
		return;
	}

	//Pass the command to the Roomba.
	Roomba_Send_Byte(cmd->opcode);
	for (int i = 0; i < cmd->num_args; i++){
		Roomba_Send_Byte(cmd->args[i]);
	}
}

void handleRoombaInput(pf_game_t* game)
{
	int16_t vx = (game->velocity_x/(255/9) - 4)*50;
	int16_t vy = (game->velocity_y/(255/9) - 4)*50;

	if( vy == 0){
		if( vx > 0){
			vx = 1;
		}else if( vx < 0){
			vx = -1;
		}
	}

	Roomba_Drive(vy,vx);

    // fire every 5th packet
    if( ir_count == 5){
        IR_transmit(player.player_id);
        ir_count = 0;
    }
    ir_count++;
}

void handleStateInput(pf_game_t* game){
    player.team = game->game_team;
    player.state = game->game_state;
    // do something about LED lights to show zombie or human
}

void send_back_packet()
{
	//Radio_Flush();
	radiopacket_t packet;

	packet.type = GAME;
	for(int i = 0;i < 5; ++i){
		packet.payload.game.sender_address[i] = ROOMBA_ADDRESSES[roomba_num][i];
	}

	packet.payload.game.game_player_id = player.player_id;
	packet.payload.game.game_team = player.team;
	packet.payload.game.game_state = player.state;
	packet.payload.game.game_hit_flag = (player.last_ir_code != 0) ? 1: 0;
	packet.payload.game.game_enemy_id = player.last_ir_code;

	PORTB ^= (1<<PB5);

    // reset the stuff
    player.hit_flag = 0;
    player.last_ir_code = 0;

    // Radio_Transmit(&packet, RADIO_WAIT_FOR_TX);
    Radio_Transmit(&packet, RADIO_RETURN_ON_TX);
}


void rr_roomba_controller() {
	//Start the Roomba for the first time.
	Roomba_Init();
	int16_t value;

	Radio_Set_Tx_Addr(base_station_address);

	for(;;) {
		Service_Subscribe(radio_receive_service,&value);
		//Handle the packets

		RADIO_RX_STATUS result;
		radiopacket_t packet;
		do {
			result = Radio_Receive(&packet);

			if(result == RADIO_RX_SUCCESS || result == RADIO_RX_MORE_PACKETS) {
				if( packet.type == GAME)
				{
					handleRoombaInput(&packet.payload.game);
					handleStateInput(&packet.payload.game);
				}
			}

		} while (result == RADIO_RX_MORE_PACKETS);

		send_back_packet();
	}
}

void rr_ir_controller()
{
    int16_t value;
    for(;;){
        Service_Subscribe(ir_receive_service,&value);
        player.last_ir_code = value;
    }
}

#define RADIO_VCC_DDR DDRL
#define RADIO_VCC_PORT PORTL
#define RADIO_VCC_PIN PL2
void power_cycle_radio()
{
	//Turn off radio power.
	RADIO_VCC_DDR |= (1 << RADIO_VCC_PIN);
	RADIO_VCC_PORT &= ~(1<<RADIO_VCC_PIN);
	_delay_ms(500);
	RADIO_VCC_PORT |= (1<<RADIO_VCC_PIN);
	_delay_ms(500);
}

int r_main(void)
{
	power_cycle_radio();

	DDRB |= (1<<PB4);
	DDRB |= (1<<PB5);
    DDRB |= (1<<PB6);
	PORTB &= ~(1<<PB4);
	PORTB &= ~(1<<PB5);
    PORTB &= ~(1<<PB6);

	//Initialize radio.
	Radio_Init();
	IR_init();
	Radio_Configure_Rx(RADIO_PIPE_0, ROOMBA_ADDRESSES[roomba_num], ENABLE);
	Radio_Configure(RADIO_1MBPS, RADIO_HIGHEST_POWER);

	radio_receive_service = Service_Init();
	ir_receive_service = Service_Init();
	Task_Create_RR(rr_roomba_controller,0);
    Task_Create_RR(rr_ir_controller,0);

    player.player_id = PLAYER_IDS[roomba_num];
    player.team = 0;
    player.state = 0;
    player.hit_flag = 0;
    player.last_ir_code = 0;

	Task_Terminate();
	return 0 ;
}