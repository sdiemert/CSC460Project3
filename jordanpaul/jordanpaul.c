#include <avr/io.h>
#include "rtos/os.h"
#include "rtos/timer.h"
#include "radio/radio.h"
#include "roomba/roomba.h"
#include "roomba/roomba_sci.h"
#include "roomba/roomba_led_sci.h"
#include "trace_uart/trace_uart.h"
#include "uart/uart.h"
#include "ir/ir.h"
#include "game.h"

SERVICE* radio_receive_service;
SERVICE* ir_receive_service;
//PLAYERS roomba_num = PLAYER1;
uint8_t roomba_num = 1;

void radio_rxhandler(uint8_t pipenumber) {
	Service_Publish(radio_receive_service,0);
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

	//char code[30];
	//sprintf(code,"%d %d\n", vx,vy);
	//trace_uart_putstr(code);
	Roomba_Drive(vx,vy);
}

void handleIRInput(pf_game_t* game)
{

}

void handleStateInput(pf_game_t* game)
{

}


void rr_roomba_controler() {
	//Start the Roomba for the first time.
	Roomba_Init();
	int16_t value;

	for(;;) {
		Service_Subscribe(radio_receive_service,&value);

		//Handle the packets
		RADIO_RX_STATUS result;
		radiopacket_t packet;
		do {
			result = Radio_Receive(&packet);

			//Profile1();
			PORTB &= ~(1<<PB4);
			for(int i = 0 ;i < 1000;++i){
				PORTB |= (1 << PB4);
			}
			PORTB &= ~(1<<PB4);


			if(result == RADIO_RX_SUCCESS || result == RADIO_RX_MORE_PACKETS) {
				if( packet.type == GAME)
				{
					handleRoombaInput(&packet.payload.game);
					handleIRInput(&packet.payload.game);
					handleStateInput(&packet.payload.game);
				}
			}

		} while (result == RADIO_RX_MORE_PACKETS);
	}
}

int r_main(void)
{
	//Turn off radio power.
	DDRL |= (1 << PL2);
	PORTL &= ~(1<<PL2);
	_delay_ms(500);
	PORTL |= (1<<PL2);
	_delay_ms(500);

	DDRB |= (1<<PB4);

	//trace_uart_init();

	//Initialize radio.
	Radio_Init();
	IR_init();
	Radio_Configure_Rx(RADIO_PIPE_0, ROOMBA_ADDRESSES[1], ENABLE);
	Radio_Configure(RADIO_2MBPS, RADIO_HIGHEST_POWER);

	radio_receive_service = Service_Init();
	ir_receive_service = Service_Init();
	Task_Create_RR(rr_roomba_controler,0);

	Task_Terminate();
	return 0 ;
}
