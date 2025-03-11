/*!
 * \file 	drvPWM.h
 * \brief 	
 * \author 	Valentin DOMINIAK
 * \date 	10/2023
 * \warning
 * MIT License
 * This software is provided "as is", without warranty of any kind. The authors
 * shall not be liable for any claims, damages, or other liability arising from
 * the use of the software. See the LICENSE file for more details.
 * \version 	0.1 (10/2023)
*/

#ifndef DRVPWM_H_
#define DRVPWM_H_

/*!==========================================================================+*/
// FICHIERS HEADER
/*+==========================================================================+*/
#include "definitionTypes.h"

/*!==========================================================================+*/
// DEFINES
/*+==========================================================================+*/
#define PWM_FREQUENCY	10000

/*!==========================================================================+*/
// STRUCTURES ET ENUMERATIONS
/*+==========================================================================+*/
typedef enum {
	PWM_CHANNEL_0,
	PWM_CHANNEL_1,
	PWM_CHANNEL_2,
	PWM_CHANNEL_3,
	PWM_CHANNEL_4,
	PWM_CHANNEL_5,
	PWM_CHANNEL_UNDEFINED,
}PWM_CHANNEL;

typedef enum {
	DRV_PWM_ERROR_NO_ERROR					= 0,
	DRV_PWM_ERROR_WRONG_CHANNEL_NUMBER		= -1,
	DRV_PWM_ERROR_CHANNEL_UNDEFINED			= -2,
	DRV_PWM_ERROR_CHANNEL_ALREADY_USED		= -3,
	DRV_PWM_ERROR_CHANNEL_NOT_USED			= -4,
	DRV_PWM_ERROR_TOO_MUCH_INSTANCE_NUMBER	= -5,
	DRV_PWM_ERROR_DUTY_CYCLE_VALUE			= -6,
	DRV_PWM_ERROR_NULL_PTR					= -7,
}DRV_PWM_ERROR;

/*!==========================================================================+*/
// PROTOTYPES
/*+==========================================================================+*/
DRV_PWM_ERROR en_drvPWM_initialization	(const PWM_CHANNEL en_channel, const UINT8 ui8_defaultDutyCycle, UINT8 *pui8_instanceNumber);
DRV_PWM_ERROR en_drvPWM_changeDutyCycle	(const UINT8 ui8_instanceNumber, const UINT8 ui8_dutyCycle);

#endif /* DRVPWM_H_ */