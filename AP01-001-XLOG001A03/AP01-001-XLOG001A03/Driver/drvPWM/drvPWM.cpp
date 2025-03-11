/*!
 * \file 	drvPWM.cpp
 * \brief 	
 * \author 	Valentin DOMINIAK
 * \date	10/2023
 * \warning
 * MIT License
 * This software is provided "as is", without warranty of any kind. The authors
 * shall not be liable for any claims, damages, or other liability arising from
 * the use of the software. See the LICENSE file for more details.
 * \version 	0.1 (10/2023)
*/

/*!==========================================================================+*/
// FICHIERS HEADER
/*+==========================================================================+*/
#include <stddef.h>
#include "drvPWM.h"
#include "drvHardware.h"

/*!==========================================================================+*/
// DEFINES
/*+==========================================================================+*/
#define TCA0_PRESCALER				8 // TODO Calculer automatiquement la valeur le define PWM_FREQUENCY (Attention, clock utilisée par le drvRPM, ne pas changer)
#define PWM_OUTPUT_CHANNEL_NUMBER	6

#define FREQUENCY_REGISTER_VALUE	(UINT32)(ui32_drvHardware_getCPUClockFrequency()/((UINT32)(PWM_FREQUENCY) * (UINT32)(TCA0_PRESCALER)))

/*!==========================================================================+*/
// VARIABLES GLOBALES
/*+==========================================================================+*/
struct PWM_DATA{
	PWM_CHANNEL	en_channel;
	UINT8		ui8_dutyCycle;
	BOOL		b_enabled;
};

static PWM_DATA ts_channelsCaract[PWM_OUTPUT_CHANNEL_NUMBER];	// Tableau de structures comprenant les caractéristiques de chaque sortie PWM
static UINT8    ui8_instanceNumber = 0x00;						// Nombre incrémental du numero d'instance du driver PWM

/*!==========================================================================+*/
// PROTOTYPES
/*+==========================================================================+*/
static void v_drvPWM_initializationChannel			(const PWM_CHANNEL en_channel, const UINT8 ui8_defaultDutyCycle);
static void v_drvPWM_initializationStructChannelsCaract(void);

/*!==========================================================================+*/
/*+
 *  \brief		Initialisation du driver PWM et de la sortie souhaitée
 *  \param[in]	en_channel					PWM_CHANNEL_0 = WO0
 *											PWM_CHANNEL_1 = WO1
 *											PWM_CHANNEL_2 = WO2
 *  										PWM_CHANNEL_3 = WO3
 *											PWM_CHANNEL_4 = WO4
 *											PWM_CHANNEL_5 = WO5
 *  \param[in]	ui8_defaultDutyCycle		Rapport cyclique par défaut 
 *											sur la sortie 0 à 100%
 *  \param[out] pui8_numeroInstance			Numéro d'instance de la sortie 
 *											initialisée
 *  \return     en_codeError				Enumeration d'erreur de la fonction
 *  \author     Valentin DOMINIAK
 *  \date       Creation: 10/2023
 *  \remarks
 */
/*+==========================================================================+*/
DRV_PWM_ERROR en_drvPWM_initialization(const PWM_CHANNEL en_channel, const UINT8 ui8_defaultDutyCycle, UINT8 *pui8_instanceNumber)
{
	DRV_PWM_ERROR	en_codeError				= DRV_PWM_ERROR_NO_ERROR;
	UINT8			ui8_instanceNumberIterator	= 0;
	
	if(pui8_instanceNumber != NULL) {

		// Verification du nombre d'instances PWM par rapport au nombre de sorties
		if(ui8_instanceNumber < PWM_OUTPUT_CHANNEL_NUMBER) {

			// Vérification si le numéro de channel est correct
			if(en_channel != PWM_CHANNEL_UNDEFINED) {
				
				// Vérification si le rapport cyclique est comprit entre 0 et 100%
				if((ui8_defaultDutyCycle >= 0) && (ui8_defaultDutyCycle <= 100)) {
					
					// S'il s'agit du premier appel de la fonction, les configurations générales sont réalisées qu'une seule fois
					if(ui8_instanceNumber == 0) {
						
						// Initialisation de la structure liant la configuration souhaitée sur un channel et son numéro d'instance associé
						v_drvPWM_initializationStructChannelsCaract();
				
						// Timer PWM TCA0 en mode splité pour utilisation des sorties WO0 à WO5
						TCA0.SPLIT.CTRLD |= TCA_SPLIT_SPLITM_bm;

						// Configuration de la fréquence PWM avec TCA = fclk/(prescaler*fpwm), même fréquence utilisée sur toutes les pins (HPER / LPER)
						TCA0.SPLIT.HPER = (UINT8)(FREQUENCY_REGISTER_VALUE);
						TCA0.SPLIT.LPER	= TCA0.SPLIT.HPER;
	
						// Fonctionnement en mode debug
						TCA0.SPLIT.DBGCTRL |= TCA_SPLIT_DBGRUN_bm;
	
						// Division de l'horloge / 8 (Attention, clock utilisée par le drvRPM, ne pas changer)
						TCA0.SPLIT.CTRLA |= TCA_SPLIT_CLKSEL_DIV8_gc;
					} else {
						// Ce n'est pas la première fois que la fonction est appelée, vérification si la sortie PWM à configurer n'est pas déjà utilisée
						for(ui8_instanceNumberIterator = 0 ; ui8_instanceNumberIterator < PWM_OUTPUT_CHANNEL_NUMBER ; ui8_instanceNumberIterator++) {
							if (en_channel == ts_channelsCaract[ui8_instanceNumberIterator].en_channel) {
								en_codeError = DRV_PWM_ERROR_CHANNEL_ALREADY_USED;
							} else {/* Ne rien faire, le channel n'est pas utilisé */}
						}
					}
				
					// S'il n'y a pas eu d'erreur
					if(en_codeError == DRV_PWM_ERROR_NO_ERROR) {
						
						// Initialisation du channel
						v_drvPWM_initializationChannel(en_channel, ui8_defaultDutyCycle);
							
						// Sauvegarde de la configuration correspondant au numéro d'instance venant d'être créé
						ts_channelsCaract[ui8_instanceNumber].en_channel	= en_channel;
						ts_channelsCaract[ui8_instanceNumber].ui8_dutyCycle = ui8_defaultDutyCycle;
						ts_channelsCaract[ui8_instanceNumber].b_enabled		= true;

						// La valeur de l'instance du module PWM est retournée par le parametre d'entrée puis incrémenté pour la prochaine sortie à initialiser
						*pui8_instanceNumber = ui8_instanceNumber;
						ui8_instanceNumber++;
					} else {/* Erreur, le channel à initialiser est déjà utilisé, ne rien faire, code mis à jour */}
				} else {
					// Erreur, le rapport cyclique n'est pas comprit entre 0 et 100%
					en_codeError = DRV_PWM_ERROR_DUTY_CYCLE_VALUE;
				}
			} else {
				// Erreur, le numéro de channel à configurer n'est pas définit
				en_codeError = DRV_PWM_ERROR_CHANNEL_UNDEFINED;
			}
		} else {
			// Erreur, trop d'instances PWM par rapport au nombre de sorties
			en_codeError = DRV_PWM_ERROR_TOO_MUCH_INSTANCE_NUMBER;
		}
	} else {
		// Erreur, pointeur nul
		en_codeError = DRV_PWM_ERROR_NULL_PTR;
	}
	
	// Log d'une erreur si erreur
	if(en_codeError != DRV_PWM_ERROR_NO_ERROR)
	{
		LOG("Erreur en_drvPWM_initialization : %d", en_codeError);
	} else {/* Nothing to do - Pas d'erreur */}
	
	return en_codeError;
}


/*!==========================================================================+*/
/*+
 *  \brief		Changement du rapport cyclique sur la sortie souhaitée
 *  \param[in]	en_channel			PWM_CHANNEL_0 = WO0
 *									PWM_CHANNEL_1 = WO1
 *									PWM_CHANNEL_2 = WO2
 *  								PWM_CHANNEL_3 = WO3
 *									PWM_CHANNEL_4 = WO4
 *									PWM_CHANNEL_5 = WO5
 *  \param[in]	ui8_dutyCycle		Rapport cyclique par défaut 
 *									sur la sortie 0 à 100%
 *  \param[in]	ui8_instanceNumber	Numéro d'instance de la sortie à contrôler
 *  \param[out] none
 *  \return     en_codeError		Enumeration d'erreur de la fonction
 *  \author     Valentin DOMINIAK
 *  \date       Creation: 10/2023
 *  \remarks
 */
/*+==========================================================================+*/
DRV_PWM_ERROR en_drvPWM_changeDutyCycle(const UINT8 ui8_instanceNumber, const UINT8 ui8_dutyCycle)
{
	DRV_PWM_ERROR	en_codeError				= DRV_PWM_ERROR_NO_ERROR;
	UINT8			ui8_dutyCycleRegisterValue	= 0;

	// Si la valeur actuelle de la PWM est différente de la valeur souhaitée
	if(ts_channelsCaract[ui8_instanceNumber].ui8_dutyCycle != ui8_dutyCycle){
		
		// Vérification si la pin à piloter est activée
		if(ts_channelsCaract[ui8_instanceNumber].b_enabled){
			
			// Vérification si le rapport cyclique est comprit entre 0 et 100%
			if((ui8_dutyCycle >= 0) && (ui8_dutyCycle <= 100)){
		
				// Calcul de la valeur et mise à jour du registre correspondant
				ui8_dutyCycleRegisterValue = (UINT8)((ui8_dutyCycle * FREQUENCY_REGISTER_VALUE) / 100);
		
				switch(ts_channelsCaract[ui8_instanceNumber].en_channel){
					case PWM_CHANNEL_0:
					TCA0.SPLIT.LCMP0 = ui8_dutyCycleRegisterValue;
					break;
					case PWM_CHANNEL_1:
					TCA0.SPLIT.LCMP1 = ui8_dutyCycleRegisterValue;
					break;
					case PWM_CHANNEL_2:
					TCA0.SPLIT.LCMP2 = ui8_dutyCycleRegisterValue;
					break;
					case PWM_CHANNEL_3:
					TCA0.SPLIT.HCMP0 = ui8_dutyCycleRegisterValue;
					break;
					case PWM_CHANNEL_4:
					TCA0.SPLIT.HCMP1 = ui8_dutyCycleRegisterValue;
					break;
					case PWM_CHANNEL_5:
					TCA0.SPLIT.HCMP2 = ui8_dutyCycleRegisterValue;
					break;
					default:
					break;
				}
			
				// Sauvegarde de la valeur modifiée du rapport cyclique
				ts_channelsCaract[ui8_instanceNumber].ui8_dutyCycle = ui8_dutyCycle;
			
			} else {
				// Erreur, le rapport cyclique n'est pas comprit entre 0 et 100%
				en_codeError = DRV_PWM_ERROR_DUTY_CYCLE_VALUE;
			}
		} else {
			// Erreur, la sortie PWM à piloter n'est pas activée
			en_codeError = DRV_PWM_ERROR_CHANNEL_NOT_USED;
		}
	} else {/* Ne rien faire, la valeur demandée de rapport cyclique est égale à l'actuelle */}
	
	// Log d'une erreur si erreur
	if(en_codeError != DRV_PWM_ERROR_NO_ERROR)
	{
		LOG("Erreur en_drvPWM_changeDutyCycle : %d", en_codeError);
	} else {/* Nothing to do - Pas d'erreur */}
	
	return en_codeError;
}

/*!==========================================================================+*/
/*+
 *  \brief		Initialisation du driver PWM et de la sortie souhaitée
 *  \param[in]	en_channel					PWM_CHANNEL_0 = WO0
 *											PWM_CHANNEL_1 = WO1
 *											PWM_CHANNEL_2 = WO2
 *  										PWM_CHANNEL_3 = WO3
 *											PWM_CHANNEL_4 = WO4
 *											PWM_CHANNEL_5 = WO5
 *  \param[in]	ui8_defaultDutyCycle		Rapport cyclique par défaut 
 *											sur la sortie de 0 à 100%
 *  \return     none
 *  \author     Valentin DOMINIAK
 *  \date       Creation: 10/2023
 *  \remarks
 */
/*+==========================================================================+*/
void v_drvPWM_initializationChannel(const PWM_CHANNEL en_channel, const UINT8 ui8_defaultDutyCycle)
{	
	UINT8 ui8_dutyCycleRegisterValue = 0;
		
	// Desactivation du timer TCA0 et configuration de la sortie (bit 0 registre CTRLA)
	TCA0.SPLIT.CTRLA &= 0xFE;
	
	// Calcul de la valeur et mise à jour du registre correspondant
	ui8_dutyCycleRegisterValue = (UINT8)((ui8_defaultDutyCycle * FREQUENCY_REGISTER_VALUE) / 100);
			
	switch(en_channel){
		case PWM_CHANNEL_0:
			TCA0.SPLIT.CTRLB |= TCA_SPLIT_LCMP0EN_bm;
			TCA0.SPLIT.LCMP0 = ui8_dutyCycleRegisterValue;
			break;
		case PWM_CHANNEL_1:
			TCA0.SPLIT.CTRLB |= TCA_SPLIT_LCMP1EN_bm;
			TCA0.SPLIT.LCMP1 = ui8_dutyCycleRegisterValue;
			break;
		case PWM_CHANNEL_2:
			TCA0.SPLIT.CTRLB |= TCA_SPLIT_LCMP2EN_bm;
			TCA0.SPLIT.LCMP2 = ui8_dutyCycleRegisterValue;
			break;
		case PWM_CHANNEL_3:
			TCA0.SPLIT.CTRLB |= TCA_SPLIT_HCMP0EN_bm;
			TCA0.SPLIT.HCMP0 = ui8_dutyCycleRegisterValue;
			break;
		case PWM_CHANNEL_4:
			TCA0.SPLIT.CTRLB |= TCA_SPLIT_HCMP1EN_bm;
			TCA0.SPLIT.HCMP1 = ui8_dutyCycleRegisterValue;
			break;
		case PWM_CHANNEL_5:
			TCA0.SPLIT.CTRLB |= TCA_SPLIT_HCMP2EN_bm;
			TCA0.SPLIT.HCMP2 = ui8_dutyCycleRegisterValue;
			break;
		default:
		break;
	}
	
	// Activation du timer TCA0 en mode splité
	TCA0.SPLIT.CTRLA |= TCA_SPLIT_ENABLE_bm;
}

/*!==========================================================================+*/
/*+
 *  \brief      Initialisation des valeurs du tableau de structures ts_channelsCaract
 *  \param[in]  none
 *  \param[out] none
 *  \return     none
 *  \author     Valentin DOMINIAK
 *  \date       Creation: 10/2023
 *  \remarks
 */
/*+==========================================================================+*/
void v_drvPWM_initializationStructChannelsCaract(void)
{
	UINT8 ui8_channelNumber = 0;
	
	// Initialisation de la structure globale des configs PWM
	for(ui8_channelNumber = 0 ; ui8_channelNumber < PWM_OUTPUT_CHANNEL_NUMBER ; ui8_channelNumber++){
		ts_channelsCaract[ui8_channelNumber].en_channel		= PWM_CHANNEL_UNDEFINED;
		ts_channelsCaract[ui8_channelNumber].b_enabled		= false;
		ts_channelsCaract[ui8_channelNumber].ui8_dutyCycle	= 0;
	}
}