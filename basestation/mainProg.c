#include <avr/io.h> 
#include <util/delay.h>
#include "os.h"
#include "kernel.h"
#include "radio.h"
#include "game.h"
#include "joystick.h"


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

    Radio_Transmit(&tx_packet, RADIO_WAIT_FOR_TX); 
    PORTB ^= 1 << 7; 
}

void updateRoomba(){
    uint8_t current_roomba;
    uint8_t joystick_x = 0;
    uint8_t joystick_y = 0;    
    uint8_t button = 0;

    while(1) {
        for(current_roomba = 0; current_roomba < 4; current_roomba ++) {
            //joystick_x = read_analog(roombas[current_roomba].joystick_port);
            //joystick_y = read_analog(roombas[current_roomba].joystick_port+1);            


            joystick_x = read_analog(1);
            joystick_y = read_analog(0);            

            // TODO Button Press

            // TODO Update State

            sendPacket(current_roomba, joystick_x, joystick_y, button);

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
