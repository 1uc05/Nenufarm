/* 
* Scheduler.h
*
* Created: 08/01/2024 15:27:32
* Author: LucasTeissier
*/

#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include "definitionTypes.h"
#include "Memory.h"
#include "PlantLight.h"
#include "AquaLight.h"

struct EVENT_TIME {
	BOOL			b_isBtwTwoDays;
	UINT32			ui32_time;
	SCHEDULED_EVENT en_name;
};

class Scheduler
{
//variables
public:	
protected:
private:
	BOOL				m_b_isManualModePlantLight		= false;
	BOOL				m_b_isManualModeAquaLight		= false;
	BOOL				m_b_isWaitingStopHManualTime	= false;
	UINT32				m_ui32_currentTime				= 0;
	SCHEDULED_EVENT		m_en_pumpEvent					= NO_EVENT;
	SCHEDULED_EVENT		m_en_plantLightEvent			= NO_EVENT;
	SCHEDULED_EVENT		m_en_aquaLightEvent				= NO_EVENT;
	EVENT_TIME			m_st_tempStartTimeAquaLight;
	EVENT_TIME			m_tst_pumpEvents[2];
	EVENT_TIME			m_tst_plantLightEvents[3];
	EVENT_TIME			m_tst_aquaLightEvents[3];

//functions
public:
							Scheduler							();
							~Scheduler							();
	void					v_Scheduler_initialization			(const SETTINGS st_settings);
	void					v_Scheduler_updateEvent				();
	const UINT32			ui32_Scheduler_getCurrentTime		() const;
	const SCHEDULED_EVENT	en_Scheduler_getPumpEvent			() const;
	const SCHEDULED_EVENT	en_Scheduler_getPlantLightEvent		() const;
	const SCHEDULED_EVENT	en_Scheduler_getAquaLightEvent		() const;
	const UINT32			en_Scheduler_getStartTimeManualAL	() const;
	void					v_Scheduler_setManualModePlantLight	();
	void					v_Scheduler_setManualModeAquaLight	();
	const BOOL				b_Scheduler_isWatingStopTimeModeAL	() const;
	
	SCHEDULED_EVENT			en_Scheduler_setTimeModePlantLight	(const PLANTLIGHT_CONFIG_TIME en_timeModeName);
	SCHEDULED_EVENT			en_Scheduler_setTimeModeAquaLight	(const AQUALIGHT_CONFIG_TIME en_timeModeName);
	void					v_Scheduler_setStopTimePlantLight	(const PLANTLIGHT_CONFIG_TIME en_timeModeName);
	void					v_Scheduler_setStopTimeAquaLight	(const AQUALIGHT_CONFIG_TIME en_timeModeName);
	
	void					v_Scheduler_stopManualModePlantLight();
	void					v_Scheduler_stopManualModeAquaLight	();
	
protected:
private:
	void					v_Scheduler_setTimePump				();
	void					v_Scheduler_updateEventPlantLight	();
	void					v_Scheduler_updateEventAquaLight	();
	void					v_Scheduler_updateEventPump			();
	inline const UINT32		v_Scheduler_calculateManualTime		() const;
			
							Scheduler							(const Scheduler &c);
							Scheduler& operator=				(const Scheduler &c);
}; //Scheduler

#endif //__SCHEDULER_H__
