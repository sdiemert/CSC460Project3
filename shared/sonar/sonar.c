#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "sonar.h"

#define SONAR_DDR DDRL
#define SONAR_PORT PORTL
#define SONAR_PIN PL0

static volatile uint16_t sonar_start = 0;
static volatile uint16_t sonar_end = 0;
static volatile uint8_t sonar_looking_for_rising = 0;
static void Sonar_enable_interrupt_capture(uint8_t on,uint8_t rising);
static void Sonar_rising();
static void Sonar_falling();
static void Sonar_stop();

void Sonar_init(){
    TCCR4A = 0;
    TCCR4B = 0;

    // enable the OC4A timer
    // TCCR4A |= (1 << COM4A1);
    // TCCR4A &= ~(1 << COM4A0);

    // // disable the OC4a timer
    // TCCR4A &= ~(1 << COM4A1);
    // TCCR4A &= ~(1 << COM4A0);

    // CTC Mode
    //TCCR4B |= (1 << WGM42) | ( 1 << WGM43);
    TCCR4B |= (1 << WGM42);

    // TCCR4B &= ~(1<<ICES4); // trigger on falliing
    // TCCR4B |= (1<<ICES4); // trigger on rising

    // 64 prescalar, 8000 ticks for 32 ms
    TCCR4B |= (1<< CS41) | (1<< CS40);

    // don't need to do anything here.
    //TCCR4C

    // set output compare counts
    TCNT4 = 0;
    OCR4A = 16000; // wait ~32 ms

    // // enable the the input capture interrupt.
    // TIMSK4 |= (1<< ICIE4);
    // TIMSK4 |= (1<< OCIE4A);
}

void Sonar_fire(){
    // do the initial ping
    SONAR_DDR |= (1 << SONAR_PIN); // output
    PORTH &= ~( 1 << SONAR_PIN); // low signal
    _delay_us(2);               // for  2 microseconds
    PORTH |= ( 1 << SONAR_PIN); // high signal
    _delay_us(5);               // for 10 microseconds
    PORTH &= ~( 1 << SONAR_PIN); // low signal

    SONAR_DDR &= ~( 1 << SONAR_PIN);  // set as input

    // clear any pending interrupts
    TIFR4 |= ( 1 << ICF4);
    TIFR4 |= ( 1 << OCF4A);

    // turn on the output compare register interrupt
    // TIMSK4 |= (1<< ICIE4);
    TIMSK4 |= ( 1<< OCIE4A);

    // enable the OC4a timer. Use it as a watch dog timer
    TCCR4A &= ~(1 << COM4A0);
    TCCR4A = (1 << COM4A1);

    TCNT4 = 0;

    sonar_looking_for_rising = 1;
    Sonar_enable_interrupt_capture(1,0);
    // look for falling edge
}

void Sonar_enable_interrupt_capture(uint8_t on,uint8_t rising){
    if(on){
        // enable the interrupt capture
        TIMSK4 |= (1<< ICIE4);
        if(rising){
            // trigger on rising
            TCCR4B |= (1<<ICES4);
        }else{
            // trigger on falling
            TCCR4B &= ~(1<<ICES4);
        }
    }else{
        // disable the interrupt capture
        TIMSK4 &= ~(1<< ICIE4);
    }
}

void Sonar_rising(){
    sonar_start = ICR4;
    sonar_looking_for_rising = 0;
    Sonar_enable_interrupt_capture(1,0);
    // wait for falling edge
}
void Sonar_falling(){
    // received the falling edge
    sonar_end = ICR4;
    Sonar_enable_interrupt_capture(0,0);
}

void Sonar_stop(){
    // turn off everything
    TIMSK4 &= ~(1<< ICIE4); // disable the input capture interrupt
    TIMSK4 &= ~(1<< OCIE4A); // disbable the output compare

    // disable the OC4a timer
    TCCR4A &= ~(1 << COM4A1);
    TCCR4A &= ~(1 << COM4A0);

    // clear any pending interrupts
    TIFR4 |= ( 1 << ICF4);
    TIFR4 |= ( 1 << OCF4A);
}

// #define LED_PORT      PORTB
// #define LED_DDR       DDRB
// #define LED_RX_PIN    PB4
// #define LED_TX_PIN    PB5
// #define LED_IR_PIN    PB6

//interrupt handlers
ISR(TIMER4_COMPA_vect){
    PORTB ^= ( 1 << PB5);
    Sonar_stop();
}

ISR(TIMER4_CAPT_vect){
    PORTB ^= ( 1 << PB4);
    if(sonar_looking_for_rising){
        Sonar_rising();
    }else{
        Sonar_falling();
        Sonar_stop();
        Sonar_rxhandler((sonar_end - sonar_start)/58);
    }
}