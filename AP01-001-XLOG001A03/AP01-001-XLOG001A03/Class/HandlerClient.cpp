/* 
* HandlerClient.cpp
*
* Created: 01/02/2024 12:35:39
* Author: LucasTeissier
*/

#include "HandlerClient.h"

/* General */
#define CURRENT_MODE					true
#define NEXT_MODE						false
#define ENABLE							true
#define DISABLE							false

/* UI/UX parameters */
#define TIME_BTW_TWO_BLINK_PB			20		/* Temps entre deux clignotements d'erreur mineure (en sec) */
#define TIME_AUTO_SAVE_SETTING			30		/* Temps avant la sauvegarde automatique des r�glages (en sec) */
#define DC_VALUE_TEST_ADC				10		/* Valeur du DC pour tester si aucune erreur sur ADC */

// default constructor
HandlerClient::HandlerClient()
{
} //HandlerClient

// default destructor
HandlerClient::~HandlerClient()
{
} //~HandlerClient

void HandlerClient::v_Handler_initialization(Memory *pMe_memory, Button *pBu_button, PlantLight *pPl_plantLight, AquaLight *pAl_aquaLight, Pump *pPu_pump)
{
	SETTINGS st_settings;
	AQUALIGHT_ADC_VALUE st_adcValues;
	
	/* Brickage produit */
#ifndef DEBUG_MOD_NO_BRICK
	m_b_isBricked = m_pMe_memory->b_Memory_isBricked();

	/* V�rification si l'option de brickage est activ�e */
	if(m_pMe_memory->b_Memory_isBrickedModeDisable()) {
		m_b_noBrickMode = true;
	} else {/* Nothing to do - Le mode brickage est activ� */}
#endif

	/* Config produit s'il n'est pas brick� */
	if(!m_b_isBricked) {
		m_pMe_memory = pMe_memory;
		m_pBu_button = pBu_button;
		m_pPl_plantLight = pPl_plantLight;
		m_pAl_aquaLight = pAl_aquaLight;
		m_pPu_pump = pPu_pump;

		/* Brickage d�sactiv�, les lectures ADC sont d�sactiv�es */
		if(m_b_noBrickMode) {
			m_pAl_aquaLight->v_AquaLight_setNoBrickMode(m_b_noBrickMode);
		} else {/* Nothing to do - Le mode brickage est activ� */}
		
		/* R�cup�ration des r�glages en m�moire */
		m_pMe_memory->v_Memory_loadSettings(&st_settings);

		/* Calcul des heures d�but/fin cycles */
		m_Sc_scheduler.v_Scheduler_initialization(st_settings);

		m_pPl_plantLight->v_PlantLight_configuration(st_settings.en_timeModePlantLight);
		m_pAl_aquaLight->v_AquaLight_configuration(st_settings);
		m_pPu_pump->v_Pump_configuration(st_settings.ui16_pumpFrequency);
		m_pPu_pump->v_Pump_startingMode(WITH_WAITING);

		/* Configuration des valeurs limites de l'ADC s'il n'a jamais d�marr� et que l'ADC est utilis� */
		if(st_settings.b_hasAlreadyProgrammed && !m_b_noBrickMode) {
			m_pMe_memory->v_Memory_getADCValues(&st_adcValues);
			m_pAl_aquaLight->v_AquaLight_configADC(st_adcValues);
		} else if (!m_b_noBrickMode) {
			m_b_isWatingADCConfig = true;
			m_pAl_aquaLight->v_AquaLight_defineADCValue();
		} else {/* Les limites ne sont pas configur�es ou lues, l'ADC a �t� d�sactiv� */}
		
		/* Enregistrement local des events */
		m_en_pumpEvent = m_Sc_scheduler.en_Scheduler_getPumpEvent();
		m_en_plantLightEvent = m_Sc_scheduler.en_Scheduler_getPlantLightEvent();
		m_en_aquaLightEvent = m_Sc_scheduler.en_Scheduler_getAquaLightEvent();
		
		/* Si une premi�re erreur critique a �t� sauvegard�, test de l'ADC */
		if(m_pMe_memory->b_Memory_isCriticalErrorRecording()) {
			LOG("CriticalError inscrite en Flash, test ADC");
			m_b_isFirstCriticalErr = true;
			m_pAl_aquaLight->v_AquaLight_testADC();
		} else {	
			//LOG("Aucune CriticalError inscrite en Flash, d�marrage classique");
		}
		
		m_b_isConfigured = true;
	} else {
		/* Nothing to do - Produit brick� */
		LOG("Produit briqu�");
	}
}

const BOOL HandlerClient::b_Handler_isBricked() const
{
	return m_b_isBricked;
}

void HandlerClient::v_Handler_everyMilSecond(const UINT16 ui16_rpmCounterValue, const BOOL b_isRpmRead)
{
	m_pPu_pump->v_Pump_event(m_en_pumpEvent, ui16_rpmCounterValue, b_isRpmRead);
	m_pPl_plantLight->v_PlantLight_event(m_en_plantLightEvent);
	m_pAl_aquaLight->v_AquaLight_event(m_en_aquaLightEvent);
	
	/* V�rification de l'�tat de la pompe */
	/* Active uniquement si l'�tat isMinor n'est pas d�j� actif */
	if(m_pPu_pump->b_Pump_isMinorError() && !m_b_isMinorError) {
		/* Si pas dans les r�glages, active l'erreur directement */
		if(m_en_statusConfig == STATUS_CONFIG_NO_SETTING_MODE) {
			v_Handler_toggleMinorError(ENABLE);
			m_pPl_plantLight->v_PlantLight_activateBlinkPb(m_en_plantLightEvent);
		} else {
			m_b_isWatingBeforeMinorErr = true;
			m_b_isMinorError = true;
		}
	} else {/* Nothing to do - La pompe n'est pas en erreur */}

	/* V�rification de l'�tat de l'�clairage aquarium */
	if(m_pAl_aquaLight->b_AquaLight_isCriticalError()) {
		v_Handler_toggleCriticalError();
	} else {/* Nothing to do - L'�clairage aquarium n'est pas en erreur */}
		
	if(m_b_isWatingBeforeReset) {
		if(!m_pPl_plantLight->b_PlantLight_getIsBlinking()) {
			_PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRE_bm);
		} else {/* Nothing to do - En attente de la fin du clignotement avant de reset */}
	}
}

void HandlerClient::v_Handler_everySecond()
{
	m_Sc_scheduler.v_Scheduler_updateEvent();
	
	m_en_pumpEvent = m_Sc_scheduler.en_Scheduler_getPumpEvent();
	m_en_plantLightEvent = m_Sc_scheduler.en_Scheduler_getPlantLightEvent();
	m_en_aquaLightEvent = m_Sc_scheduler.en_Scheduler_getAquaLightEvent();
	
	/* Explication de l'�tat Minor Error :
	 * Est utilis� pour des erreurs ne bloquant pas le fonctionbnement (ex: pompe hors de l'eau)
	 * Se configure et se d�sactive avec v_Handler_toggleMinorError()
	 * S'active avec m_pPl_plantLight->v_PlantLight_activateBlinkPb(m_en_plantLightEvent);
	 * Si en mode r�glage quand l'err se d�clanche, m_b_isWatingBeforeMinorErr est mis � 1 dans everyMilSec puis le blink activ� dans everySec
	 * Si activ�, toute les secondes m_ui8_blinkPbTimer est incr�ment� et au bout de TIME_BTW_TWO_BLINK_PB,
	 * v_PlantLight_activateBlinkPb() est appel� pour pr�venir PlantLight qu'il faudra clignotter au prochain event
	 */
	if(m_b_isMinorError) {
		if(m_en_statusConfig == STATUS_CONFIG_NO_SETTING_MODE) {
			if(m_b_isWatingBeforeMinorErr) {
				m_b_isWatingBeforeMinorErr = false;
				v_Handler_toggleMinorError(ENABLE);
			} else {/* Nothing to do - L'erreur a d�ja �t� configur�e */}

			m_ui8_blinkPbTimer++;
			if(m_ui8_blinkPbTimer > TIME_BTW_TWO_BLINK_PB) {
				m_ui8_blinkPbTimer = 0;
				m_pPl_plantLight->v_PlantLight_activateBlinkPb(m_en_plantLightEvent);
			} else {/* Nothing to do - Le temps entre deux clignotements n'est pas pass� */}
		} else {/* Nothing to do - Pas de clignottement quand on est en r�glage */}
	} else {/* Nothing to do - Aucune erreur mineure */}
	
	/* Sauvegarde automatique des r�glages */
	if(m_b_isInSettingMode) {
		m_ui8_settingTimer++;
		if(m_ui8_settingTimer > TIME_AUTO_SAVE_SETTING) {
			v_Handler_autoSaveSetting();
		} else {/* Nothing to do - Timeout dans les settings pas atteind */}
	} else {/* Nothing to do - Les settings ne sont pas activ�s */}

	/* Suppression de FirstCriticalError si AquaLight a r�ussi � s'allumer */
	if(m_b_isFirstCriticalErr) {
		if(m_pAl_aquaLight->b_AquaLight_firstCritErrRemoved()) {
			LOG("Suppression de Critical error");
			m_b_isFirstCriticalErr = false;
			m_pMe_memory->v_Memory_clearFirstCriticalError();
		} else {/* Nothing to do - En attente de la suppression de l'erreur */}
	}
	
	/* R�cup�ration des valeurs ADC de r�f�rence une fois celles-ci d�finies (utilis� qu'au 1er d�marrage) */
	if(m_b_isWatingADCConfig) {
		if(m_pAl_aquaLight->b_AquaLight_isADCCOnfigured()) {
			m_b_isWatingADCConfig = false;
			m_pMe_memory->v_Memory_saveADCValues(m_pAl_aquaLight->st_AquaLight_getADCValues());
			
			/* Fin de la configuration, marquage en m�moire d'un flag pour ne plus refaire les config */
			m_pMe_memory->v_Memory_markAsAlreadyStarted();
		} else {/* Nothing to do - En attente de la fin de la d�finition des valeurs ADC */}
	}
}

void HandlerClient::v_Handler_everyTime()
{
	v_Handler_manageButtonEvent();
}

inline void HandlerClient::v_Handler_manageButtonEvent()
{
	const BUTTON_EVENT en_buttonEvent = m_pBu_button->en_Button_getEvent();

	switch(en_buttonEvent) {
/***** Bouton aquarium *****/
	/* Appui simple */
		case BUTTON_EVENT_AQUA_SIMPLE_PUSH:
			LOG("BP Aqua - Appui simple");
			switch(m_en_statusConfig) {
				/* Si pas dans les r�glages : active/desactive le mode manuel */
				case STATUS_CONFIG_NO_SETTING_MODE:
					if(!m_b_isWatingADCConfig && !m_b_isFirstCriticalErr)
					{
						m_pAl_aquaLight->v_AquaLight_manualMode(m_en_aquaLightEvent);
						m_Sc_scheduler.v_Scheduler_setManualModeAquaLight();
						m_en_aquaLightEvent = m_Sc_scheduler.en_Scheduler_getAquaLightEvent();
					} else {/* L'ADC est en configuration ou il y a un test sur erreur critique, ne rien faire */}
					break;

				/* Si en mode r�glage aqua : active le mode par d�faut du r�glage temps d'�clairage (blink) */
				case STATUS_CONFIG_SETTING_AQUALIGHT:
					m_ui8_settingTimer = 0;
					m_en_statusConfig = STATUS_CONFIG_TIME_CYCLE_AQUALIGHT;
					m_pAl_aquaLight->v_AquaLight_timeSettingMode(CURRENT_MODE);
					break;

				/* Si dans le r�glage du temps d'�clairage aqua: passe au mode suivant du temps d'�clairage */
				case STATUS_CONFIG_TIME_CYCLE_AQUALIGHT:
					m_ui8_settingTimer = 0;
					m_pAl_aquaLight->v_AquaLight_timeSettingMode(NEXT_MODE);
					break;

				/* Si dans le r�glage de l'intensit� d'�clairage aqua: svg valeur et passe au reglage temps */
				case STATUS_CONFIG_INTENSITY_AQUALIGHT:
					m_ui8_settingTimer = 0;
					m_en_statusConfig = STATUS_CONFIG_TIME_CYCLE_AQUALIGHT;
					m_pAl_aquaLight->v_AquaLight_saveIntensityMode();
					m_pMe_memory->v_Memory_saveIntensityAquaLight(m_pAl_aquaLight->en_AquaLight_getIntensityName());
					m_pAl_aquaLight->v_AquaLight_timeSettingMode(CURRENT_MODE);
					break;
					
				case STATUS_CONFIG_SETTING_PLANTLIGHT:
					/* Nothing to do - Aucune config pour cet appuis */		
					break;
					
				default:
					/* Nothing to do - Aucun action pour les autres appuis */
					LOG("CodeError : cas switch non trait�");
					break;
			}
			break;
			
	/* Appui 3 secondes */
		case BUTTON_EVENT_AQUA_SHORT_PUSH:
			LOG("BP Aqua - Appui 3sec");
			/* Si erreur mineur (pompe hors de l'eau), l'appui 3sec d�sactive cette erreur */
				switch(m_en_statusConfig) {
					/* Si pas dans les r�glages: active le mode r�glage ou d�sactive l'erreur s'il en a une */
					case STATUS_CONFIG_NO_SETTING_MODE:
						/* S'il n'y a pas d'erreur, rentre dans les r�glages */
						if(!m_b_isMinorError) {
							if(!m_b_isWatingADCConfig && !m_b_isFirstCriticalErr) {
								m_b_isInSettingMode = true;
								m_ui8_settingTimer = 0;
								m_en_statusConfig = STATUS_CONFIG_SETTING_AQUALIGHT;
								m_pAl_aquaLight->v_AquaLight_enterSettingMode();
							} else {/* L'ADC est en configuration ou il y a un test sur erreur critique, ne rien faire */}
						/* S'il y a une erreur pompe et qu'on est pas dans les r�glages, d�sactive l'erreur */
						} else {
							/* En cas d'erreur mineure, appui de 3sec d�sactive l'erreur */
							v_Handler_toggleMinorError(DISABLE);
						}
						break;
					
					/* Si en mode r�glage aqua : quitte le mode */
					case STATUS_CONFIG_SETTING_AQUALIGHT:
						/* R�initialisation du compteur pour que l'erreur ne soit pas directement affich� � la sortie des r�glages */
						m_ui8_blinkPbTimer = 0;
						m_b_isInSettingMode = false;
						m_en_statusConfig = STATUS_CONFIG_NO_SETTING_MODE;
						m_pAl_aquaLight->v_AquaLight_exitSettingMode(m_en_aquaLightEvent);
						break;
					
					/* Si dans les r�glages plante: d�sactive le mode r�glage plante et l'active � l'aquarium */
					case STATUS_CONFIG_SETTING_PLANTLIGHT:
						m_ui8_settingTimer = 0;
						m_en_statusConfig = STATUS_CONFIG_SETTING_AQUALIGHT;
						m_pPl_plantLight->v_PlantLight_exitSettingMode(m_en_plantLightEvent);
						m_pAl_aquaLight->v_AquaLight_enterSettingMode();
						break;

					/* Si dans les r�glages temps de cycle: quitte les r�glages et les sauvegarde */
					case STATUS_CONFIG_TIME_CYCLE_AQUALIGHT:
						m_b_isInSettingMode = false;
						m_en_statusConfig = STATUS_CONFIG_NO_SETTING_MODE;
						m_pAl_aquaLight->v_AquaLight_saveTimeMode();
						v_Handler_saveMemoryTimeModeAL(m_pAl_aquaLight->en_AquaLight_getTimeModeName());
						m_en_aquaLightEvent = m_Sc_scheduler.en_Scheduler_setTimeModeAquaLight(m_pAl_aquaLight->en_AquaLight_getTimeModeName());
						m_pAl_aquaLight->v_AquaLight_exitSettingMode(m_en_aquaLightEvent);
						break;

					/* Si dans les r�glages d'intensit�: quitte les r�glages et sauvegarde */
					case STATUS_CONFIG_INTENSITY_AQUALIGHT:
						m_b_isInSettingMode = false;
						m_en_statusConfig = STATUS_CONFIG_NO_SETTING_MODE;
						m_pAl_aquaLight->v_AquaLight_saveIntensityMode();
						m_pMe_memory->v_Memory_saveIntensityAquaLight(m_pAl_aquaLight->en_AquaLight_getIntensityName());
						m_pAl_aquaLight->v_AquaLight_exitSettingMode(m_en_aquaLightEvent);
						break;

					default:
						/* Nothing to do - Aucun action pour les autres appuis */
						LOG("CodeError : cas switch non trait�");
						break;
				}
			break;
			
	/* Appui 10 secondes */
		case BUTTON_EVENT_AQUA_LONG_PUSH:
			LOG("BP Aqua - Appui 10sec");
			break;
		
/***** Bouton plante *****/
	/* Appui simple */
		case BUTTON_EVENT_PLANT_SIMPLE_PUSH:
			LOG("BP Plante - Appui simple");
			switch(m_en_statusConfig) {
				/* Si pas dans les r�glages : active/desactive le mode manuel */
				case STATUS_CONFIG_NO_SETTING_MODE:
					m_pPl_plantLight->v_PlantLight_manualMode(m_en_plantLightEvent);
					m_Sc_scheduler.v_Scheduler_setManualModePlantLight();
					m_en_plantLightEvent = m_Sc_scheduler.en_Scheduler_getPlantLightEvent();
					break;
					
				/* Si en mode r�glage plante : active le mode par d�faut du r�glage temps d'�clairage (blink) */
				case STATUS_CONFIG_SETTING_PLANTLIGHT:
					m_ui8_settingTimer = 0;
					m_en_statusConfig = STATUS_CONFIG_TIME_CYCLE_PLANTLIGHT;
					m_pPl_plantLight->v_PlantLight_timeSettingMode(CURRENT_MODE);
					break;
					
				/* Si dans le r�glage du temps d'�clairage plante: passe au mode suivant du temps d'�clairage */
				case STATUS_CONFIG_TIME_CYCLE_PLANTLIGHT:
					m_ui8_settingTimer = 0;
					m_pPl_plantLight->v_PlantLight_timeSettingMode(NEXT_MODE);
					break;
					
				/* Si en mode r�glage aqua : active le mode par d�faut du r�glage d'intensit� */
				case STATUS_CONFIG_SETTING_AQUALIGHT:
					m_ui8_settingTimer = 0;
					m_en_statusConfig = STATUS_CONFIG_INTENSITY_AQUALIGHT;
					m_pAl_aquaLight->v_AquaLight_intensityMode(CURRENT_MODE);
					break;
					
				/* Si dans le r�glage de l'intensit� d'�clairage aqua: passe au mode suivant de l'intensit� d'�clairage */
				case STATUS_CONFIG_INTENSITY_AQUALIGHT:
					m_ui8_settingTimer = 0;
					m_pAl_aquaLight->v_AquaLight_intensityMode(NEXT_MODE);
					break;
					
				/* Si dans le r�glage du temps d'�clairage aqua: svg et passe au reglage intensit� */
				case STATUS_CONFIG_TIME_CYCLE_AQUALIGHT:
					m_ui8_settingTimer = 0;
					m_en_statusConfig = STATUS_CONFIG_INTENSITY_AQUALIGHT;
					m_pAl_aquaLight->v_AquaLight_saveTimeMode();
					m_pAl_aquaLight->v_AquaLight_intensityMode(CURRENT_MODE);
					m_pMe_memory->v_Memory_saveTimeModeAquaLight(m_pAl_aquaLight->en_AquaLight_getTimeModeName());
					m_en_aquaLightEvent = m_Sc_scheduler.en_Scheduler_setTimeModeAquaLight(m_pAl_aquaLight->en_AquaLight_getTimeModeName());
					break;
					
				default:
					/* Nothing to do - Aucun action pour les autres appuis */
					LOG("CodeError : cas switch non trait�");
					break;
			}
			break;
			
	/* Appui 3 secondes */
		case BUTTON_EVENT_PLANT_SHORT_PUSH:
			LOG("BP Plante - Appui 3sec");
			switch(m_en_statusConfig) {
				/* Si pas dans les r�glages: active le mode r�glage */
				case STATUS_CONFIG_NO_SETTING_MODE:
					/* V�rifie qu'il n'y ai pas d'erreur pompe avant de rentrer */
					if(!m_b_isMinorError) {
						m_b_isInSettingMode = true;
						m_ui8_settingTimer = 0;
						m_en_statusConfig = STATUS_CONFIG_SETTING_PLANTLIGHT;
						m_pPl_plantLight->v_PlantLight_enterSettingMode();
					} else {/* Nothing to do - Si erreur pompe, impossible de rentrer dans les r�glages */}
					break;
					
				/* Si en mode r�glage plante : quitte le mode */
				case STATUS_CONFIG_SETTING_PLANTLIGHT:
					m_b_isInSettingMode = false;
					m_en_statusConfig = STATUS_CONFIG_NO_SETTING_MODE;
					m_pPl_plantLight->v_PlantLight_exitSettingMode(m_en_plantLightEvent);
					break;
					
				/* Si dans les r�glages aquarium: d�sactive le mode r�glage aquarium et l'active aux plantes */
				case STATUS_CONFIG_SETTING_AQUALIGHT:
					m_ui8_settingTimer = 0;
					m_en_statusConfig = STATUS_CONFIG_SETTING_PLANTLIGHT;
					m_pAl_aquaLight->v_AquaLight_exitSettingMode(m_en_aquaLightEvent);
					m_pPl_plantLight->v_PlantLight_enterSettingMode();
					break;

				/* Si dans les r�glages temps de cycle: quitte les r�glage et sauvegarde */
				case STATUS_CONFIG_TIME_CYCLE_PLANTLIGHT:
					m_b_isInSettingMode = false;
					m_en_statusConfig = STATUS_CONFIG_NO_SETTING_MODE;
					m_pPl_plantLight->v_PlantLight_saveTimeMode();
					m_pMe_memory->v_Memory_saveTimeModePlantLight(m_pPl_plantLight->en_PlantLight_getTimeModeName());
					m_en_plantLightEvent = m_Sc_scheduler.en_Scheduler_setTimeModePlantLight(m_pPl_plantLight->en_PlantLight_getTimeModeName());
					m_pPl_plantLight->v_PlantLight_exitSettingMode(m_en_plantLightEvent);
					break;

				default:
					/* Nothing to do - Aucun action pour les autres appuis */
					LOG("CodeError : cas switch non trait�");
					break;
			}
			break;

	/* Appui 10 secondes */
		case BUTTON_EVENT_PLANT_LONG_PUSH:
			LOG("BP Plante - Appui 10sec");
			/* Nothing to do - Aucune config pour cet appuis */
			break;

/***** Bouton aquarium + plante ******/
		case BUTTON_EVENT_AQUA_PLANT_SIMPLE_PUSH:
			LOG("BP Aqua + Plante - Appui simple");
			/* Nothing to do - Aucune config pour cet appuis */
			break;

		case BUTTON_EVENT_AQUA_PLANT_SHORT_PUSH:
			LOG("BP Aqua + Plante - Appui 3sec");
			switch(m_en_statusConfig) {
				/* Si  dans les r�glages plantes: change les heures plantes, sauvegarde ces heures, quitte le mode manuel (qu'importe si actif ou pas), clignotte pour inform� du changement et reste dans les r�glages */
				case STATUS_CONFIG_SETTING_PLANTLIGHT:
					m_ui8_settingTimer = 0;
					m_pMe_memory->v_Memory_saveStopTimePlantLight(m_Sc_scheduler.ui32_Scheduler_getCurrentTime());
					m_Sc_scheduler.v_Scheduler_setStopTimePlantLight(m_pPl_plantLight->en_PlantLight_getTimeModeName());
					m_Sc_scheduler.v_Scheduler_stopManualModePlantLight();
					m_en_plantLightEvent = STOP_PLANTLIGHT_CYCLE; /* Si r�glage heure de fin, le prochain event est forcement stop */
					m_pPl_plantLight->v_PlantLight_blinkConfirmSetTime();
					break;

				/* Si  dans les r�glages aqua: change les heures aqua, sauvegarde ces heures, quitte le mode manuel (qu'importe si actif ou pas), clignotte pour inform� du changement et reste dans les r�glages */
				case STATUS_CONFIG_SETTING_AQUALIGHT:
					m_ui8_settingTimer = 0;
					m_pMe_memory->v_Memory_saveStopTimeAquaLight(m_Sc_scheduler.ui32_Scheduler_getCurrentTime());
					m_Sc_scheduler.v_Scheduler_setStopTimeAquaLight(m_pAl_aquaLight->en_AquaLight_getTimeModeName());
					m_Sc_scheduler.v_Scheduler_stopManualModeAquaLight();
					m_en_aquaLightEvent = STOP_AQUALIGHT_CYCLE; /* Si r�glage heure de fin, le prochain event est forcement stop */
					m_pAl_aquaLight->v_AquaLight_blinkConfirmSetHour();
					break;

				default:
					/* Nothing to do - Aucun action pour les autres appuis */
					LOG("CodeError : cas switch non trait�");
					break;
			}
			break;

		case BUTTON_EVENT_AQUA_PLANT_LONG_PUSH:
			LOG("BP Aqua + Plante - Appui 10sec");
			v_Handler_resetProduct();
			break;

		default:
			/* Nothing to do - Aucun action pour les autres appuis */
			break;
	}
}

inline void HandlerClient::v_Handler_toggleMinorError(const BOOL b_isMinorError)
{
	m_b_isMinorError = b_isMinorError;

	if(m_b_isMinorError) {
		/* Activation du mode Erreur Mineure */
		m_ui8_blinkPbTimer = 0;
		
		m_pPl_plantLight->v_PlantLight_configBlinkPb();
	} else {
		/* D�sactivation du mode Erreur Mineure */
		(m_en_plantLightEvent == START_PLANTLIGHT_CYCLE || m_en_plantLightEvent == ON_MANUAL_LIGHT_MODE)
			? m_pPl_plantLight->v_PlantLight_blinkConfirmation(BLINK_OFF, NB_BLINK_CONFIRMATION)
			: m_pPl_plantLight->v_PlantLight_blinkConfirmation(BLINK_ON, NB_BLINK_CONFIRMATION);
		m_pPu_pump->v_Pump_disableMinorError();
	}
}

void HandlerClient::v_Handler_toggleCriticalError() const
{
	/* Si CriticalError n'est jamais arriv�e: marque FirstCriticalError en m�moire puis reboot */
	/* Si CriticalError est d�j� arriv�	: marque BrickCriticalError en m�moire puis reboot. Au red�marrage, cette valeur sera lu et le soft bloqu� */
	if(m_pMe_memory->b_Memory_isCriticalErrorRecording()) {
		LOG("2nd critical error, brick product");
		m_pMe_memory->v_Memory_writeBrickCriticalError();
	} else {
		LOG("First critical error");
		m_pMe_memory->v_Memory_writeFirstCriticalError();
	}
	v_Handler_saveTimeInMemory();
	
	_PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRE_bm);
}

void HandlerClient::v_Handler_autoSaveSetting()
{
	/* R�initialisation du compteur pour que l'erreur ne soit pas directement affich� � la sortie des r�glages */
	m_ui8_blinkPbTimer = 0;
	
	switch(m_en_statusConfig) {
		case STATUS_CONFIG_SETTING_PLANTLIGHT:
			m_pPl_plantLight->v_PlantLight_exitSettingMode(m_en_plantLightEvent);
			break;

		case STATUS_CONFIG_SETTING_AQUALIGHT:
			m_pAl_aquaLight->v_AquaLight_exitSettingMode(m_en_aquaLightEvent);
			break;

		case STATUS_CONFIG_TIME_CYCLE_PLANTLIGHT:
			m_pPl_plantLight->v_PlantLight_saveTimeMode();
			m_pMe_memory->v_Memory_saveTimeModePlantLight(m_pPl_plantLight->en_PlantLight_getTimeModeName());
			m_en_plantLightEvent = m_Sc_scheduler.en_Scheduler_setTimeModePlantLight(m_pPl_plantLight->en_PlantLight_getTimeModeName());
			m_pPl_plantLight->v_PlantLight_exitSettingMode(m_en_plantLightEvent);
			break;

		case STATUS_CONFIG_TIME_CYCLE_AQUALIGHT:
			m_pAl_aquaLight->v_AquaLight_saveTimeMode();
			m_en_aquaLightEvent = m_Sc_scheduler.en_Scheduler_setTimeModeAquaLight(m_pAl_aquaLight->en_AquaLight_getTimeModeName());
			v_Handler_saveMemoryTimeModeAL(m_pAl_aquaLight->en_AquaLight_getTimeModeName());
			m_pAl_aquaLight->v_AquaLight_exitSettingMode(m_en_aquaLightEvent);
			break;

		case STATUS_CONFIG_INTENSITY_AQUALIGHT:
			m_pMe_memory->v_Memory_saveIntensityAquaLight(m_pAl_aquaLight->en_AquaLight_getIntensityName());
			m_pAl_aquaLight->v_AquaLight_saveIntensityMode();
			m_pAl_aquaLight->v_AquaLight_exitSettingMode(m_en_aquaLightEvent);
			break;
		
		default:
			/* Nothing to do - Cas impossible, le Soft n'est jamais sens� passer par ici */
			LOG("CodeError : cas switch non trait�");
			break;
	}
	
	m_en_statusConfig = STATUS_CONFIG_NO_SETTING_MODE;
	m_b_isInSettingMode = false;
	m_ui8_settingTimer = 0;
}

void HandlerClient::v_Handler_saveMemoryTimeModeAL(const AQUALIGHT_CONFIG_TIME en_configToSaved)
{
	if(en_configToSaved == MANUAL_LIGHTING) {
		if(m_Sc_scheduler.b_Scheduler_isWatingStopTimeModeAL()) {
			m_pMe_memory->v_Memory_saveTimeModeAquaLight(en_configToSaved);
			m_pMe_memory->v_Memory_saveManualModeAquaLight(m_Sc_scheduler.en_Scheduler_getStartTimeManualAL(),
														   m_Sc_scheduler.ui32_Scheduler_getCurrentTime());
		} else {
			/* En mode manuel, on n'enregistre pas les param au premier appui, seulement au 2eme */
			LOG("1ere selection de MANUAL_LIGHTING, pas d'enregistrement memoire");
		}
	} else {
		m_pMe_memory->v_Memory_saveTimeModeAquaLight(en_configToSaved);
	}
}

void HandlerClient::v_Handler_resetProduct()
{
	LOG("Reset product");
	m_pMe_memory->v_Memory_clearMemory();
	m_b_isWatingBeforeReset = true;
	m_pPl_plantLight->v_PlantLight_blinkConfirmation(BLINK_ON, TWO_BLINK);
}

void HandlerClient::v_Handler_saveTimeInMemory() const
{
	if(m_b_isConfigured) {
		m_pMe_memory->v_Memory_saveCurrentTime(m_Sc_scheduler.ui32_Scheduler_getCurrentTime());
	} else {/* Nothing to do - Sauvegarde uniquement si le Handler est config, �vite les appel en HandlerProd */}
}