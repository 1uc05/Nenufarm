/*!
 * \file 	drvPTC.c
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

#ifndef TOUCH_C
#define TOUCH_C

/*!==========================================================================+*/
// FICHIERS HEADER
/*+==========================================================================+*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include "drvPTC.h"

/*!==========================================================================+*/
// DEFINES
/*+==========================================================================+*/
#ifdef __XC32__
	#warning "The library is not tested for XC32 compiler, use ARM-GCC compiler instead"
#endif

#if DEF_PTC_CAL_OPTION != CAL_AUTO_TUNE_NONE
	#error "Autotune feature is NOT supported by this acquisition library. Enable Autotune featuers in START."
#endif

#if DEF_TOUCH_DATA_STREAMER_ENABLE == 0u
	#if DEF_PTC_CAL_OPTION != CAL_AUTO_TUNE_NONE
		#warning "Automatic charge time option is enabled without enabling datastreamer. So, automatic tuning of charge time option is disabled."
		#define DEF_PTC_CAL_OPTION CAL_AUTO_TUNE_NONE
	#endif
#endif

/*!==========================================================================+*/
// VARIABLES GLOBALES
/*+==========================================================================+*/
volatile uint8_t time_to_measure_touch_flag						= 0;								// Flag to indicate time for touch measurement
volatile uint8_t touch_postprocess_request						= 0;								// Postporcess request flag
volatile uint8_t measurement_done_touch							= 0;								// Measurement Done Touch Flag
uint8_t module_error_code										= 0;								// Error Handling
uint16_t touch_acq_signals_raw[DEF_NUM_CHANNELS];													// Acquisition module internal data - Size to largest acquisition set

qtm_acq_node_group_config_t ptc_qtlib_acq_gen1					= {DEF_NUM_CHANNELS, 
																   DEF_SENSOR_TYPE, 
																   DEF_PTC_CAL_AUTO_TUNE, 
																   DEF_SEL_FREQ_INIT};				// Acquisition set 1 - General settings
qtm_acq_node_data_t ptc_qtlib_node_stat1[DEF_NUM_CHANNELS];											// Node status, signal, calibration values
qtm_acq_t321x_node_config_t ptc_seq_node_cfg1[DEF_NUM_CHANNELS] = {NODE_0_PARAMS, 
																   NODE_1_PARAMS};					// Node configurations
qtm_acquisition_control_t qtlib_acq_set1						= {&ptc_qtlib_acq_gen1, 
																   &ptc_seq_node_cfg1[0],
																   &ptc_qtlib_node_stat1[0]};		// Container
																   
uint16_t noise_filter_buffer[DEF_NUM_SENSORS * NUM_FREQ_STEPS];										// Buffer used with various noise filtering functions
uint8_t  freq_hop_delay_selection[NUM_FREQ_STEPS]				= {DEF_MEDIAN_FILTER_FREQUENCIES};
uint8_t  freq_hop_autotune_counters[NUM_FREQ_STEPS];
qtm_freq_hop_autotune_config_t qtm_freq_hop_autotune_config1	= {DEF_NUM_CHANNELS,
																   NUM_FREQ_STEPS,
																   &ptc_qtlib_acq_gen1.freq_option_select,
																   &freq_hop_delay_selection[0],
																   DEF_FREQ_AUTOTUNE_ENABLE,
																   FREQ_AUTOTUNE_MAX_VARIANCE,
																   FREQ_AUTOTUNE_COUNT_IN};			// Configuration
qtm_freq_hop_autotune_data_t qtm_freq_hop_autotune_data1		= {0, 
																   0,
																   &noise_filter_buffer[0],
																   &ptc_qtlib_node_stat1[0],
																   &freq_hop_autotune_counters[0]};	// Data
qtm_freq_hop_autotune_control_t qtm_freq_hop_autotune_control1	= {&qtm_freq_hop_autotune_data1, 
																   &qtm_freq_hop_autotune_config1};	// Container

qtm_touch_key_group_config_t qtlib_key_grp_config_set1			= {DEF_NUM_SENSORS,
																   DEF_TOUCH_DET_INT,
																   DEF_MAX_ON_DURATION,
																   DEF_ANTI_TCH_DET_INT,
																   DEF_ANTI_TCH_RECAL_THRSHLD,
																   DEF_TCH_DRIFT_RATE,
																   DEF_ANTI_TCH_DRIFT_RATE,
																   DEF_DRIFT_HOLD_TIME,
																   DEF_REBURST_MODE};				// Keys set 1 - General settings
qtm_touch_key_group_data_t qtlib_key_grp_data_set1;
qtm_touch_key_data_t qtlib_key_data_set1[DEF_NUM_SENSORS];											// Key data
qtm_touch_key_config_t qtlib_key_configs_set1[DEF_NUM_SENSORS]	= {KEY_0_PARAMS, KEY_1_PARAMS};		// Key Configurations
qtm_touch_key_control_t qtlib_key_set1							= {&qtlib_key_grp_data_set1, 
																   &qtlib_key_grp_config_set1,
																   &qtlib_key_data_set1[0],
																   &qtlib_key_configs_set1[0]};		// Container

/*!==========================================================================+*/
// PROTOTYPES
/*+==========================================================================+*/
static touch_ret_t touch_sensors_config(void);
static void qtm_measure_complete_callback(void);
static void qtm_error_callback(uint8_t error);

/*============================================================================
static touch_ret_t touch_sensors_config(void)
------------------------------------------------------------------------------
Purpose: Initialization of touch key sensors
Input  : none
Output : none
Notes  :
============================================================================*/
/* Touch sensors config - assign nodes to buttons / wheels / sliders / surfaces / water level / etc */
static touch_ret_t touch_sensors_config(void)
{
	uint16_t    sensor_nodes;
	touch_ret_t touch_ret = TOUCH_SUCCESS;

	/* Init acquisition module */
	qtm_ptc_init_acquisition_module(&qtlib_acq_set1);

	/* Init pointers to DMA sequence memory */
	qtm_ptc_qtlib_assign_signal_memory(&touch_acq_signals_raw[0]);

	/* Initialize sensor nodes */
	for (sensor_nodes = 0u; sensor_nodes < DEF_NUM_CHANNELS; sensor_nodes++) {
		/* Enable each node for measurement and mark for calibration */
		qtm_enable_sensor_node(&qtlib_acq_set1, sensor_nodes);
		qtm_calibrate_sensor_node(&qtlib_acq_set1, sensor_nodes);
	}

	/* Enable sensor keys and assign nodes */
	for (sensor_nodes = 0u; sensor_nodes < DEF_NUM_CHANNELS; sensor_nodes++) {
		qtm_init_sensor_key(&qtlib_key_set1, sensor_nodes, &ptc_qtlib_node_stat1[sensor_nodes]);
	}

	return (touch_ret);
}

/*============================================================================
static void qtm_measure_complete_callback( void )
------------------------------------------------------------------------------
Purpose: this function is called after the completion of
         measurement cycle. This function sets the post processing request
         flag to trigger the post processing.
Input  : none
Output : none
Notes  :
============================================================================*/
static void qtm_measure_complete_callback(void)
{
	touch_postprocess_request = 1u;
}

/*============================================================================
static void qtm_error_callback(uint8_t error)
------------------------------------------------------------------------------
Purpose: this function is used to report error in the modules.
Input  : error code
Output : decoded module error code
Notes  :
Derived Module_error_codes:
    Acquisition module error =1
    post processing module1 error = 2
    post processing module2 error = 3
    ... and so on
============================================================================*/
static void qtm_error_callback(uint8_t error)
{
	module_error_code = error + 1u;

#if DEF_TOUCH_DATA_STREAMER_ENABLE == 1
	datastreamer_output();
#endif
}

/*============================================================================
void v_drvPTC_initialization(void)
------------------------------------------------------------------------------
Purpose: Initialization of touch library. PTC and
         datastreamer modules are initialized in this function.
Input  : none
Output : none
Notes  :
============================================================================*/
void v_drvPTC_initialization(void)
{
	/* Configure touch sensors with Application specific settings */
	touch_sensors_config();
}

/*============================================================================
void v_drvPTC_process(void)
------------------------------------------------------------------------------
Purpose: Main processing function of touch library. This function initiates the
         acquisition, calls post processing after the acquisition complete and
         sets the flag for next measurement based on the sensor status.
Input  : none
Output : none
Notes  :
============================================================================*/
void v_drvPTC_process(void)
{
	touch_ret_t touch_ret;

	/* check the time_to_measure_touch_flag flag for Touch Acquisition */
	if (time_to_measure_touch_flag == 1u) {
		/* Do the acquisition */
		touch_ret = qtm_ptc_start_measurement_seq(&qtlib_acq_set1, qtm_measure_complete_callback);

		/* if the Acquistion request was successful then clear the request flag */
		if (TOUCH_SUCCESS == touch_ret) {
			/* Clear the Measure request flag */
			time_to_measure_touch_flag = 0u;
		}
	}

	/* check the flag for node level post processing */
	if (touch_postprocess_request == 1u) {
		/* Reset the flags for node_level_post_processing */
		touch_postprocess_request = 0u;

		/* Run Acquisition module level post processing*/
		touch_ret = qtm_acquisition_process();

		/* Check the return value */
		if (TOUCH_SUCCESS == touch_ret) {
			/* Returned with success: Start module level post processing */
			touch_ret = qtm_freq_hop_autotune(&qtm_freq_hop_autotune_control1);
			if (TOUCH_SUCCESS != touch_ret) {
				qtm_error_callback(1);
			}
			touch_ret = qtm_key_sensors_process(&qtlib_key_set1);
			if (TOUCH_SUCCESS != touch_ret) {
				qtm_error_callback(2);
			}
		} else {
			/* Acq module Eror Detected: Issue an Acq module common error code 0x80 */
			qtm_error_callback(0);
		}

		if ((0u != (qtlib_key_set1.qtm_touch_key_group_data->qtm_keys_status & 0x80u))) {
			time_to_measure_touch_flag = 1u;
		} else {
			measurement_done_touch = 1u;
		}
	}
}

uint8_t interrupt_cnt;
/*============================================================================
void v_drvPTC_timerHandler(void)
------------------------------------------------------------------------------
Purpose: This function updates the time elapsed to the touch key module to
         synchronize the internal time counts used by the module.
Input  : none
Output : none
Notes  :
============================================================================*/
void v_drvPTC_timerHandler(void)
{
	interrupt_cnt++;
	if (interrupt_cnt >= DEF_TOUCH_MEASUREMENT_PERIOD_MS) {
		interrupt_cnt = 0;
		/* Count complete - Measure touch sensors */
		time_to_measure_touch_flag = 1u;
		qtm_update_qtlib_timer(DEF_TOUCH_MEASUREMENT_PERIOD_MS);
	}
}

UINT16 ui16_drvPTC_getSensorNodeSignal(UINT16 ui16_sensorNode)
{
	return (ptc_qtlib_node_stat1[ui16_sensorNode].node_acq_signals);
}

void v_drvPTC_updateSensorNodeSignal(UINT16 ui16_sensorNode, UINT16 ui16_newSignal)
{
	ptc_qtlib_node_stat1[ui16_sensorNode].node_acq_signals = ui16_newSignal;
}

UINT16 ui16_drvPTC_getSensorNodeReference(UINT16 ui16_sensorNode)
{
	return (qtlib_key_data_set1[ui16_sensorNode].channel_reference);
}

void v_drvPTC_updateSensorNodeReference(UINT16 ui16_sensorNode, UINT16 ui16_newReference)
{
	qtlib_key_data_set1[ui16_sensorNode].channel_reference = ui16_newReference;
}

UINT16 ui16_drvPTC_getSensorCCVal(UINT16 ui16_sensorNode)
{
	return (ptc_qtlib_node_stat1[ui16_sensorNode].node_comp_caps);
}

void v_drvPTC_updateSensorCCVal(UINT16 ui16_sensorNode, UINT16 ui16_newCCValue)
{
	ptc_qtlib_node_stat1[ui16_sensorNode].node_comp_caps = ui16_newCCValue;
}

UINT8 ui8_drvPTC_getSensorState(UINT16 ui16_sensorNode)
{
	return (qtlib_key_set1.qtm_touch_key_data[ui16_sensorNode].sensor_state);
}

void v_drvPTC_updateSensorState(UINT16 ui16_sensorNode, UINT8 ui8_newState)
{
	qtlib_key_set1.qtm_touch_key_data[ui16_sensorNode].sensor_state = ui8_newState;
}

void v_drvPTC_calibrateNode(UINT16 ui16_sensorNode)
{
	/* Calibrate Node */
	qtm_calibrate_sensor_node(&qtlib_acq_set1, ui16_sensorNode);
	/* Initialize key */
	qtm_init_sensor_key(&qtlib_key_set1, ui16_sensorNode, &ptc_qtlib_node_stat1[ui16_sensorNode]);
}

void v_drvPTC_suspendSensor(UINT8 ui8_sensorNode)
{	
	qtm_key_suspend(ui8_sensorNode, &qtlib_key_set1);
}

void v_drvPTC_resumeSensor(UINT8 ui8_sensorNode)
{
	qtm_key_resume(ui8_sensorNode, &qtlib_key_set1);
}

/*============================================================================
ISR(ADC0_RESRDY_vect)
------------------------------------------------------------------------------
Purpose:  Interrupt handler for ADC / PTC EOC Interrupt
Input    :  none
Output  :  none
Notes    :  none
============================================================================*/
ISR(ADC0_RESRDY_vect)
{
	qtm_t321x_ptc_handler_eoc();
}

#endif /* TOUCH_C */
