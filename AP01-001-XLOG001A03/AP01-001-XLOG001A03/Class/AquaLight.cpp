/* 
* AquaLight.cpp
*
* Created: 08/01/2024 18:43:39
* Author: LucasTeissier
*/

#include "AquaLight.h"
#include "drvHardware.h"
#include "drvPWM.h"
#include "drvADC.h"

/* Hardware setting */
#define CHANEL_PWM_AQUALIGHT							PWM_CHANNEL_4
#define CHANEL_ADC_AQUALIGHT							ADC_CHANNEL_9
#define INSTANCE_ADC_AQUALIGHT							ADC_INSTANCE_1

/* ADC setting */
#define AVERAGE_READING									 30
#define READING_HW_DELAY						(UINT16)(500)	/* Temps d'attente avant la lecture HW (en ms) */
#define MIN_DC_READING_ADC								 10		/* En dessous de cette valeur, pas de lecture de l'ADC pendant les rampes */
#define RANGE_LIMIT_LIGHT_ON_ADC						 10		/* Ecart permis entre la mesure de l'ADC et la détection d'une erreur allumé, en % */
//#define RANGE_LIMIT_LIGHT_OFF_ADC						 300	/* Ecart permis entre la mesure de l'ADC et la détection d'une erreur éteint, en % */

/* UI/UX parameter */
#define TIME_BTW_TWO_DC_STEPS							 5		/* Temps en ms entre deux pas du RC pendant les rampes on/off */
#define TIME_BTW_TWO_BREATHE_STEPS						 9		/* Temps en ms entre deux pas de l'effet respiration (mode reglage) */
#define DC_VALUE_TO_TEST_ADC							 10		/* Valeur du DC pour tester l'ADC au démarrage après une CriticalError */
#define TIME_TEST_ADC									 1000	/* Temps en ms de test de l'ADC au démarrage après une CriticalError  */

/* Breath Mode */
#define MIN_DC_BREATHE									 20		/* Valeur MIN du rapport cyclique de la respiration */
#define MAX_DC_BREATHE									 100	/* Valeur MAX du rapport cyclique de la respiration */

/* Blink Mode */
#define BLINK_STG_TIME_PERIOD					(UINT16)(3000)	/* Temps entre deux clignotements (en ms) */
#define BLINK_STG_TIME_OFF								 200	/* Temps entre deux clignotements (en ms) */
#define BLINK_STG_TIME_ON								 150	/* Temps d'un clignotement allumé (en ms) */
#define	BLINK_DUTY_CYCLE								 20		/* Rapport cyclique max du clignotement */
#define NB_BLINK_CONFIRMATION							 2
#define BLINK_CONFIRM_TIME_ON_OFF						 80
#define BLINK_STG_OFFSET								 300	/* Temps d'attente avant le premier clignotement */

/* Time Mode */
#define NB_CONFIG_TIME_MODE								 3		/* Nombre de mode de temps d'éclairage (0 étant compté, 4 mode ici) */

/* Intensity Mode */
#define NB_CONFIG_INTENSITY_MODE						 3		/* Nombre de mode d'intensité d'éclairage (0 étant compté, 4 mode ici) */
#define LUMEN_0_DC										 0		/* Ne compte pas comme un mode d'éclairage, utile pour définir les valeur d'ADC */
#define LUMEN_20_DC										 40
#define LUMEN_25_DC										 60
#define LUMEN_30_DC										 80
#define LUMEN_35_DC										 100
#define NB_CONFIG_TIME_MODE								 3		/* Nombre de mode de temps d'éclairage (0 étant compté, 4 mode ici) */

// default constructor
AquaLight::AquaLight()
{
} //AquaLight

// default destructor
AquaLight::~AquaLight()
{
} //~AquaLight

void AquaLight::v_AquaLight_initialization()
{
	en_drvPWM_initialization(CHANEL_PWM_AQUALIGHT, 0, &m_ui8_aquaLightInstance);
#ifndef DEBUG_MOD_NO_ADC
	en_drvADC_initialization(CHANEL_ADC_AQUALIGHT, INSTANCE_ADC_AQUALIGHT, &m_ui8_instanceADC);
#endif
}

void AquaLight::v_AquaLight_configuration(const SETTINGS st_setttings)
{
	m_st_config.b_isManualMode = false;
	m_st_config.en_timeModeName = st_setttings.en_timeModeAquaLight;
	m_st_config.en_intensityModeName = st_setttings.en_intensityAquaLight;
	
	m_st_blinkConfig.ui8_nbOfBlink = 0;
	m_st_blinkConfig.ui16_offsetTime = 0;
	m_st_blinkConfig.ui16_firstPhaseTime = BLINK_STG_TIME_ON;
	m_st_blinkConfig.ui16_secondPhaseTime = BLINK_STG_TIME_OFF;
	m_st_blinkConfig.ui16_totalPeriodeTime = BLINK_STG_TIME_PERIOD;
	m_st_blinkConfig.ui8_firstPhaseDC = BLINK_DUTY_CYCLE;
	m_st_blinkConfig.ui8_secondPhaseDC = 0;
	
	v_AquaLight_setIntensityValue(m_st_config.en_intensityModeName);
	
	/* Config des limites de l'ADC :
	 * - Si premier lancement du soft, les limites sont calculées après v_AquaLight_defineADCValue(), appelé dans le Handler
	 * - Si le soft a déja été lancé, les limites sont calculé dans v_AquaLight_configADC(), appelé dans le Handler
	 */
}

void AquaLight::v_AquaLight_event(const SCHEDULED_EVENT en_scheduledEvent)
{
	/* Excecute certains états prioritaires de PlantLight avant les Events */
	switch(m_en_currentStatus) {
		/* Problème détecté */
		case AQUALIGHT_CRITICAL_PROBLEM:
			if(!m_b_isCriticalError) {
				m_b_isCriticalError = true;
				en_drvPWM_changeDutyCycle(m_ui8_aquaLightInstance, 0);
			} else {/* Nothing to do - Le mode est déjà activé */}
			break;

		/* Rampe d'allumage */
		case AQUALIGHT_LIGHTING_UP:
			v_AquaLight_lightingUp();
			break;
		
		/* Rampe d'extinction */
		case AQUALIGHT_LIGHTING_DOWN:
			v_AquaLight_lightingDown();
			break;
		
		/* Mode réglage (respiration) */
		case AQUALIGHT_BREATHE:
			v_AquaLight_breathe();
			break;
			
		/* Mode choix temps eclairage (clignotement) */
		case AQUALIGHT_TIME_SETTING:
			v_AquaLight_blink();
			break;
			
		/* Mode choix intensité lumineuse */
		case AQUALIGHT_INTENSITY_SETTING:
			/* Nothing to do - L'intensité a déja été configurée et ne varie pas */
			break;
			
		/* Clignotement de confirmation du changement d'heure */
		case AQUALIGHT_BLINK_CONFIRM_SET_HOUR:
			v_AquaLight_blink();
			break;
			
		/* Mode pour tester l'ADC après un démarrage avec une CriticalError */
		case AQUALIGHT_TEST_ADC:
			v_AquaLight_testADC();
			break;
			
		/* Mode utilisé qu'au 1er démarrage pour définir les valeurs de référence de l'adc */
		case AQUALIGHT_DEFINE_ADC_VALUE:
			v_AquaLight_defineADCValue();
			break;

		/* Aucun états prioritaire, exécute les Events */
		default:
			switch(en_scheduledEvent) {
				/* Début du cycle lumineux */
				case START_AQUALIGHT_CYCLE:
					/* Si l'état AQUALIGHT_MAX n'est pas appliqué, début de la rampe */
					if(m_en_currentStatus != AQUALIGHT_MAX) {
						m_en_currentStatus = AQUALIGHT_LIGHTING_UP;
					} else {/* Nothing to do - Les plantes sont déjà au max */}
					break;

					/* Fin du cycle lumineux */
				case STOP_AQUALIGHT_CYCLE:
					/* Si l'état AQUALIGHT_MIN n'est pas appliqué, début de la rampe */
					if(m_en_currentStatus != AQUALIGHT_STOP) {
						m_en_currentStatus = AQUALIGHT_LIGHTING_DOWN;
					} else {/* Nothing to do - Les plantes sont déjà au max */}
					break;

				/* Mode manuel ON */
				case ON_MANUAL_LIGHT_MODE:
					/* Si l'état PLANTLIGHT_MAX n'est pas appliqué, début de la rampe */
					if(m_en_currentStatus != AQUALIGHT_MANUAL_ON) {
						m_en_currentStatus = AQUALIGHT_LIGHTING_UP;
					} else {/* Nothing to do - Les plantes sont déjà au max */}
					break;

				/* Mode manuel OFF */
				case OFF_MANUAL_LIGHT_MODE:
					/* Si l'état PLANTLIGHT_MIN n'est pas appliqué, début de la rampe */
					if(m_en_currentStatus != AQUALIGHT_MANUAL_OFF) {
						m_en_currentStatus = AQUALIGHT_LIGHTING_DOWN;
					} else {/* Nothing to do - Les plantes sont déjà au max */}
					break;

				/* Evenement inconnu, éteint la lumières */
				default:
					LOG("CodeError : cas switch non traité, %d", en_scheduledEvent);
					en_drvPWM_changeDutyCycle(m_ui8_aquaLightInstance, 0);
					m_en_currentStatus = AQUALIGHT_NO_STATUS;
					break;
			}
			break;
	}
#ifndef DEBUG_MOD_NO_ADC
	if(!m_b_noBrickMode) {
		v_AquaLight_checkADCValue();
	} else {/* Ne rien fire, le produit ne fait plu de test ADC car la fonction de brick est désactivée */}
#endif
}

void AquaLight::v_AquaLight_lightingUp()
{	
	if(m_en_currentStatus != AQUALIGHT_LIGHTING_UP) {
		m_en_currentStatus = AQUALIGHT_LIGHTING_UP;
		m_ui8_tmpLightingValue = 0;
		m_ui16_aquaLightTimer = 0;
		m_ui16_readingHWTimer = 0;
	} else {/* Nothing to do - Mode allumage déjà activé */}

	/* Attente entre deux pas */
	if(m_ui16_aquaLightTimer < TIME_BTW_TWO_DC_STEPS) {
		m_ui16_aquaLightTimer++;
	} else {
		m_ui16_aquaLightTimer = 0;
		
		/* Si rampe d'allumage en cours */
		if(m_ui8_tmpLightingValue < m_st_config.ui8_intensityValue) {
			
			m_ui8_tmpLightingValue++;
			en_drvPWM_changeDutyCycle(m_ui8_aquaLightInstance, m_ui8_tmpLightingValue);

		/* Fin de la rampe d'allumage */	
		} else {
			if(m_st_config.b_isManualMode) {
				m_en_currentStatus = AQUALIGHT_MANUAL_ON;
			} else {
				m_en_currentStatus = AQUALIGHT_MAX;
			}
			
			m_ui16_aquaLightTimer = 0;
			m_ui16_readingHWTimer = 0;
		}
	}	
}

void AquaLight::v_AquaLight_lightingDown()
{
	if(m_en_currentStatus != AQUALIGHT_LIGHTING_DOWN) {
		m_en_currentStatus = AQUALIGHT_LIGHTING_DOWN;
		m_ui8_tmpLightingValue = m_st_config.ui8_intensityValue;
		m_ui16_aquaLightTimer = 0;
		m_ui16_readingHWTimer = 0;
	} else {/* Nothing to do - Mode allumage déjà activé */}

	/* Attente entre deux pas */
	if(m_ui16_aquaLightTimer < TIME_BTW_TWO_DC_STEPS) {
		m_ui16_aquaLightTimer++;
	} else {
		m_ui16_aquaLightTimer = 0;
			
		/* Si rampe d'extinction en cours */
		if(m_ui8_tmpLightingValue > 0) {
			m_ui8_tmpLightingValue--;
				
			en_drvPWM_changeDutyCycle(m_ui8_aquaLightInstance, m_ui8_tmpLightingValue);
			/* Fin de la rampe d'allumage */
		} else {
			if(m_st_config.b_isManualMode) {
				m_en_currentStatus = AQUALIGHT_MANUAL_OFF;
			} else {
				m_en_currentStatus = AQUALIGHT_STOP;
			}
			
			m_ui16_aquaLightTimer = 0;
			m_ui16_readingHWTimer = 0;
		}
	}
}

void AquaLight::v_AquaLight_checkADCValue()
{	
	AQUALIGHT_STATUS	en_tmpCurrentStatus;
	UINT16				ui16_adcValue = 0;

	m_ui16_readingHWTimer++;
		
	if(m_ui16_readingHWTimer >= READING_HW_DELAY) {
		
		en_tmpCurrentStatus = m_en_currentStatus;
		m_ui16_readingHWTimer = 0;
				
		/* L'ADC est lu que si le 12.6V de la carte est OK (PGOOD_12VDC = 1) */
		if(b_drvHardware_getPinValue(GPIO_PORTA, 4)) {

			en_drvADC_getInputValue(m_ui8_instanceADC, AVERAGE_READING, &ui16_adcValue);
		
			switch(m_en_currentStatus) {
				case AQUALIGHT_MAX:
				case AQUALIGHT_MANUAL_ON:
				case AQUALIGHT_INTENSITY_SETTING:
				case AQUALIGHT_TEST_ADC:
					if(ui16_adcValue < m_st_adcLimit.ui16_adcMinLimitActual || ui16_adcValue > m_st_adcLimit.ui16_adcMaxLimitActual) {
						LOG("ADC Problem - value : %d", ui16_adcValue);
						m_en_currentStatus = AQUALIGHT_CRITICAL_PROBLEM;
					} else {}
					break;

				case AQUALIGHT_STOP:
				case AQUALIGHT_MANUAL_OFF:
					if(ui16_adcValue > m_st_adcLimit.ui16_adcMaxLimitOff) {
						LOG("ADC Problem - value : %d", ui16_adcValue);
						m_en_currentStatus = AQUALIGHT_CRITICAL_PROBLEM;
					} else {}
					break;

				case AQUALIGHT_BREATHE:
					if((ui16_adcValue < m_st_adcLimit.ui16_adcMinLimit20 || ui16_adcValue > m_st_adcLimit.ui16_adcMaxLimit35) && (m_ui8_tmpLightingValue > LUMEN_20_DC)) {
						LOG("ADC Problem - value : %d", ui16_adcValue);
						m_en_currentStatus = AQUALIGHT_CRITICAL_PROBLEM;
					} else {}
					break;
				
				case AQUALIGHT_LIGHTING_UP:
				case AQUALIGHT_LIGHTING_DOWN:
				case AQUALIGHT_TIME_SETTING:
				case AQUALIGHT_BLINK_CONFIRM_SET_HOUR:
					/* Nothing to do - Etat oscillant, impossible de mesurer l'ADC */
					break;
				
				case AQUALIGHT_DEFINE_ADC_VALUE:
					/* Nothing to do - Définition des valeurs de l'ADC dans v_AquaLight_defineADCValue() */
					break;
				
				default:
					LOG("CodeError : cas switch non traité, %d", m_en_currentStatus);
					break;
			}
			
			/* Re vérification de l'alimentation 12.6V de la carte, si NOK après les mesures (PGOOD_12VDC = 0), l'erreur est écrasée par l'ancien état */
			if(!b_drvHardware_getPinValue(GPIO_PORTA, 4)) {
				m_en_currentStatus = en_tmpCurrentStatus;
			} else {/* Alimentation 12.6V OK, si erreur elle est remontée */}
				
				
		} else {/* Alimentation 12.6V NOK */}
	}
}

void AquaLight::v_AquaLight_enterSettingMode()
{
	v_AquaLight_settingMode(ENABLE);
}

void AquaLight::v_AquaLight_exitSettingMode(const SCHEDULED_EVENT en_nextEvent)
{
	v_AquaLight_settingMode(DISABLE);
	
	if(en_nextEvent == START_AQUALIGHT_CYCLE || en_nextEvent == ON_MANUAL_LIGHT_MODE) {
		v_AquaLight_lightingUp();
	} else {
		v_AquaLight_lightingDown();
	}
}

inline void AquaLight::v_AquaLight_settingMode(const BOOL b_enableSetting)
{	
	/* Si déjà en mode réglage, quitte le mode, sinon configure le mode réglage */
	if(m_en_currentStatus == AQUALIGHT_BREATHE || !b_enableSetting) {
		m_en_currentStatus = AQUALIGHT_NO_STATUS;
	} else {
		m_en_currentStatus = AQUALIGHT_BREATHE;
		m_b_isBreatheUp = false;
		m_ui16_aquaLightTimer = 0;
		m_ui8_tmpLightingValue = MIN_DC_BREATHE;
		m_ui16_readingHWTimer = 0;

		v_AquaLight_breathe();
	}
}

void AquaLight::v_AquaLight_manualMode(const SCHEDULED_EVENT en_aquaLightEvent)
{
	m_st_config.b_isManualMode = !m_st_config.b_isManualMode;
	
	/* Si la lumière était allumée, l'eteint par une rampe et inversement */
	if(en_aquaLightEvent == ON_MANUAL_LIGHT_MODE || en_aquaLightEvent == START_AQUALIGHT_CYCLE) {
		v_AquaLight_lightingDown();
	} else {
		v_AquaLight_lightingUp();
	}
}

void AquaLight::v_AquaLight_timeSettingMode(const BOOL b_isCurrentMode)
{
	m_ui16_aquaLightTimer = 0;
	m_ui8_blinkIterator = 0;
	m_en_currentStatus = AQUALIGHT_TIME_SETTING;
	
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
	
	m_st_blinkConfig.ui8_firstPhaseDC = BLINK_DUTY_CYCLE;
	m_st_blinkConfig.ui8_secondPhaseDC = 0;
	m_st_blinkConfig.ui16_offsetTime = BLINK_STG_OFFSET;
	m_st_blinkConfig.ui16_firstPhaseTime = BLINK_STG_TIME_ON;
	m_st_blinkConfig.ui16_secondPhaseTime = BLINK_STG_TIME_OFF;
	m_st_blinkConfig.ui16_totalPeriodeTime = BLINK_STG_TIME_PERIOD;
	
	v_AquaLight_blink();
}

void AquaLight::v_AquaLight_blinkConfirmSetHour()
{
	m_ui16_aquaLightTimer = 0;
	m_ui8_tmpLightingValue = 0;
	m_ui8_blinkIterator = 0;
	m_en_currentStatus = AQUALIGHT_BLINK_CONFIRM_SET_HOUR;
	
	m_st_blinkConfig.ui8_nbOfBlink = NB_BLINK_CONFIRMATION;
	m_st_blinkConfig.ui16_offsetTime = 0;
	m_st_blinkConfig.ui16_firstPhaseTime = BLINK_CONFIRM_TIME_ON_OFF;
	m_st_blinkConfig.ui16_secondPhaseTime = BLINK_CONFIRM_TIME_ON_OFF;
	m_st_blinkConfig.ui16_totalPeriodeTime = BLINK_CONFIRM_TIME_ON_OFF * NB_BLINK_CONFIRMATION * 2;

	m_st_blinkConfig.ui8_firstPhaseDC = 0;
	m_st_blinkConfig.ui8_secondPhaseDC = BLINK_DUTY_CYCLE;
		
	v_AquaLight_blink();
}

void AquaLight::v_AquaLight_setBrightness(const UINT8 ui8_brightnessValue) const
{
	en_drvPWM_changeDutyCycle(m_ui8_aquaLightInstance, ui8_brightnessValue);
}

void AquaLight::v_AquaLight_testADC()
{
	if(m_en_currentStatus != AQUALIGHT_TEST_ADC) {
		m_en_currentStatus = AQUALIGHT_TEST_ADC;
		 en_drvPWM_changeDutyCycle(m_ui8_aquaLightInstance, m_st_config.ui8_intensityValue);
		m_ui16_aquaLightTimer = 0;
		m_ui16_readingHWTimer = 0;
	} else {
		m_ui16_aquaLightTimer++;
		
		if(m_ui16_aquaLightTimer > TIME_TEST_ADC) {
			m_ui16_aquaLightTimer = 0;
			m_en_currentStatus = AQUALIGHT_NO_STATUS;
			m_b_isDeletedFirstCritErr = true;
			m_ui16_readingHWTimer = 0;
			en_drvPWM_changeDutyCycle(m_ui8_aquaLightInstance, 0);
		} else {/* Nothing to do - En attente de la fin du test */}
	}
}

const BOOL AquaLight::b_AquaLight_firstCritErrRemoved() const
{
	return m_b_isDeletedFirstCritErr;
}

void AquaLight::v_AquaLight_configADC(const AQUALIGHT_ADC_VALUE st_adcValues)
{
	m_st_adcValues = st_adcValues;
	LOG("Valeurs ADC enregistrées :");
	LOG("	20 Lum : %u", m_st_adcValues.ui16_adc20LumValue);
	LOG("	25 Lum : %u", m_st_adcValues.ui16_adc25LumValue);
	LOG("	30 Lum : %u", m_st_adcValues.ui16_adc30LumValue);
	LOG("	35 Lum : %u", m_st_adcValues.ui16_adc35LumValue);
	
	v_AquaLight_calculateADCLimit();
}

void AquaLight::v_AquaLight_defineADCValue()
{
	UINT16 ui16_adcReadValue;
	
	if(m_en_currentStatus != AQUALIGHT_DEFINE_ADC_VALUE) {
		m_en_currentStatus = AQUALIGHT_DEFINE_ADC_VALUE;
		m_en_adcConfigStatus = AQUALIGHT_UNDEF_CONFIG;
		m_ui16_aquaLightTimer = 0;
		m_b_isADCConfigured = false;
		LOG("Définition des valeurs ADC :");
	} else {/* Nothing to do - Mode déjà activé */}

	m_ui16_aquaLightTimer++;

	if(m_ui16_aquaLightTimer > READING_HW_DELAY) {
		m_ui16_aquaLightTimer = 0;
		
		en_drvADC_getInputValue(m_ui8_instanceADC, AVERAGE_READING, &ui16_adcReadValue);
		
		switch(m_en_adcConfigStatus) {
			case AQUALIGHT_UNDEF_CONFIG:
				en_drvPWM_changeDutyCycle(m_ui8_aquaLightInstance, LUMEN_20_DC);
				m_en_adcConfigStatus = AQUALIGHT_20LUM_CONFIG;
				break;
				
			case AQUALIGHT_20LUM_CONFIG:
				m_st_adcValues.ui16_adc20LumValue = ui16_adcReadValue;
				LOG("	20 Lum : %u", m_st_adcValues.ui16_adc20LumValue);
				m_en_adcConfigStatus = AQUALIGHT_25LUM_CONFIG;
				en_drvPWM_changeDutyCycle(m_ui8_aquaLightInstance, LUMEN_25_DC);
				break;
				
			case AQUALIGHT_25LUM_CONFIG:
				m_st_adcValues.ui16_adc25LumValue = ui16_adcReadValue;
				LOG("	25 Lum : %u", m_st_adcValues.ui16_adc25LumValue);
				m_en_adcConfigStatus = AQUALIGHT_30LUM_CONFIG;
				en_drvPWM_changeDutyCycle(m_ui8_aquaLightInstance, LUMEN_30_DC);
				break;
				
			case AQUALIGHT_30LUM_CONFIG:
				m_st_adcValues.ui16_adc30LumValue = ui16_adcReadValue;
				LOG("	30 Lum : %u", m_st_adcValues.ui16_adc30LumValue);
				m_en_adcConfigStatus = AQUALIGHT_35LUM_CONFIG;
				en_drvPWM_changeDutyCycle(m_ui8_aquaLightInstance, LUMEN_35_DC);
				break;
				
			case AQUALIGHT_35LUM_CONFIG:
				m_st_adcValues.ui16_adc35LumValue = ui16_adcReadValue;
				LOG("	35 Lum : %u", m_st_adcValues.ui16_adc35LumValue);
				m_en_adcConfigStatus = AQUALIGHT_UNDEF_CONFIG;
				
				/* Fin de la récupération des valeurs de l'ADC */
				m_en_currentStatus = AQUALIGHT_NO_STATUS;
				m_b_isADCConfigured = true;
				v_AquaLight_calculateADCLimit();
				break;
				
			default:
				LOG("CodeError : cas switch non traité");
				break;
		}
	}
}

AQUALIGHT_ADC_VALUE AquaLight::st_AquaLight_getADCValues() const
{
	return m_st_adcValues;
}


const BOOL AquaLight::b_AquaLight_isADCCOnfigured() const
{
	return m_b_isADCConfigured;
}

void AquaLight::v_AquaLight_intensityMode(const BOOL b_isCurrentMode)
{
	/* Changement d'état de la lumière, RAZ du délai de lecture HW */
	m_ui16_readingHWTimer = 0;
	
	if(m_en_currentStatus != AQUALIGHT_INTENSITY_SETTING) {
		m_en_currentStatus = AQUALIGHT_INTENSITY_SETTING;
	} else {/* Nothing to do - Mode déjà activé */}
		
	if(b_isCurrentMode) {
		m_ui8_intensityNumber = static_cast<UINT8>(m_st_config.en_intensityModeName);
	} else {
		m_ui8_intensityNumber++;
		if(m_ui8_intensityNumber > NB_CONFIG_INTENSITY_MODE) {
			m_ui8_intensityNumber = 0;
		} else {/* Nothing to do - Le mode a correctement été incrémenté */}
			
		v_AquaLight_defineActualADCLimit(static_cast<AQUALIGHT_CONFIG_INTENSITY>(m_ui8_intensityNumber));
	}
	
	v_AquaLight_intensity();
}

inline void AquaLight::v_AquaLight_breathe()
{
	/* Attente entre deux pas */
	if(m_ui16_aquaLightTimer < TIME_BTW_TWO_BREATHE_STEPS) {
		m_ui16_aquaLightTimer++;
	} else {
		m_ui16_aquaLightTimer = 0;
		
		en_drvPWM_changeDutyCycle(m_ui8_aquaLightInstance, m_ui8_tmpLightingValue);
		
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

void AquaLight::v_AquaLight_blink()
{
	/* S'il y a un offset avant le clignotement, applique la valeur de la 2nd phase et attend le début de la 1ere phase */
	if(m_ui16_aquaLightTimer == 0 && m_st_blinkConfig.ui16_offsetTime != 0) {
			en_drvPWM_changeDutyCycle(m_ui8_aquaLightInstance, m_st_blinkConfig.ui8_secondPhaseDC);
	} else {/* Nothing to do - Pas d'offset ou pas au début de la période */}
	
	/* Début des clignotements */
	if(m_ui8_blinkIterator <= m_st_blinkConfig.ui8_nbOfBlink) {
		/* 1ere phase */
		if(m_ui16_aquaLightTimer ==
			(m_st_blinkConfig.ui16_offsetTime + 
			(m_st_blinkConfig.ui16_firstPhaseTime + m_st_blinkConfig.ui16_secondPhaseTime) * m_ui8_blinkIterator)) {
			en_drvPWM_changeDutyCycle(m_ui8_aquaLightInstance, m_st_blinkConfig.ui8_firstPhaseDC);
		/* 2eme phase */
		} else if(m_ui16_aquaLightTimer ==
			(m_st_blinkConfig.ui16_offsetTime + m_st_blinkConfig.ui16_firstPhaseTime +
			(m_st_blinkConfig.ui16_firstPhaseTime + m_st_blinkConfig.ui16_secondPhaseTime) * m_ui8_blinkIterator)) {
			en_drvPWM_changeDutyCycle(m_ui8_aquaLightInstance, m_st_blinkConfig.ui8_secondPhaseDC);

			/* La periode de clignotement a été exécuté, incrémente m_ui8_blinkIterator pour faire un clignotement de plus */
			m_ui8_blinkIterator++;
		} else {/* Nothing to do - Actuellement entre deux fronts de blink */}
	} else {/* Nothing to do - Tous les clignotements de la periode ont été réalisés */}
		
	m_ui16_aquaLightTimer++;
	
	/* Tous les clignotements ont eu lieu, fin de la période */
	if(m_ui16_aquaLightTimer >= (m_st_blinkConfig.ui16_offsetTime + m_st_blinkConfig.ui16_totalPeriodeTime)) {

		/* Réinitialisation des paramètres */
		m_ui16_aquaLightTimer = 0;
		m_ui8_blinkIterator = 0;
		
		/* Si blink de confirmation de changement d'heure, on retourne en mode réglage (Breath) */
		if(m_en_currentStatus == AQUALIGHT_BLINK_CONFIRM_SET_HOUR) {
			m_en_currentStatus = AQUALIGHT_BREATHE;
		} else {/* Nothing to do - Le status n'est pas changé, le blink continu */}
		
		/* Suppression de l'offset après la 1ere période */
		m_st_blinkConfig.ui16_offsetTime = 0;
		
	} else {/* Nothing to do - La période totale de clignotement n'est pas fini */}
}

void AquaLight::v_AquaLight_intensity() const
{
	switch(static_cast<AQUALIGHT_CONFIG_INTENSITY>(m_ui8_intensityNumber)) {
		case LUMEN_20:
			en_drvPWM_changeDutyCycle(m_ui8_aquaLightInstance, LUMEN_20_DC);
			break;
		case LUMEN_25:
			en_drvPWM_changeDutyCycle(m_ui8_aquaLightInstance, LUMEN_25_DC);
			break;
		case LUMEN_30:
			en_drvPWM_changeDutyCycle(m_ui8_aquaLightInstance, LUMEN_30_DC);
			break;
		case LUMEN_35:
			en_drvPWM_changeDutyCycle(m_ui8_aquaLightInstance, LUMEN_35_DC);
			break;
		default:
			LOG("CodeError : cas switch non traité");
			break;
	}
}

void AquaLight::v_AquaLight_saveTimeMode()
{
	m_st_config.en_timeModeName = static_cast<AQUALIGHT_CONFIG_TIME>(m_ui8_numberOfBlink);
}

void AquaLight::v_AquaLight_saveIntensityMode()
{
	m_st_config.en_intensityModeName = static_cast<AQUALIGHT_CONFIG_INTENSITY>(m_ui8_intensityNumber);
	
	v_AquaLight_setIntensityValue(m_st_config.en_intensityModeName);
}

inline void AquaLight::v_AquaLight_setIntensityValue(const AQUALIGHT_CONFIG_INTENSITY en_intensityModeName)
{
	switch(en_intensityModeName) {
		case LUMEN_20:
			m_st_config.ui8_intensityValue = LUMEN_20_DC;
			break;
		case LUMEN_25:
			m_st_config.ui8_intensityValue = LUMEN_25_DC;
			break;
		case LUMEN_30:
			m_st_config.ui8_intensityValue = LUMEN_30_DC;
			break;
		case LUMEN_35:
			m_st_config.ui8_intensityValue = LUMEN_35_DC;
			break;
		default:
			 /* Nothing to do - Valeur inconnue, on ne sauvegarde pas */
			 LOG("CodeError : cas switch non traité");
			break;
	}
}

const AQUALIGHT_CONFIG_TIME AquaLight::en_AquaLight_getTimeModeName() const
{
	return m_st_config.en_timeModeName;
}

const AQUALIGHT_CONFIG_INTENSITY AquaLight::en_AquaLight_getIntensityName() const
{
	return m_st_config.en_intensityModeName;
}

const BOOL AquaLight::b_AquaLight_isCriticalError() const
{
	return m_b_isCriticalError;
}

void AquaLight::v_AquaLight_calculateADCLimit()
{
	m_st_adcLimit.ui16_adcMinLimit20 = (UINT16)(((UINT32)(m_st_adcValues.ui16_adc20LumValue) * (100 - RANGE_LIMIT_LIGHT_ON_ADC)) / 100);
	m_st_adcLimit.ui16_adcMaxLimit20 = (UINT16)(((UINT32)(m_st_adcValues.ui16_adc20LumValue) * (100 + RANGE_LIMIT_LIGHT_ON_ADC)) / 100);
	
	m_st_adcLimit.ui16_adcMinLimit25 = (UINT16)(((UINT32)(m_st_adcValues.ui16_adc25LumValue) * (100 - RANGE_LIMIT_LIGHT_ON_ADC)) / 100);
	m_st_adcLimit.ui16_adcMaxLimit25 = (UINT16)(((UINT32)(m_st_adcValues.ui16_adc25LumValue) * (100 + RANGE_LIMIT_LIGHT_ON_ADC)) / 100);
				
	m_st_adcLimit.ui16_adcMinLimit30 = (UINT16)(((UINT32)(m_st_adcValues.ui16_adc30LumValue) * (100 - RANGE_LIMIT_LIGHT_ON_ADC)) / 100);
	m_st_adcLimit.ui16_adcMaxLimit30 = (UINT16)(((UINT32)(m_st_adcValues.ui16_adc30LumValue) * (100 + RANGE_LIMIT_LIGHT_ON_ADC)) / 100);
	
	m_st_adcLimit.ui16_adcMinLimit35 = (UINT16)(((UINT32)(m_st_adcValues.ui16_adc35LumValue) * (100 - RANGE_LIMIT_LIGHT_ON_ADC)) / 100);
	m_st_adcLimit.ui16_adcMaxLimit35 = (UINT16)(((UINT32)(m_st_adcValues.ui16_adc35LumValue) * (100 + RANGE_LIMIT_LIGHT_ON_ADC)) / 100);

	m_st_adcLimit.ui16_adcMaxLimitOff = m_st_adcValues.ui16_adc20LumValue;
	
	v_AquaLight_defineActualADCLimit(m_st_config.en_intensityModeName);
}

void AquaLight::v_AquaLight_defineActualADCLimit(const AQUALIGHT_CONFIG_INTENSITY en_intensityModeName)
{
	/* Définition du palier min et max lors la lumière est allumée */
	switch(en_intensityModeName) {
		case LUMEN_20:
			m_st_adcLimit.ui16_adcMinLimitActual = m_st_adcLimit.ui16_adcMinLimit20;
			m_st_adcLimit.ui16_adcMaxLimitActual = m_st_adcLimit.ui16_adcMaxLimit20;
			break;
		case LUMEN_25:
			m_st_adcLimit.ui16_adcMinLimitActual = m_st_adcLimit.ui16_adcMinLimit25;
			m_st_adcLimit.ui16_adcMaxLimitActual = m_st_adcLimit.ui16_adcMaxLimit25;
			break;
		case LUMEN_30:
			m_st_adcLimit.ui16_adcMinLimitActual = m_st_adcLimit.ui16_adcMinLimit30;
			m_st_adcLimit.ui16_adcMaxLimitActual = m_st_adcLimit.ui16_adcMaxLimit30;
			break;
		case LUMEN_35:
			m_st_adcLimit.ui16_adcMinLimitActual = m_st_adcLimit.ui16_adcMinLimit35;
			m_st_adcLimit.ui16_adcMaxLimitActual = m_st_adcLimit.ui16_adcMaxLimit35;
			break;
		default:
			/* Nothing to do - Cas impossible */
			LOG("CodeError : cas switch non traité");
			break;
	}
	
	LOG("ADC Limites :");
	LOG("	Max Off : %u", m_st_adcLimit.ui16_adcMaxLimitOff);
	LOG("	Min On : %u", m_st_adcLimit.ui16_adcMinLimitActual);
	LOG("	Max On : %u", m_st_adcLimit.ui16_adcMaxLimitActual);
}

void AquaLight::v_AquaLight_setNoBrickMode(const BOOL b_noBrickMode)
{
	m_b_noBrickMode = b_noBrickMode;
}