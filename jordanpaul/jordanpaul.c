#include <avr/io.h>
#include "rtos/os.h"
#include "rtos/timer.h"
#include "radio/radio.h"

SERVICE* radio_receive_service;
COPS_AND_ROBBERS roomba_num = 1;

void radio_rxhandler(uint8_t pipenumber) {
	Service_Publish(radio_receive_service,0);
}

//Handle expected IR values, record unexpected values to pass on via radio.
//	(Get Roomba state via state packets)
void ir_rxhandler() {
}

void rr_roomba_controler() {
	//Start the Roomba for the first time.
	//Roomba_Init();
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
				//Profile1();
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
	//cli();
	Radio_Init();
	//IR_init();
	Radio_Configure_Rx(RADIO_PIPE_0, ROOMBA_ADDRESSES[1], ENABLE);
	Radio_Configure(RADIO_2MBPS, RADIO_HIGHEST_POWER);
	//sei();

	radio_receive_service = Service_Init();
	//ir_receive_service = Service_Init();
	Task_Create_RR(rr_roomba_controler,0);
	// Task_Create_Periodic(p2,0,200,10,250);
	//Task_Create_Periodic(per_roomba_timeout,0,100,9,250);

	//Task_Create_Periodic(transmit_ir,0,250,10, 303);
	//Task_Create_RR(transmit_ir, 0);

	Task_Terminate();
	return 0 ;
}
