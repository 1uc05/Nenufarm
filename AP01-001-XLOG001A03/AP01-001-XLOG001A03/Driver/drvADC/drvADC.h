/*!
 * \file 	drvADC.h
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

#ifndef DRVADC_H_
#define DRVADC_H_

/*!==========================================================================+*/
// FICHIERS HEADER
/*+==========================================================================+*/
#include "definitionTypes.h"

/*!==========================================================================+*/
// DEFINES
/*+==========================================================================+*/

/*!==========================================================================+*/
// STRUCTURES ET ENUMERATIONS
/*+==========================================================================+*/
typedef enum {
	ADC_CHANNEL_0,
	ADC_CHANNEL_1,
	ADC_CHANNEL_2,
	ADC_CHANNEL_3,
	ADC_CHANNEL_4,
	ADC_CHANNEL_5,
	ADC_CHANNEL_6,
	ADC_CHANNEL_7,
	ADC_CHANNEL_8,
	ADC_CHANNEL_9,
	ADC_CHANNEL_10,
	ADC_CHANNEL_11,
	ADC_CHANNEL_UNDEFINED,
}ADC_CHANNEL;

typedef enum {
	ADC_INSTANCE_0,
	ADC_INSTANCE_1,
	ADC_INSTANCE_UNDEFINED,
}ADC_INSTANCE;

typedef enum {
	DRV_ADC_ERROR_NO_ERROR					= 0,
	DRV_ADC_ERROR_CHANNEL_UNDEFINED			= -1,
	DRV_ADC_ERROR_INSTANCE_UNDEFINED		= -2,
	DRV_ADC_ERROR_CHANNEL_ALREADY_USED		= -3,
	DRV_ADC_ERROR_CHANNEL_NOT_USED			= -4,
	DRV_ADC_ERROR_TOO_MUCH_INSTANCE_NUMBER	= -5,
	DRV_ADC_ERROR_NULL_PTR					= -6,
	DRV_ADC_ERROR_READING_TIMEOUT			= -7,
}DRV_ADC_ERROR;

/*!==========================================================================+*/
// PROTOTYPES
/*+==========================================================================+*/
DRV_ADC_ERROR en_drvADC_initialization	(const ADC_CHANNEL en_channel, const ADC_INSTANCE en_instance, UINT8 *pui8_instanceNumber);
DRV_ADC_ERROR en_drvADC_getInputValue	(const UINT8 ui8_instanceNumber, const UINT8 ui8_averageReading, UINT16 *pui16_value);

#endif /* DRVADC_H_ */