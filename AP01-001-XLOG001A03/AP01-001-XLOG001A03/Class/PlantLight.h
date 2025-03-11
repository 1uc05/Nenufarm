/* 
* PlantLight.h
*
* Created: 04/01/2024 12:59:09
* Author: LucasTeissier
*/

#ifndef __PLANTLIGHT_H__
#define __PLANTLIGHT_H__

#include "definitionTypes.h"

typedef enum {
	PLANTLIGHT_NO_STATUS,
	PLANTLIGHT_STOP,
	PLANTLIGHT_MAX,
	PLANTLIGHT_LIGHTING_UP,
	PLANTLIGHT_LIGHTING_DOWN,
	PLANTLIGHT_BREATHE,
	PLANTLIGHT_MANUAL,
	PLANTLIGHT_TIME_SETTING,
	PLANTLIGHT_BLINK_PB,
	PLANTLIGHT_BLINK_CONFIRMATION,
} PLANTLIGHT_STATUS;

struct PLANTLIGHT_CONFIG {
	BOOL					b_isManualMode;
	PLANTLIGHT_CONFIG_TIME	en_timeModeName;
};

#define BLINK_OFF									false		/* Utilisé si on veut un clignotement eteind */
#define BLINK_ON									true		/* Utilisé si on veut un clignotement allumé */
#define NB_BLINK_CONFIRMATION							2		/* Nombre de clignotement de confirmation */

class PlantLight
{
//variables
public:
protected:
private:
	BOOL				m_b_isBreatheUp				= false;
	BOOL				m_b_needToBlink				= false;
	BOOL				m_b_isBlinking				= false;
	UINT8				m_ui8_plantLightInstance	= 0;
	UINT8				m_ui8_tmpLightingValue		= 0;
	UINT8				m_ui8_blinkIterator			= 0;
	UINT8				m_ui8_numberOfBlink			= 0;
	UINT16				m_ui16_plantLightTimer		= 0;
	PLANTLIGHT_STATUS	m_en_currentStatus			= PLANTLIGHT_NO_STATUS;
	PLANTLIGHT_STATUS	m_en_nextStatus				= PLANTLIGHT_NO_STATUS;
	PLANTLIGHT_CONFIG	m_st_config;
	BLINK_CONFIG		m_st_blinkConfig;

//functions
public:
						PlantLight						();
						~PlantLight						();
	void				v_PlantLight_initialization		();
	void				v_PlantLight_configuration		(const PLANTLIGHT_CONFIG_TIME en_timeMode);
	void				v_PlantLight_event				(const SCHEDULED_EVENT en_scheduledEvent);
	void				v_PlantLight_manualMode			(const SCHEDULED_EVENT en_plantLightEvent);
	void				v_PlantLight_timeSettingMode	(const BOOL b_isCurrentMode);
	void				v_PlantLight_enterSettingMode	();
	void				v_PlantLight_exitSettingMode	(const SCHEDULED_EVENT en_nextEvent);
	void				v_PlantLight_activateBlinkPb	(const SCHEDULED_EVENT en_scheduledEvent);
	void				v_PlantLight_configBlinkPb		();
	void				v_PlantLight_saveTimeMode		();
	void				v_PlantLight_blinkConfirmation	(const BOOL b_isPositifBlink, const UINT8 ui8_nbBlink);
	void				v_PlantLight_choseSoftMode		(const UINT8 ui8_blinkNumber);
	void				v_PlantLight_setBrightness		(const UINT8 ui8_brightnessValue) const;
	void				v_PlantLight_blinkConfirmSetTime();
	void				v_PlantLight_blinkSlow			(const BOOL b_isPositifBlink, const UINT8 ui8_nbBlink);
	const BOOL			b_PlantLight_getIsBlinking		() const;
	
	const PLANTLIGHT_CONFIG_TIME en_PlantLight_getTimeModeName() const;

protected:
private:
	inline void			v_PlantLight_settingMode		(const BOOL b_enableSetting);
	void				v_PlantLight_lightingUp			();
	void				v_PlantLight_lightingDown		();
	void				v_PlantLight_breathe			();
	inline void			v_PlantLight_setBlinkDirection	(const BOOL isPositifBlink);
	void				v_PlantLight_blinkConfig		(const BOOL b_isPositifBlink, const UINT8 ui8_nbBlink, const UINT16 ui16_blinkTimeOff);
	void				v_PlantLight_blink				();

						PlantLight						(const PlantLight &c);
						PlantLight& operator=			(const PlantLight &c);
}; //PlantLight

#endif //__PLANTLIGHT_H__
