#include <avr/io.h> 
#include <util/delay.h>
#include "os.h"
#include "kernel.h"
#include "radio.h"
#include "game.h"
#include "trace_uart.h"
#include "joystick.h"


radiopacket_t tx_packet; 
radiopacket_t rx_packet; 
pf_game_t game_packet;

SERVICE * rx_service; 


uint8_t basestation_address[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t zombie_count = 1;

void radio_rxhandler(uint8_t); 

//a struct to hold all the information for a player
typedef struct{
    uint8_t id;
    uint8_t joystick_port; //the port the joystick is located on. 
    TEAM team;
    uint8_t status;
    uint8_t stun_duration; // Stun duration in MS
    uint8_t stun_elapsed; 
    uint8_t hits;
} roomba_t; 

roomba_t roombas[4];

// End game task
void end_game() {
    while(1) {
        // TODO End game logic
    }
}

void radio_rxhandler(uint8_t val){
    PORTB ^= 1 << 6; 
    Service_Publish(rx_service, 1);
}

void sendPacket(uint8_t id, uint8_t x, uint8_t y, uint8_t b){
    
    Radio_Set_Tx_Addr(ROOMBA_ADDRESSES[id]);

    game_packet.velocity_x = x;
    game_packet.velocity_y = y;
    game_packet.button ^= game_packet.button;
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
    char output[20]; 

    while(1) {
        for(current_roomba = 0; current_roomba < 4; current_roomba++) {
            //Radio_Flush();

            PORTB |= 1 << 5; 
            
            joystick_x = read_analog(roombas[current_roomba].joystick_port);
            joystick_y = read_analog(roombas[current_roomba].joystick_port+1);                      

            //sprintf(output, "%d: (%d,%d)\n\r",current_roomba,joystick_x, joystick_y);

            //trace_uart_putstr(output);

            // TODO Button Press

            // Handle stunned zombies here. Turn their state back to normal after a certain amount of time.
            if(roombas[current_roomba].status == (uint8_t)STUNNED){
                if(roombas[current_roomba].stun_elapsed >= roombas[current_roomba].stun_duration){
                    //don't need to stun anymore.
                    roombas[current_roomba].status = (uint8_t)NORMAL; 
                    roombas[current_roomba].stun_elapsed = 0;
                    sendPacket(current_roomba, joystick_x, joystick_y, button);    
                }else{
                    //increment the stun
                    roombas[current_roomba].stun_elapsed += 1;
                    sendPacket(current_roomba, 127, 127, 0);
                }
            }else{
                //not stunned, just continue as is.
                sendPacket(current_roomba, joystick_x, joystick_y, button);
            }


            PORTB &= ~(1 << 5); 
            Task_Next();
        }
    }
}

void manageReceive(){

    int16_t x; 
    char output[50];
    uint8_t result;
    pf_game_t roomba_game_state;
    while(1){
        Service_Subscribe(rx_service, &x);
        PORTB |= (1 << 4);
        result = Radio_Receive(&rx_packet);

        while(result == RADIO_RX_MORE_PACKETS){
            result = Radio_Receive(&rx_packet);            
        }

        sprintf(output, "from: %d, enemy: %d, hit: %d, team: %d, state: %d\n\r",
                rx_packet.payload.game.game_player_id, 
                rx_packet.payload.game.game_enemy_id, 
                rx_packet.payload.game.game_hit_flag,
                rx_packet.payload.game.game_team,
                rx_packet.payload.game.game_state
        );

        trace_uart_putstr(output);
        //roomba_game_state = rx_packet.payload.game;

        // Handle a hit
        if(roomba_game_state.game_hit_flag == 1) {
            uint8_t player_hit = roomba_game_state.game_player_id;
            uint8_t player_shooting = roomba_game_state.game_enemy_id;

            // TODO Error Handling

            // If the player hit is a human and the player shooting is a zombie, the human should lose his/her shield
            if(roombas[player_hit].team == HUMAN && roombas[player_hit].status == (uint8_t)SHIELDED && roombas[player_shooting].team == ZOMBIE) {
                roombas[player_hit].status = (uint8_t)SHIELDLESS;
            } else if(roombas[player_hit].team == ZOMBIE && roombas[player_shooting].team == HUMAN) {
                roombas[player_hit].status = (uint8_t)STUNNED;
                roombas[player_hit].stun_duration += 6;
                roombas[player_hit].stun_elapsed = 0;
            } else if(roombas[player_hit].team == HUMAN  && roombas[player_hit].status == (uint8_t)SHIELDLESS && roombas[player_shooting].team == ZOMBIE) {
                roombas[player_hit].team = ZOMBIE;
                roombas[player_hit].status = (uint8_t)NORMAL;
                zombie_count ++;
            }

            // Increment the player shooting's hit counter
            roombas[player_shooting].hits += 1;
        }

        // Check if we should end the game
        if(zombie_count == 4) {
            Task_Create_System(end_game, 0);
        }

        PORTB &= ~(1 << 4);

        Task_Next(); 
    }

}

void init_game() {
    // Setup Roombas
    roombas[0].id = 0;
    roombas[0].joystick_port = 0;
    roombas[0].team = ZOMBIE;
    roombas[0].status = (uint8_t)NORMAL; 
    roombas[0].stun_duration = 0;
    roombas[0].hits = 0;

    roombas[1].id = 1;
    roombas[1].joystick_port = 4;
    roombas[1].team = HUMAN;
    roombas[1].status = (uint8_t)SHIELDED;
    roombas[1].stun_duration = 0;
    roombas[1].hits = 0;    

    roombas[2].id = 2;
    roombas[2].joystick_port = 8;
    roombas[2].team = HUMAN;
    roombas[2].status = (uint8_t)SHIELDED; 
    roombas[2].stun_duration = 0;
    roombas[2].hits = 0;

    roombas[3].id = 3;
    roombas[3].joystick_port = 12;
    roombas[3].team = HUMAN;
    roombas[3].status = (uint8_t)SHIELDED;
    roombas[3].stun_duration = 0;
    roombas[3].hits = 0;
}


int r_main(){

    //set up LED
    DDRB |= 1 << 7; 
    DDRB |= 1 << 6; 
    DDRB |= 1 << 5; 
    DDRB |= 1 << 4; 

    /* Set up radio */ 
    DDRL |= (1 << PL2); 
    PORTL &= ~(1 << PL2);
    _delay_ms(500); 
    PORTL |= (1 << PL2);
    _delay_ms(500); 
    Radio_Init(); 
    Radio_Configure_Rx(RADIO_PIPE_0, basestation_address, ENABLE); 
    Radio_Configure(RADIO_1MBPS, RADIO_HIGHEST_POWER); 


    rx_service = Service_Init();

    trace_uart_init(); 

    /*setup joystick controllers*/
    setup_controllers();
    init_game(); 

    /*Create our tasks for the base station*/
    Task_Create_Periodic(updateRoomba, 1, 20, 15, 250);
    Task_Create_RR(manageReceive, 0);

    return 0; 
}
