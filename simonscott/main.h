#include <avr/io.h>
#include "os.h"
#include "radio.h"
#include "roomba.h"
#include "roomba_sci.h"
#include "uart.h"
#include "ir.h"
#include "game.h"

//INDICATOR DEFINITIONS: 

#define MODIFY_INDICATOR PC0 	//Digital I/O pin 37 on arduino mega, 	A8 on ATMega 2560 pinout. 
#define HUMAN_INDICATOR PC1 	//Digital I/O pin 36 on arduino mega, 	A9 on ATMega 2560 pinout. 
#define ZOMBIE_INDICATOR PC2 	//Digital I/O pin 35 on arduino mega, 	A10 on ATMega 2560 pinout. 

#define RX_RADIO_PACKET PB4 	//Blinks when a radio packet is recieved Digital I/O pin 10 on arduino. 
#define TX_RADIO_PACKET PB5		//Blinks when radio packet is transmitted. Digital I/O 11 on arduino. 

#define IR_RX PB6				//Blinks when an IR packet is received. 


#define MODIFY_INDICATOR_ON() PORTC |= (1 << MODIFY_INDICATOR) 
#define MODIFY_INDICATOR_OFF() PORTC &= ~(1<<MODIFY_INDICATOR) 

#define ZOMBIE_STUN_INDICATOR_OFF() MODIFY_INDICATOR_OFF()
#define ZOMBIE_STUN_INDICATOR_ON() MODIFY_INDICATOR_ON() 
#define ZOMBIE_TEAM_INDICATOR_ON() PORTC |= (1 << PC2)
#define ZOMBIE_TEAM_INDICATOR_OFF() PORTC &= ~(1 << PC2)

#define HUMAN_SHIELD_INDICATOR_ON() MODIFY_INDICATOR_ON()
#define HUMAN_SHIELD_INDICATOR_OFF() MODIFY_INDICATOR_OFF() 
#define HUMAN_TEAM_INDICATOR_ON() PORTC |= (1 << PC1)
#define HUMAN_TEAM_INDICATOR_OFF() PORTC &= ~(1 << PC1)

#define RADIO_PACKET_RX_TOGGLE() PORTB ^= (1 << RX_RADIO_PACKET)
#define RADIO_PACKET_TX_TOGGLE() PORTB ^= (1 << TX_RADIO_PACKET)

#define IR_RX_TOGGLE() PORTB ^= (1 << IR_RX)