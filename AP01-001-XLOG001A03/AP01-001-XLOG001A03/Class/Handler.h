/* 
* Handler.h
*
* Created: 12/01/2024 11:59:01
* Author: LucasTeissier
*/


#ifndef __HANDLER_H__
#define __HANDLER_H__

#include "drvHardware.h"	/* Utilisé pour le reset */
#include "Memory.h"
#include "Scheduler.h"
#include "Button.h"
#include "PlantLight.h"
#include "AquaLight.h"
#include "Pump.h"

#define ONE_BLINK					1
#define TWO_BLINK					2
#define THREE_BLINK					3

class Handler
{
//variables
public:
private:
protected:
	Memory				*m_pMe_memory		= nullptr;
	Button				*m_pBu_button		= nullptr;
	PlantLight			*m_pPl_plantLight	= nullptr;
	AquaLight			*m_pAl_aquaLight	= nullptr;
	Pump				*m_pPu_pump			= nullptr;

//functions
public:
						Handler							();
						~Handler						();
	virtual void		v_Handler_initialization		(Memory *pMe_memory,
														 Button *pBu_button,
														 PlantLight *pPl_plantLight,
														 AquaLight *pAl_aquaLight,
														 Pump *pPu_pump);
	virtual void		v_Handler_everyMilSecond		(UINT16 ui16_rpmCounterValue, BOOL b_isRpmRead);
	virtual void		v_Handler_everySecond			();
	virtual void		v_Handler_everyTime				();
	virtual void		v_Handler_saveTimeInMemory		() const;
	
private:
protected:
						Handler& operator=				(const Handler &c);
						Handler							(const Handler &c);
}; //Handler

#endif //__HANDLER_H__
