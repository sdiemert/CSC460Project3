#if 0

#include <avr/io.h>
#include <util/delay.h>
#include "rtos/os.h"
#include "rtos/kernel.h"
#include "radio/radio.h"
#include "game.h"
#include "trace_uart/trace_uart.h"
#include "joystick/joystick.h"

radiopacket_t tx_packet;
radiopacket_t rx_packet;
pf_game_t game_packet;

SERVICE * rx_service;

uint8_t basestation_address[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t zombie_count = 1;

void radio_rxhandler(uint8_t);
void ir_rxhandler(){}

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
    // PORTB ^= 1 << 7;
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

            if(current_roomba==1){
               sprintf(output, "%d: (%d,%d)\n\r",current_roomba,joystick_x, joystick_y) ;
            }
            trace_uart_putstr(output);

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

        //trace_uart_putstr(output);
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

#else

#include <avr/io.h>
#include "rtos/os.h"
#include "rtos/timer.h"
#include "radio/radio.h"
#include "roomba/roomba.h"
#include "roomba/roomba_sci.h"
#include "uart/uart.h"
#include "ir/ir.h"
#include "game.h"
#include "profiler.h"
#include "trace_uart/trace_uart.h"
// #include "sonar/sonar.h"

#define LED_PORT      PORTB
#define LED_DDR       DDRB
#define LED_RX_PIN    PB4
#define LED_TX_PIN    PB5
#define LED_IR_PIN    PB6

// TODO: Change this to a proper pin number
#define TEAM_DDR            DDRC
#define TEAM_PORT           PORTC
#define TEAM_HUMAN_PIN      PC5
#define TEAM_ZOMBIE_PIN     PC4

SERVICE* radio_receive_service;
SERVICE* ir_receive_service;

// everyone hard-codes this number when they begin the game
uint8_t roomba_num = 2;
uint8_t ir_count = 0;
radiopacket_t tx_packet;

// keep track of the our current model
typedef struct _model_t {
    uint8_t player_id; // 0x4A, 0x4B, 0x4C, 0x4D
    uint8_t team;   // ZOMBIE = 0, HUMAN = 1
    uint8_t state; // NORMAL/STUNNED or SHIELDED/SHIELDLESS
    uint8_t last_ir_code; // 0 if no code

    uint8_t old_team; // previous team from last packet recv
    uint8_t old_state; // previous team from last packet recv

    int16_t old_vx;
    int16_t old_vy;
    int16_t old_button;
    int16_t vx;
    int16_t vy;
    uint8_t button;

    // function pointers to how we want to handle stuff
} Model_t;
Model_t model;


// Just wake up the task that is waiting for a radio packet
void radio_rxhandler(uint8_t pipenumber) {
	LED_PORT ^= ( 1<< LED_RX_PIN);
	Service_Publish(radio_receive_service,0);
}

// We have been hit. Process the ir hit.
void ir_rxhandler() {
    // LED_PORT ^= ( 1 << LED_IR_PIN);
    int16_t value = IR_getLast();
    if( Game_is_player_id(value) && value != PLAYER_IDS[roomba_num])
    {
        LED_PORT ^= (1 << LED_IR_PIN);
        Service_Publish(ir_receive_service,value);
    }
}
// ----------------
// END rxhandler functions
// ----------------

void handleRoombaInput(pf_game_t* game)
{
    int16_t x_value = (((game->velocity_x*5)/256) - 2)*40;
    int16_t y_value = (((game->velocity_y*5)/256) - 2)*250;

    if( x_value == 0 && y_value == 0){
        Roomba_Drive(0,0x8000);
    } else if( x_value == 0){
        y_value = -y_value;
        Roomba_Drive(y_value,0x8000);
    } else if( y_value == 0){
      // turn in place
      uint16_t deg = 1;
      if( x_value < 0 ){
        deg = -1;
      }
      Roomba_Drive(250,deg);
    }else{
      x_value = x_value;
      y_value = -y_value;
      Roomba_Drive(y_value,x_value);
    }

    // fire every 5th packet
    ir_count+= 1;
    if(ir_count == 5){
        IR_transmit(PLAYER_IDS[roomba_num]);
        ir_count = 0;
    }

    // if( game->button ){
    //     IR_transmit(PLAYER_IDS[roomba_num]);
    // }
}

void handleStateInput(pf_game_t* game){
    model.old_team = model.team;
    model.old_state = model.team;
    model.team = game->game_team;
    model.state = game->game_state;

    if( model.old_team != model.team){
        // we have changed teamd
        if(model.team == ZOMBIE){
            // We went from human to zombie

            // want to do anything special?
        }else{
            // we went from zombie to human. SHOULD NOT HAPPEN
            //OS_Abort();
        }

        TEAM_PORT &= ~( 1<< TEAM_HUMAN_PIN);
        TEAM_PORT &= ~( 1<< TEAM_ZOMBIE_PIN);

    }else{
        // we have not changed teams
        if( model.team == HUMAN){
            // HUMAN
        }else{
            // ZOMBIE
            if( model.state == STUNNED && model.old_state == NORMAL)
            {
                // we have just became stunned, start the autonomous control
                // model.autonomous_flag = 1;
            }
            else if( model.state == NORMAL && model.old_state == STUNNED)
            {
                // no longer stunner, start using input from the user.
                // model.autonomous_flag = 0;
            }
        }
    }

    // TODO: turn of the team leds
    if( model.team == HUMAN){
        // set the green led
        if(model.state == SHIELDED){
            // solid green

        }else{
            // blinking green

        }
    }else{
        // set the red led
        if( model.state == NORMAL){
            // solid red

        }else{
            // blinking red

        }
    }
}


// Return a packet back to the base station reporting
// if we have been hit, and by whom
void send_back_packet()
{
    LED_PORT ^= (1<<LED_TX_PIN);

    tx_packet.type = GAME;
    for(int i = 0;i < 5; ++i){
        tx_packet.payload.game.sender_address[i] = ROOMBA_ADDRESSES[roomba_num][i];
    }

    // We send this info back in order to inform the Roomba
    // of what state we think we are in.
    // tx_packet.payload.game.game_player_id = model.player_id;
    tx_packet.payload.game.game_player_id = PLAYER_IDS[roomba_num];
    tx_packet.payload.game.game_team = model.team;
    tx_packet.payload.game.game_state = model.state;

    // This is the info we REALLY have to send back to the base-station
    tx_packet.payload.game.game_hit_flag = (model.last_ir_code != 0) ? 1: 0;
    tx_packet.payload.game.game_enemy_id = model.last_ir_code;

    // reset the stuff.
    model.last_ir_code = 0;

    // Radio_Transmit(&tx_packet, RADIO_WAIT_FOR_TX);
    Radio_Transmit(&tx_packet, RADIO_RETURN_ON_TX);
}


void rr_roomba_controller() {
	//Start the Roomba for the first time.
	Roomba_Init();
    Radio_Set_Tx_Addr(base_station_address);

	int16_t value;
    RADIO_RX_STATUS result;
    radiopacket_t packet;

	for(;;) {
		Service_Subscribe(radio_receive_service,&value);

		do {

			result = Radio_Receive(&packet);

			if(result == RADIO_RX_SUCCESS || result == RADIO_RX_MORE_PACKETS) {
				if( packet.type == GAME)
				{
                    // wake up our roomba controller?
					handleRoombaInput(&packet.payload.game);
					handleStateInput(&packet.payload.game);
				}
			}

		} while (result == RADIO_RX_MORE_PACKETS);

        // POTENTIAL STRAT, don' send packet back until as late as possible
		send_back_packet();
	}
}

void rr_ir_controller()
{
    int16_t value;
    for(;;){
        Service_Subscribe(ir_receive_service,&value);
        model.last_ir_code = value;

        // we can play our own little jingle..
        // or do something autonomous
    }
}

void p_blink_led()
{
    uint8_t mask;
    for(;;)
    {
        if( model.team == ZOMBIE){
            if( model.state == NORMAL){
                TEAM_PORT |= (1<< TEAM_ZOMBIE_PIN);
            }else{
                TEAM_PORT ^= ( 1 <<TEAM_ZOMBIE_PIN);
            }
        }else{
            if( model.state == SHIELDED){
                TEAM_PORT |= (1 << TEAM_HUMAN_PIN);
            }else{
                TEAM_PORT ^= (1<< TEAM_HUMAN_PIN);
            }
        }

        Task_Next();
    }
}

// Note that this will delay by 1 second (200 ticks)
void power_cycle_radio()
{
	//Turn off radio power.
	DDRL |= (1 << PL2);
	PORTL &= ~(1<< PL2);
	_delay_ms(500);
	PORTL |= (1<<PL2);
	_delay_ms(500);
}

void setup_leds()
{
    // Enable LEDs
    LED_DDR |= (1<<LED_RX_PIN);
    LED_DDR |= (1<<LED_TX_PIN);
    LED_DDR |= (1<<LED_IR_PIN);
    LED_PORT &= ~(1<<LED_RX_PIN);
    LED_PORT &= ~(1<<LED_TX_PIN);
    LED_PORT &= ~(1<<LED_IR_PIN);


    TEAM_DDR |= (1<<TEAM_HUMAN_PIN);
    TEAM_DDR |= (1<<TEAM_ZOMBIE_PIN);
    TEAM_PORT &= ~(1<<TEAM_HUMAN_PIN);
    TEAM_PORT &= ~(1<<TEAM_ZOMBIE_PIN);
}

void init_model(Model_t* model)
{
    model->player_id = PLAYER_IDS[roomba_num];
    model->team = HUMAN;
    model->state = SHIELDED;
    model->last_ir_code= 0;
    model->old_team = 0;
    model->old_state = 0;
    model->old_vx = 0;
    model->old_vy = 0;
    model->old_button = 0;
    model->vx = 0;
    model->vy = 0;
    model->button = 0;
}

void Sonar_rxhandler(int16_t distance){
}

void sonar_value()
{
    int16_t dist;

    for(;;){
        PORTB ^= ( 1 << PB6);
        Sonar_fire();
        Task_Next();
    }
}

void p_roomba_lifted()
{
    Task_Next();
    roomba_sensor_data data;
    for(;;){
        Roomba_UpdateSensorPacket(EXTERNAL,&data);

        if( data )

    }
}

int r_main(void)
{
	power_cycle_radio();
    setup_leds();
    init_model(&model);

	//Initialize radio.
	Radio_Init();
	IR_init();
	Radio_Configure_Rx(RADIO_PIPE_0, ROOMBA_ADDRESSES[roomba_num], ENABLE);
	Radio_Configure(RADIO_1MBPS, RADIO_HIGHEST_POWER);

    //Create the services
	radio_receive_service = Service_Init();
	ir_receive_service = Service_Init();

    //Create the tasks
    Task_Create_RR(rr_roomba_controller,0);
    Task_Create_RR(rr_ir_controller,0);

    // periodic tasks to control blinking the led + controlling a stunned zombie
    // they will be flag gaurded such that they only run once they are allowed.
    Task_Create_Periodic(p_blink_led ,0,10,3,250);
    Task_Create_Periodic(p_roomba_lifted,0,1000,800,250);

    // Sonar_init();
    // trace_uart_init();
    // Task_Create_Periodic(sonar_value,0,20,5,5);

	Task_Terminate();
	return 0 ;
}

#endif