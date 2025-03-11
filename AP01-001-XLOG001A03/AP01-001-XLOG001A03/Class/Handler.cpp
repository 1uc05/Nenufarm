/* 
* Handler.cpp
*
* Created: 12/01/2024 11:59:00
* Author: LucasTeissier
*/

#include "Handler.h"

// default constructor
Handler::Handler()
{
} //Handler

// default destructor 
Handler::~Handler()
{
} //~Handler

void Handler::v_Handler_initialization(Memory *pMe_memory, Button *pBu_button, PlantLight *pPl_plantLight, AquaLight *pAl_aquaLight, Pump *pPu_pump)
{}

void Handler::v_Handler_everyMilSecond(UINT16 ui16_rpmCounterValue, BOOL b_isRpmRead)
{}

void Handler::v_Handler_everySecond()
{}

void Handler::v_Handler_everyTime()
{}

void Handler::v_Handler_saveTimeInMemory() const
{}