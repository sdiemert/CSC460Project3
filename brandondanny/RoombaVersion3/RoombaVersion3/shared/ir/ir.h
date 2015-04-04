/*
 * ir.h
 *
 * Created: 2015-01-28 12:15:37
 *  Author: Daniel
 */


#ifndef IR_H_
#define IR_H_

#include "avr/io.h"

//Arbitrary random values which
//are fairly distinct in binary.
typedef enum _ir_command_bytes {
    IR_SHOOT = 0x92,
    IR_WAKE_COP1 = 0x24,
    IR_WAKE_COP2 = 0x11,
    IR_WAKE_ROBBER1 = 0x84,
    IR_WAKE_ROBBER2 = 0xE3
} IR_COMMAND_CODE;

void IR_transmit(uint8_t data);
void IR_init();
uint8_t IR_getLast();


#endif /* IR_H_ */
/*
DDRB -- data direction register B
ICRn - input compare register
OCRn[ABC?] - output compare register
    OCFnx - output compare flag
        flag se twhen the TCNT equals OCRn
    OCIENnx - output compare interrupt enable n on x
        if set then when the TCNT matches, then
            it will generate the interrupt signal

TCNTn - Timer counter
    TCNTnH - timer counter high
    TCNTnL - timer counter low
OCRnA/B/C - Output compare Register
ICRn - Input Capture Register
TCCRnA/B/C - timer counter control register
TIFRn - timer interrupt flag regsiter
TIMSKn - timer interrupt mask register
WGMn3:0 - Wave form generation mode n bits 3-0
    found in the TCCRnA and TCCRnB
COMnx1:0 - compare output mode
CTC - compare timer counter


TCCRnA  - [COM1A1,COM1A0,COM1B1,COM1B0,COM1C1,COM1C0,WGM11,WGM10]
    COMnA1:0 compare output mode
        controls the output compare pins (OCnA, OCnB, OCnC)
        on the board.
        depends on the timer mode ( ie. normal,PWM,CTC)
    WGMn1:0
        used to the set the counter count mode (normal,PWM,CTC)

TCCRnB
    ICNCn - Input Capture Noise Canceller
    ICESn - Input Compare Edge Select
    [empty not use bit]
    WGMn3:2 - Wave form generate mode
        used to set the counter count mode (normal,PWM, CTC)
    CSn2:0 - Clock select bits
        Used to set the prescalar on the counter
    000 - no clock source
    001 - 1
    010 - 2
    011 - 64
    100 - 256
    101 - 1024
    110 - external clock source from Tn pin (falling edge)
    111 - extrenal clock source from Tn pin (rising edge)

TCCRnC
    FOCnA  - Force output compare for channel A
    FOCnB  - Force output compare for channel B
    FOCnC  - Force output compare for channel C

TCNTn
    Timer counter register... yea we get the value from here.

OCRnA, OCRnB
    OCRnA[H|L]
    output compare register which is constantly being check against TCTn

ICRn - Input Capture Register

TIMSKn -- timer interrupt mask register
    enables/disables the interrupts
    ICIEn - input capture interrupt enable
    OCIEnC - output compare C interrupt enable
    OCIEnB - output compare B interrupt enable
    OCIEnA - output compare A interrupt enable
    TOIEn - Overflow interrupt enable


TIFRn --timer interrupt flag register
    status register/flag register when an interrupt has been signaled
    ICFn - input capture flag
    OCFnC - output compare C Flag  ( set high once TCNTn matches OCRnC)
    OCFnB - output compare B Flag  ( set high once TCNTn matches OCRnB)
    OCFnA - output compare A Flag  ( set high once TCNTn matches OCRnA)
    TOVn - timer overflow flag


EICRA - External Interrupt Control Register A
EICRB - External Interrupt Control Register B


EIFR - external interrupt flag register
    INTF7:0
    flag is set once a interrupt has occurred.
    Cleared by writing a 1 to the bit

EIMSK - external interrupt mask


ISCn - interrupt service control

*/