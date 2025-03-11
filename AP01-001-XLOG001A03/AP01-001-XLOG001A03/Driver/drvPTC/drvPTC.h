/*!
 * \file 	drvPTC.h
 * \brief 	
 * \author 	Valentin DOMINIAK
 * \date	01/2024
 * \warning
 * MIT License
 * This software is provided "as is", without warranty of any kind. The authors
 * shall not be liable for any claims, damages, or other liability arising from
 * the use of the software. See the LICENSE file for more details.
 * \version 	0.1 (01/2024)
*/

#ifndef TOUCH_H
#define TOUCH_H

#ifdef __cplusplus
	extern "C" {
#endif // __cplusplus

/*!==========================================================================+*/
// FICHIERS HEADER
/*+==========================================================================+*/
#include "definitionTypes.h"
#include "qtm_common_components_api.h"
#include "qtm_acq_t321x_0x0017_api.h"
#include "qtm_touch_key_0x0002_api.h"
#include "qtm_freq_hop_auto_0x0004_api.h"

/*!==========================================================================+*/
// DEFINES
/*+==========================================================================+*/
#define DEF_TOUCH_MEASUREMENT_PERIOD_MS 20									// Measurement Time in milli seconds (1 to 255)
#define DEF_SENSOR_TYPE					NODE_SELFCAP						// Type of sensor
#define DEF_PTC_CAL_OPTION				CAL_AUTO_TUNE_NONE					// Sensor calibration mode for charge share delay, prescaler or series resistor
#define DEF_PTC_INTERRUPT_PRIORITY		None								// Interrupt priority for the PTC (0 to 2)
#define DEF_PTC_TAU_TARGET				CAL_CHRG_5TAU						// Calibration option to ensure full charge transfer
#define DEF_PTC_CAL_AUTO_TUNE			(uint8_t)((DEF_PTC_TAU_TARGET << CAL_CHRG_TIME_POS) | DEF_PTC_CAL_OPTION) // Bits 7:0 = XX | TT SELECT_TAU | X | CAL_OPTION
#define DEF_SEL_FREQ_INIT				FREQ_SEL_0							// Set default bootup acquisition frequency

#define DEF_NUM_CHANNELS				(2)									// Number of sensor nodes in the acquisition set (1 to 65535)
#define NODE_0_PARAMS					{X_NONE, Y(1), 24, PRSC_DIV_SEL_8, NODE_GAIN(GAIN_1, GAIN_8), FILTER_LEVEL_64} // Self-cap node parameter setting 
#define NODE_1_PARAMS					{X_NONE, Y(7), 5, PRSC_DIV_SEL_8, NODE_GAIN(GAIN_1, GAIN_8), FILTER_LEVEL_64} // {X-line, Y-line, Charge Share Delay, Prescaler, NODE_G(Analog Gain , Digital Gain), filter level} 

#define DEF_NUM_SENSORS					(2)									// Number of key sensor (1 to 65535)
#define KEY_0_PARAMS					{30, HYST_25, NO_AKS_GROUP}			// Key Sensor setting {Sensor Threshold, Sensor Hysterisis, Sensor AKS}
#define KEY_1_PARAMS					{30, HYST_25, NO_AKS_GROUP}

#define DEF_TOUCH_DET_INT				4									// De-bounce counter for additional measurements to confirm touch detection (0 to 255)
#define DEF_ANTI_TCH_DET_INT			1									// De-bounce counter for additional measurements to confirm away from touch signal to initiate Away from touch re-calibration (0 to 255)
#define DEF_ANTI_TCH_RECAL_THRSHLD		RECAL_50							// Threshold beyond with automatic sensor recalibration is initiated.
#define DEF_TCH_DRIFT_RATE				1									// Rate at which sensor reference value is adjusted towards sensor signal value when signal value is greater than reference (Units 200ms, 0-255, 20 = 4 seconds)
#define DEF_ANTI_TCH_DRIFT_RATE			1									// Rate at which sensor reference value is adjusted towards sensor signal value when signal value is less than reference (Units 200ms, 0-255, 5 = 1 second)
#define DEF_DRIFT_HOLD_TIME				1									// Time to restrict drift on all sensor when one or more sensors are activated (Units 200ms, 0-255, 20 = 4 seconds)
#define DEF_REBURST_MODE				REBURST_UNRESOLVED					// Set mode for additional sensor measurements based on touch activity
#define DEF_MAX_ON_DURATION				200									// Sensor maximum ON duration upon touch (0 to 255)

#define NUM_FREQ_STEPS					3									// Sets the frequency steps for hop (3 to 7)
#define DEF_MEDIAN_FILTER_FREQUENCIES	FREQ_SEL_0, FREQ_SEL_1, FREQ_SEL_2	// PTC Sampling Delay Selection - 0 to 15 PTC CLK cycles
#define DEF_FREQ_AUTOTUNE_ENABLE		1									// Enable / Disable the frequency hop auto tune (0 or 1)
#define FREQ_AUTOTUNE_MAX_VARIANCE		5									// Sets the maximum variance for Frequency Hop Auto tune (1 to 255)
#define FREQ_AUTOTUNE_COUNT_IN			6									// Sets the Tune in count for Frequency Hop Auto tune (1 to 255)

#define DEF_TOUCH_DATA_STREAMER_ENABLE	0u

/*!==========================================================================+*/
// STRUCTURES ET ENUMERATIONS
/*+==========================================================================+*/

/*!==========================================================================+*/
// PROTOTYPES
/*+==========================================================================+*/
void	v_drvPTC_initialization				(void);
void	v_drvPTC_timerHandler				(void);
void	v_drvPTC_process					(void);
UINT8	ui8_drvPTC_getSensorState			(UINT16 ui16_sensorNode);
void	v_drvPTC_updateSensorState			(UINT16 ui16_sensorNode, UINT8 ui8_newState);
UINT16	ui16_drvPTC_getSensorNodeSignal		(UINT16 ui16_sensorNode);
void    v_drvPTC_updateSensorNodeSignal		(UINT16 ui16_sensorNode, UINT16 ui16_newSignal);
UINT16	ui16_drvPTC_getSensorNodeReference	(UINT16 ui16_sensorNode);
void    v_drvPTC_updateSensorNodeReference	(UINT16 ui16_sensorNode, UINT16 ui16_newReference);
UINT16	ui16_drvPTC_getSensorCCVal			(UINT16 ui16_sensorNode);
void    v_drvPTC_updateSensorCCVal			(UINT16 ui16_sensorNode, UINT16 ui16_newCCValue);
void    v_drvPTC_calibrateNode				(UINT16 ui16_sensorNode);
void	v_drvPTC_suspendSensor				(UINT8 ui8_sensorNode);
void	v_drvPTC_resumeSensor				(UINT8 ui8_sensorNode);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // TOUCH_C
