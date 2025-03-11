/* 
* HandlerProd.cpp
*
* Created: 29/01/2024 10:54:04
* Author: LucasTeissier
*/

#include "HandlerProd.h"

/* UI/UX parameters */
#define PLANTLIGHT_DC				6
#define AQUALIGHT_DC				3

#define NB_VALUE_TO_AVERAGE			6
#define DEFAULT_MIN_DC				23

// default constructor
HandlerProd::HandlerProd()
{
} //HandlerProd

// default destructor
HandlerProd::~HandlerProd()
{
} //~HandlerProd

void HandlerProd::v_Handler_initialization(Memory *pMe_memory, Button *pBu_button, PlantLight *pPl_plantLight, AquaLight *pAl_aquaLight, Pump *pPu_pump) 
{
	m_pMe_memory = pMe_memory;
	m_pBu_button = pBu_button;
	m_pPl_plantLight = pPl_plantLight;
	m_pAl_aquaLight = pAl_aquaLight;
	m_pPu_pump = pPu_pump;

	m_pMe_memory->v_Memory_clearMemory();

	m_ui8_speedPumpDC = DEFAULT_MIN_DC;
	m_ui8_speedPumpDCTmp = m_ui8_speedPumpDC;
	m_pPu_pump->v_Pump_configuration(m_pMe_memory->ui32_Memory_getPumpFrequency());
	LOG("Fréquence pompe : %lu", m_pMe_memory->ui32_Memory_getPumpFrequency());
	
	v_getSoftVersion();
}


void HandlerProd::v_Handler_everyMilSecond(const UINT16 ui16_rpmCounterValue, const BOOL b_isRpmRead)
{
	m_pPu_pump->v_Pump_event(m_en_pumpEvent, ui16_rpmCounterValue, b_isRpmRead);
	m_pPl_plantLight->v_PlantLight_event(m_en_plantLightEvent);
	//m_pAl_aquaLight->v_AquaLight_event(m_en_aquaLightEvent); // Inutile en HandlerProd
	
	if(m_b_needToSaveFreq & b_isRpmRead) {
		v_saveRPMValue(ui16_rpmCounterValue);
	} else {/* Nothing to do - Aucune valeur à enrigistrer */}
		
	/* clignotement pour indiquer la version du soft */
	if(m_en_statusConf == STATUS_PROD_MODE_DISPLAYING_SW_VERSION) {
		
		switch(m_en_displayingSWVers) {
			case DISPLAYING_SW_VERSION_BREAK_INTEGER:
				/* Etape 1: Pause entre l'appui et le début d'affichage des entiers si la version du soft est >= 1.X */
				if(m_ui8_softVersionInteger > 0) {
					m_ui32_handlerProdTimer++;

					if(m_ui32_handlerProdTimer > 1000) {
						m_ui32_handlerProdTimer = 0;
						m_en_displayingSWVers = DISPLAYING_SW_VERSION_INTEGER;
						m_pPl_plantLight->v_PlantLight_blinkSlow(BLINK_ON, m_ui8_softVersionInteger);
					} else {/* Nothing to do - En attente de la fin de la pause */}
				} else {
					m_en_displayingSWVers = DISPLAYING_SW_VERSION_BREAK_DECIMAL;
				}
			break;
			
			case DISPLAYING_SW_VERSION_INTEGER:
				/* Etape 2: Attente fin affichage des entiers */
				if(!m_pPl_plantLight->b_PlantLight_getIsBlinking()) {
					m_en_displayingSWVers = DISPLAYING_SW_VERSION_BREAK_DECIMAL;
				} else {/* Nothing to do - En attente de la fin de l'affichage des entiers */}
			break;
			
			case DISPLAYING_SW_VERSION_BREAK_DECIMAL:
				/* Etape 3: Pause entre entiers et décimales */
				m_ui32_handlerProdTimer++;
			
				if(m_ui32_handlerProdTimer > 1000) {
					m_en_displayingSWVers = DISPLAYING_SW_VERSION_DECIMAL;
					m_ui32_handlerProdTimer = 0;
					m_pPl_plantLight->v_PlantLight_blinkSlow(BLINK_ON, m_ui8_softVersionDecimal);
				} else {/* Nothing to do - En attente de la fin de la pause */}
			break;
			
			case DISPLAYING_SW_VERSION_DECIMAL:
				/* Etape 4: Attente fin affichage des décimales */
				if(!m_pPl_plantLight->b_PlantLight_getIsBlinking()) {
					m_en_displayingSWVers = DISPLAYING_SW_VERSION_BREAK_END;
				} else {/* Nothing to do - En attente de la fin de l'affichage des décimales */}
			break;
			
			case DISPLAYING_SW_VERSION_BREAK_END:
				/* Etape 5: Pause avant de reprendre le mode de config */
				m_ui32_handlerProdTimer++;
						
				if(m_ui32_handlerProdTimer > 1000) {
					m_en_displayingSWVers = DISPLAYING_SW_VERSION_NO_STATUS;
					m_en_statusConf = STATUS_PROD_MODE_SOFT_SETTING;
					m_pPl_plantLight->v_PlantLight_enterSettingMode();
					m_ui32_handlerProdTimer = 0;
				} else {/* Nothing to do - En attente de la fin de la pause */}
			break;
			
			default:
				/* Nothing to do */
			break;
		}	
	} else {/* Nothing to do - Le mode n'est pas activé */}
		
	/* Attente de la fin de l'affichage de on/off brick pour repasser en mode respiration */
	if(m_en_statusConf == STATUS_PROD_MODE_DISPLAYING_ON_OFF_BRICK) {
		if(!m_pPl_plantLight->b_PlantLight_getIsBlinking()) {
			m_pPl_plantLight->v_PlantLight_enterSettingMode();
			m_en_statusConf = STATUS_PROD_MODE_SOFT_SETTING;
		} else {/* Nothing to do - L'affichage est toujours en cours */}
	} else {/* Nothing to do - Le mode n'est pas activé */}
		
	/* Attente de la fin de l'affichage de validation  pour repasser en mode respiration */
	if(m_en_statusConf == STATUS_PROD_MODE_DISPLAYING_CONFIRMATION) {
		if(!m_pPl_plantLight->b_PlantLight_getIsBlinking()) {
			m_pPl_plantLight->v_PlantLight_enterSettingMode();
			m_en_statusConf = STATUS_PROD_MODE_PUMP_SETTING;
		} else {/* Nothing to do - L'affichage est toujours en cours */}
	} else {/* Nothing to do - Le mode n'est pas activé */}
}

void HandlerProd::v_Handler_everySecond()
{
	/* Nothing to do - Aucune action programmée dans ce Handler
	 * /!\ NE PAS UTILISER LE SCHEDULER /!\ :
	 * Des évènements HandlerProd pourraient être supprimés (ex: PUMP_VARIATION_SPEED_PROD)
	 */
}

void HandlerProd::v_Handler_everyTime()
{
	v_Handler_manageButtonEvent();
}

inline void HandlerProd::v_Handler_manageButtonEvent()
{
	const BUTTON_EVENT en_buttonEvent = m_pBu_button->en_Button_getEvent();

	switch(en_buttonEvent) {
/***** Bouton aquarium *****/
	/* Appui simple */
		case BUTTON_EVENT_AQUA_SIMPLE_PUSH:
			switch(m_en_statusConf) {
				/* On/Off Lumière */
				case STATUS_PROD_MODE_ON_OFF_LIGHT:
					if(m_b_isOnAquaLight) {
						m_b_isOnAquaLight = false;
						m_pAl_aquaLight->v_AquaLight_setBrightness(0);
					} else {
						m_b_isOnAquaLight = true;
						m_pAl_aquaLight->v_AquaLight_setBrightness(AQUALIGHT_DC);
					}
					break;
					
				/* Incrémentation de la fréquence pompe */
				case STATUS_PROD_MODE_PUMP_SETTING:
					m_pPu_pump->v_Pump_variationSpeedProd();
					m_ui8_speedPumpDCTmp++;
					LOG("	+1 DC Pump : %d", m_ui8_speedPumpDCTmp);
					m_pPu_pump->v_Pump_setSpeedPumpProd(m_ui8_speedPumpDCTmp);
					break;
				
				/* Indique la version du soft */
				case STATUS_PROD_MODE_SOFT_SETTING:
						m_en_statusConf = STATUS_PROD_MODE_DISPLAYING_SW_VERSION;
						m_en_displayingSWVers = DISPLAYING_SW_VERSION_BREAK_INTEGER;
						m_pPl_plantLight->v_PlantLight_blinkSlow(BLINK_OFF, 0);
					break;
					
				default:
					LOG("CodeError : cas switch non traité");
					break;
				}
			break;
	
	/* Appui 3 secondes */
		case BUTTON_EVENT_AQUA_SHORT_PUSH:
			switch(m_en_statusConf) {
				/* Rentre dans le mode reglage vitesse pompe */
				case STATUS_PROD_MODE_ON_OFF_LIGHT:
					m_en_statusConf = STATUS_PROD_MODE_PUMP_SETTING;
					m_pPl_plantLight->v_PlantLight_enterSettingMode();
					LOG("Reglage vitesse pompe ON");
					m_pPu_pump->v_Pump_startingMode(WITHOUT_WAITING);
					break;
					
				/* Sort des reglages vitesse pompe sans sauvegarder */
				case STATUS_PROD_MODE_PUMP_SETTING:
					m_en_statusConf = STATUS_PROD_MODE_ON_OFF_LIGHT;
					m_pPl_plantLight->v_PlantLight_exitSettingMode(OFF_MANUAL_LIGHT_MODE);
					LOG("Reglage vitesse pompe OFF");
					m_ui8_speedPumpDCTmp = m_ui8_speedPumpDC;
					m_pPu_pump->v_Pump_stopPumpProd();
					break;
					
				default:
					LOG("CodeError : cas switch non traité");
					break;
			}
			break;

/***** Bouton plante *****/
	/* Appui simple */
		case BUTTON_EVENT_PLANT_SIMPLE_PUSH:
			switch(m_en_statusConf) {
				/* On/Off Lumière */
				case STATUS_PROD_MODE_ON_OFF_LIGHT:
					if(m_b_isOnPlantLight) {
						m_b_isOnPlantLight = false;
						m_pPl_plantLight->v_PlantLight_setBrightness(0);
					} else {
						m_b_isOnPlantLight = true;
						m_pPl_plantLight->v_PlantLight_setBrightness(PLANTLIGHT_DC);
					}
					break;
					
				/* Décrémentaion de la fréquence pompe */
				case STATUS_PROD_MODE_PUMP_SETTING:
					m_pPu_pump->v_Pump_variationSpeedProd();
					m_ui8_speedPumpDCTmp--;
					LOG("	-1 DC Pump : %d", m_ui8_speedPumpDCTmp);
					m_pPu_pump->v_Pump_setSpeedPumpProd(m_ui8_speedPumpDCTmp);
					break;
					
					default:
						LOG("CodeError : cas switch non traité");
						break;
						
				/* Active/désactive le brickage produit */
				case STATUS_PROD_MODE_SOFT_SETTING:
					m_en_statusConf = STATUS_PROD_MODE_DISPLAYING_ON_OFF_BRICK;
					if(m_pMe_memory->b_Memory_isBrickedModeDisable()) {
						LOG("Activation du mode brickage");
						m_pMe_memory->v_Memory_enableBrickMode();
						m_pPl_plantLight->v_PlantLight_blinkConfirmation(BLINK_ON, ONE_BLINK);
					} else {
						LOG("Désactivation du mode brickage");
						m_pMe_memory->v_Memory_disableBrickMode();
						m_pPl_plantLight->v_PlantLight_blinkConfirmation(BLINK_ON, TWO_BLINK);
					}
					break;
			}
			break;
	
	/* Appui 3 secondes */
		case BUTTON_EVENT_PLANT_SHORT_PUSH:
			switch(m_en_statusConf) {
				/* Rentre dans les réglages soft */
				case STATUS_PROD_MODE_ON_OFF_LIGHT:
					m_en_statusConf = STATUS_PROD_MODE_SOFT_SETTING;
					m_pPl_plantLight->v_PlantLight_enterSettingMode();
					break;
					
				/* Sort des réglages soft */
				case STATUS_PROD_MODE_SOFT_SETTING:
					m_en_statusConf = STATUS_PROD_MODE_ON_OFF_LIGHT;
					m_pPl_plantLight->v_PlantLight_exitSettingMode(OFF_MANUAL_LIGHT_MODE);
					break;
				
				/* Sauvegarde et sort du mode reglage pompe */
				case STATUS_PROD_MODE_PUMP_SETTING:
					m_en_statusConf = STATUS_PROD_MODE_ON_OFF_LIGHT;
					LOG("Frequence save, DC : %d", m_ui8_speedPumpDCTmp);
					LOG("Reglage vitesse pompe OFF");
					m_pPl_plantLight->v_PlantLight_blinkConfirmation(BLINK_ON, TWO_BLINK);
					m_ui8_speedPumpDC = m_ui8_speedPumpDCTmp;
					m_b_needToSaveFreq = true;
					break;
					
				default:
					LOG("CodeError : cas switch non traité - %d", m_en_statusConf);
					break;
				}
			break;

/***** Bouton aqua + plante *****/
	/* Appui simple */
		case BUTTON_EVENT_AQUA_PLANT_SIMPLE_PUSH:
			/* Active la pompe à 100% */
			if(m_en_statusConf == STATUS_PROD_MODE_PUMP_SETTING) {
				m_pPu_pump->v_Pump_variationSpeedProd();
				LOG("	DC Pump : 100");
				if(m_b_isSpeedPumpMax) {
					m_b_isSpeedPumpMax = false;
					m_pPu_pump->v_Pump_setSpeedPumpProd(0);
				} else {
					m_b_isSpeedPumpMax = true;
					m_pPu_pump->v_Pump_setSpeedPumpProd(100);
				}
			} else {/* Nothing to do - Aucune intéraction de configurée */}
			break;

	/* Appui 3 secondes */
		case BUTTON_EVENT_AQUA_PLANT_SHORT_PUSH:
			/* Supprime le réglage pompe en mémoire */
			if(m_en_statusConf == STATUS_PROD_MODE_PUMP_SETTING) {
				m_en_statusConf = STATUS_PROD_MODE_DISPLAYING_CONFIRMATION;
				m_ui8_speedPumpDC = DEFAULT_MIN_DC;
				m_pMe_memory->v_Memory_clearFrequency();
				m_pPl_plantLight->v_PlantLight_blinkConfirmation(BLINK_OFF, TWO_BLINK);
				m_pPu_pump->v_Pump_configuration(LOW_LEVEL_FREQUENCY_VALUE);
			} else {/* Nothing to do - Aucune intéraction de configurée */}
			break;

		default:
			/* Nothing to do - Pas de config sur les autres appuis bouton */
			break;		
	}
}

void HandlerProd::v_Handler_saveTimeInMemory() const
{
	/* Nothing to do - Implémentation d'une fonction virtuelle
	 * Pas de sauvegarde mémoire en HandlerProd */
}

void HandlerProd::v_saveRPMValue(UINT16 ui16_RPMValue)
{
	m_ui32_RPMValueTmp += ui16_RPMValue;

	m_ui32_RPMCounter++;

	if(m_ui32_RPMCounter >= NB_VALUE_TO_AVERAGE) {
		m_b_needToSaveFreq = false;
		m_ui32_RPMCounter = 0;
		m_ui32_RPMValueTmp /= NB_VALUE_TO_AVERAGE;
		m_pPu_pump->v_Pump_configuration(m_ui32_RPMValueTmp);
		m_pPu_pump->v_Pump_stopPumpProd();
		m_pMe_memory->v_Memory_saveSpeedPumpFreq(m_ui32_RPMValueTmp);
		m_ui32_RPMValueTmp = 0;
	} else {/* Nothing to do - En attente de toutes les valeurs pour moyenner */}
}

void HandlerProd::v_getSoftVersion(void)
{
	const char *pc_version				= SOFT_VERSION;
	BOOL		b_numberOverflow		= false;
	UINT8		ui8_versionIterator		= 0;
	UINT8		ui8_OverflowIterator	= 0;
	
	/* Extraction de la partie entière de la version du logiciel */
	while ((pc_version[ui8_versionIterator] != '.') && (pc_version[ui8_versionIterator] != '\0') && (b_numberOverflow == false)) {
		if(ui8_versionIterator >= 2) {
			b_numberOverflow = true;
		} else {
			m_ui8_softVersionInteger = m_ui8_softVersionInteger * 10 + (pc_version[ui8_versionIterator] - '0');
			ui8_versionIterator++;
		}
	}

	/* Le "." est trouvé, pas d'erreur, passe le point */
	if ((pc_version[ui8_versionIterator] == '.') && (b_numberOverflow == false)) {
		ui8_versionIterator++;
		
		/* Extraction de la partie décimale de la version du logiciel (inférieur à 6 pour 2 numéros max XX.XX) */
		while ((pc_version[ui8_versionIterator] != '\0') && (b_numberOverflow == false)) {
			if(ui8_OverflowIterator >= 2) {
				b_numberOverflow = true;
			} else {
				m_ui8_softVersionDecimal = m_ui8_softVersionDecimal * 10 + (pc_version[ui8_versionIterator] - '0');
				ui8_versionIterator++;
				ui8_OverflowIterator++;
			}
		}
		
		/* Fin de chaine non trouvé, ou overflow détecté */
		if ((pc_version[ui8_versionIterator] != '\0') && (b_numberOverflow == true)) {
			m_ui8_softVersionInteger = 0;
			m_ui8_softVersionDecimal = 0;
		} else {/* Nothing to do - Pas d'erreur */}
		
	} else {
		/* Point non trouvé, ou overflow détecté */
		m_ui8_softVersionInteger = 0;
	}
}