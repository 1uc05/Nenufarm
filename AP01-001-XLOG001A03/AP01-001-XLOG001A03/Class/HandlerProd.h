/* 
* HandlerProd.h
*
* Created: 29/01/2024 10:54:05
* Author: LucasTeissier
*/


#ifndef __HANDLERPROD_H__
#define __HANDLERPROD_H__

#include "Handler.h"

typedef enum {
	STATUS_PROD_MODE_ON_OFF_LIGHT,
	STATUS_PROD_MODE_PUMP_SETTING,
	STATUS_PROD_MODE_SOFT_SETTING,
	STATUS_PROD_MODE_DISPLAYING_SW_VERSION,
	STATUS_PROD_MODE_DISPLAYING_ON_OFF_BRICK,
	STATUS_PROD_MODE_DISPLAYING_CONFIRMATION,
} STATUS_PROD_MODE;

typedef enum {
	DISPLAYING_SW_VERSION_NO_STATUS,
	DISPLAYING_SW_VERSION_BREAK_INTEGER,
	DISPLAYING_SW_VERSION_INTEGER,
	DISPLAYING_SW_VERSION_BREAK_DECIMAL,
	DISPLAYING_SW_VERSION_DECIMAL,
	DISPLAYING_SW_VERSION_BREAK_END,
} DISPLAYING_SW_VERSION;

class HandlerProd : public Handler
{
//variables
public:
protected:
private:
	BOOL					m_b_isPumpConfigured			= false;
	BOOL					m_b_isOnPlantLight				= false;
	BOOL					m_b_isOnAquaLight				= false;
	BOOL					m_b_needToSaveFreq				= false;
	BOOL					m_b_isDisplayingSWVersion		= false;
	BOOL					m_b_isDisplayingDecimalSWVer	= false;
	BOOL					m_b_isSpeedPumpMax				= false;
	UINT8					m_ui8_speedPumpDC				= 0;
	UINT8					m_ui8_speedPumpDCTmp			= 0;
	UINT8					m_ui32_RPMCounter				= 0;
	UINT8					m_ui8_softVersionInteger		= 0;
	UINT8					m_ui8_softVersionDecimal		= 0;
	UINT32					m_ui32_RPMValueTmp				= 0;
	UINT32					m_ui32_handlerProdTimer			= 0;
	SCHEDULED_EVENT			m_en_plantLightEvent			= STOP_PLANTLIGHT_CYCLE;
	SCHEDULED_EVENT			m_en_aquaLightEvent				= STOP_AQUALIGHT_CYCLE;
	SCHEDULED_EVENT			m_en_pumpEvent					= STOP_PUMP_HIGH_LVL;
	STATUS_PROD_MODE		m_en_statusConf					= STATUS_PROD_MODE_ON_OFF_LIGHT;
	DISPLAYING_SW_VERSION	m_en_displayingSWVers			= DISPLAYING_SW_VERSION_NO_STATUS;

//functions
public:
						HandlerProd						();
						~HandlerProd					();
	virtual void		v_Handler_initialization		(Memory *pMe_memory,
														 Button *pBu_button,
														 PlantLight *pPl_plantLight,
														 AquaLight *pAl_aquaLight,
														 Pump *pPu_pump) override;
	virtual void		v_Handler_everyMilSecond		(const UINT16 ui16_rpmCounterValue, const BOOL b_isRpmRead) override;
	virtual void		v_Handler_everySecond			() override;
	virtual void		v_Handler_everyTime				() override;
	virtual void		v_Handler_saveTimeInMemory		() const override;
	
protected:
private:
	inline void			v_Handler_manageButtonEvent		();
	void				v_saveRPMValue					(UINT16 ui16_RPMValue);
	void				v_getSoftVersion				();

}; //HandlerProd

#endif //__HANDLERPROD_H__
