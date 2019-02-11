/*-----------------------------------------------------*\
|  USI I2C Slave Master                                 |
|                                                       |
| This library provides a robust I2C master protocol    |
| implementation on top of Atmel's Universal Serial     |
| Interface (USI) found in many ATTiny microcontrollers.|
|                                                       |
| Adam Honse (GitHub: CalcProgrammer1) - 7/29/2012      |
|            -calcprogrammer1@gmail.com                 |
\*-----------------------------------------------------*/
#ifndef USI_I2C_MASTER_H
#define USI_I2C_MASTER_H

#include <avr/io.h>
#include <util/delay.h>

//I2C Bus Specification v2.1 FAST mode timing limits
#ifdef I2C_FAST_MODE	
	#define I2C_TLOW	1.3
	#define I2C_THIGH	0.6

//I2C Bus Specification v2.1 STANDARD mode timing limits
#else
	#define I2C_TLOW	4.7
	#define I2C_THIGH	4.0
#endif

#define DDR_USI		DDRB
#define PORT_USI		PORTB
#define PIN_USI		PINB
#define PORT_USI_SDA	PB0
#define PORT_USI_SCL	PB2
#define PIN_USI_SDA		PINB0
#define PIN_USI_SCL		PINB2

//USI I2C Master Transceiver Start
// Starts the transceiver to send or receive data based on the r/w bit
char USI_I2C_Master_Write(unsigned char *msg, char msg_size);
char USI_I2C_Master_Read(unsigned char *msg, char msg_size);

#endif
