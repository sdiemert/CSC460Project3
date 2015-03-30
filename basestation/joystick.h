#ifndef JOYSTICK_H
#define JOYSTICK_H


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

void setup_controllers(); 

uint8_t read_analog(uint8_t);

#endif