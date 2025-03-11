/* 
* Pump.cpp
*
* Created: 06/12/2023 21:02:49
* Author: LucasTeissier
*/

#include "Pump.h"
#include "drvHardware.h"
#include "drvPWM.h"
#include "drvRPM.h"

/* Hardware setting */
#define CHANEL_PWM_PUMP									PWM_CHANNEL_5

/* Fr�quence de rotation de la pompe d�finie pour le niveau bas en fonction de la version du bac */
#ifdef BAC_VERSION_1
	#define ROTATION_FREQUENCY_LOW_LIMIT				48		/* (A) Fr�quence de rotation (limite basse) avant erreur de pompe qui ne tourne plus ou trop lentement (rotor arr�t� ou obstru�) */
	#define LOW_LEVEL_ROTATION_FREQUENCY_HIGH_LIMIT		65		/* (B) Fr�quence de rotation (limite haute) de d�tection de la pompe hors de l'eau pendant un niveau bas */
#elif defined BAC_VERSION_2
	#define ROTATION_FREQUENCY_LOW_LIMIT				60		/* (A) */
	#define LOW_LEVEL_ROTATION_FREQUENCY_HIGH_LIMIT		80		/* (B) */
#endif

/* Fr�quence de rotation (limite haute) de d�tection de la pompe hors de l'eau pendant un niveau haut */
#define HIGH_LEVEL_ROTATION_FREQUENCY_HIGH_LIMIT		150

/* Conversion des fr�quences de rotation en valeur du timer TCB1 (RPM pompe) */
#define ROTATION_FREQUENCY_LOW_LIMIT_VALUE				(UINT16)((CONF_CPU_FREQUENCY/8)/ROTATION_FREQUENCY_LOW_LIMIT)
#define LOW_LEVEL_ROTATION_FREQUENCY_HIGH_LIMIT_VALUE	(UINT16)((CONF_CPU_FREQUENCY/8)/LOW_LEVEL_ROTATION_FREQUENCY_HIGH_LIMIT)
#define HIGH_LEVEL_ROTATION_FREQUENCY_HIGH_LIMIT_VALUE	(UINT16)((CONF_CPU_FREQUENCY/8)/HIGH_LEVEL_ROTATION_FREQUENCY_HIGH_LIMIT)

/* UI/UX parameter */
#define	MAX_DUTY_CYCLE									100
#define READING_HW_DELAY						(UINT16)(500)	/* Temps d'attente avant la lecture HW (en ms) */
#define	PUMP_PRIMING_TIME						(UINT16)(5000)	/* Temps d'amor�age de la pompe (en ms) */
#define WAITING_TIME_BEFORE_PRIMING				(UINT16)(10000)	/* Temps d'attente avant de lancer l'amor�age, utilis� au d�marrage (en ms)*/
#define DC_MIDRANGE_FIND_MIN						    50		/* Valeur interm�diaire du rapport cyclique lors de la recherche du min DC */
#define CRITICAL_MIN_DC_VALUE							10		/* Valeur minimale du rapport cyclique avant d�clenchement erreur */
#define WAITING_STEP_NB_RPM_VALUE						6		/* Nombre de pas de lecture RPM lors de la phase d'attente  de gotoMinValue (correspond � un temps car li� � READING_HW_DELAY - chaque pas = 1/2sec) */
#define LOW_LEVEL_CORRECTION_FREQUENCY					500		/* Valeur de fr�quence (en mHz) � soustraire de la valeur de la fr�quence cible du niveau bas avant augmentation de la vitesse de rotation (par d�faut 56-0.5 = 55.5Hz) */

// default constructor
Pump::Pump()
{
} //Pump

// default destructor
Pump::~Pump()
{
} //~Pump

void Pump::v_Pump_initialization()
{
	v_drvRPM_initialization();
	en_drvPWM_initialization(CHANEL_PWM_PUMP, 0, &m_ui8_pumpInstance);
}

void Pump::v_Pump_configuration(const UINT16 ui16_lowLevelFrequency)
{
	/* Pour simplifier le calcul, conversion du RPM lu dans la flash en fr�quence */
	const UINT16 ui16_flashFreqValue = (UINT8)((CONF_CPU_FREQUENCY/8)/ui16_lowLevelFrequency);
	
	/* Sauvegarde de la nouvelle valeur dans la classe */
	m_ui16_lowLevelFreqValue = ui16_lowLevelFrequency;
	
	/*  Calcul du RPM de la fr�quence minimale avant augmentation de la vitesse de rotation (par d�faut 22522 ou 56-0.5 = 55.5Hz) */
	m_ui16_lowLevelFreqValueLimit = (UINT16)(((float)(CONF_CPU_FREQUENCY)/8)/((float)(ui16_flashFreqValue) - ((float)(LOW_LEVEL_CORRECTION_FREQUENCY)/1000)));
}

void Pump::v_Pump_event(const SCHEDULED_EVENT en_scheduledEvent, const UINT16 ui16_rpmCounterValue, const BOOL b_flagRpmRead)
{
	/* Excecute certains �tats prioritaires de la pompe avant les Events */
	switch(m_en_currentStatus) {
		/* Probl�me d�tect� */
		case PUMP_PROBLEM:
			v_Pump_problemMode();
			break;

		/* En attente avant de lancer l'amor�age - Utilis� au d�marrage */
		case PUMP_WAITING_BEFORE_PRIMING:
			v_Pump_startingMode(WITH_WAITING);
			break;

		/* Amor�age de la pompe en cours */
		case PUMP_PRIMING:
			v_Pump_pumpPriming();
			break;
			
		/* Recherche de la vitesse min en cours */
		case PUMP_GOTO_SPEED_MIN:
			v_Pump_gotoMinDutyCycle();
			break;
			
		/* Pompe � l'arret */
		case PUMP_STOP:
			v_Pump_stopPumpProd();
			break;
			
		/* Mode r�glage de la vitesse (HandlerProd) */
		case PUMP_VARIATION_SPEED_PROD:
			/* Nothing to do - La commande se fait par le handler */
			break;
		
		/* Aucun �tats prioritaire, ex�cute les Events */
		default:
			switch(en_scheduledEvent) {
				/* D�but niveau haut */
				case START_PUMP_HIGH_LVL:
					if(m_en_currentStatus != PUMP_SPEED_MAX) {
						v_Pump_changeDutyCycle(MAX_DUTY_CYCLE);
						m_en_currentStatus = PUMP_SPEED_MAX;
					} else {/* Nothing to do - La pompe est d�j� � la vitesse max */}
					break;
			
				/* D�but niveau bas */
				case STOP_PUMP_HIGH_LVL:
					if(m_en_currentStatus != PUMP_SPEED_MIN) {
						v_Pump_gotoMinDutyCycle();
					} else {/* Nothing to do - La pompe est d�j� � la vitesse min */}
					break;
			
				/* Evenement inconnu, pompe coup�e */
				default:
					v_Pump_changeDutyCycle(0);
					m_en_currentStatus = PUMP_NO_STATUS;
					break;
			}
			break;
	}
		
	/* La pompe est en amor�age, en mode prod ou ne tourne pas, on ne lit pas le RPM */
	if(m_en_currentStatus != PUMP_VARIATION_SPEED_PROD && m_en_currentStatus != PUMP_NO_STATUS && m_en_currentStatus != PUMP_STOP && 
	   m_en_currentStatus != PUMP_PROBLEM && m_en_currentStatus != PUMP_PRIMING && m_en_currentStatus != PUMP_WAITING_BEFORE_PRIMING) {
		v_Pump_checkSpeedPump(ui16_rpmCounterValue, b_flagRpmRead);
	} else {/* Nothing to do - La vitesse ne doit pas �tre lue */}

	#ifdef DEBUG_MOD_LOG
		static PUMP_STATUS		en_lastCurrentStatus	= PUMP_STOP;
		static SCHEDULED_EVENT	en_lastScheduledEvent	= NO_EVENT;
		
		if(en_lastCurrentStatus != m_en_currentStatus || en_lastScheduledEvent != en_scheduledEvent) {
			LOG("Pump : Mode %u, Event %u", m_en_currentStatus, en_scheduledEvent);
			
			en_lastCurrentStatus = m_en_currentStatus;
			en_lastScheduledEvent = en_scheduledEvent;
		} else {/* Nothing to do - L'�tat n'a pas chang� */}
				
		if(m_ui16_readingHWDelayTimer > READING_HW_DELAY || m_ui16_pumpTimer > WAITING_TIME_BEFORE_PRIMING) {
			LOG("Pump Problem - Timer HW %u, timer pump %u", m_ui16_readingHWDelayTimer, m_ui16_pumpTimer);
			m_ui16_readingHWDelayTimer = 0;
			m_ui16_pumpTimer = 0;
		} else {}
	#endif

}

inline void Pump::v_Pump_checkSpeedPump(const UINT16 ui16_rpmCounterValue, const BOOL b_flagRpmRead) 
{
	#ifdef DEBUG_MOD_LOG
		static BOOL b_logData				= false;
		static BOOL b_firstCallBeforeReset	= true;
		
		if(b_firstCallBeforeReset && m_ui16_readingHWDelayTimer != 0)
		{
			LOG("Pump Problem - Timer reset HW %u, timer pump %u", m_ui16_readingHWDelayTimer, m_ui16_pumpTimer);
		} else {
			b_firstCallBeforeReset = false;
		}
	#endif
	
    m_ui16_readingHWDelayTimer++;

    /* Nouvelle valeur RPM lue */
    if (b_flagRpmRead) {
		
        #ifdef DEBUG_MOD_LOG
			/* Probl�me de lecture, valeur lue affich�e, le tableau complet de lecture est aussi affich� � la fin du d�lais de lecture */
		    if ((m_en_currentStatus == PUMP_SPEED_MIN) && ((ui16_rpmCounterValue < LOW_LEVEL_ROTATION_FREQUENCY_HIGH_LIMIT_VALUE) || (ui16_rpmCounterValue > ROTATION_FREQUENCY_LOW_LIMIT_VALUE))) {
				LOG("Pump Problem - PUMP_SPEED_MIN valeur hors plage : %u", ui16_rpmCounterValue);
				b_logData = true;
			} else if ((m_en_currentStatus == PUMP_SPEED_MAX) && ((ui16_rpmCounterValue < HIGH_LEVEL_ROTATION_FREQUENCY_HIGH_LIMIT_VALUE) || (ui16_rpmCounterValue > ROTATION_FREQUENCY_LOW_LIMIT_VALUE))) {
				LOG("Pump Problem - PUMP_SPEED_MAX valeur hors plage : %u", ui16_rpmCounterValue);
				b_logData = true;
			} else {/* Nothing to do - Les valeurs lues sont ok */}
			
			/* Stockage valeur RPM lue pour debug */
            if (m_ui16_rpmAverageNumber < DEBUG_VALUE_SIZE) {
	            m_tui16_debugRPMValue[m_ui16_rpmAverageNumber] = ui16_rpmCounterValue;
	        } else {/* Nothing to do - Taille du tableau d�pass�e */}
	    #endif

	    /* Accumulation de la valeur lue pour le calcul de la moyenne */
	    m_ui32_rpmCounterAverageValue += ui16_rpmCounterValue;
	    m_ui16_rpmAverageNumber++;
	    m_b_isRpmRead = true;
		
    } else {/* Nothing to do - En attente d'une lecture RPM */}

    if (m_ui16_readingHWDelayTimer >= READING_HW_DELAY) {
        if (m_b_isRpmRead) {
            /* Des valeurs RPM ont �t� lues m_b_isRpmRead = true, m_ui16_rpmAverageNumber ne peut pas �tre �gal � 0 calcul moyenne */
	        m_ui32_rpmCounterAverageValue /= m_ui16_rpmAverageNumber;

            switch (m_en_currentStatus) {
	            case PUMP_SPEED_MAX:
					v_Pump_checkMaxSpeed();
					break;
	            case PUMP_SPEED_MIN:
					v_Pump_checkMinSpeed();
					break;
	            case PUMP_GOTO_SPEED_MIN:
					v_Pump_checkGotoSpeedMin();
					break;
	            default:
					/* Nothing to do - Aucun contr�le de vitesse n�cessaire dans son mode actuel */
					break;
            }
		} else {
            /* L'IT de lecture ne s'est jamais d�clench�, pompe en d�faut ou eteinte */
            v_Pump_problemMode();
            LOG("Pump Problem : IT non d�clench�e");
        }
		
		#ifdef DEBUG_MOD_LOG
			b_firstCallBeforeReset = true;
			if(b_logData) {
				b_logData = false;
				v_Pump_logDebugData();
			} else {/* Nothing to do - Il n'y a pas eu d'erreur, pas besoin d'afficher le tableau de log */}
		#endif

        /* Reset des variables utilis�es apr�s traitement */
		m_b_isRpmRead					= false;
        m_ui32_rpmCounterAverageValue	= 0;
        m_ui16_rpmAverageNumber			= 0;
		m_ui16_readingHWDelayTimer		= 0;
    } else {/* Nothing to do - D�lais avant lecture insuffisant */}
}

inline void Pump::v_Pump_checkMaxSpeed() 
{
    /* Pompe hors limites, erreur */
    if ((m_ui32_rpmCounterAverageValue < HIGH_LEVEL_ROTATION_FREQUENCY_HIGH_LIMIT_VALUE) ||
    (m_ui32_rpmCounterAverageValue > ROTATION_FREQUENCY_LOW_LIMIT_VALUE)) {
		v_Pump_problemMode();
		v_Pump_logDebugData();
    } else {/* Nothing to do - Les valeurs pompe sont correctes */}
}

inline void Pump::v_Pump_checkMinSpeed() 
{
    /* Pompe hors limites, erreur */
    if ((m_ui32_rpmCounterAverageValue < LOW_LEVEL_ROTATION_FREQUENCY_HIGH_LIMIT_VALUE) ||
    (m_ui32_rpmCounterAverageValue > ROTATION_FREQUENCY_LOW_LIMIT_VALUE)) {
		v_Pump_problemMode();
		v_Pump_logDebugData();
    } else {
        m_ui16_pumpTimer++;
		
        /* Pour �viter les parasites, le DC est incr�ment� si toutes les moyennes sont mauvaises sur une p�riode WAITING_STEP_NB_RPM_VALUE */
        if (m_ui16_pumpTimer < WAITING_STEP_NB_RPM_VALUE) {
            if (m_ui32_rpmCounterAverageValue <= m_ui16_lowLevelFreqValueLimit) {
                m_b_isRpmGood = true;
            } else {/* Nothing to do - Une seule bonne valeur suffit � valider le DC */}
        } else {
            if (!m_b_isRpmGood) {
                m_ui8_actualDCValue++;
                v_Pump_changeDutyCycle(m_ui8_actualDCValue);
				LOG("Pump : SPEED_MIN +1DC");
            } else {
                m_b_isRpmGood = false;
            }

            m_ui16_pumpTimer = 0;
        }
    }
}

inline void Pump::v_Pump_checkGotoSpeedMin() 
{
    m_b_isReadyToDecrease = true;

    /* Phase de recherche du niveau bas */
    if (m_en_stepFindMinDC == STEP_DECREASE_DC) {
        /* Limite niveau bas atteinte */
        if (m_ui32_rpmCounterAverageValue > m_ui16_lowLevelFreqValue) {
            m_en_stepFindMinDC			= STEP_WAITING;
			m_ui16_pumpTimer			= 0;
			m_ui16_readingHWDelayTimer	= 0;
        } else {/* Nothing to do - Fr�quence niveau bas non touv�e, DC d�cr�ment� */}
    } else if (m_en_stepFindMinDC == STEP_WAITING) {
		
        /* Pompe hors limites, erreur */
        if ((m_ui32_rpmCounterAverageValue < LOW_LEVEL_ROTATION_FREQUENCY_HIGH_LIMIT_VALUE) ||
        (m_ui32_rpmCounterAverageValue > ROTATION_FREQUENCY_LOW_LIMIT_VALUE)) {
			v_Pump_problemMode();
			v_Pump_logDebugData();
        } else {
            m_ui16_pumpTimer++;
			
            /* Pour �viter les parasites, le DC est incr�ment� si toutes les moyennes sont mauvaises sur une p�riode WAITING_STEP_NB_RPM_VALUE */
            if (m_ui16_pumpTimer < WAITING_STEP_NB_RPM_VALUE) {
                if (m_ui32_rpmCounterAverageValue <= m_ui16_lowLevelFreqValueLimit) {
                    m_b_isRpmGood = true;
                } else {/* Nothing to do - Une seule bonne valeur suffit � valider le DC */}
            } else {
                if (!m_b_isRpmGood) {
                    m_ui8_actualDCValue++;
                    v_Pump_changeDutyCycle(m_ui8_actualDCValue);
					LOG("Pump : STEP_WAITING +1DC");
                } else {
                    /* Toutes les valeurs sont correcte, �tat niveau bas */
                    m_en_currentStatus = PUMP_SPEED_MIN;
                    m_b_isRpmGood = false;

                    if (m_b_isFirstMinorError) {
                        m_b_isFirstMinorError = false;
                        LOG("Pump : Suppression erreur");
                    } else {/* Nothing to do - Aucune erreur ne s'�tait d�clench�e */}
                }

                m_ui16_pumpTimer = 0;
            }
        }
    } else {/* Nothing to do - On ne fait rien pendant les autres �tats */}
}

void Pump::v_Pump_gotoMinDutyCycle()
{
	if(m_en_currentStatus != PUMP_GOTO_SPEED_MIN) {
		m_en_currentStatus			= PUMP_GOTO_SPEED_MIN;
		m_en_stepFindMinDC			= STEP_MAX_DC;
		m_ui8_actualDCValue			= DC_MIDRANGE_FIND_MIN;
		m_ui16_pumpTimer			= 0;
		m_ui16_readingHWDelayTimer	= 0;
	} else {/* Nothing to do - Le status est d�ja appliqu� */}
	
	if(m_b_isReadyToDecrease) {
		m_b_isReadyToDecrease = false;
		
		switch(m_en_stepFindMinDC) {
			case STEP_MAX_DC:
				v_Pump_changeDutyCycle(MAX_DUTY_CYCLE);
				m_en_stepFindMinDC = STEP_MIDRANGE_DC;
				break;
				
			case STEP_MIDRANGE_DC:
				v_Pump_changeDutyCycle(DC_MIDRANGE_FIND_MIN);
				m_en_stepFindMinDC = STEP_DECREASE_DC;
				break;

			case STEP_DECREASE_DC:
				m_ui8_actualDCValue--;
				
				if(m_ui8_actualDCValue < CRITICAL_MIN_DC_VALUE) {
					v_Pump_problemMode();
					LOG("Pump Problem : STEP_DECREASE_DC");
				} else {
					v_Pump_changeDutyCycle(m_ui8_actualDCValue);
				}
				break;
				
			case STEP_WAITING:
				/* Nothing to do - Les �tapes sont r�alis�es dans checkSpeedPump() (+1DC et application de PUMP_SPEED_MIN) */
				break;
				
			default:
				break;
		}
	} else {/* Nothing to do - En attente de la lecture du RPM avant de continuer */}
}

void Pump::v_Pump_pumpPriming()
{
	if(m_en_currentStatus != PUMP_PRIMING) {
		m_en_currentStatus			= PUMP_PRIMING;
		m_ui16_pumpTimer			= 0;
		m_ui16_readingHWDelayTimer	= 0;
	} else {/* Mode amor�age d�j� activ� */}
		
	m_ui16_pumpTimer++;
	
	if(m_ui16_pumpTimer < PUMP_PRIMING_TIME) {
		if(!m_b_isPumpPiming) {
			v_Pump_changeDutyCycle(MAX_DUTY_CYCLE);
			m_b_isPumpPiming = true;
		} else {/* Nothing to do - En cours d'amor�age */}
	
	/* Fin de l'amor�age */
	} else {
		m_ui16_pumpTimer			= 0;
		m_ui16_readingHWDelayTimer	= 0;
		m_b_isPumpPiming			= false;
		v_Pump_gotoMinDutyCycle();
	}
}

void Pump::v_Pump_problemMode()
{	
	/* Si la pompe n'est pas encore en mode erreur */
	if(!m_b_isMinorError) {
		
		/* Stop pompe */
		v_Pump_changeDutyCycle(0);
	
		/* Si m_b_isFirstMinorError est � 1, alors une erreur a d�ja �t� d�clench�e */
		if(m_b_isFirstMinorError) {
			if(m_en_currentStatus != PUMP_PROBLEM) {
				m_en_currentStatus			= PUMP_PROBLEM;
				m_ui16_pumpTimer			= 0;
				m_ui16_readingHWDelayTimer	= 0;
				m_b_isMinorError			= true;
				m_b_isPumpPiming			= false;
				m_b_isFirstMinorError		= false;
			} else {/* Nothing to do - La pompe est d�ja configur�e en erreur */}
		} else {
			m_b_isFirstMinorError = true;
			v_Pump_startingMode(WITH_WAITING);
		}
	} else {/* Nothing to do - La pompe est d�ja configur�e en erreur */}
}

void Pump::v_Pump_disableMinorError()
{
	m_b_isFirstMinorError = false;
	m_b_isMinorError = false;
	v_Pump_pumpPriming();
}

void Pump::v_Pump_startingMode(BOOL b_withWaitingMode)
{
	if(b_withWaitingMode) {
		if(m_en_currentStatus != PUMP_WAITING_BEFORE_PRIMING) {
			m_en_currentStatus = PUMP_WAITING_BEFORE_PRIMING;
			m_ui16_pumpTimer = 0;
			m_ui16_readingHWDelayTimer = 0;
		} else {
			if(m_ui16_pumpTimer < WAITING_TIME_BEFORE_PRIMING) {
				m_ui16_pumpTimer++;
			} else {
				m_ui16_pumpTimer = 0;
				m_ui16_readingHWDelayTimer = 0;
				v_Pump_pumpPriming();
			}
		}
	} else {
		m_ui16_pumpTimer = 0;
		m_ui16_readingHWDelayTimer = 0;
		v_Pump_pumpPriming();
	}
}

void Pump::v_Pump_stopPumpProd()
{
	if(m_en_currentStatus != PUMP_STOP) {
		m_en_currentStatus = PUMP_STOP;
		v_Pump_changeDutyCycle(0);
	} else {/* Nothing to do - L'�tat est d�ja activ� */}
}

const BOOL Pump::b_Pump_isMinorError() const
{
	return m_b_isMinorError;
}

void Pump::v_Pump_setSpeedPumpProd(const UINT8 ui8_frequency)
{
	v_Pump_changeDutyCycle(ui8_frequency);
}

void Pump::v_Pump_variationSpeedProd()
{
	if(m_en_currentStatus != PUMP_VARIATION_SPEED_PROD) {
		m_en_currentStatus = PUMP_VARIATION_SPEED_PROD;
	} else {/* Nothing to do - Le mode a d�ja �t� s�lectionn� */}
}

void Pump::v_Pump_changeDutyCycle(const UINT8 ui8_dutyCycle) const 
{
	en_drvPWM_changeDutyCycle(m_ui8_pumpInstance, ui8_dutyCycle);
}

void Pump::v_Pump_logDebugData() const 
{
	#ifdef DEBUG_MOD_LOG
		LOG("Pump Problem - RPM : %lu", m_ui32_rpmCounterAverageValue);
	
		for (UINT16 ui16_debugRPMIterator = 0; ui16_debugRPMIterator < m_ui16_rpmAverageNumber; ui16_debugRPMIterator++) {
			LOG("    Derni�res valeurs lues : [%u] = %u", ui16_debugRPMIterator, m_tui16_debugRPMValue[ui16_debugRPMIterator]);
		}
	#endif
}