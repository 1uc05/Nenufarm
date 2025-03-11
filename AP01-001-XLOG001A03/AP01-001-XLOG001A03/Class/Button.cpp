/*!
 * \file 	Button.cpp
 * \brief 	
 * \author 	Valentin DOMINIAK
 * \date	01/2024
 * \warning
 * MIT License
 * This software is provided "as is", without warranty of any kind. The authors
 * shall not be liable for any claims, damages, or other liability arising from
 * the use of the software. See the LICENSE file for more details.
 * \version 	0.1 (01/2024)
*/

/*!==========================================================================+*/
// FICHIERS HEADER
/*+==========================================================================+*/
#include "Button.h"
#include "drvPTC.h"

/*!==========================================================================+*/
// DEFINES
/*+==========================================================================+*/
#define DOUBLE_PUSH_TIMEOUT		300		// Timeout de detection du double appui (en ms)
#define SHORT_PUSH_TIME			2000	// Temps de detection d'un appui court (en ms)
#define LONG_PUSH_TIME			10000	// Temps de detection d'un appui long (en ms)
#define TIMEOUT_KEY_PUSH		LONG_PUSH_TIME * 3

/*!==========================================================================+*/
// STRUCTURES ET ENUMERATIONS
/*+==========================================================================+*/


// default constructor
Button::Button()
{
	m_b_isInitialized = false;
} //Button

// default destructor
Button::~Button()
{
} //~Button

void Button::v_Button_initialization()
{
	if(!m_b_isInitialized) {
		// Initialisation du driver de gestion du tactile
		v_drvPTC_initialization();
	
		// La librairie est initialisée
		m_b_isInitialized	= true;
	} else {/* Driver déjà initialisé */}
}
	
void Button::v_Button_millisRefreshButton()
{
	if(m_b_isInitialized) {
		// Horloge du driver, doit être appelé toutes les ms
		v_drvPTC_timerHandler();
	
		// Si un appui est en cours, le timer de durée d'appui est incrémenté
		if(m_b_timerIsRunning) {
			m_ui32_buttonTimer++;
		} else {/* Ne rien faire, pas d'appui en cours */}
	} else {/* Ne rien faire, le driver n'est pas initialisé */}
}

BUTTON_EVENT Button::en_Button_getEvent()
{
	BUTTON_EVENT		en_buttonEvent			= BUTTON_EVENT_NO_EVENT;
	UINT8				ui8_keyStatusAquarium	= 0;
	UINT8				ui8_keyStatusPlant		= 0;
	static BOOL			b_shortPushGenerated	= false;
	static BOOL			b_longPushGenerated		= false;
	
	if(m_b_isInitialized) {
		// Mise à jour de l'état des boutons
		v_drvPTC_process();
	
		// Récupération de leur états
		ui8_keyStatusAquarium = ui8_drvPTC_getSensorState(AQUARIUM_SENSOR_NODE) & KEY_TOUCHED_MASK;
		ui8_keyStatusPlant = ui8_drvPTC_getSensorState(PLANT_SENSOR_NODE) & KEY_TOUCHED_MASK;
	
		switch (m_en_lastKeyStatus) {
			case KEY_IDLE:
				// Un bouton a été appuyé
				if((ui8_keyStatusPlant != 0u) || (ui8_keyStatusAquarium != 0u)) {
					if(ui8_keyStatusPlant != 0u) {
						m_en_lastKeyStatus = PLANT_DOUBLE_KEY_TIMEOUT;
					} else {
						m_en_lastKeyStatus = AQUA_DOUBLE_KEY_TIMEOUT;
					}
				
					// Activation du timer (double appui, appui simple, court, long)
					m_b_timerIsRunning = true;
				} else {/* Pas de bouton appuyé, ne rien faire */}
				break;
			
			case PLANT_DOUBLE_KEY_TIMEOUT:
				// Attente que le timeout de vérification du double appui soit dépassé
				if(m_ui32_buttonTimer >= DOUBLE_PUSH_TIMEOUT)
				{
					// Seul le bouton des plantes a été appuyé
					m_en_lastKeyStatus = PLANT_KEY_PUSHED;
				} else {
					// Les deux boutons sont appuyés
					if((ui8_keyStatusPlant != 0u) && (ui8_keyStatusAquarium != 0u)) {
						m_en_lastKeyStatus = AQUA_PLANT_KEY_PUSHED;
					} else {/* Ne rien faire, timeout non dépassé */}
				}
				break;
			
			case AQUA_DOUBLE_KEY_TIMEOUT:
				// Attente que le timeout de vérification du double appui soit dépassé
				if(m_ui32_buttonTimer >= DOUBLE_PUSH_TIMEOUT)
				{
					// Seul le bouton de l'aquarium a été appuyé
					m_en_lastKeyStatus = AQUA_KEY_PUSHED;
				} else {
					// Les deux boutons sont appuyés
					if((ui8_keyStatusPlant != 0u) && (ui8_keyStatusAquarium != 0u)) {
						m_en_lastKeyStatus = AQUA_PLANT_KEY_PUSHED;
					} else {/* Ne rien faire, timeout non dépassé */}
				}
				break;

			case PLANT_KEY_PUSHED:
				// Bouton relaché
				if(ui8_keyStatusPlant == 0u) {
					if(m_ui32_buttonTimer >= LONG_PUSH_TIME) {
						en_buttonEvent = BUTTON_EVENT_PLANT_LONG_PUSH;
					} else if(m_ui32_buttonTimer < SHORT_PUSH_TIME) {
						en_buttonEvent = BUTTON_EVENT_PLANT_SIMPLE_PUSH;
					} else {/* Appui court, évènement déjà généré */}

					m_en_lastKeyStatus = KEY_RELEASED;
				} else {
					// L'évènement d'un appui court est généré avant la détection d'un appui long
					if(m_ui32_buttonTimer >= SHORT_PUSH_TIME && !b_shortPushGenerated) {
						en_buttonEvent = BUTTON_EVENT_PLANT_SHORT_PUSH;
						b_shortPushGenerated = true;
					} else if(m_ui32_buttonTimer > TIMEOUT_KEY_PUSH) {
						m_en_lastKeyStatus = KEY_RELEASED;
						v_drvPTC_initialization();
					} else {}
				}
				break;

			case AQUA_KEY_PUSHED:
				// Bouton relaché
				if(ui8_keyStatusAquarium == 0u) {
					if(m_ui32_buttonTimer >= LONG_PUSH_TIME) {
						en_buttonEvent = BUTTON_EVENT_AQUA_LONG_PUSH;
					} else if(m_ui32_buttonTimer < SHORT_PUSH_TIME) {
						en_buttonEvent = BUTTON_EVENT_AQUA_SIMPLE_PUSH;
					} else {/* Appui court, évènement déjà généré */}

					m_en_lastKeyStatus = KEY_RELEASED;
				} else {
					// L'évènement d'un appui court est généré avant la détection d'un appui long
					if(m_ui32_buttonTimer >= SHORT_PUSH_TIME && !b_shortPushGenerated) {
						en_buttonEvent = BUTTON_EVENT_AQUA_SHORT_PUSH;
						b_shortPushGenerated = true;
					} else if(m_ui32_buttonTimer > TIMEOUT_KEY_PUSH) {
						m_en_lastKeyStatus = KEY_RELEASED;
						v_drvPTC_initialization();
					} else {}
				}
				break;
			
			case AQUA_PLANT_KEY_PUSHED:
				// Bouton relaché
				if((ui8_keyStatusPlant == 0u) && (ui8_keyStatusAquarium == 0u)) {
					if(m_ui32_buttonTimer >= LONG_PUSH_TIME) {
						en_buttonEvent = BUTTON_EVENT_AQUA_PLANT_LONG_PUSH;
					} else if(m_ui32_buttonTimer < SHORT_PUSH_TIME) {
						en_buttonEvent = BUTTON_EVENT_AQUA_PLANT_SIMPLE_PUSH;
					} else {/* Appui court, évènement déjà généré */}
				
					m_en_lastKeyStatus = KEY_RELEASED;
				} else {
					// L'évènement d'un appui court est généré avant la détection d'un appui long
					if(m_ui32_buttonTimer >= SHORT_PUSH_TIME && !b_shortPushGenerated) {
						en_buttonEvent = BUTTON_EVENT_AQUA_PLANT_SHORT_PUSH;
						b_shortPushGenerated = true;
					} else if(m_ui32_buttonTimer >= LONG_PUSH_TIME && !b_longPushGenerated) {
						en_buttonEvent = BUTTON_EVENT_AQUA_PLANT_LONG_PUSH;
						b_longPushGenerated = true;
					} else {}
				}
				break;

			case KEY_RELEASED:
					m_en_lastKeyStatus		= KEY_IDLE;
					m_b_timerIsRunning		= false;
					b_shortPushGenerated	= false;
					b_longPushGenerated		= false;
					m_ui32_buttonTimer		= 0;
				break;
			
			default:
				break;
		}
	} else {/* Ne rien faire, le driver n'est pas initialisé */}
	
	return en_buttonEvent;
}

void Button::v_Button_setSensorState(UINT8 ui8_sensorNode, BOOL b_sensorState)
{
	if(m_b_isInitialized) {
		// Le bouton est désactivé
		if(!b_sensorState) {
			v_drvPTC_suspendSensor(ui8_sensorNode);
		} else {
			// Réactivation du bouton
			v_drvPTC_resumeSensor(ui8_sensorNode);
		}
	} else {/* Ne rien faire, le driver n'est pas initialisé */}
}