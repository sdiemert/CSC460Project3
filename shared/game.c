/*
 * cops_and_robbers.h
 *
 * Created: 2015-01-26 17:09:37
 *  Author: Daniel
 */

#include "game.h"
#include "avr/io.h"
#include "roomba/roomba_sci.h"

uint8_t ROOMBA_ADDRESSES[4][5] = {
	{0x4A,0x4A,0x4A,0x4A,0x4A},
	{0x4B,0x4B,0x4B,0x4B,0x4B},
	{0x4C,0x4C,0x4C,0x4C,0x4C},
	{0x4D,0x4D,0x4D,0x4D,0x4D}
};
uint8_t ROOMBA_FREQUENCIES [4] = {104, 106, 108, 110};

uint8_t base_station_address[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// each roomba on a differnent freq
// base station