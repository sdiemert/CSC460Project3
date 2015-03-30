#include <avr/io.h> 
#include <util/delay.h>
#include "os.h"
#include "kernel.h"
#include "../shared/radio.h"
#include "../shared/game.h"
#include "joystick.h"


radiopacket_t rx_packet; 

void radio_rxhandler(uint8_t); 

//a struct to hold all the information for a player
typedef struct{
    uint8_t joystick_port; //the port the joystick is located on. 
    TEAM team
    uint8_t status;
} roomba_t; 

roomba_t[4] roombas;

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

void updateRoomba(){
    uint8_t current_roomba;
    uint8_t joystick_left = 0;
    uint8_t joystick_right = 0;    

    uint8_t joystick_up = 0;
    uint8_t joystick_down = 0;    

    while(1) {
        for(current_roomba = 0; current_roomba < 4; current_roomba ++) {
            joystick = read_analog(roombas[current_roomba].joystick_port);
            Task_Next();
        }
    }
}

void init_game() {
    // Setup Roombas
    roombas[0].joystick_port = 0;
    roombas[0].team = ZOMBIE;
    roombas[0].status = (uint8_t)NORMAL;    

    roombas[1].joystick_port = 4;
    roombas[1].team = HUMAN;
    roombas[1].status = (uint8_t)SHIELDED;

    roombas[2].joystick_port = 8;
    roombas[2].team = HUMAN;
    roombas[2].status = (uint8_t)SHIELDED;    

    roombas[3].joystick_port = 12;
    roombas[3].team = HUMAN;
    roombas[3].status = (uint8_t)SHIELDED;  
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

    /*setup joystick controllers*/
    setup_controllers();
    init_game(); 

    /*Create our tasks for the base station*/
    Task_Create_Periodic(updateRoomba, 1, 20, 15, 0);

    return 0; 
}
