/* 
* Scheduler.cpp
*
* Created: 08/01/2024 15:27:31
* Author: LucasTeissier
*/

#include "Scheduler.h"

/* General */
#define	START_EVENT						0					/* Défini le numéro de l'event dans la structure d'event */
#define	STOP_EVENT						1
#define	MANUAL_EVENT					2

/* UI/UX parameters */
#ifndef DEBUG_MOD_SHORT_DAY
#define	NB_SECOND_PER_DAY				(UINT32)(86400)		/* 60*60*24 = 86400 */
#define	TIME_IN_MANUAL_MODE				(UINT16)(7200)		/* 60*60*2  = 7200 */
#define	NB_SECOND_IN_5MIN				(UINT16)(300)
#define	NB_SECOND_IN_6H					(UINT16)(21600)
#define	NB_SECOND_IN_8H					(UINT16)(28800)
#define	NB_SECOND_IN_12H				(UINT16)(43200)
#define	NB_SECOND_IN_14H				(UINT16)(50400)
#define	NB_SECOND_IN_15H				(UINT16)(54000)		/* Timeout du mode MANUAL_LIGHTING (temps cycle éclairage manuel) */
#define TIME_BTW_PLANTLIGHT_PUMP		(UINT16)(21600)		/* Temps entre la fin de l'éclairage plante et l'allumage de la pompe (en sec) */
#else
#define	NB_SECOND_PER_DAY				(UINT32)(864)		/* 60*60*24 = 86400 */
#define	TIME_IN_MANUAL_MODE				(UINT16)(72)		/* 60*60*2  = 7200 */
#define	NB_SECOND_IN_5MIN				(UINT16)(150)
#define	NB_SECOND_IN_6H					(UINT16)(216)
#define	NB_SECOND_IN_8H					(UINT16)(288)
#define	NB_SECOND_IN_12H				(UINT16)(432)
#define	NB_SECOND_IN_14H				(UINT16)(504)
#define	NB_SECOND_IN_15H				(UINT16)(540)		/* Timeout du mode MANUAL_LIGHTING (temps cycle éclairage manuel) */
#define TIME_BTW_PLANTLIGHT_PUMP		(UINT16)(1)		/* Temps entre la fin de l'éclairage plante et l'allumage de la pompe (en sec) */
#endif
#define	DEFAULT_HOUR_MANUAL						 0			/* Mode manuel (on ou off 2h)			*/

#define	TIME_PUMP_HIGHT_TIDE			NB_SECOND_IN_5MIN	/* Temps de la marée haute */ 
#define TIME_MAX_MANUAL_MODE			NB_SECOND_IN_15H

// default constructor
Scheduler::Scheduler()
{
} //Scheduler

// default destructor
Scheduler::~Scheduler()
{
} //~Scheduler

void Scheduler::v_Scheduler_initialization(const SETTINGS st_settings)
{
	LOG("Current Time : %lu", st_settings.ui32_currentTime);
	LOG("Pump freq : %d", st_settings.ui16_pumpFrequency);
	LOG("Setting plantes :");
	LOG("	Stop Time : %lu", st_settings.ui32_stopTimePlantLigh);
	LOG("	Time Mode : %d", st_settings.en_timeModePlantLight);
	LOG("Setting aqua :");
	LOG("	Stop Time : %lu", st_settings.ui32_stopTimeAquaLight);
	LOG("	Time Mode : %d", st_settings.en_timeModeAquaLight);
	LOG("	Intensity : %d", st_settings.en_intensityAquaLight);
	
	m_ui32_currentTime = st_settings.ui32_currentTime;

	/* Pour PlantLight et AquaLight: Le stopTime est configuré par Mémory, soit avec les info en flash soit avec les defines */
	m_tst_plantLightEvents[START_EVENT].b_isBtwTwoDays	= false;
	m_tst_plantLightEvents[START_EVENT].en_name			= START_PLANTLIGHT_CYCLE;
	m_tst_plantLightEvents[STOP_EVENT].b_isBtwTwoDays	= false;
	m_tst_plantLightEvents[STOP_EVENT].en_name			= STOP_PLANTLIGHT_CYCLE;
	m_tst_plantLightEvents[STOP_EVENT].ui32_time		= st_settings.ui32_stopTimePlantLigh;
	m_tst_plantLightEvents[MANUAL_EVENT].b_isBtwTwoDays	= false;
	m_tst_plantLightEvents[MANUAL_EVENT].en_name		= OFF_MANUAL_LIGHT_MODE;
	m_tst_plantLightEvents[MANUAL_EVENT].ui32_time		= DEFAULT_HOUR_MANUAL;
	m_en_plantLightEvent = en_Scheduler_setTimeModePlantLight(st_settings.en_timeModePlantLight);
	
	m_tst_aquaLightEvents[START_EVENT].b_isBtwTwoDays	= false;
	m_tst_aquaLightEvents[START_EVENT].en_name			= START_AQUALIGHT_CYCLE;
	m_tst_aquaLightEvents[STOP_EVENT].b_isBtwTwoDays	= false;
	m_tst_aquaLightEvents[STOP_EVENT].en_name			= STOP_AQUALIGHT_CYCLE;
	m_tst_aquaLightEvents[STOP_EVENT].ui32_time			= st_settings.ui32_stopTimeAquaLight;
	m_tst_aquaLightEvents[MANUAL_EVENT].b_isBtwTwoDays	= false;
	m_tst_aquaLightEvents[MANUAL_EVENT].en_name			= OFF_MANUAL_LIGHT_MODE;
	m_tst_aquaLightEvents[MANUAL_EVENT].ui32_time		= DEFAULT_HOUR_MANUAL;
	
	if(st_settings.en_timeModeAquaLight != MANUAL_LIGHTING) {
		m_en_aquaLightEvent = en_Scheduler_setTimeModeAquaLight(st_settings.en_timeModeAquaLight);
	} else {
		/* Si le mode manuel était sélectionné, on récup start & stop, pas besoin de recalculer start avec en_Scheduler_setTimeModeAquaLight() */
		m_tst_aquaLightEvents[START_EVENT].ui32_time	= st_settings.ui32_startTimeManualAquaLight;
		m_tst_aquaLightEvents[START_EVENT].b_isBtwTwoDays = 
			(m_tst_aquaLightEvents[START_EVENT].ui32_time > m_tst_aquaLightEvents[STOP_EVENT].ui32_time)
				? true : false;
				
		LOG("Set Time Mode AL H[%lu] :", m_ui32_currentTime);
		LOG("	Mode : %d", st_settings.en_timeModeAquaLight);
		LOG("	START : %lu", m_tst_aquaLightEvents[START_EVENT].ui32_time);
		LOG("	STOP : %lu", m_tst_aquaLightEvents[STOP_EVENT].ui32_time);
		LOG("	BTW2D : %d", m_tst_aquaLightEvents[START_EVENT].b_isBtwTwoDays);
		v_Scheduler_updateEventAquaLight();
	}
	
	m_tst_pumpEvents[START_EVENT].b_isBtwTwoDays		= false;
	m_tst_pumpEvents[START_EVENT].en_name				= START_PUMP_HIGH_LVL;
	m_tst_pumpEvents[STOP_EVENT].b_isBtwTwoDays			= false;
	m_tst_pumpEvents[STOP_EVENT].en_name				= STOP_PUMP_HIGH_LVL;
	v_Scheduler_setTimePump();
}

void Scheduler::v_Scheduler_updateEvent()
{
	/* Mise à jour de l'heure */
	m_ui32_currentTime++;
	if(m_ui32_currentTime >= NB_SECOND_PER_DAY) {
		m_ui32_currentTime = 0;
		LOG("RAZ Current Time H[%lu]", m_ui32_currentTime);
		if(m_b_isWaitingStopHManualTime) {
			LOG("MANUAL_LIGHTING est entre 2j H[%lu]", m_ui32_currentTime);
			m_st_tempStartTimeAquaLight.b_isBtwTwoDays = true;
		} else {/* nothing to do - Le mode MANUAL_LIGHTING n'est pas en attente d'une heure de fin */}
	} else {/* Nothing to do - Pas encore un jour écoulé */}

	/* Mise à jour de l'event Lumière Plante */
	v_Scheduler_updateEventPlantLight();

	/* Mise à jour de l'event Lumière Aquarium */
	v_Scheduler_updateEventAquaLight();

	/* Mise à jour de l'event Pompe */
	v_Scheduler_updateEventPump();
}

const UINT32 Scheduler::ui32_Scheduler_getCurrentTime() const
{
	return m_ui32_currentTime;
}

const SCHEDULED_EVENT Scheduler::en_Scheduler_getPumpEvent() const
{
	return m_en_pumpEvent;
}

const SCHEDULED_EVENT Scheduler::en_Scheduler_getPlantLightEvent() const
{
	return m_en_plantLightEvent;
}

const SCHEDULED_EVENT Scheduler::en_Scheduler_getAquaLightEvent() const
{
	return m_en_aquaLightEvent;
}

const UINT32 Scheduler::en_Scheduler_getStartTimeManualAL() const
{
	return m_st_tempStartTimeAquaLight.ui32_time;
}

void Scheduler::v_Scheduler_setManualModePlantLight()
{
	if(m_en_plantLightEvent == START_PLANTLIGHT_CYCLE || m_en_plantLightEvent == STOP_PLANTLIGHT_CYCLE) {
		LOG("Activation mode manuel PL H[%lu]", m_ui32_currentTime);
		m_b_isManualModePlantLight = true;
		
		/* Configure l'heure d'arret du mode manuel */
		if((m_ui32_currentTime + TIME_IN_MANUAL_MODE) < NB_SECOND_PER_DAY) {
			/* Si le mode manuel se termine le même jour */
			m_tst_plantLightEvents[MANUAL_EVENT].b_isBtwTwoDays = false;
			m_tst_plantLightEvents[MANUAL_EVENT].ui32_time = m_ui32_currentTime + TIME_IN_MANUAL_MODE;
		} else {
			/* Sinon, le mode manuel termine le jour d'après */
			m_tst_plantLightEvents[MANUAL_EVENT].b_isBtwTwoDays = true;
			m_tst_plantLightEvents[MANUAL_EVENT].ui32_time = TIME_IN_MANUAL_MODE - (NB_SECOND_PER_DAY - m_ui32_currentTime);
			//m_tst_plantLightEvents[MANUAL_EVENT].ui32_time = m_ui32_currentTime + TIME_IN_MANUAL_MODE - NB_SECOND_PER_DAY;
		}
		LOG("Heure de fin mode manuel H[%lu] : %lu", m_ui32_currentTime, m_tst_plantLightEvents[MANUAL_EVENT].ui32_time);
		LOG("	Est entre deux jours : %d", m_tst_plantLightEvents[MANUAL_EVENT].b_isBtwTwoDays);
		
		/* Si la lumière est eteinte, l'allume et inversement */
		if(m_en_plantLightEvent == STOP_PLANTLIGHT_CYCLE) {
			m_tst_plantLightEvents[MANUAL_EVENT].en_name = ON_MANUAL_LIGHT_MODE;
		} else {
			m_tst_plantLightEvents[MANUAL_EVENT].en_name = OFF_MANUAL_LIGHT_MODE;
		}
	} else {
		LOG("Desactivation mode manuel PL H[%lu]", m_ui32_currentTime);
		m_b_isManualModePlantLight = false;
	}
	v_Scheduler_updateEventPlantLight();
}

void Scheduler::v_Scheduler_setManualModeAquaLight()
{
	if(m_en_aquaLightEvent == START_AQUALIGHT_CYCLE || m_en_aquaLightEvent == STOP_AQUALIGHT_CYCLE) {
		LOG("Activation mode manuel AL H[%lu]", m_ui32_currentTime);
		m_b_isManualModeAquaLight = true;
		
		/* Configure l'heure d'arret du mode manuel */
		if((m_ui32_currentTime + TIME_IN_MANUAL_MODE) < NB_SECOND_PER_DAY) {
			/* Si le mode manuel se termine le même jour */
			m_tst_aquaLightEvents[MANUAL_EVENT].b_isBtwTwoDays = false;
			m_tst_aquaLightEvents[MANUAL_EVENT].ui32_time = m_ui32_currentTime + TIME_IN_MANUAL_MODE;
		} else {
			/* Sinon, le mode manuel termine le jour d'après */
			m_tst_aquaLightEvents[MANUAL_EVENT].b_isBtwTwoDays = true;
			m_tst_aquaLightEvents[MANUAL_EVENT].ui32_time = m_ui32_currentTime + TIME_IN_MANUAL_MODE - NB_SECOND_PER_DAY;
		}
		LOG("Heure de fin mode manuel H[%lu] : %lu", m_ui32_currentTime, m_tst_aquaLightEvents[MANUAL_EVENT].ui32_time);
		LOG("	Est entre deux jours : %d", m_tst_aquaLightEvents[MANUAL_EVENT].b_isBtwTwoDays);
		
		/* Si la lumière est eteinte, l'allume et inversement */
		if(m_en_aquaLightEvent == STOP_AQUALIGHT_CYCLE) {
			m_tst_aquaLightEvents[MANUAL_EVENT].en_name = ON_MANUAL_LIGHT_MODE;
		} else {
			m_tst_aquaLightEvents[MANUAL_EVENT].en_name = OFF_MANUAL_LIGHT_MODE;
		}
	} else {
		if(m_b_isWaitingStopHManualTime) {
			m_en_aquaLightEvent = START_AQUALIGHT_CYCLE;
		} else {/* Nothing to do - Le mode MANUAL_LIGHTING n'est pas activé */}
			
		LOG("Desactivation mode manuel AL H[%lu]", m_ui32_currentTime);
		m_b_isManualModeAquaLight = false;
	}
	v_Scheduler_updateEventAquaLight();
}

void Scheduler::v_Scheduler_stopManualModePlantLight()
{
	m_b_isManualModePlantLight = false;
}

void Scheduler::v_Scheduler_stopManualModeAquaLight()
{
	m_b_isManualModeAquaLight = false;
}

const BOOL Scheduler::b_Scheduler_isWatingStopTimeModeAL() const
{
	return m_b_isWaitingStopHManualTime;
}

void Scheduler::v_Scheduler_updateEventPlantLight()
{
	/* Mise à jour de l'event Lumière Plante */
	if(!m_b_isManualModePlantLight) {
		/* Mode cyclique */
		/* Si l'evènement se passe le jour même */
		if(!m_tst_plantLightEvents[START_EVENT].b_isBtwTwoDays) {
			if(m_ui32_currentTime >= m_tst_plantLightEvents[START_EVENT].ui32_time && m_ui32_currentTime < m_tst_plantLightEvents[STOP_EVENT].ui32_time) {
				#ifdef DEBUG_MOD_LOG
					if(m_tst_plantLightEvents[START_EVENT].en_name != m_en_plantLightEvent) {
						LOG("PL Event START H[%lu]", m_ui32_currentTime);
					} else {/* Nothing to do - L'Event n'a pas changé */}
				#endif				
				m_en_plantLightEvent = m_tst_plantLightEvents[START_EVENT].en_name;
			} else {
				#ifdef DEBUG_MOD_LOG				
					if(m_tst_plantLightEvents[STOP_EVENT].en_name != m_en_plantLightEvent) {
						LOG("PL Event STOP H[%lu]", m_ui32_currentTime);
					} else {/* Nothing to do - L'Event n'a pas changé */}
				#endif				
				m_en_plantLightEvent = m_tst_plantLightEvents[STOP_EVENT].en_name;
			}
		} else {
		/* Si l'évènement est entre 2 jours */
			if(m_ui32_currentTime >= m_tst_plantLightEvents[STOP_EVENT].ui32_time && m_ui32_currentTime < m_tst_plantLightEvents[START_EVENT].ui32_time) {
				#ifdef DEBUG_MOD_LOG
					if(m_tst_plantLightEvents[STOP_EVENT].en_name != m_en_plantLightEvent) {
						LOG("PL Event STOP H[%lu]", m_ui32_currentTime);
					} else {/* Nothing to do - L'Event n'a pas changé */}
				#endif
				m_en_plantLightEvent = m_tst_plantLightEvents[STOP_EVENT].en_name;
			} else {
				#ifdef DEBUG_MOD_LOG				
					if(m_tst_plantLightEvents[START_EVENT].en_name != m_en_plantLightEvent) {
						LOG("PL Event START H[%lu]", m_ui32_currentTime);
					} else {/* Nothing to do - L'Event n'a pas changé */}
				#endif	
				m_en_plantLightEvent = m_tst_plantLightEvents[START_EVENT].en_name;
			}
		}
	} else {
		/* Mode manuel */
		if(!m_tst_plantLightEvents[MANUAL_EVENT].b_isBtwTwoDays) {
			if(m_ui32_currentTime <= m_tst_plantLightEvents[MANUAL_EVENT].ui32_time) {
				m_en_plantLightEvent = m_tst_plantLightEvents[MANUAL_EVENT].en_name;
			} else {
				/* Délais du mode manuel dépassé, on écrase pas l'event pour repartir en mode cyclique */
				LOG("Delais du mode manuel PL atteint, desactivation H[%lu]", m_ui32_currentTime);
				m_b_isManualModePlantLight = false;
			}
		} else {
			if(m_ui32_currentTime <= m_tst_plantLightEvents[MANUAL_EVENT].ui32_time ||
			   m_ui32_currentTime >= NB_SECOND_PER_DAY - (TIME_IN_MANUAL_MODE - m_tst_plantLightEvents[MANUAL_EVENT].ui32_time)) {
				m_en_plantLightEvent = m_tst_plantLightEvents[MANUAL_EVENT].en_name;
			} else {
				/* Délais du mode manuel dépassé, on écrase pas l'event pour repartir en mode cyclique */
				LOG("Delais du mode manuel PL atteint, desactivation H[%lu]", m_ui32_currentTime);
				m_b_isManualModePlantLight = false;
			}
		}
	}
}

void Scheduler::v_Scheduler_updateEventPump()
{
	/* Mise à jour de l'event Pump */
	/* Si l'evènement se passe le jour même */
	if(!m_tst_pumpEvents[START_EVENT].b_isBtwTwoDays) {
		if(m_ui32_currentTime >= m_tst_pumpEvents[START_EVENT].ui32_time && m_ui32_currentTime < m_tst_pumpEvents[STOP_EVENT].ui32_time) {
			#ifdef DEBUG_MOD_LOG
				if(m_tst_pumpEvents[START_EVENT].en_name != m_en_pumpEvent) {
					LOG("Pump Event START H[%lu]", m_ui32_currentTime);
				} else {/* Nothing to do - L'Event n'a pas changé */}
			#endif
			m_en_pumpEvent = m_tst_pumpEvents[START_EVENT].en_name;
		} else {
			#ifdef DEBUG_MOD_LOG				
				if(m_tst_pumpEvents[STOP_EVENT].en_name != m_en_pumpEvent) {
					LOG("Pump Event STOP H[%lu]", m_ui32_currentTime);
				} else {/* Nothing to do - L'Event n'a pas changé */}
			#endif
			m_en_pumpEvent = m_tst_pumpEvents[STOP_EVENT].en_name;
		}
	} else {
	/* Si l'évènement est entre 2 jours */
		if(m_ui32_currentTime >= m_tst_pumpEvents[STOP_EVENT].ui32_time && m_ui32_currentTime < m_tst_pumpEvents[START_EVENT].ui32_time) {
			#ifdef DEBUG_MOD_LOG
				if(m_tst_pumpEvents[STOP_EVENT].en_name != m_en_pumpEvent) {
					LOG("Pump Event STOP H[%lu]", m_ui32_currentTime);
				} else {/* Nothing to do - L'Event n'a pas changé */}
			#endif
			m_en_pumpEvent = m_tst_pumpEvents[STOP_EVENT].en_name;
		} else {
			#ifdef DEBUG_MOD_LOG				
				if(m_tst_pumpEvents[START_EVENT].en_name != m_en_pumpEvent) {
					LOG("Pump Event START H[%lu]", m_ui32_currentTime);
				} else {/* Nothing to do - L'Event n'a pas changé */}
			#endif	
			m_en_pumpEvent = m_tst_pumpEvents[START_EVENT].en_name;
		}
	}
}

void Scheduler::v_Scheduler_updateEventAquaLight()
{
	/* Mise à jour de l'event Lumière Aquarium */
	if(!m_b_isManualModeAquaLight) {
		/* Si le réglage du temps de cycle manuel est en cours, on ne récupère pas l'event (déja config à START_AQUALIGHT_CYCLE) */
		if(!m_b_isWaitingStopHManualTime) {
			/* Mode cyclique */
			/* Si l'evènement se passe le jour même */
			if(!m_tst_aquaLightEvents[START_EVENT].b_isBtwTwoDays) {
				if(m_ui32_currentTime >= m_tst_aquaLightEvents[START_EVENT].ui32_time && m_ui32_currentTime < m_tst_aquaLightEvents[STOP_EVENT].ui32_time) {
					#ifdef DEBUG_MOD_LOG
						if(m_tst_aquaLightEvents[START_EVENT].en_name != m_en_aquaLightEvent) {
							LOG("AL Event START H[%lu]", m_ui32_currentTime);
						} else {/* Nothing to do - L'Event n'a pas changé */}
					#endif
					m_en_aquaLightEvent = m_tst_aquaLightEvents[START_EVENT].en_name;
				} else {
					#ifdef DEBUG_MOD_LOG
						if(m_tst_aquaLightEvents[STOP_EVENT].en_name != m_en_aquaLightEvent) {
							LOG("AL Event STOP H[%lu]", m_ui32_currentTime);
						} else {/* Nothing to do - L'Event n'a pas changé */}
					#endif
					m_en_aquaLightEvent = m_tst_aquaLightEvents[STOP_EVENT].en_name;
				}
			} else {
			/* Si l'évènement est entre 2 jours */
				if(m_ui32_currentTime >= m_tst_aquaLightEvents[STOP_EVENT].ui32_time && m_ui32_currentTime < m_tst_aquaLightEvents[START_EVENT].ui32_time) {
					#ifdef DEBUG_MOD_LOG
						if(m_tst_aquaLightEvents[STOP_EVENT].en_name != m_en_aquaLightEvent) {
							LOG("AL Event STOP H[%lu]", m_ui32_currentTime);
						} else {/* Nothing to do - L'Event n'a pas changé */}
					#endif
					m_en_aquaLightEvent = m_tst_aquaLightEvents[STOP_EVENT].en_name;
				} else {
					#ifdef DEBUG_MOD_LOG
						if(m_tst_aquaLightEvents[START_EVENT].en_name != m_en_aquaLightEvent) {
							LOG("AL Event START H[%lu]", m_ui32_currentTime);
						} else {/* Nothing to do - L'Event n'a pas changé */}
					#endif
					m_en_aquaLightEvent = m_tst_aquaLightEvents[START_EVENT].en_name;
				}
			}
		} else {
			/* Controle du Timeout du mode MANUAL_LIGHTING */
			if(!m_st_tempStartTimeAquaLight.b_isBtwTwoDays) {
				/* Si l'evènement se passe le jour même */
				if(m_ui32_currentTime - m_st_tempStartTimeAquaLight.ui32_time > TIME_MAX_MANUAL_MODE) {
					/* Temps dépassé, on n'enregistre pas les réglages */
					LOG("Temps max atteint pour MANUAL_LIGHTING, reglage non enregistre H[%lu]", m_ui32_currentTime);
					m_b_isWaitingStopHManualTime = false;
				} else {/* Nothing to do - Le timeout n'est pas atteint */}
			} else {
				/* Si l'évènement est entre 2 jours */
				if((m_ui32_currentTime + (NB_SECOND_PER_DAY - m_st_tempStartTimeAquaLight.ui32_time)) > TIME_MAX_MANUAL_MODE) {
					LOG("Temps max atteint pour MANUAL_LIGHTING, reglage non enregistre H[%lu]", m_ui32_currentTime);
					m_b_isWaitingStopHManualTime = false;
				} else {/* Nothing to do - Le timeout n'est pas atteint */}
			}
		}
	} else {
		/* Mode manuel */
		if(!m_tst_aquaLightEvents[MANUAL_EVENT].b_isBtwTwoDays) {
			if(m_ui32_currentTime <= m_tst_aquaLightEvents[MANUAL_EVENT].ui32_time) {
				m_en_aquaLightEvent = m_tst_aquaLightEvents[MANUAL_EVENT].en_name;
			} else {
				/* Délais du mode manuel dépassé, on écrase pas l'event pour repartir en mode cyclique */
				LOG("Delais du mode manuel AL atteint, desactivation H[%lu]", m_ui32_currentTime);
				m_b_isManualModeAquaLight = false;
			}
		} else {
			if(m_ui32_currentTime <= m_tst_aquaLightEvents[MANUAL_EVENT].ui32_time ||
			   m_ui32_currentTime >= NB_SECOND_PER_DAY - (TIME_IN_MANUAL_MODE - m_tst_aquaLightEvents[MANUAL_EVENT].ui32_time)) {
				m_en_aquaLightEvent = m_tst_aquaLightEvents[MANUAL_EVENT].en_name;
			} else {
				/* Délais du mode manuel dépassé, on écrase pas l'event pour repartir en mode cyclique */
				LOG("Delais du mode manuel AL atteint, desactivation H[%lu]", m_ui32_currentTime);
				m_b_isManualModeAquaLight = false;
			}
		}
	}
}

SCHEDULED_EVENT Scheduler::en_Scheduler_setTimeModePlantLight(const PLANTLIGHT_CONFIG_TIME en_timeModeName)
{
	switch(en_timeModeName) {
		case H12_LIGHTING:
			if((INT32)(m_tst_plantLightEvents[STOP_EVENT].ui32_time - NB_SECOND_IN_12H) >= 0) {
				m_tst_plantLightEvents[START_EVENT].ui32_time = m_tst_plantLightEvents[STOP_EVENT].ui32_time - NB_SECOND_IN_12H;
				m_tst_plantLightEvents[START_EVENT].b_isBtwTwoDays = false;
			} else {
				m_tst_plantLightEvents[START_EVENT].ui32_time = NB_SECOND_PER_DAY - (NB_SECOND_IN_12H -  m_tst_plantLightEvents[STOP_EVENT].ui32_time);
				m_tst_plantLightEvents[START_EVENT].b_isBtwTwoDays = true;
			}
			break;
			
		case H14_LIGHTING:
			if((INT32)(m_tst_plantLightEvents[STOP_EVENT].ui32_time - NB_SECOND_IN_14H) >= 0) {
				m_tst_plantLightEvents[START_EVENT].ui32_time = m_tst_plantLightEvents[STOP_EVENT].ui32_time - NB_SECOND_IN_14H;
				m_tst_plantLightEvents[START_EVENT].b_isBtwTwoDays = false;
			} else {
				m_tst_plantLightEvents[START_EVENT].ui32_time = NB_SECOND_PER_DAY - (NB_SECOND_IN_14H -  m_tst_plantLightEvents[STOP_EVENT].ui32_time);
				m_tst_plantLightEvents[START_EVENT].b_isBtwTwoDays = true;
			}
			break;
	}

	LOG("Set Time Mode PL H[%lu] :", m_ui32_currentTime);
	LOG("	Mode : %d", en_timeModeName);
	LOG("	START : %lu", m_tst_plantLightEvents[START_EVENT].ui32_time);
	LOG("	STOP : %lu", m_tst_plantLightEvents[STOP_EVENT].ui32_time);
	LOG("	BTW2D : %d", m_tst_plantLightEvents[START_EVENT].b_isBtwTwoDays);

	v_Scheduler_updateEventPlantLight();

	return m_en_plantLightEvent;
}

SCHEDULED_EVENT Scheduler::en_Scheduler_setTimeModeAquaLight(const AQUALIGHT_CONFIG_TIME en_timeModeName)
{
	switch(en_timeModeName) {
		case H0_LIGHTING:
			m_b_isWaitingStopHManualTime = false;
			m_tst_aquaLightEvents[START_EVENT].ui32_time = m_tst_aquaLightEvents[STOP_EVENT].ui32_time;
			m_tst_aquaLightEvents[START_EVENT].b_isBtwTwoDays = false;
			break;

		case H6_LIGHTING:
			m_b_isWaitingStopHManualTime = false;
			if((INT32)(m_tst_aquaLightEvents[STOP_EVENT].ui32_time - NB_SECOND_IN_6H) >= 0) {
				m_tst_aquaLightEvents[START_EVENT].ui32_time = m_tst_aquaLightEvents[STOP_EVENT].ui32_time - NB_SECOND_IN_6H;
				m_tst_aquaLightEvents[START_EVENT].b_isBtwTwoDays = false;
			} else {
				m_tst_aquaLightEvents[START_EVENT].ui32_time = NB_SECOND_PER_DAY - (NB_SECOND_IN_6H -  m_tst_aquaLightEvents[STOP_EVENT].ui32_time);
				m_tst_aquaLightEvents[START_EVENT].b_isBtwTwoDays = true;
			}
			break;

		case H8_LIGHTING:
			m_b_isWaitingStopHManualTime = false;
			if((INT32)(m_tst_aquaLightEvents[STOP_EVENT].ui32_time - NB_SECOND_IN_8H) >= 0) {
				m_tst_aquaLightEvents[START_EVENT].ui32_time = m_tst_aquaLightEvents[STOP_EVENT].ui32_time - NB_SECOND_IN_8H;
				m_tst_aquaLightEvents[START_EVENT].b_isBtwTwoDays = false;
			} else {
				m_tst_aquaLightEvents[START_EVENT].ui32_time = NB_SECOND_PER_DAY - (NB_SECOND_IN_8H -  m_tst_aquaLightEvents[STOP_EVENT].ui32_time);
				m_tst_aquaLightEvents[START_EVENT].b_isBtwTwoDays = true;
			}
			break;

		case MANUAL_LIGHTING:
			if(!m_b_isWaitingStopHManualTime) {
				m_b_isWaitingStopHManualTime = true;
				m_en_aquaLightEvent = START_AQUALIGHT_CYCLE;
				m_st_tempStartTimeAquaLight.ui32_time = m_ui32_currentTime;
				m_st_tempStartTimeAquaLight.b_isBtwTwoDays = false;
			} else {
				m_b_isWaitingStopHManualTime = false;
				m_tst_aquaLightEvents[START_EVENT].b_isBtwTwoDays = m_st_tempStartTimeAquaLight.b_isBtwTwoDays;
				m_tst_aquaLightEvents[START_EVENT].ui32_time = m_st_tempStartTimeAquaLight.ui32_time;
				m_tst_aquaLightEvents[STOP_EVENT].ui32_time = m_ui32_currentTime;
			}
			break;
	}
	
	LOG("Set Time Mode AL H[%lu] :", m_ui32_currentTime);
	LOG("	Mode : %d", en_timeModeName);
	LOG("	START : %lu", m_tst_aquaLightEvents[START_EVENT].ui32_time);
	LOG("	STOP : %lu", m_tst_aquaLightEvents[STOP_EVENT].ui32_time);
	LOG("	BTW2D : %d", m_tst_aquaLightEvents[START_EVENT].b_isBtwTwoDays);
	
	v_Scheduler_updateEventAquaLight();
	
	return m_en_aquaLightEvent;
}

void Scheduler::v_Scheduler_setStopTimePlantLight(const PLANTLIGHT_CONFIG_TIME en_timeModeName)
{
	/* Si réglage de l'heure, le prochain event est forcement un stop */
	m_en_plantLightEvent = STOP_PLANTLIGHT_CYCLE;
	
	m_tst_plantLightEvents[STOP_EVENT].ui32_time = m_ui32_currentTime;
	
	switch(en_timeModeName) {
		case H12_LIGHTING:
			if((INT32)(m_tst_plantLightEvents[STOP_EVENT].ui32_time - NB_SECOND_IN_12H) >= 0) {
				m_tst_plantLightEvents[START_EVENT].ui32_time = m_tst_plantLightEvents[STOP_EVENT].ui32_time - NB_SECOND_IN_12H;
				m_tst_plantLightEvents[START_EVENT].b_isBtwTwoDays = false;
			} else {
				m_tst_plantLightEvents[START_EVENT].ui32_time = NB_SECOND_PER_DAY - (NB_SECOND_IN_12H - m_ui32_currentTime);
				m_tst_plantLightEvents[START_EVENT].b_isBtwTwoDays = true;
			}
			break;

		case H14_LIGHTING:
			if((INT32)(m_tst_plantLightEvents[STOP_EVENT].ui32_time - NB_SECOND_IN_14H) >= 0) {
				m_tst_plantLightEvents[START_EVENT].ui32_time = m_tst_plantLightEvents[STOP_EVENT].ui32_time - NB_SECOND_IN_14H;
				m_tst_plantLightEvents[START_EVENT].b_isBtwTwoDays = false;
			} else {
				m_tst_plantLightEvents[START_EVENT].ui32_time = NB_SECOND_PER_DAY - (NB_SECOND_IN_14H - m_ui32_currentTime);
				m_tst_plantLightEvents[START_EVENT].b_isBtwTwoDays = true;
			}
			break;
	}
	
	LOG("Set Stop Time PL H[%lu] :", m_ui32_currentTime);
	LOG("	Mode : %d", en_timeModeName);
	LOG("	START : %lu", m_tst_plantLightEvents[START_EVENT].ui32_time);
	LOG("	STOP : %lu", m_tst_plantLightEvents[STOP_EVENT].ui32_time);
	LOG("	BTW2D : %d", m_tst_plantLightEvents[START_EVENT].b_isBtwTwoDays);
	
	v_Scheduler_setTimePump();
}

void Scheduler::v_Scheduler_setStopTimeAquaLight(const AQUALIGHT_CONFIG_TIME en_timeModeName)
{
	UINT32 ui32_nbSecondManualMode = 0;
	
	/* Si réglage de l'heure, le prochain event est forcement un stop */
	m_en_aquaLightEvent = STOP_AQUALIGHT_CYCLE;
	
	m_tst_aquaLightEvents[STOP_EVENT].ui32_time = m_ui32_currentTime;
	
	switch(en_timeModeName) {
		case H0_LIGHTING:
			m_tst_aquaLightEvents[START_EVENT].ui32_time = m_ui32_currentTime;
			m_tst_aquaLightEvents[START_EVENT].b_isBtwTwoDays = false;
			break;
			
		case H6_LIGHTING:
			if((INT32)(m_ui32_currentTime - NB_SECOND_IN_6H) >= 0) {
				m_tst_aquaLightEvents[START_EVENT].ui32_time = m_ui32_currentTime - NB_SECOND_IN_6H;
				m_tst_aquaLightEvents[START_EVENT].b_isBtwTwoDays = false;
			} else {
				m_tst_aquaLightEvents[START_EVENT].ui32_time = NB_SECOND_PER_DAY - (NB_SECOND_IN_6H - m_ui32_currentTime);
				m_tst_aquaLightEvents[START_EVENT].b_isBtwTwoDays = true;
			}
			break;
			
		case H8_LIGHTING:
			if((INT32)(m_ui32_currentTime - NB_SECOND_IN_8H) >= 0) {
				m_tst_aquaLightEvents[START_EVENT].ui32_time = m_ui32_currentTime - NB_SECOND_IN_8H;
				m_tst_aquaLightEvents[START_EVENT].b_isBtwTwoDays = false;
			} else {
				m_tst_aquaLightEvents[START_EVENT].ui32_time = NB_SECOND_PER_DAY - (NB_SECOND_IN_8H - m_ui32_currentTime);
				m_tst_aquaLightEvents[START_EVENT].b_isBtwTwoDays = true;
			}
			break;
			
		case MANUAL_LIGHTING:
			ui32_nbSecondManualMode = v_Scheduler_calculateManualTime();
			if((INT32)(m_ui32_currentTime - ui32_nbSecondManualMode) >= 0) {
				m_tst_aquaLightEvents[START_EVENT].ui32_time = m_ui32_currentTime - ui32_nbSecondManualMode;
				m_tst_aquaLightEvents[START_EVENT].b_isBtwTwoDays = false;
			} else {
				m_tst_aquaLightEvents[START_EVENT].ui32_time = NB_SECOND_PER_DAY - (ui32_nbSecondManualMode - m_ui32_currentTime);
				m_tst_aquaLightEvents[START_EVENT].b_isBtwTwoDays = true;
			}
			break;
	}
	
	LOG("Set Stop Time AL H[%lu] :", m_ui32_currentTime);
	LOG("	Mode : %d", en_timeModeName);
	LOG("	START : %lu", m_tst_aquaLightEvents[START_EVENT].ui32_time);
	LOG("	STOP : %lu", m_tst_aquaLightEvents[STOP_EVENT].ui32_time);
	LOG("	BTW2D : %d", m_tst_aquaLightEvents[START_EVENT].b_isBtwTwoDays);
}

inline const UINT32 Scheduler::v_Scheduler_calculateManualTime() const
{
	return (m_tst_aquaLightEvents[START_EVENT].ui32_time < m_tst_aquaLightEvents[STOP_EVENT].ui32_time)
		/* L'evenement se passe le même jour */
		? m_tst_aquaLightEvents[STOP_EVENT].ui32_time - m_tst_aquaLightEvents[START_EVENT].ui32_time
		/* L'évènement est entre 2 jours */
		: NB_SECOND_PER_DAY - (m_tst_aquaLightEvents[START_EVENT].ui32_time - m_tst_aquaLightEvents[STOP_EVENT].ui32_time);
}


void Scheduler::v_Scheduler_setTimePump()
{
	/* Réglage de l'heure de début de l'event Marée Haute (Pompe Max) */
	if(m_tst_plantLightEvents[STOP_EVENT].ui32_time + TIME_BTW_PLANTLIGHT_PUMP <= NB_SECOND_PER_DAY) {
		m_tst_pumpEvents[START_EVENT].ui32_time	= m_tst_plantLightEvents[STOP_EVENT].ui32_time + TIME_BTW_PLANTLIGHT_PUMP;
	} else {
		m_tst_pumpEvents[START_EVENT].ui32_time	= TIME_BTW_PLANTLIGHT_PUMP - (NB_SECOND_PER_DAY - m_tst_plantLightEvents[STOP_EVENT].ui32_time);
	}
	
	/* Réglage de l'heure de fin de l'event Marée Haute (Pompe Max) */
	if(m_tst_pumpEvents[START_EVENT].ui32_time + TIME_PUMP_HIGHT_TIDE <= NB_SECOND_PER_DAY) {
		m_tst_pumpEvents[STOP_EVENT].ui32_time	= m_tst_pumpEvents[START_EVENT].ui32_time + TIME_PUMP_HIGHT_TIDE;
		m_tst_pumpEvents[START_EVENT].b_isBtwTwoDays = false;
	} else {
		m_tst_pumpEvents[STOP_EVENT].ui32_time	= TIME_PUMP_HIGHT_TIDE - (NB_SECOND_PER_DAY - m_tst_pumpEvents[START_EVENT].ui32_time);
		m_tst_pumpEvents[START_EVENT].b_isBtwTwoDays = true;
	}
	
#ifdef DEBUG_MOD_SHORT_DAY
	m_tst_pumpEvents[START_EVENT].ui32_time	= 100;
	m_tst_pumpEvents[STOP_EVENT].ui32_time = 250;
	m_tst_pumpEvents[START_EVENT].b_isBtwTwoDays = false;
#endif

	LOG("Set Time Pump H[%lu] :", m_ui32_currentTime);
	LOG("	START : %lu", m_tst_pumpEvents[START_EVENT].ui32_time);
	LOG("	STOP : %lu", m_tst_pumpEvents[STOP_EVENT].ui32_time);
	LOG("	BTW2D : %d", m_tst_pumpEvents[START_EVENT].b_isBtwTwoDays);
}