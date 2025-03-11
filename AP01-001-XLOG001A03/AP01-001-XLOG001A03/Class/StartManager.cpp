/* 
* StartManager.cpp
*
* Created: 29/01/2024 15:10:54
* Author: LucasTeissier
*/

#include "StartManager.h"
#include "drvHardware.h"
#include "drvTimerSysteme.h"
#include "drvRTC.h"

/* UI/UX parameter */
#define CHOOSE_SOFT_TIMEOUT		3500	/* Temps laissé pour le choix du soft (en ms) */
#define NB_SOFT_MODE			2		/* Nombre de soft disponible */

// default constructor
StartManager::StartManager()
{
} //StartManager

// default destructor
StartManager::~StartManager()
{
} //~StartManager

void StartManager::v_StartManager_initialization(Button *pBu_button, PlantLight *pPl_plantLight, AquaLight *pAl_aquaLight, Pump *pPu_pump)
{	
	v_drvHardware_initialization();
	v_drvTimerSysteme_initialization();
	v_drvRTC_initialization();

	m_pBu_button = pBu_button;
	m_pBu_button->v_Button_initialization();
	
	m_pPl_plantLight = pPl_plantLight;
	pPl_plantLight->v_PlantLight_initialization();
	pAl_aquaLight->v_AquaLight_initialization();
	pPu_pump->v_Pump_initialization();
}

void StartManager::v_StartManagerr_everyMilSecond() const
{
	/* Déclenchement des états de l'éclairage (clignotement) */
	m_pPl_plantLight->v_PlantLight_event(STOP_PLANTLIGHT_CYCLE);
}

void StartManager::v_StartManagerr_everyTime()
{
	/* Scan des boutons */
	v_StartManager_choseTheSoft();
}

inline void StartManager::v_StartManager_choseTheSoft()
{
	const BUTTON_EVENT en_buttonEvent = m_pBu_button->en_Button_getEvent();
	
	/* Activation du choix de Soft */
	if(en_buttonEvent == BUTTON_EVENT_AQUA_SHORT_PUSH) {
		m_ui16_timer = 0;
		m_b_isChoseSoftMode = true;		
		m_pPl_plantLight->v_PlantLight_choseSoftMode(m_ui8_softModeIterator);
		
	} else {/* Nothing to do - Appui 3sec bouton plante non détecté */}
	
	/* Défilement des Soft */
	if(en_buttonEvent == BUTTON_EVENT_AQUA_SIMPLE_PUSH && m_b_isChoseSoftMode) {
		m_ui16_timer = 0;
		m_ui8_softModeIterator++;
		if(m_ui8_softModeIterator > NB_SOFT_MODE) {
			m_ui8_softModeIterator = 0;
		} else {/* Nothing to do - L'incrémentation n'a pas atteint le nb max de mode */}
		
		m_pPl_plantLight->v_PlantLight_choseSoftMode(m_ui8_softModeIterator);
	} else {/* Nothing to do - Appui court bouton aqua non détecté en ChoseSoftMode*/}
		
	/* Sauvegarde du choix du Soft */
	if(en_buttonEvent == BUTTON_EVENT_PLANT_SIMPLE_PUSH && m_b_isChoseSoftMode) {
		m_b_isSoftChoose = true;
		
		m_pPl_plantLight->v_PlantLight_exitSettingMode(STOP_PLANTLIGHT_CYCLE);
		
		switch(static_cast<SOFT_MODE>(m_ui8_softModeIterator)) {
			case PROD_SOFT:
				m_en_softChosen = PROD_SOFT;
				break;
				
			case CLIENT_SOFT:
				m_en_softChosen = CLIENT_SOFT;
				break;
				
			case EXPO_SOFT:
				//m_en_softChosen = EXPO_SOFT;
				break;
		}
	} else {/* Nothing to do - Appui court bouton plante non détecté en ChoseSoftMode */}
}

BOOL StartManager::b_StartManager_isTimeout()
{
	return ((++m_ui16_timer >= CHOOSE_SOFT_TIMEOUT) || m_b_isSoftChoose)  ? true : false;
}

const SOFT_MODE StartManager::en_startManager_getSoftChoice() const
{
	return m_en_softChosen;
}

/* Optimisation du StartManager:
- Créer en global un Handler mère,
- Demander le choix du soft,
- StartManager crée et initialise les instance de Pump, AquaLight et PlantLight,
- Il crée le Handler fille et lui attribut les pointeur des instances
- Il retourne le Handler fille dans le Handler global (polymorphisme)
- StartManager est détruit
-> Seule une instance de chaque classe est créé et une seule boucle while(1) controle les Handler
PB: pas possible de faire de polymorphisme ni d'allocation dynamique
-> pas possible d'ecraser le Handler mère par une fille
-> pas possible d'utiliser new et delet (gcc limité en c++, lib ATTiny réduite sur archi AVR)
*/