#if 1

#include <avr/io.h>
#include <util/delay.h>
#include "rtos/os.h"
#include "rtos/kernel.h"
#include "radio/radio.h"
#include "game.h"
//#include "joystick.h"


radiopacket_t tx_packet;
pf_game_t game_packet;

uint8_t basestation_address[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

void radio_rxhandler(uint8_t);

//a struct to hold all the information for a player
typedef struct{
    uint8_t id;
    uint8_t joystick_port; //the port the joystick is located on.
    TEAM team;
    uint8_t status;
} roomba_t;

roomba_t roombas[4];

void radio_rxhandler(uint8_t val){
	PORTB ^= ( 1<< PB6);
	Radio_Flush();
}

void sendPacket(uint8_t id, uint8_t x, uint8_t y, uint8_t b){

    Radio_Set_Tx_Addr(ROOMBA_ADDRESSES[id]);

    game_packet.velocity_x = x;
    game_packet.velocity_y = y;
    game_packet.button = b;
    game_packet.game_player_id = id;
    game_packet.game_team = (uint8_t)roombas[id].team;
    game_packet.game_state = (uint8_t)roombas[id].status;

    memcpy(tx_packet.payload.game.sender_address, basestation_address, RADIO_ADDRESS_LENGTH);
    tx_packet.payload.game = game_packet;

    uint8_t status= Radio_Transmit(&tx_packet, RADIO_WAIT_FOR_TX);
    if( status == RADIO_TX_MAX_RT){
    	Radio_Flush();
    }
    PORTB ^= 1 << 7;
}

void updateRoomba(){
    uint8_t current_roomba;
    uint8_t joystick_x = 0;
    uint8_t joystick_y = 0;
    uint8_t button = 0;

    while(1) {
        for(current_roomba = 0; current_roomba < 4; current_roomba ++) {

    	// for(current_roomba = 0; current_roomba < 2; current_roomba ++) {
            //joystick_x = read_analog(roombas[current_roomba].joystick_port);
            //joystick_y = read_analog(roombas[current_roomba].joystick_port+1);


            // joystick_x = read_analog(1);
            // joystick_y = read_analog(0);

            // TODO Button Press

            // TODO Update State

            sendPacket(current_roomba, joystick_x, joystick_y, button);
            // sendPacket(1, joystick_x, joystick_y, button);
            //PORTB ^= 1 << 7;


            Task_Next();
        }
    }
}

void init_game() {
    // Setup Roombas
    roombas[0].id = 0;
    roombas[0].joystick_port = 0;
    roombas[0].team = ZOMBIE;
    roombas[0].status = (uint8_t)NORMAL;

    roombas[1].id = 1;
    roombas[1].joystick_port = 4;
    roombas[1].team = HUMAN;
    roombas[1].status = (uint8_t)SHIELDED;

    roombas[2].id = 2;
    roombas[2].joystick_port = 8;
    roombas[2].team = HUMAN;
    roombas[2].status = (uint8_t)SHIELDED;

    roombas[3].id = 3;
    roombas[3].joystick_port = 12;
    roombas[3].team = HUMAN;
    roombas[3].status = (uint8_t)SHIELDED;
}


int r_main(){

    //set up LED
    DDRB |= 1 << 7;
    DDRB |= ( 1<< PB6);

    /* Set up radio */
    DDRL |= (1 << PL2);
    PORTL &= ~(1 << PL2);
    _delay_ms(500);
    PORTL |= (1 << PL2);
    _delay_ms(500);
    Radio_Init();
    Radio_Configure_Rx(RADIO_PIPE_0, basestation_address , ENABLE);
    Radio_Configure(RADIO_1MBPS, RADIO_HIGHEST_POWER);

    /*setup joystick controllers*/
    //setup_controllers();
    init_game();


    /*Create our tasks for the base station*/
    Task_Create_Periodic(updateRoomba, 1,20, 15, 250);

    return 0;
}


#else

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
//PLAYERS roomba_num = PLAYER1;
uint8_t roomba_num = 0;

uint8_t base[5] = {0xff,0xff,0xff,0xff,0xff};

void radio_rxhandler(uint8_t pipenumber) {
	PORTB ^= ( 1<< PB4);
	Radio_Flush();
	//Profile1();
	//Service_Publish(radio_receive_service,0);
}

//Handle expected IR values, record unexpected values to pass on via radio.
//	(Get Roomba state via state packets)
void ir_rxhandler() {
	Service_Publish(ir_receive_service,0);
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
	int16_t vx = (game->velocity_x/(255/5) - 2)*50;
	int16_t vy = (game->velocity_y/(255/5) - 2)*50;

	if( vx == 0){
		if( vy > 0){
			vy = 1;
		}else{
			vy = -1;
		}
	}

	//Roomba_Drive(vx,vy);
}

void handleIRInput(pf_game_t* game)
{

}

void handleStateInput(pf_game_t* game)
{

}

void send_back_packet()
{
	//Radio_Flush();
	radiopacket_t packet;

	packet.type = GAME;
	for(int i = 0;i < 5; ++i){
		packet.payload.game.sender_address[i] = ROOMBA_ADDRESSES[roomba_num][i];
	}

	packet.payload.game.velocity_x = 200;
	packet.payload.game.velocity_y = 200;
	packet.payload.game.button = 200;

	packet.payload.game.game_player_id = 200;
	packet.payload.game.game_team = 200;
	packet.payload.game.game_state = 200;
	packet.payload.game.game_hit_flag = 200;
	packet.payload.game.game_enemy_id = 200;

	PORTB ^= (1<<PB5);

    //Radio_Transmit(&packet, RADIO_WAIT_FOR_TX);
    //Radio_Transmit(&packet, RADIO_RETURN_ON_TX);
}


void rr_roomba_controler() {
	//Start the Roomba for the first time.
	//Roomba_Init();
	int16_t value;

	Radio_Set_Tx_Addr(base);

	for(;;) {
		Service_Subscribe(radio_receive_service,&value);

		Radio_Flush();
		PORTB ^= (1<<PB7);
		//Handle the packets
		// RADIO_RX_STATUS result;
		// radiopacket_t packet;
		// do {
		// 	result = Radio_Receive(&packet);
		// 	Radio_Flush();

		// 	//Profile1();
		// 	//PORTB ^= (1<<PB4);

		// 	if(result == RADIO_RX_SUCCESS || result == RADIO_RX_MORE_PACKETS) {
		// 		if( packet.type == GAME)
		// 		{
		// 			handleRoombaInput(&packet.payload.game);
		// 			handleIRInput(&packet.payload.game);
		// 			handleStateInput(&packet.payload.game);
		// 		}
		// 	}

		// } while (result == RADIO_RX_MORE_PACKETS);

		//PORTB ^= (1<<PB4);
		//send_back_packet();
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
	PORTB &= ~(1<<PB4);
	PORTB &= ~(1<<PB5);

	//Initialize radio.
	Radio_Init();
	//IR_init();
	// Radio_Configure_Rx(RADIO_PIPE_0, ROOMBA_ADDRESSES[roomba_num], ENABLE);
	Radio_Configure_Rx(RADIO_PIPE_0, ROOMBA_ADDRESSES[1], ENABLE);
	// Radio_Configure_Rx(RADIO_PIPE_0, ROOMBA_ADDRESSES[0], ENABLE);
	// Radio_Configure_Rx(RADIO_PIPE_1, ROOMBA_ADDRESSES[1], ENABLE);
	// Radio_Configure_Rx(RADIO_PIPE_2, ROOMBA_ADDRESSES[2], ENABLE);
	// Radio_Configure_Rx(RADIO_PIPE_3, ROOMBA_ADDRESSES[3], ENABLE);
	Radio_Configure(RADIO_1MBPS, RADIO_HIGHEST_POWER);

	radio_receive_service = Service_Init();
	//ir_receive_service = Service_Init();
	Task_Create_RR(rr_roomba_controler,0);

	Task_Terminate();
	return 0 ;
}
#endif
