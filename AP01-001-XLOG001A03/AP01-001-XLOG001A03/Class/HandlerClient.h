/* 
* HandlerClient.h
*
* Created: 01/02/2024 12:35:39
* Author: LucasTeissier
*/


#ifndef __HANDLERCLIENT_H__
#define __HANDLERCLIENT_H__

#include "Handler.h"

typedef enum {
	STATUS_CONFIG_NO_SETTING_MODE,
	STATUS_CONFIG_SETTING_PLANTLIGHT,
	STATUS_CONFIG_SETTING_AQUALIGHT,
	STATUS_CONFIG_TIME_CYCLE_PLANTLIGHT,
	STATUS_CONFIG_TIME_CYCLE_AQUALIGHT,
	STATUS_CONFIG_INTENSITY_AQUALIGHT,
} STATUS_CONFIG;

class HandlerClient : public Handler
{
//variables
public:
protected:
private:
	Scheduler				m_Sc_scheduler;
	BOOL					m_b_isConfigured			= false;
	BOOL					m_b_isBricked				= false;
	BOOL					m_b_isMinorError			= false;
	BOOL					m_b_isInSettingMode			= false;
	BOOL					m_b_isWatingStopManualAL	= false;
	BOOL					m_b_isFirstCriticalErr		= false;
	BOOL					m_b_isWatingADCConfig		= false;
	BOOL					m_b_noBrickMode				= false;
	BOOL					m_b_isWatingBeforeReset		= false;
	BOOL					m_b_isWatingBeforeMinorErr	= false;
	UINT8					m_ui8_blinkPbTimer			= 0;
	UINT8					m_ui8_settingTimer			= 0;
	SCHEDULED_EVENT			m_en_pumpEvent				= NO_EVENT;
	SCHEDULED_EVENT			m_en_plantLightEvent		= NO_EVENT;
	SCHEDULED_EVENT			m_en_aquaLightEvent			= NO_EVENT;
	STATUS_CONFIG			m_en_statusConfig			= STATUS_CONFIG_NO_SETTING_MODE;

//functions
public:
						HandlerClient					();
						~HandlerClient					();
	virtual void		v_Handler_initialization		(Memory *pMe_memory,
														 Button *pBu_button,
														 PlantLight *pPl_plantLight,
														 AquaLight *pAl_aquaLight,
														 Pump *pPu_pump) override;
	const BOOL			b_Handler_isBricked				() const;
	virtual void		v_Handler_everyMilSecond		(const UINT16 ui16_rpmCounterValue, const BOOL b_isRpmRead) override;
	virtual void		v_Handler_everySecond			() override;
	virtual void		v_Handler_everyTime				() override;
	virtual void		v_Handler_saveTimeInMemory		() const override;
	
protected:	
private:
	inline void			v_Handler_manageButtonEvent		();
	inline void			v_Handler_toggleMinorError		(const BOOL b_isMinorError);
	void				v_Handler_toggleCriticalError	() const;
	void				v_Handler_resetProduct			();
	void				v_Handler_autoSaveSetting		();
	void				v_Handler_saveMemoryTimeModeAL	(const AQUALIGHT_CONFIG_TIME en_configToSaved);
						HandlerClient					(const HandlerClient &c);
						HandlerClient& operator=		(const HandlerClient &c);

}; //HandlerClient

#endif //__HANDLERCLIENT_H__
