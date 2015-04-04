#include <avr/io.h>
#include "roomba_led_sci.h"
#include "roomba.h"

static int8_t _color;
static int8_t _intensity;
static int8_t _led_bits;

static void _set_bits(int on,int8_t type){
    if( on ){
        _led_bits |= (1<<type);
    }else{
        _led_bits &= ~(1<<type);
    }
}

void Roomba_led_debris(int on){
    _set_bits(on,ROOMBA_LED_DEBRIS);
    Roomba_LED(_led_bits,_color,_intensity);
}

void Roomba_led_spot(int on){
    _set_bits(on,ROOMBA_LED_SPOT);
    Roomba_LED(_led_bits,_color,_intensity);
}

void Roomba_led_dock(int on){
    _set_bits(on,ROOMBA_LED_DOCK);
    Roomba_LED(_led_bits,_color,_intensity);
}

void Roomba_led_warn(int on){
    _set_bits(on,ROOMBA_LED_WARN);
    Roomba_LED(_led_bits,_color,_intensity);
}

void Roomba_led_main_led(int8_t color, int8_t intensity){
    _color = color;
    _intensity = intensity;
    Roomba_LED(_led_bits,_color,_intensity);
}