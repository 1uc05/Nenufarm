/* 
* HandlerExpo.h
*
* Created: 22/10/2024 16:15:19
* Author: LucasTeissier
*/


#ifndef __HANDLEREXPO_H__
#define __HANDLEREXPO_H__

#include "Handler.h"

typedef enum {
	STATUS_EXPO_MODE_NO8MODE,
	STATUS_EXPO_MODE_BRIGHTNESS_PL,
	STATUS_EXPO_MODE_BRIGHTNESS_AL,
} STATUS_EXPO_MODE;

class HandlerExpo : public Handler
{
//variables
public:
protected:
private:
	INT8					m_i8_valueBrightnessPL		= 100;
	INT8					m_i8_valueBrightnessAL		= 100;
	STATUS_EXPO_MODE		m_en_statusConf				= STATUS_EXPO_MODE_NO8MODE;

//functions
public:
						HandlerExpo();
						~HandlerExpo();
	virtual void		v_Handler_initialization		(Memory *pMe_memory,
														 Button *pBu_button,
														 PlantLight *pPl_plantLight,
														 AquaLight *pAl_aquaLight,
														 Pump *pPu_pump) override;
	virtual void		v_Handler_everyMilSecond		(const UINT16 ui16_rpmCounterValue, const BOOL b_isRpmRead) override;
	virtual void		v_Handler_everySecond			() override;
	virtual void		v_Handler_everyTime				() override;
	
protected:
private:
	inline void			v_Handler_manageButtonEvent		();
	INT8				i8_Handler_checkValue			(INT8 i8_valueToCheck) const;

}; //HandlerExpo

#endif //__HANDLEREXPO_H__
