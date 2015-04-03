#include "rtos/os.h"
#include "rtos/timer.h"
#include "radio/radio.h"
#include "roomba/roomba.h"
#include "roomba/roomba_sci.h"
#include "roomba/roomba_led_sci.h"
#include "uart/uart.h"
#include "ir/ir.h"
#include "game.h"
#include "profiler.h"
#include "trace_uart/trace_uart.h"
// #include "sonar/sonar.h"
// #include "roomba/roomba_music.h"
// #include "music_stream.h"

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
uint8_t roomba_num = 1;
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

    // reset into safe mode.
    Roomba_Send_Byte(SAFE);

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
        // we have changed teams
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
                // no longer stunned, start using input from the user.
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

    Roomba_led_debris(1);
    Roomba_led_spot(1);
    Roomba_led_warn(1);
    Roomba_led_dock(1);
    Roomba_led_main_led(255,255);

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
					//handleRoombaInput(&packet.payload.game);
					//handleStateInput(&packet.payload.game);
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

        //LED_PORT ^= ( 1<< LED_RX_PIN);
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

    // Sonar_init();
    // trace_uart_init();
    // Task_Create_Periodic(sonar_value,0,20,5,5);

	Task_Terminate();
	return 0 ;
}

//void load_music_stream(){
    // eponas song.
    // Music_Stream_init();
    // Music_Stream_add_note(65, 16);
    // Music_Stream_add_note(69, 16);
    // Music_Stream_add_note(71, 24);
    // Music_Stream_add_note(65, 16);
    // Music_Stream_add_note(69, 16);
    // Music_Stream_add_note(71, 24);
    // Music_Stream_add_note(65, 16);
    // Music_Stream_add_note(69, 16);
    // Music_Stream_add_note(71, 16);
    // Music_Stream_add_note(76, 16);
    // Music_Stream_add_note(74, 24);
    // Music_Stream_add_note(71, 16);
    // Music_Stream_add_note(72, 16);
    // Music_Stream_add_note(71, 16);
    // Music_Stream_add_note(67, 16);
    // Music_Stream_add_note(64, 36);

    //
    // Music_Stream_init();
    // Music_Stream_add_note_char("g2",24);
    // Music_Stream_add_note_char("a2",24);
    // Music_Stream_add_note_char("b2",24);
    // Music_Stream_add_note_char("c#3",24);
    // Music_Stream_add_note_char("g2",24);
    // Music_Stream_add_note_char("a2",24);
    // Music_Stream_add_note_char("b2",24);
    // Music_Stream_add_note_char("c#3",24);
    // Music_Stream_add_note_char("g#2",22);
    // Music_Stream_add_note_char("a#2",22);
    // Music_Stream_add_note_char("c3",22);
    // Music_Stream_add_note_char("d3",22);
    // Music_Stream_add_note_char("g#2",22);
    // Music_Stream_add_note_char("a#2",22);
    // Music_Stream_add_note_char("c3",22);
    // Music_Stream_add_note_char("d3",22);
    // Music_Stream_add_note_char("a2",18);
    // Music_Stream_add_note_char("b2",18);
    // Music_Stream_add_note_char("c#3",18);
    // Music_Stream_add_note_char("d#3",18);
    // Music_Stream_add_note_char("a2",18);
    // Music_Stream_add_note_char("b2",18);
    // Music_Stream_add_note_char("c#3",18);
    // Music_Stream_add_note_char("d#3",18);
    // Music_Stream_add_note_char("a#2",14);
    // Music_Stream_add_note_char("c3",14);
    // Music_Stream_add_note_char("d3",14);
    // Music_Stream_add_note_char("e3",14);
    // Music_Stream_add_note_char("a#2",14);
    // Music_Stream_add_note_char("c3",14);
    // Music_Stream_add_note_char("d3",14);
    // Music_Stream_add_note_char("e3",14);
    // Music_Stream_add_note_char("b2",12);
    // Music_Stream_add_note_char("c#3",12);
    // Music_Stream_add_note_char("d#3",12);
    // Music_Stream_add_note_char("f3",12);
    // Music_Stream_add_note_char("c3",10);
    // Music_Stream_add_note_char("d3",10);
    // Music_Stream_add_note_char("e3",10);
    // Music_Stream_add_note_char("f#3",10);
    // Music_Stream_add_note_char("c#3",8);
    // Music_Stream_add_note_char("d#3",8);
    // Music_Stream_add_note_char("f3",8);
    // Music_Stream_add_note_char("g3",8);
    // Music_Stream_add_note_char("d3",6);
    // Music_Stream_add_note_char("e3",6);
    // Music_Stream_add_note_char("f#3",6);
    // Music_Stream_add_note_char("g#3",6);
    // Music_Stream_add_note_char("a4",26);
    // Music_Stream_add_note_char("a#4",26);
    // Music_Stream_add_note_char("b4",26);
    // Music_Stream_add_note_char("c5",128);
//}