/* 
* Memory.h
*
* Created: 23/01/2024 17:13:33
* Author: LucasTeissier
*/

#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "Scheduler.h"

class Memory
{
//variables
public:
protected:
private:
	UINT32						m_ui32_currentTime			= 0;
	UINT32						m_ui32_stopTimePlantLight	= 0;
	UINT32						m_ui32_stopTimeAquaLight	= 0;
	PLANTLIGHT_CONFIG_TIME		m_en_timeModePlantLight		= H14_LIGHTING;
	AQUALIGHT_CONFIG_TIME		m_en_timeModeAquaLight		= H6_LIGHTING;
	AQUALIGHT_CONFIG_INTENSITY	m_en_intensityAquaLight		= LUMEN_25;
	
//functions
public:
					Memory								();
					~Memory								();
	const BOOL		b_Memory_isBricked					() const;
	void			v_Memory_loadSettings				(SETTINGS *st_settings);
	void			v_Memory_saveCurrentTime			(const UINT32 ui32_currentTime) const;
	void			v_Memory_saveStopTimePlantLight		(const UINT32 ui32_stopTimePlantLight);
	void			v_Memory_saveStopTimeAquaLight		(const UINT32 ui32_stopTimeAquaLight);
	void			v_Memory_saveTimeModePlantLight		(const PLANTLIGHT_CONFIG_TIME en_timeModePlantLight);
	void			v_Memory_saveTimeModeAquaLight		(const AQUALIGHT_CONFIG_TIME en_timeModeAquaLight);
	void			v_Memory_saveIntensityAquaLight		(const AQUALIGHT_CONFIG_INTENSITY en_intensityAquaLight);
	void			v_Memory_saveManualModeAquaLight	(const UINT32 ui32_startTime, const UINT32 ui32_stopTime);
	void			v_Memory_saveSpeedPumpFreq			(const UINT16 ui16_pumpFrequency) const;
	void			v_Memory_clearMemory				() const;
	const BOOL		b_Memory_isCriticalErrorRecording	() const;
	void			v_Memory_writeFirstCriticalError	() const;
	void			v_Memory_writeBrickCriticalError	() const;
	void			v_Memory_clearFirstCriticalError	() const;
	UINT32			ui32_Memory_getPumpFrequency		() const;
	void			v_Memory_clearFrequency				() const;
	void			v_Memory_getADCValues				(AQUALIGHT_ADC_VALUE * pst_adcValues) const;
	void			v_Memory_saveADCValues				(const AQUALIGHT_ADC_VALUE pst_adcValues) const;
	void			v_Memory_markAsAlreadyStarted		() const;
	BOOL			b_Memory_isBrickedModeDisable		() const;
	void			v_Memory_enableBrickMode			() const;
	void			v_Memory_disableBrickMode			() const;
	
protected:
private:
	void			v_Memory_svgFourByte				(const UINT8 ui8_adress, const UINT32 ui32_valueToSvg) const;

					Memory								(const Memory &c);
					Memory& operator=					(const Memory &c);

}; //Memory

#endif //__MEMORY_H__
