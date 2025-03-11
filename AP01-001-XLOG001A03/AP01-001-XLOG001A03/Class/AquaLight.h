/* 
* AquaLight.h
*
* Created: 08/01/2024 18:43:39
* Author: LucasTeissier
*/


#ifndef __AQUALIGHT_H__
#define __AQUALIGHT_H__

#include "definitionTypes.h"

typedef enum {
	AQUALIGHT_NO_STATUS,
	AQUALIGHT_STOP,
	AQUALIGHT_MAX,
	AQUALIGHT_CRITICAL_PROBLEM,
	AQUALIGHT_LIGHTING_UP,
	AQUALIGHT_LIGHTING_DOWN,
	AQUALIGHT_BREATHE,
	AQUALIGHT_MANUAL_ON,
	AQUALIGHT_MANUAL_OFF,
	AQUALIGHT_INTENSITY_SETTING,
	AQUALIGHT_TIME_SETTING,
	AQUALIGHT_BLINK_CONFIRM_SET_HOUR,
	AQUALIGHT_TEST_ADC,
	AQUALIGHT_DEFINE_ADC_VALUE,
} AQUALIGHT_STATUS;

struct AQUALIGHT_CONFIG {
	BOOL						b_isManualMode;
	AQUALIGHT_CONFIG_INTENSITY	en_intensityModeName;
	UINT8						ui8_intensityValue;
	AQUALIGHT_CONFIG_TIME		en_timeModeName;
};

struct AQUALIGHT_LIMIT_ADC {
	UINT16						ui16_adcMaxLimitActual;
	UINT16						ui16_adcMinLimitActual;
	UINT16						ui16_adcMaxLimit20;
	UINT16						ui16_adcMinLimit20;
	UINT16						ui16_adcMaxLimit25;
	UINT16						ui16_adcMinLimit25;
	UINT16						ui16_adcMaxLimit30;
	UINT16						ui16_adcMinLimit30;
	UINT16						ui16_adcMaxLimit35;
	UINT16						ui16_adcMinLimit35;
	UINT16						ui16_adcMaxLimitOff;
};

typedef enum {
	AQUALIGHT_UNDEF_CONFIG,
	AQUALIGHT_20LUM_CONFIG,
	AQUALIGHT_25LUM_CONFIG,
	AQUALIGHT_30LUM_CONFIG,
	AQUALIGHT_35LUM_CONFIG,
} AQUALIGHT_ADC_CONFIG_STATUS;

class AquaLight
{
//variables
public:
protected:
private:
	BOOL				m_b_isBreatheUp				= false;
	BOOL				m_b_isCriticalError			= false;
	BOOL				m_b_isDeletedFirstCritErr	= false;
	BOOL				m_b_isADCConfigured			= true;
	BOOL				m_b_noBrickMode				= false;
	UINT8				m_ui8_aquaLightInstance		= 0;
	UINT8				m_ui8_tmpLightingValue		= 0;
	UINT8				m_ui8_instanceADC			= 0;
	UINT8				m_ui8_blinkIterator			= 0;
	UINT8				m_ui8_numberOfBlink			= 0;
	UINT8				m_ui8_intensityNumber		= 0;
	UINT16				m_ui16_aquaLightTimer		= 0;
	UINT16				m_ui16_readingHWTimer		= 0;
	AQUALIGHT_STATUS	m_en_currentStatus			= AQUALIGHT_NO_STATUS;
	AQUALIGHT_ADC_CONFIG_STATUS m_en_adcConfigStatus;
	BLINK_CONFIG		m_st_blinkConfig;
	AQUALIGHT_CONFIG	m_st_config;
	AQUALIGHT_ADC_VALUE m_st_adcValues;
	AQUALIGHT_LIMIT_ADC m_st_adcLimit = {0};



//functions
public:
						AquaLight						();
						~AquaLight						();
	void				v_AquaLight_initialization		();
	void				v_AquaLight_configuration		(const SETTINGS st_setttings);
	void				v_AquaLight_event				(const SCHEDULED_EVENT en_scheduledEvent);
	void				v_AquaLight_manualMode			(const SCHEDULED_EVENT en_aquaLightEvent);
	void				v_AquaLight_timeSettingMode		(const BOOL b_isCurrentMode);
	void				v_AquaLight_intensityMode		(const BOOL b_isCurrentMode);
	void				v_AquaLight_saveTimeMode		();
	void				v_AquaLight_saveIntensityMode	();
	void				v_AquaLight_enterSettingMode	();
	void				v_AquaLight_exitSettingMode		(const SCHEDULED_EVENT en_nextEvent);
	void				v_AquaLight_blinkConfirmSetHour	();
	void				v_AquaLight_setBrightness		(const UINT8 ui8_brightnessValue) const;
	void				v_AquaLight_testADC				();
	const BOOL			b_AquaLight_firstCritErrRemoved	() const;
	void				v_AquaLight_configADC			(const AQUALIGHT_ADC_VALUE st_adcValues);
	void				v_AquaLight_defineADCValue		();
	AQUALIGHT_ADC_VALUE st_AquaLight_getADCValues		() const;
	const BOOL			b_AquaLight_isADCCOnfigured		() const;
	
	const BOOL							b_AquaLight_isCriticalError		() const;
	const AQUALIGHT_CONFIG_TIME			en_AquaLight_getTimeModeName	() const;
	const AQUALIGHT_CONFIG_INTENSITY	en_AquaLight_getIntensityName	() const;

	void				v_AquaLight_setNoBrickMode		(const BOOL b_noBrickMode);
	
protected:
private:
	inline void			v_AquaLight_settingMode			(const BOOL b_enableSetting);
	void				v_AquaLight_lightingUp			();
	void				v_AquaLight_lightingDown		();
	void				v_AquaLight_checkADCValue		();
	void				v_AquaLight_breathe				();
	void				v_AquaLight_blink				();
	void				v_AquaLight_intensity			() const;
	inline void			v_AquaLight_setIntensityValue	(const AQUALIGHT_CONFIG_INTENSITY en_intensityModeName);
	void				v_AquaLight_calculateADCLimit	();
	void				v_AquaLight_defineActualADCLimit(const AQUALIGHT_CONFIG_INTENSITY en_intensityModeName);
	
						AquaLight						(const AquaLight &c);
						AquaLight& operator=			(const AquaLight &c);
}; //AquaLight

#endif //__AQUALIGHT_H__
