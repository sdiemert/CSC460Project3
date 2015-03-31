#include <avr/io.h> 
#include <util/delay.h>
#include "os.h"
#include "kernel.h"
#include "radio.h"
#include "game.h"
#include "roomba.h"

radiopacket_t tx_packet; 
radiopacket_t rx_packet; 
pf_game_t game_packet;

SERVICE* radio_rx_service; 

uint8_t basestation_address[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

void radio_rxhandler(uint8_t); 

void radio_rxhandler(uint8_t val){
    Service_Publish(radio_rx_service, 0); 
    PORTB ^= 1 << 7; 
}

void sendPacket(){

}

void updateRoomba(){
    Roomba_Init();
    uint16_t val;  
    radiopacket_t packet;
    RADIO_RX_STATUS result;
    while(1){

        Service_Subscribe(radio_rx_service, &val); 

        result = Radio_Receive(&packet); 

    }
}

void init_game() {

}


int r_main(){

    //set up LED
    DDRB |= 1 << 7; 

    /* Set up radio */ 
    DDRL |= (1 << PL2); 
    PORTL &= ~(1 << PL2);
    _delay_ms(500); 
    PORTL |= (1 << PL2);
    _delay_ms(500); 
    Radio_Init(); 
    Radio_Configure_Rx(RADIO_PIPE_0, ROOMBA_ADDRESSES[0], ENABLE); 
    Radio_Configure(RADIO_1MBPS, RADIO_HIGHEST_POWER); 

    //trace_uart_init(); 

    init_game(); 

    radio_rx_service = Service_Init(); 

    /*Create our tasks for the base station*/
    Task_Create_RR(updateRoomba, 0);

    return 0; 
}
