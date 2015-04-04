#ifndef _ROOMBA_LED_SCI_H_
#define _ROOMBA_LED_SCI_H_

#include <avr/io.h>

#define ROOMBA_LED_DEBRIS 0
#define ROOMBA_LED_SPOT 1
#define ROOMBA_LED_DOCK 2
#define ROOMBA_LED_WARN 3

void Roomba_led_debris(int on);
void Roomba_led_spot(int on);
void Roomba_led_dock(int on);
void Roomba_led_warn(int on);
void Roomba_led_main_led(int8_t color, int8_t intensity);

#endif