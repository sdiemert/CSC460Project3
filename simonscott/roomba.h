/*
 * roomba.h
 *
 *  Created on: 4-Feb-2009
 *      Author: nrqm
 */

#ifndef ROOMBA_H_
#define ROOMBA_H_

#include <stdio.h>
#include "sensor_struct.h"

typedef enum _rsg
{
	EXTERNAL=1,		// group 1 (bumper/wheeldrops, cliff sensors, virtual wall, motor overcurrents, dirt sensors)
	CHASSIS=2,		// group 2 (remote, buttons, distance, angle)
	INTERNAL=3,		// group 3 (charging state; battery voltage, current, charge and capacity; internal temperature)
} ROOMBA_SENSOR_GROUP;

#define HIGH_BYTE(x) (x>>8)
#define LOW_BYTE(x)  (x&0xFF)

/**
 * Connect to the Roomba at 38400 baud and put it into safe mode.
 */
void Roomba_Init();

/**
 * Query the Roomba for one of its sensor packet groups.  This function should not be interrupted by the RTOS kernel.
 * Updating sensor group 2 causes the Roomba's distance and angle sensor values to be reset.  In other words, they
 * will start over at 0 for the next time the group is updated.  You'll need to keep a running total in your program
 * if you're tracking the distance and angle.
 *
 * \param type The sensor group to update.  This does not support the 0 group (i.e. all sensors) because that takes
 * 		too long to download over UART when using the RTOS.
 * \param sensor_packet A pointer to a sensor packet structure that will be populated by sensor data.  Only the data
 * 		in the group being updated are changed; the other data are left intact.
 */
void Roomba_UpdateSensorPacket(ROOMBA_SENSOR_GROUP group, roomba_sensor_data_t* sensor_packet);

/**
 * Send a drive command to the Roomba.
 *
 * \param velocity The velocity at which to drive, approximately in mm/sec.  Negative values mean drive backwards.
 * 		The range of valid values is -500 to 500.
 * \param radius The radius of the Roomba's turn, in mm.  A negative radius is a left turn and a positive radius is
 * 		a right turn.  A radius value of 0x8000 means go as straight as possible (the Roomba is not capable of going
 * 		truly straight).  A radius value of -1 means turn in place clockwise, and 1 means turn in place counter-
 * 		clockwise.  The range of valid values is -2000 to 2000.
 */
void Roomba_Drive( int16_t velocity, int16_t radius );

/**
 * Send a set LED command to the Roomba
 * \param led_bits - a bit mask determine which LED to light up.
 *      - | - | - | - |  check robot |  Dock | Spot | Debris
 *      Set the bit mask to 1 turns on the LED, clearing it to 0
 * \param color  - The color for the main (center) LED. 0 = green, 255 = red
 * \param intensity  - The intensity of the main/center LED light. 0 = Off, 255 = full
 * Example Commnad -
 *  Turn on the HOME LED and light the Power LED green at half intensity
 *  [139][4][0][128]
 */
void Roomba_LED(int8_t led_bits,int8_t color, int8_t intensity);

void Roomba_Finish();

#endif /* ROOMBA_H_ */
