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

#define LED_PORT      PORTB
#define LED_DDR       DDRB
#define LED_RX_PIN    PB4
#define LED_TX_PIN    PB5
#define LED_IR_PIN    PB6
#define LED_GREEN_PIN PB7
// TODO: Change this to a proper pin number
#define LED_RED_PIN   PB7

#define ZOMBIE_STOP 0
#define ZOMBIE_GO 1

// ONLY TEMPORARY
uint8_t ir_count = 0;

SERVICE* radio_receive_service;
SERVICE* ir_receive_service;

// everyone hard-codes this number when they begin the game
uint8_t roomba_num = 1;
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


    // control the if the periodic tasks run. 0 for off ,1 for on
    uint8_t blink_led_flag;
    uint8_t autonomous_flag;

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
    LED_PORT ^= ( 1 << LED_IR_PIN);
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
    // map the given input into stuff.

	int16_t vx = (game->velocity_x/(255/9) - 4)*50;
	int16_t vy = (game->velocity_y/(255/9) - 4)*50;

    model.old_vx     =  model.vx;
    model.old_vy     =  model.vy;
    model.old_button =  model.button;
    model.vx     =  vx;
    model.vy     =  vy;
    model.button =  game->button;

	if( vy == 0){
		if( vx > 0){
            vy = 200;
			vx = 1;
		}else if( vx < 0){
            vy = 200;
			vx = -1;
		}
	}

    // pass of to our controller
	Roomba_Drive(vy,vx);

    // fire every 5th packet
    if( ir_count == 5){
        IR_transmit(model.player_id);
        ir_count = 0;
    }
    ir_count++;
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
            OS_Abort();
        }

    }else{
        // we have not changed teams
        if( model.team == HUMAN){
            // HUMAN
        }else{
            // ZOMBIE
            if( model.state == STUNNED && model.old_state == NORMAL)
            {
                // we have just became stunned, start the autonomous control
                model.autonomous_flag = 1;
            }
            else if( model.state == NORMAL && model.old_state == STUNNED)
            {
                // no longer stunner, start using input from the user.
                model.autonomous_flag = 0;
            }
        }
    }

    if( model.team == HUMAN){
        // set the green led
        if(model.state == SHIELDED){
            // solid green
            model.blink_led_flag = 0;
            LED_PORT |= (1 << LED_GREEN_PIN);
        }else{
            // blinking green
            model.blink_led_flag = 1;
        }
    }else{
        // set the red led
        if( model.state == NORMAL){
            // solid red
            model.blink_led_flag = 0;
            LED_PORT |= (1 << LED_RED_PIN);
        }else{
            // blinking red
            model.blink_led_flag = 1;
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
    tx_packet.payload.game.game_player_id = model.player_id;
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
    for(;;)
    {
        while(model.blink_led_flag)
        {
            if( model.team == ZOMBIE){
                LED_PORT ^= (1 << LED_RED_PIN);
            }else{
                LED_PORT ^= (1 << LED_GREEN_PIN);
            }
            Task_Next();
        }
        Task_Next();
    }
}

void p_autonomous()
{
    for(;;)
    {
        while(model.autonomous_flag)
        {
            // do whatever
            // read roomba sensors..
            // do drive command
            // fire ir gun

            Task_Next();
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
    LED_DDR |= (1<<LED_GREEN_PIN);
    LED_DDR |= (1<<LED_RED_PIN);
    LED_PORT &= ~(1<<LED_RX_PIN);
    LED_PORT &= ~(1<<LED_TX_PIN);
    LED_PORT &= ~(1<<LED_IR_PIN);
    LED_PORT &= ~(1<<LED_GREEN_PIN);
    LED_PORT &= ~(1<<LED_RED_PIN);
}

void init_model(Model_t* model)
{
    model->player_id = PLAYER_IDS[roomba_num];
    model->team = 0;
    model->state = 0;
    model->last_ir_code= 0;
    model->old_team = 0;
    model->old_state = 0;
    model->old_vx = 0;
    model->old_vy = 0;
    model->old_button = 0;
    model->vx = 0;
    model->vy = 0;
    model->button = 0;

    model->blink_led_flag = 0;
    model->autonomous_flag= 0;
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

    // Create the services
	radio_receive_service = Service_Init();
	ir_receive_service = Service_Init();

    // Create the tasks
    Task_Create_RR(rr_roomba_controller,0);
    Task_Create_RR(rr_ir_controller,0);

    // periodic tasks to control blinking the led + controlling a stunned zombie
    // they will be flag gaurded such that they only run once they are allowed.
    Task_Create_Periodic(p_blink_led ,0,50,3,250);
    Task_Create_Periodic(p_autonomous,0,100,50,251);

	Task_Terminate();
	return 0 ;
}
