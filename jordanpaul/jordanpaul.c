#include <avr/io.h>
#include "rtos/os.h"
#include "rtos/timer.h"
#include "radio/radio.h"
#include "roomba/roomba.h"
#include "roomba/roomba_sci.h"
#include "ir/ir.h"
#include "game.h"

SERVICE* radio_receive_service;
PLAYERS roomba_num = PLAYER1;

void radio_rxhandler(uint8_t pipenumber) {
	Service_Publish(radio_receive_service,0);
}

//Handle expected IR values, record unexpected values to pass on via radio.
//	(Get Roomba state via state packets)
void ir_rxhandler() {
}


void handlePacket(radiopacket_t *packet) {
	//Filter out unwanted commands.
	// if (packet->payload.command.command == START ||
	// 	packet->payload.command.command == BAUD ||
	// 	packet->payload.command.command == SAFE ||
	// 	packet->payload.command.command == FULL ||
	// 	packet->payload.command.command == SENSORS)
	// {
	// 	return;
	// }

	//If the Roomba is dead it should not start moving.
	// if(packet->payload.command.command != DRIVE || current_roomba_status == ROOMBA_ALIVE) {
	pf_game_t* game = &packet->payload.game;

	int16_t vx = (game->vx/(255/5) - 2)*200;
	int16_t vy = (game->vy/(255/5) - 2)*200;
	Roomba_Drive(vx,vy);

	// if(current_roomba_status == ROOMBA_ALIVE) {
	// 	//Pass the command to the Roomba.
	// 	Roomba_Send_Byte(packet->payload.command.command);
	// 	for (int i = 0; i < packet->payload.command.num_arg_bytes; i++)
	// 	{
	// 		Roomba_Send_Byte(packet->payload.command.arguments[i]);
	// 	}
	// }

	// Set the radio's destination address to be the remote station's address
	// Radio_Set_Tx_Addr(packet->payload.command.sender_address);

	// Update the Roomba sensors into the packet structure that will be transmitted.
	//Roomba_UpdateSensorPacket(EXTERNAL, &packet.payload.sensors.sensors);
	//Roomba_UpdateSensorPacket(CHASSIS, &packet.payload.sensors.sensors);
	//Roomba_UpdateSensorPacket(INTERNAL, &packet.payload.sensors.sensors);

	// send the sensor packet back to the remote station.
	// packet->type = SENSOR_DATA;
	// packet->payload.sensors.roomba_number = roomba_num;
	// Radio_Transmit(packet, RADIO_RETURN_ON_TX);
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
					handlePacket(&packet);
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

	//Initialize radio.
	Radio_Init();
	IR_init();
	Radio_Configure_Rx(RADIO_PIPE_0, ROOMBA_ADDRESSES[1], ENABLE);
	Radio_Configure(RADIO_2MBPS, RADIO_HIGHEST_POWER);

	radio_receive_service = Service_Init();
	//ir_receive_service = Service_Init();
	Task_Create_RR(rr_roomba_controler,0);

	Task_Terminate();
	return 0 ;
}
