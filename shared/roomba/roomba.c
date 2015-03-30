/*
 * roomba.c
 *
 *  Created on: 4-Feb-2009
 *      Author: nrqm
 */

#include <util/delay.h>
#include "../uart/uart.h"
#include "roomba.h"
#include "roomba_sci.h"

void Roomba_Init()
{
	ROOMBA_DD_DDR |= 1<<ROOMBA_DD_PIN;
	ROOMBA_DD_PORT &= ~(1<<ROOMBA_DD_PIN);

	// At 8 MHz, the AT90 generates a 57600 bps signal with a framing error rate of over 2%, which means that more than
	// 1 out of every 50 bits is wrong.  The fastest bitrate with a low error rate that the Roomba supports is
	// 38400 bps (0.2% error rate, or 1 bit out of every 500).

	// Try 57.6 kbps to start (this is the Roomba's default baud rate after the battery is installed).
	Roomba_UART_Init(UART_115200);

	// Try to start the SCI
	Roomba_Send_Byte(START);
	_delay_ms(20);

	// change the baud rate to 38400 bps.  Have to wait for 100 ms after changing the baud rate.
	Roomba_Send_Byte(BAUD);
	Roomba_Send_Byte(ROOMBA_19200BPS);
	_delay_ms(100);		// this delay will not work on old versions of WinAVR (new versions will see reduced but
						// still acceptable resolution; see _delay_ms definition)

	// change the AT90's UART clock
	Roomba_UART_Init(UART_19200);

	// start the SCI again in case the first start didn't go through.
	Roomba_Send_Byte(START);
	_delay_ms(20);

	// finally put the Roomba into safe mode.
	Roomba_Send_Byte(SAFE);
	_delay_ms(20);
}

void Roomba_Finish() {
	Roomba_Send_Byte(STOP);
}

void Roomba_UpdateSensorPacket(ROOMBA_SENSOR_GROUP group, roomba_sensor_data_t* sensor_packet)
{
	Roomba_Send_Byte(SENSORS);
	Roomba_Send_Byte(group);
	switch(group)
	{
	case EXTERNAL:
		// environment sensors
		while (uart_bytes_received() != 10);
		sensor_packet->bumps_wheeldrops = uart_get_byte(0);
		sensor_packet->wall = uart_get_byte(1);
		sensor_packet->cliff_left = uart_get_byte(2);
		sensor_packet->cliff_front_left = uart_get_byte(3);
		sensor_packet->cliff_front_right = uart_get_byte(4);
		sensor_packet->cliff_right = uart_get_byte(5);
		sensor_packet->virtual_wall = uart_get_byte(6);
		sensor_packet->motor_overcurrents = uart_get_byte(7);
		sensor_packet->dirt_left = uart_get_byte(8);
		sensor_packet->dirt_right = uart_get_byte(9);
		break;
	case CHASSIS:
		// chassis sensors
		while (uart_bytes_received() != 6);
		sensor_packet->remote_opcode = uart_get_byte(0);
		sensor_packet->buttons = uart_get_byte(1);
		sensor_packet->distance.bytes.high_byte = uart_get_byte(2);
		sensor_packet->distance.bytes.low_byte = uart_get_byte(3);
		sensor_packet->angle.bytes.high_byte = uart_get_byte(4);
		sensor_packet->angle.bytes.low_byte = uart_get_byte(5);
		break;
	case INTERNAL:
		// internal sensors
		while (uart_bytes_received() != 10);
		sensor_packet->charging_state = uart_get_byte(0);
		sensor_packet->voltage.bytes.high_byte = uart_get_byte(1);
		sensor_packet->voltage.bytes.low_byte = uart_get_byte(2);
		sensor_packet->current.bytes.high_byte = uart_get_byte(3);
		sensor_packet->current.bytes.low_byte = uart_get_byte(4);
		sensor_packet->temperature = uart_get_byte(5);
		sensor_packet->charge.bytes.high_byte = uart_get_byte(6);
		sensor_packet->charge.bytes.low_byte = uart_get_byte(7);
		sensor_packet->capacity.bytes.high_byte = uart_get_byte(8);
		sensor_packet->capacity.bytes.low_byte = uart_get_byte(9);
		break;
	}
	uart_reset_receive();
}

//Used to stop the Roomba when it gets shot.
void Roomba_Drive( int16_t velocity, int16_t radius )
{
	Roomba_Send_Byte(DRIVE);
	Roomba_Send_Byte(HIGH_BYTE(velocity));
	Roomba_Send_Byte(LOW_BYTE(velocity));
	Roomba_Send_Byte(HIGH_BYTE(radius));
	Roomba_Send_Byte(LOW_BYTE(radius));
}

// Set the Roomba's main LED color,intensity and stuff
void Roomba_LED(int8_t led_bits,int8_t color, int8_t intensity)
{
	Roomba_Send_Byte(LEDS);
	Roomba_Send_Byte(led_bits);
	Roomba_Send_Byte(color);
	Roomba_Send_Byte(intensity);
}
