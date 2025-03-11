/* 
* HandlerExpo.cpp
*
* Created: 22/10/2024 16:15:19
* Author: LucasTeissier
*/

#include "HandlerExpo.h"

#define ONE_PUSH_STEP	5
#define MAX_VALUE		100
#define MIN_VALUE		0

// default constructor
HandlerExpo::HandlerExpo()
{
} //HandlerExpo

// default destructor
HandlerExpo::~HandlerExpo()
{
} //~HandlerExpo

void HandlerExpo::v_Handler_initialization(Memory *pMe_memory, Button *pBu_button, PlantLight *pPl_plantLight, AquaLight *pAl_aquaLight, Pump *pPu_pump)
{
	m_pBu_button = pBu_button;
	m_pPl_plantLight = pPl_plantLight;
	m_pAl_aquaLight = pAl_aquaLight;
	
	m_pPl_plantLight->v_PlantLight_setBrightness(m_i8_valueBrightnessPL);
	m_pAl_aquaLight->v_AquaLight_setBrightness(m_i8_valueBrightnessAL);
}

void HandlerExpo::v_Handler_everyMilSecond(const UINT16 ui16_rpmCounterValue, const BOOL b_isRpmRead)
{
}

void HandlerExpo::v_Handler_everySecond()
{
	/* Nothing to do - Aucune action programmée dans ce Handler
	 * /!\ NE PAS UTILISER LE SCHEDULER /!\ :
	 * Des évènements HandlerProd pourraient être supprimés (ex: PUMP_VARIATION_SPEED_PROD)
	 */
}

void HandlerExpo::v_Handler_everyTime()
{
	v_Handler_manageButtonEvent();
}

inline void HandlerExpo::v_Handler_manageButtonEvent()
{
	const BUTTON_EVENT en_buttonEvent = m_pBu_button->en_Button_getEvent();

	switch(en_buttonEvent) {
		/***** Bouton aquarium *****/
		/* Appui simple */
		case BUTTON_EVENT_AQUA_SIMPLE_PUSH:
			switch(m_en_statusConf) {
				case STATUS_EXPO_MODE_BRIGHTNESS_AL:
					m_i8_valueBrightnessAL += ONE_PUSH_STEP;
					m_i8_valueBrightnessAL = i8_Handler_checkValue(m_i8_valueBrightnessAL);
					m_pAl_aquaLight->v_AquaLight_setBrightness(m_i8_valueBrightnessAL);
					break;
				case STATUS_EXPO_MODE_BRIGHTNESS_PL:
					m_i8_valueBrightnessPL += ONE_PUSH_STEP;
					m_i8_valueBrightnessPL = i8_Handler_checkValue(m_i8_valueBrightnessPL);
					m_pPl_plantLight->v_PlantLight_setBrightness(m_i8_valueBrightnessPL);
					break;
			}
			break;
			
		/* Appui 3 secondes */
		case BUTTON_EVENT_AQUA_SHORT_PUSH:
			m_en_statusConf = STATUS_EXPO_MODE_BRIGHTNESS_AL;
			break;

		/***** Bouton plante *****/
		/* Appui simple */
		case BUTTON_EVENT_PLANT_SIMPLE_PUSH:
			switch(m_en_statusConf) {
				case STATUS_EXPO_MODE_BRIGHTNESS_AL:
					m_i8_valueBrightnessAL -= ONE_PUSH_STEP;
					m_i8_valueBrightnessAL = i8_Handler_checkValue(m_i8_valueBrightnessAL);
					m_pAl_aquaLight->v_AquaLight_setBrightness(m_i8_valueBrightnessAL);
					break;
				case STATUS_EXPO_MODE_BRIGHTNESS_PL:
					m_i8_valueBrightnessPL -= ONE_PUSH_STEP;
					m_i8_valueBrightnessPL = i8_Handler_checkValue(m_i8_valueBrightnessPL);
					m_pPl_plantLight->v_PlantLight_setBrightness(m_i8_valueBrightnessPL);
					break;
			}
			break;
			
		/* Appui 3 secondes */
		case BUTTON_EVENT_PLANT_SHORT_PUSH:
			m_en_statusConf = STATUS_EXPO_MODE_BRIGHTNESS_PL;
			break;
			
		
		/***** Bouton aqua + plante *****/
		/* Appui 3 secondes */
		case BUTTON_EVENT_AQUA_PLANT_SHORT_PUSH:
			m_i8_valueBrightnessPL = MAX_VALUE;
			m_i8_valueBrightnessAL = MAX_VALUE;
			m_pPl_plantLight->v_PlantLight_setBrightness(m_i8_valueBrightnessPL);
			m_pAl_aquaLight->v_AquaLight_setBrightness(m_i8_valueBrightnessAL);
			break;
	}
}

INT8 HandlerExpo::i8_Handler_checkValue(INT8 i8_valueToCheck) const
{
	if(i8_valueToCheck > MAX_VALUE) {
		i8_valueToCheck = MAX_VALUE;	
	} else if(i8_valueToCheck < MIN_VALUE) {
		i8_valueToCheck = MIN_VALUE;	
	} else {/* Nothing to do - La valeur est correcte */}

	return i8_valueToCheck;
}