/*
* Pump.h
*
* Created: 06/12/2023 21:02:49
* Author: LucasTeissier
*/

#ifndef __PPUMP_H__
#define __PPUMP_H__

#include "definitionTypes.h"

typedef enum {
	PUMP_NO_STATUS,
	PUMP_STOP,
	PUMP_GOTO_SPEED_MIN,
	PUMP_SPEED_MIN,
	PUMP_SPEED_MAX,
	PUMP_PROBLEM,
	PUMP_PRIMING,
	PUMP_WAITING_BEFORE_PRIMING,
	PUMP_VARIATION_SPEED_PROD,
} PUMP_STATUS;

typedef enum {
	STEP_MAX_DC,
	STEP_MIDRANGE_DC,
	STEP_DECREASE_DC,
	STEP_WAITING,
} FIND_MIN_DC;

#define WITH_WAITING		1
#define WITHOUT_WAITING		0

#define DEBUG_VALUE_SIZE	100

class Pump
{
//variables
public:
protected:
private:
	BOOL				m_b_isRpmRead							= false;
	BOOL				m_b_isPumpPiming						= false;
	BOOL				m_b_isMinorError						= false;
	BOOL				m_b_isReadyToDecrease					= false;
	BOOL				m_b_isFirstMinorError					= false;
	BOOL				m_b_isFrequencyConfigured				= false;
	BOOL				m_b_isRpmGood							= false;
	UINT8				m_ui8_pumpInstance						= 0;
	UINT8				m_ui8_actualDCValue						= 0;
	UINT16				m_ui16_rpmAverageNumber					= 0;
	UINT16				m_ui16_pumpTimer						= 0;
	UINT16				m_ui16_readingHWDelayTimer				= 0;
	UINT16				m_ui16_lowLevelFreqValue				= 0;
	UINT16				m_ui16_lowLevelFreqValueLimit			= 0;
	UINT16				m_tui16_debugRPMValue[DEBUG_VALUE_SIZE]	= {0};
	UINT32				m_ui32_rpmCounterAverageValue			= 0;
	FIND_MIN_DC			m_en_stepFindMinDC						= STEP_MAX_DC;
	PUMP_STATUS			m_en_currentStatus						= PUMP_STOP;

//functions
public:
						Pump					();
						~Pump					();
	void				v_Pump_initialization	();
	void				v_Pump_configuration	(const UINT16 ui16_lowLevelFrequency);
	void				v_Pump_event			(const SCHEDULED_EVENT en_scheduledAction, const UINT16 ui16_rpmCounterValue, const BOOL b_flagRpmRead);
	const BOOL			b_Pump_isMinorError		() const;
	void				v_Pump_disableMinorError();
	void				v_Pump_startingMode		(BOOL b_withWatingMode);
	void				v_Pump_stopPumpProd			();
	void				v_Pump_gotoMinDutyCycle	();
	void				v_Pump_setSpeedPumpProd	(const UINT8 ui8_frequency);
	void				v_Pump_variationSpeedProd();

protected:
private:
	inline void			v_Pump_checkSpeedPump	(const UINT16 ui16_rpmCounterValue, const BOOL b_isRpmRead);
	inline void			v_Pump_checkMaxSpeed	();
	inline void			v_Pump_checkMinSpeed	();
	inline void			v_Pump_checkGotoSpeedMin();
	void				v_Pump_changeDutyCycle	(const UINT8 ui8_dutyCycle) const;
	void				v_Pump_pumpPriming		();
	void				v_Pump_problemMode		();
	void				v_Pump_logDebugData		() const;
	
				Pump					(const Pump &c);
				Pump& operator=			(const Pump &c);
}; //Pump

#endif //__PUMP_H__
