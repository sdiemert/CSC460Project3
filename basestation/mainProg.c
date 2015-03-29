#include <avr/io.h> 
#include <util/delay.h>
#include "os.h"
#include "kernel.h"
#include "radio.h"
#include "cops_and_robbers.h"


radiopacket_t rx_packet; 

void radio_rxhandler(uint8_t); 

void radio_rxhandler(uint8_t val){

}

void sendPacket(){
    
    Radio_Set_Tx_Addr(ROOMBA_ADDRESSES[1]); 
    while(1){
        Radio_Transmit(&rx_packet, RADIO_WAIT_FOR_TX); 
        PORTB ^= 1 << 7; 
        Task_Next(); 
    }
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
    Radio_Configure(RADIO_2MBPS, RADIO_HIGHEST_POWER); 

    Task_Create_Periodic(sendPacket, 1, 20, 15, 0);

    return 0; 
}
