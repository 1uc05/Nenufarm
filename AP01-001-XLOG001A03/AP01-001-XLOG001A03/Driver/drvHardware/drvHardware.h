/*!
 * \file 		drvHardware.h
 * \brief
 * \author      Valentin DOMINIAK
 * \date        01/2024
 * \warning
 * Copyright (C) 2025 NENUFARM - See the file LICENSE for copying permission.
 */

#ifndef DRVHARDWARE_H_
#define DRVHARDWARE_H_

/*!==========================================================================+*/
// FICHIERS HEADER
/*+==========================================================================+*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include "definitionTypes.h"

/*!==========================================================================+*/
// DEFINES
/*+==========================================================================+*/

/*!==========================================================================+*/
// STRUCTURES ET ENUMERATIONS
/*+==========================================================================+*/
typedef enum {
	GPIO_PORTA,
	GPIO_PORTB,
	GPIO_PORTC,
} GPIO_PORT;

typedef enum {
	PORT_DIRECION_INPUT,
	PORT_DIRECION_OUTPUT,
	PORT_DIRECION_OFF,
} PORT_DIRECTION;

typedef enum {
	PORT_PULL_MODE_OFF,
	PORT_PULL_MODE_UP,
} PORT_PULL_MODE;

/*!==========================================================================+*/
// PROTOTYPES
/*+==========================================================================+*/
void	v_drvHardware_initialization			(void);
void	v_drvHardware_setPinDirection			(const GPIO_PORT en_port, const UINT8 ui8_pin, const PORT_DIRECTION en_direction);
void	v_drvHardware_setPinConfiguration		(const GPIO_PORT en_port, const UINT8 ui8_pin, const PORT_ISC_t en_configuration);
void	v_drvHardware_setPinPullMode			(const GPIO_PORT en_port, const UINT8 ui8_pin, const PORT_PULL_MODE en_pullMode);
void	v_drvHardware_setPinValue				(const GPIO_PORT en_port, const UINT8 ui8_pin, const BOOL b_value);
BOOL 	b_drvHardware_getPinValue				(const GPIO_PORT en_port, const UINT8 ui8_pin);
UINT32	ui32_drvHardware_getCPUClockFrequency	(void);

#endif /* DRVHARDWARE_H_ */

