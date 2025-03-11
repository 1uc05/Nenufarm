/* 
* PlantLight.cpp
*
* Created: 04/01/2024 12:59:08
* Author: LucasTeissier
*/

#include "PlantLight.h"
#include "drvPWM.h"

/* Hardware setting */
#define CHANEL_PWM_PLANTLIGHT							PWM_CHANNEL_1

/* UI/UX parameter */
#define	MAX_DUTY_CYCLE									100
#define TIME_BTW_TWO_DC_STEPS							5		/* Temps en ms entre deux pas du RC pendant les rampes on/off */

/* Breath Mode */
#define TIME_BTW_TWO_BREATHE_STEPS						12		/* Temps en ms entre deux pas de l'effet respiration (mode reglage) */
#define MIN_DC_BREATHE									20		/* Valeur MIN du rapport cyclique de la respiration */
#define MAX_DC_BREATHE									80		/* Valeur MAX du rapport cyclique de la respiration */

/* Blink Mode */
#define	BLINK_DUTY_CYCLE								20		/* Valeur du rapport cyclique pour un clignotement */
#define BLINK_STG_TIME_PERIOD							3000	/* Temps entre deux clignotement (en ms) */
#define BLINK_STG_TIME_OFF								200		/* Temps entre deux clignotement (en ms) */
#define BLINK_STG_TIME_ON								150		/* Temps d'un clignotement allumé (en ms) */
#define BLINK_PB_ON_OFF_TIME							300		/* Temps du clignotement (en ms) */
#define BLINK_PB_NB										2		/* Nombre de clignotement pour signaler un pb */
#define BLINK_CONFIRM_TIME_ON_OFF						80		/* Temps à l'état on et off du clignotement de confirmation */							
#define BLINK_STG_OFFSET								300		/* Temps d'attente avant le premier clignotement */

/* Time Mode */
#define NB_CONFIG_TIME_MODE								1		/* Nombre de mode de temps d'éclairage (0 étant compté, 2 mode ici) */

// default constructor
PlantLight::PlantLight()
{
} //PlantLight

// default destructor
PlantLight::~PlantLight()
{
} //~PlantLight

void PlantLight::v_PlantLight_initialization()
{
	en_drvPWM_initialization(CHANEL_PWM_PLANTLIGHT, 0, &m_ui8_plantLightInstance);
}

void PlantLight::v_PlantLight_configuration(const PLANTLIGHT_CONFIG_TIME en_timeMode)
{
	/* Remise à 0 de tous les attribut car utilisé par StartManager */
	m_b_isBreatheUp				= false;
	m_ui8_plantLightInstance	= 0;
	m_ui8_tmpLightingValue		= 0;
	m_ui8_blinkIterator			= 0;
	m_ui8_numberOfBlink			= 0;
	m_ui16_plantLightTimer		= 0;
	m_en_currentStatus			= PLANTLIGHT_NO_STATUS;
	
	m_st_blinkConfig.ui8_nbOfBlink = BLINK_PB_NB;
	m_st_blinkConfig.ui16_offsetTime = 0;
	m_st_blinkConfig.ui16_firstPhaseTime = BLINK_PB_ON_OFF_TIME;
	m_st_blinkConfig.ui16_secondPhaseTime = BLINK_PB_ON_OFF_TIME;
	m_st_blinkConfig.ui16_totalPeriodeTime = (BLINK_PB_ON_OFF_TIME + BLINK_PB_ON_OFF_TIME) * BLINK_PB_NB;
	m_st_blinkConfig.ui8_firstPhaseDC = BLINK_DUTY_CYCLE;
	m_st_blinkConfig.ui8_secondPhaseDC = 0;
	
	m_st_config.b_isManualMode	= false;
	m_st_config.en_timeModeName	= en_timeMode;
}

void PlantLight::v_PlantLight_event(const SCHEDULED_EVENT en_scheduledEvent)
{
	/* Excecute certains états prioritaires de PlantLight avant les Events */
	switch(m_en_currentStatus) {
		/* Rampe d'allumage */
		case PLANTLIGHT_LIGHTING_UP:
			v_PlantLight_lightingUp();
			break;
			
		/* Rampe d'extinction */
		case PLANTLIGHT_LIGHTING_DOWN:
			v_PlantLight_lightingDown();
			break;
			
		/* Mode réglage (respiration) */
		case PLANTLIGHT_BREATHE:
			v_PlantLight_breathe();
			break;
			
		/* Mode clignotement */
		case PLANTLIGHT_TIME_SETTING:
			v_PlantLight_blink();
			break;
			
		/* Informe d'une erreur mineure (p.ex. pompe hors de l'eau) */
		case PLANTLIGHT_BLINK_PB:
			v_PlantLight_activateBlinkPb(en_scheduledEvent);
			break;
			
		/* Clignotement d'information de changement de réglage */
		case PLANTLIGHT_BLINK_CONFIRMATION:
			v_PlantLight_blink();
			break;
			
		/* Aucun état prioritaire, exécute les Events */
		default:
			switch(en_scheduledEvent) {
				/* Début du cycle lumineux */
				case START_PLANTLIGHT_CYCLE:
					/* Si l'état PLANTLIGHT_MAX n'est pas appliqué, début de la rampe */
					if(m_en_currentStatus != PLANTLIGHT_MAX) {
						m_en_currentStatus = PLANTLIGHT_LIGHTING_UP;
					} else {/* Nothing to do - Les plantes sont déjà au max */}
					break;

				/* Fin du cycle lumineux */
				case STOP_PLANTLIGHT_CYCLE:
					/* Si l'état PLANTLIGHT_MIN n'est pas appliqué, début de la rampe */
					if(m_en_currentStatus != PLANTLIGHT_STOP) {
						m_en_currentStatus = PLANTLIGHT_LIGHTING_DOWN;
					} else {/* Nothing to do - Les plantes sont déjà au max */}
					break;

				/* Mode manuel ON */
				case ON_MANUAL_LIGHT_MODE:
					/* Si l'état PLANTLIGHT_MAX n'est pas appliqué, début de la rampe */
					if(m_en_currentStatus != PLANTLIGHT_MANUAL) {
						m_en_currentStatus = PLANTLIGHT_LIGHTING_UP;
					} else {/* Nothing to do - Les plantes sont déjà au max */}
					break;

				/* Mode manuel OFF */
				case OFF_MANUAL_LIGHT_MODE:
					/* Si l'état PLANTLIGHT_MIN n'est pas appliqué, début de la rampe */
					if(m_en_currentStatus != PLANTLIGHT_MANUAL) {
						m_en_currentStatus = PLANTLIGHT_LIGHTING_DOWN;
					} else {/* Nothing to do - Les plantes sont déjà au max */}
					break;

				/* Evenement inconnu, éteint la lumières */
				default:
					LOG("CodeError : cas switch non traité");
					en_drvPWM_changeDutyCycle(m_ui8_plantLightInstance, 0);
					m_en_currentStatus = PLANTLIGHT_NO_STATUS;
					break;
			}
			break;
	}
/* Explication de l'état BLINK_PB :
 * Si problème, HandlerClient appelle v_PlantLight_activateBlinkPb() pour activer l'état PLANTLIGHT_BLINK_PB.
 * v_PlantLight_blink() retire l'état après sa période de clignotement terminé
 * Tant que la période de blink n'est pas fini, le Status PLANTLIGHT_BLINK_PB est prioritaire sur l'Event
 */
}

void PlantLight::v_PlantLight_lightingUp()
{
	if(m_en_currentStatus != PLANTLIGHT_LIGHTING_UP) {
		m_en_currentStatus = PLANTLIGHT_LIGHTING_UP;
		m_ui8_tmpLightingValue = 0;
		m_ui16_plantLightTimer = 0;
	} else {/* Nothing to do - Mode allumage déjà activé */}

	/* Attente entre deux pas */
	if(m_ui16_plantLightTimer < TIME_BTW_TWO_DC_STEPS) {
		m_ui16_plantLightTimer++;
	} else {
		m_ui16_plantLightTimer = 0;
		
		/* Si rampe d'allumage en cours */
		if(m_ui8_tmpLightingValue < MAX_DUTY_CYCLE) {
			
			m_ui8_tmpLightingValue++;
			en_drvPWM_changeDutyCycle(m_ui8_plantLightInstance, m_ui8_tmpLightingValue);

		/* Fin de la rampe d'allumage */	
		} else {
			if(m_st_config.b_isManualMode) {
				m_en_currentStatus = PLANTLIGHT_MANUAL;
			} else {
				m_en_currentStatus = PLANTLIGHT_MAX;
			}
			m_ui16_plantLightTimer = 0;
		}
	}	
}

void PlantLight::v_PlantLight_lightingDown()
{	
	if(m_en_currentStatus != PLANTLIGHT_LIGHTING_DOWN) {
		m_en_currentStatus = PLANTLIGHT_LIGHTING_DOWN;
		m_ui8_tmpLightingValue = MAX_DUTY_CYCLE;
		m_ui16_plantLightTimer = 0;
	} else {/* Nothing to do - Mode extinction déjà activé */}

	/* Attente entre deux pas */
	if(m_ui16_plantLightTimer < TIME_BTW_TWO_DC_STEPS) {
		m_ui16_plantLightTimer++;
	} else {
		m_ui16_plantLightTimer = 0;
		
		/* Si rampe d'extinction en cours */
		if(m_ui8_tmpLightingValue > 0) {
			m_ui8_tmpLightingValue--;
			
			en_drvPWM_changeDutyCycle(m_ui8_plantLightInstance, m_ui8_tmpLightingValue);		
		/* Fin de la rampe d'allumage */	
		} else {
			if(m_st_config.b_isManualMode) {
				m_en_currentStatus = PLANTLIGHT_MANUAL;
			} else {
				m_en_currentStatus = PLANTLIGHT_STOP;
			}
			m_ui16_plantLightTimer = 0;
		}
	}
}

void PlantLight::v_PlantLight_enterSettingMode()
{
	v_PlantLight_settingMode(ENABLE);
}

void PlantLight::v_PlantLight_exitSettingMode(const SCHEDULED_EVENT en_nextEvent)
{
	v_PlantLight_settingMode(DISABLE);
	
	if(en_nextEvent == START_PLANTLIGHT_CYCLE || en_nextEvent == ON_MANUAL_LIGHT_MODE) {
		v_PlantLight_lightingUp();
	} else {
		v_PlantLight_lightingDown();
	}
}

inline void PlantLight::v_PlantLight_settingMode(const BOOL b_enableSetting)
{
	/* Si déjà en mode réglage, quitte le mode, sinon configure le mode réglage */
	if(m_en_currentStatus == PLANTLIGHT_BREATHE || !b_enableSetting) {
		m_en_currentStatus = PLANTLIGHT_NO_STATUS;
	} else {
		m_en_currentStatus = PLANTLIGHT_BREATHE;
		m_b_isBreatheUp = false;
		m_ui16_plantLightTimer = 0;
		m_ui8_tmpLightingValue = MIN_DC_BREATHE;

		v_PlantLight_breathe();
	}
}

void PlantLight::v_PlantLight_manualMode(const SCHEDULED_EVENT en_plantLightEvent)
{
	m_st_config.b_isManualMode = !m_st_config.b_isManualMode;
	
	/* Si la lumière était allumée, l'eteint par une rampe et inversement */
	if(en_plantLightEvent == ON_MANUAL_LIGHT_MODE || en_plantLightEvent == START_PLANTLIGHT_CYCLE) {
		v_PlantLight_lightingDown();
	} else {
		v_PlantLight_lightingUp();
	}
}

void PlantLight::v_PlantLight_timeSettingMode(const BOOL b_isCurrentMode)
{
	m_ui16_plantLightTimer = 0;
	m_ui8_tmpLightingValue = 0;
	m_ui8_blinkIterator = 0;
	m_en_currentStatus = PLANTLIGHT_TIME_SETTING;
	
	if(b_isCurrentMode) {
		m_st_blinkConfig.ui8_nbOfBlink = static_cast<UINT8>(m_st_config.en_timeModeName);
		m_ui8_numberOfBlink = m_st_blinkConfig.ui8_nbOfBlink;
		m_st_blinkConfig.ui16_offsetTime = BLINK_STG_OFFSET;
	} else {
		m_ui8_numberOfBlink++;
		if(m_ui8_numberOfBlink > NB_CONFIG_TIME_MODE) {
			m_ui8_numberOfBlink = 0;
		} else {/* Nothing to do - Le mode a correctement été incrémenté */}
		m_st_blinkConfig.ui8_nbOfBlink = m_ui8_numberOfBlink;
		m_st_blinkConfig.ui16_offsetTime = 0;
	}
	
	//m_st_blinkConfig.ui16_offsetTime = 0;
	m_st_blinkConfig.ui8_firstPhaseDC = BLINK_DUTY_CYCLE;
	m_st_blinkConfig.ui8_secondPhaseDC = 0;
	m_st_blinkConfig.ui16_firstPhaseTime = BLINK_STG_TIME_ON;
	m_st_blinkConfig.ui16_secondPhaseTime = BLINK_STG_TIME_OFF;
	m_st_blinkConfig.ui16_totalPeriodeTime = BLINK_STG_TIME_PERIOD;
	
	v_PlantLight_blink();
}

void PlantLight::v_PlantLight_choseSoftMode(const UINT8 ui8_blinkNumber)
{
	m_ui16_plantLightTimer = 0;
	m_ui8_blinkIterator = 0;
	m_en_currentStatus = PLANTLIGHT_TIME_SETTING;

	m_st_blinkConfig.ui8_nbOfBlink = ui8_blinkNumber;

	m_st_blinkConfig.ui8_firstPhaseDC = BLINK_DUTY_CYCLE;
	m_st_blinkConfig.ui8_secondPhaseDC = 0;
	m_st_blinkConfig.ui16_offsetTime = 0;
	m_st_blinkConfig.ui16_firstPhaseTime = BLINK_STG_TIME_ON;
	m_st_blinkConfig.ui16_secondPhaseTime = BLINK_STG_TIME_OFF;
	m_st_blinkConfig.ui16_totalPeriodeTime = BLINK_STG_TIME_PERIOD;

	v_PlantLight_blink();
}

void PlantLight::v_PlantLight_setBrightness(const UINT8 ui8_brightnessValue) const
{
	en_drvPWM_changeDutyCycle(m_ui8_plantLightInstance, ui8_brightnessValue);
}

void PlantLight::v_PlantLight_breathe()
{
	/* Attente entre deux pas */
	if(m_ui16_plantLightTimer < TIME_BTW_TWO_BREATHE_STEPS) {
		m_ui16_plantLightTimer++;
	} else {
		m_ui16_plantLightTimer = 0;
		
		en_drvPWM_changeDutyCycle(m_ui8_plantLightInstance, m_ui8_tmpLightingValue);
		
		/* Si respiration positive (augmentation de la luminosité) */
		if(m_b_isBreatheUp) {
			if(m_ui8_tmpLightingValue < MAX_DC_BREATHE) {
				m_ui8_tmpLightingValue++;
			} else {
				m_b_isBreatheUp = false;
			}
		/* Sinon respiration negative (diminution de la luminosité) */
		} else {
			if(m_ui8_tmpLightingValue > MIN_DC_BREATHE && m_ui8_tmpLightingValue <= MAX_DC_BREATHE) {
				m_ui8_tmpLightingValue--;
			} else {
				m_b_isBreatheUp = true;
			}
		}
	}
}

void PlantLight::v_PlantLight_saveTimeMode()
{
	m_st_config.en_timeModeName = static_cast<PLANTLIGHT_CONFIG_TIME>(m_ui8_numberOfBlink);
}

const PLANTLIGHT_CONFIG_TIME PlantLight::en_PlantLight_getTimeModeName() const
{
	return m_st_config.en_timeModeName;
}

void PlantLight::v_PlantLight_blinkConfig(const BOOL b_isPositifBlink, const UINT8 ui8_nbBlink, const UINT16 ui16_blinkTimeOff)
{
	m_ui16_plantLightTimer = 0;
	m_ui8_tmpLightingValue = 0;
	m_ui8_blinkIterator = 0;
	m_b_isBlinking = true;
	
	m_st_blinkConfig.ui8_nbOfBlink = ui8_nbBlink;
	m_st_blinkConfig.ui16_offsetTime = 0;
	m_st_blinkConfig.ui16_firstPhaseTime = BLINK_CONFIRM_TIME_ON_OFF;
	m_st_blinkConfig.ui16_secondPhaseTime = ui16_blinkTimeOff;
	m_st_blinkConfig.ui16_totalPeriodeTime = (BLINK_CONFIRM_TIME_ON_OFF + ui16_blinkTimeOff) * ui8_nbBlink;

	v_PlantLight_setBlinkDirection(b_isPositifBlink);

	v_PlantLight_blink();
}

void PlantLight::v_PlantLight_blinkConfirmation(const BOOL b_isPositifBlink, const UINT8 ui8_nbBlink)
{
	m_en_currentStatus = PLANTLIGHT_BLINK_CONFIRMATION;
	v_PlantLight_blinkConfig(b_isPositifBlink, ui8_nbBlink, BLINK_CONFIRM_TIME_ON_OFF);
}

void PlantLight::v_PlantLight_blinkSlow(const BOOL b_isPositifBlink, const UINT8 ui8_nbBlink)
{
	m_en_currentStatus = PLANTLIGHT_BLINK_CONFIRMATION;
	v_PlantLight_blinkConfig(b_isPositifBlink, ui8_nbBlink, 300);
}

void PlantLight::v_PlantLight_blinkConfirmSetTime()
{
	m_en_nextStatus = PLANTLIGHT_BREATHE;
	v_PlantLight_blinkConfirmation(BLINK_ON, NB_BLINK_CONFIRMATION);
}

const BOOL PlantLight::b_PlantLight_getIsBlinking() const
{
	return m_b_isBlinking;
}

void PlantLight::v_PlantLight_configBlinkPb()
{
	m_ui16_plantLightTimer = 0;
	m_ui8_blinkIterator = 0;
	
	m_st_blinkConfig.ui8_nbOfBlink = BLINK_PB_NB;
	m_st_blinkConfig.ui16_offsetTime = 0;
	m_st_blinkConfig.ui16_firstPhaseTime = BLINK_PB_ON_OFF_TIME;
	m_st_blinkConfig.ui16_secondPhaseTime = BLINK_PB_ON_OFF_TIME;
	m_st_blinkConfig.ui16_totalPeriodeTime = (BLINK_PB_ON_OFF_TIME + BLINK_PB_ON_OFF_TIME) * BLINK_PB_NB;
}

void PlantLight::v_PlantLight_activateBlinkPb(const SCHEDULED_EVENT en_scheduledEvent)
{
	if(m_en_currentStatus != PLANTLIGHT_BLINK_PB) {
		m_en_currentStatus = PLANTLIGHT_BLINK_PB;
		v_PlantLight_configBlinkPb();
		
		(en_scheduledEvent == ON_MANUAL_LIGHT_MODE || en_scheduledEvent == START_PLANTLIGHT_CYCLE)
			? v_PlantLight_setBlinkDirection(BLINK_OFF)
			: v_PlantLight_setBlinkDirection(BLINK_ON);
	} else {/* Nothing to do - Le status est déja configuré */}
	
	v_PlantLight_blink();
}

inline void PlantLight::v_PlantLight_setBlinkDirection(const BOOL b_isPositifBlink)

{
	if(b_isPositifBlink) {
		m_st_blinkConfig.ui8_firstPhaseDC = BLINK_DUTY_CYCLE;
		m_st_blinkConfig.ui8_secondPhaseDC = 0;
	} else {
		m_st_blinkConfig.ui8_firstPhaseDC = 0;
		m_st_blinkConfig.ui8_secondPhaseDC = MAX_DUTY_CYCLE;
	}
}

void PlantLight::v_PlantLight_blink()
{
	/* S'il y a un offset avant le clignotement, applique la valeur de la 2nd phase et attend le début de la 1ere phase */
	if(m_ui16_plantLightTimer == 0 && m_st_blinkConfig.ui16_offsetTime != 0) {
		en_drvPWM_changeDutyCycle(m_ui8_plantLightInstance, m_st_blinkConfig.ui8_secondPhaseDC);
	} else {/* Nothing to do - Pas d'offset ou pas au début de la période */}
	
	/* Début des clignotements */
	if(m_ui8_blinkIterator <= m_st_blinkConfig.ui8_nbOfBlink) {
		/* 1ere phase */
		if(m_ui16_plantLightTimer ==
			(m_st_blinkConfig.ui16_offsetTime + 
			(m_st_blinkConfig.ui16_firstPhaseTime + m_st_blinkConfig.ui16_secondPhaseTime) * m_ui8_blinkIterator)) {
			en_drvPWM_changeDutyCycle(m_ui8_plantLightInstance, m_st_blinkConfig.ui8_firstPhaseDC);
		/* 2eme phase */
		} else if(m_ui16_plantLightTimer ==
			(m_st_blinkConfig.ui16_offsetTime + m_st_blinkConfig.ui16_firstPhaseTime +
			(m_st_blinkConfig.ui16_firstPhaseTime + m_st_blinkConfig.ui16_secondPhaseTime) * m_ui8_blinkIterator)) {
			en_drvPWM_changeDutyCycle(m_ui8_plantLightInstance, m_st_blinkConfig.ui8_secondPhaseDC);

			/* La periode de clignotement a été exécuté, incrémente m_ui8_blinkIterator pour faire un clignotement de plus */
			m_ui8_blinkIterator++;
		} else {/* Nothing to do - Actuellement entre deux fronts de blink */}
	} else {/* Nothing to do - Tous les clignotement de la periode ont été réalisés */}
		
	m_ui16_plantLightTimer++;
	
	/* Tous les clignotement ont eu lieu, fin de la période */
	if(m_ui16_plantLightTimer >= (m_st_blinkConfig.ui16_offsetTime + m_st_blinkConfig.ui16_totalPeriodeTime)) {

		/* Réinitialisation des paramètres */
		m_ui16_plantLightTimer = 0;
		m_ui8_blinkIterator = 0;
		m_b_isBlinking = false;
		
		/* Suppression de l'offset après la 1ere période */
		m_st_blinkConfig.ui16_offsetTime = 0;
		
		/* S'il s'agit d'un clignotement d'information de pb, supprime l'état car il sera réappliqué par le Scheduler */
		if(m_en_currentStatus == PLANTLIGHT_BLINK_PB) {
			m_en_currentStatus = PLANTLIGHT_NO_STATUS;
		} else if(m_en_currentStatus == PLANTLIGHT_BLINK_CONFIRMATION) {
			/* Supprime l'état après le blink */
			m_en_currentStatus = PLANTLIGHT_NO_STATUS;
		} else {/* Nothing to do - Le mode de blink en court tourne à l'infini */}
			
		if(m_en_nextStatus != PLANTLIGHT_NO_STATUS) {
			m_en_currentStatus = m_en_nextStatus;
			m_en_nextStatus = PLANTLIGHT_NO_STATUS;
		} else {/* Nothing to do - Aucun état de programmé après le blink */}
	} else {/* Nothing to do - La période totale de clignotement n'est pas fini */}
}