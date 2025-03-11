/* 
* StartManager.h
*
* Created: 29/01/2024 15:10:54
* Author: LucasTeissier
*/

#ifndef __STARTMANAGER_H__
#define __STARTMANAGER_H__

#include "Button.h"
#include "PlantLight.h"
#include "AquaLight.h"
#include "Pump.h"

typedef enum {
	PROD_SOFT,
	CLIENT_SOFT,
	EXPO_SOFT,
} SOFT_MODE;

class StartManager
{
//variables
public:
protected:
private:
	BOOL			m_b_isChoseSoftMode		= false;
	BOOL			m_b_isSoftChoose		= false;
	UINT8			m_ui8_softModeIterator	= 0;
	UINT16			m_ui16_timer			= 0;
	SOFT_MODE		m_en_softChosen			= CLIENT_SOFT;
	Button			*m_pBu_button			= nullptr;
	PlantLight		*m_pPl_plantLight		= nullptr;

//functions
public:
					StartManager							();
					~StartManager							();
	void			v_StartManager_initialization			(Button *pBu_button,
															 PlantLight *pPl_plantLight,
															 AquaLight *pAl_aquaLight,
															 Pump *pPu_pump);
	void			v_StartManagerr_everyMilSecond			() const;
	void			v_StartManagerr_everyTime				();
	BOOL			b_StartManager_isTimeout				();
	const SOFT_MODE	en_startManager_getSoftChoice			() const;
	
protected:
private:
	inline void		v_StartManager_choseTheSoft				();
					StartManager							(const StartManager &c);
					StartManager& operator=					(const StartManager &c);

}; //StartManager

#endif //__STARTMANAGER_H__
