/*!
 * \file 	drvADC.cpp
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
#include <avr/io.h>
#include <stddef.h>
#include "drvADC.h"

/*!==========================================================================+*/
// DEFINES
/*+==========================================================================+*/
#define MAX_ADC_INPUTS			24
#define ADC_READING_TIMEOUT		1000000

/*!==========================================================================+*/
// VARIABLES GLOBALES
/*+==========================================================================+*/
struct DONNEES_ENTREE_ADC {
	ADC_CHANNEL		en_channel;
	ADC_INSTANCE	en_instance;
	BOOL			b_enabled;
};

static DONNEES_ENTREE_ADC	ts_channelsCaract[MAX_ADC_INPUTS];				// Tableau de structures comprenant les caractéristiques de chaque sortie ADC
static UINT8				ui8_instanceNumber = 0x00;						// Nombre incrémental du numero d'instance du driver ADC

/*!==========================================================================+*/
// PROTOTYPES
/*+==========================================================================+*/
static DRV_ADC_ERROR	en_drvADC_selectChannel						(const UINT8 ui8_instanceNumber);
static DRV_ADC_ERROR	en_drvADC_getADCValue						(const UINT8 ui8_instanceNumber, UINT16 *pui16_value);
static void				v_drvADC_initializationStructChannelsCaract	(void);

/*!==========================================================================+*/
/*+
 *  \brief      
 *  \param[in]  none
 *  \param[out] none
 *  \return     none
 *  \author     Valentin DOMINIAK
 *  \date       Creation: 10/2023
 *  \remarks
 */
/*+==========================================================================+*/
DRV_ADC_ERROR en_drvADC_initialization(const ADC_CHANNEL en_channel, const ADC_INSTANCE en_instance, UINT8 *pui8_instanceNumber)
{
	DRV_ADC_ERROR	en_codeError				= DRV_ADC_ERROR_NO_ERROR;
	UINT8			ui8_instanceNumberIterator	= 0;
	
	if(pui8_instanceNumber != NULL) {

		/* Verification du nombre d'instances ADC par rapport au nombre de sorties physiques */
		if(ui8_instanceNumber < MAX_ADC_INPUTS) {
				
			/* Vérification si le numéro d'instance est correct */
			if(en_instance != ADC_INSTANCE_UNDEFINED) {
	
				/* Vérification si le numéro de channel est correct */
				if(en_channel != ADC_CHANNEL_UNDEFINED) {
		
					/* Premiere initialisation, les configurations générales sont réalisées qu'une seule fois */
					if(ui8_instanceNumber == 0) {
		
						/* Initialisation de la structure liant la configuration souhaitée sur un channel et son numéro d'instance associé */
						v_drvADC_initializationStructChannelsCaract();
		
						/* Division de l'horloge /4 (ADC_PRESC_DIV4_gc), tension de référence Vdd (ADC_REFSEL_VDDREF_gc), impédance d'entrée recommandée pour la référence interne (ADC_SAMPCAP_bp) */
						ADC0.CTRLC |= ADC_PRESC_DIV4_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
						ADC1.CTRLC |= ADC_PRESC_DIV4_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
		
						/* Fonctionnement en mode debug */
						ADC0.DBGCTRL = 1 << ADC_DBGRUN_bp;
						ADC1.DBGCTRL = 1 << ADC_DBGRUN_bp;
		
						/* Selection par défaut de l'entrée 0 */
						ADC0.MUXPOS = ADC_MUXPOS_AIN0_gc;
						ADC1.MUXPOS = ADC_MUXPOS_AIN0_gc;

						/* Activation de l'ADC en mode 10 bits */
						ADC0.CTRLA |= ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc;
						ADC1.CTRLA |= ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc;
					} else {
						/* Ce n'est pas la première fois que la fonction est appelée, vérification si la sortie à configurer n'est pas déjà utilisée */
						for(ui8_instanceNumberIterator = 0 ; ui8_instanceNumberIterator < MAX_ADC_INPUTS ; ui8_instanceNumberIterator++) {
							if (en_channel == ts_channelsCaract[ui8_instanceNumberIterator].en_channel && en_instance == ts_channelsCaract[ui8_instanceNumberIterator].en_instance) {
								en_codeError = DRV_ADC_ERROR_CHANNEL_ALREADY_USED;
							} else {/* Ne rien faire, le channel n'est pas utilisé */}
						}
					}
	
					/* S'il n'y a pas eu d'erreur */
					if(en_codeError == DRV_ADC_ERROR_NO_ERROR) {
				
						/* Sauvegarde de la configuration correspondant au numéro d'instance venant d'être créé */
						ts_channelsCaract[ui8_instanceNumber].en_channel	= en_channel;
						ts_channelsCaract[ui8_instanceNumber].en_instance	= en_instance;
						ts_channelsCaract[ui8_instanceNumber].b_enabled		= true;

						/* La valeur de l'instance du module ADC est retournée par le parametre d'entrée puis incrémenté pour la prochaine entrée à initialiser */
						*pui8_instanceNumber = ui8_instanceNumber;
						ui8_instanceNumber++;
					} else {/* Nothing to do - Erreur code mis à jour */}
				} else {
					/* Erreur, le numéro de channel à configurer n'est pas définit */
					en_codeError = DRV_ADC_ERROR_CHANNEL_UNDEFINED;
				}
			} else {
				/* Erreur, le numéro d'instance à configurer n'est pas définit */
				en_codeError = DRV_ADC_ERROR_INSTANCE_UNDEFINED;
			}
		} else {
			/* Erreur, trop d'instances ADC par rapport au nombre de sorties physiques */
			en_codeError = DRV_ADC_ERROR_TOO_MUCH_INSTANCE_NUMBER;
		}
	} else {
		/* Erreur, pointeur nul */
		en_codeError = DRV_ADC_ERROR_NULL_PTR;
	}
	
	/* Log d'une erreur si erreur */
	if(en_codeError != DRV_ADC_ERROR_NO_ERROR)
	{
		LOG("Erreur en_drvADC_initialization : %d", en_codeError);
	} else {/* Nothing to do - Pas d'erreur */}
		
	return en_codeError;
}

/*!==========================================================================+*/
/*+
 *  \brief      
 *  \param[in]  none
 *  \param[out] none
 *  \return     none
 *  \author     Valentin DOMINIAK
 *  \date       Creation: 10/2023
 *  \remarks
 */
/*+==========================================================================+*/
DRV_ADC_ERROR en_drvADC_getInputValue(const UINT8 ui8_instanceNumber, const UINT8 ui8_averageReading, UINT16 *pui16_value)
{
	DRV_ADC_ERROR	en_codeError			= DRV_ADC_ERROR_NO_ERROR;
	UINT32			ui32_accumulatedValue	= 0;
	UINT8			ui8_readings			= 0;

	if(pui16_value != NULL) {
		
		/* Vérification si la pin est configurée */
		if(ts_channelsCaract[ui8_instanceNumber].b_enabled){
		
			en_codeError = en_drvADC_selectChannel(ui8_instanceNumber);
		
			/* La sélection du channel a bien été faite */
			if(en_codeError == DRV_ADC_ERROR_NO_ERROR) {
			
				/* Récupération de/des valeur(s) sur l'entrée */
				while ((ui8_readings < ui8_averageReading) && (en_codeError == DRV_ADC_ERROR_NO_ERROR)) {
					en_codeError = en_drvADC_getADCValue(ui8_instanceNumber, pui16_value);
					ui32_accumulatedValue += *pui16_value;
					ui8_readings++;
				}
				
				/* Pas d'erreur, calcul de la moyenne */
				if(en_codeError == DRV_ADC_ERROR_NO_ERROR) {
					*pui16_value = (UINT16)(ui32_accumulatedValue / ui8_averageReading);
				} else {/* Nothing to do - Erreur code mis à jour */}
			} else {/* Nothing to do - Erreur code mis à jour */}
		} else {
			/* Erreur, l'entrée ADC n'est pas activée */
			en_codeError = DRV_ADC_ERROR_CHANNEL_NOT_USED;
		}
	} else {
		/* Erreur, pointeur nul */
		en_codeError = DRV_ADC_ERROR_NULL_PTR;
	}
	
	/* Log d'une erreur si erreur */
	if(en_codeError != DRV_ADC_ERROR_NO_ERROR)
	{
		LOG("Erreur en_drvADC_getInputValue : %d", en_codeError);
	} else {/* Nothing to do - Pas d'erreur */}
	
	return en_codeError;
}

/*!==========================================================================+*/
/*+
 *  \brief      
 *  \param[in]  none
 *  \param[out] none
 *  \return     none
 *  \author     Valentin DOMINIAK
 *  \date       Creation: 10/2023
 *  \remarks
 */
/*+==========================================================================+*/
DRV_ADC_ERROR en_drvADC_selectChannel(const UINT8 ui8_instanceNumber)
{	
	DRV_ADC_ERROR		en_codeError		= DRV_ADC_ERROR_NO_ERROR;
	volatile UINT8*		pui8_muxPos			= NULL;
	static const UINT8	ui8_muxPosValues[]	= {
		ADC_MUXPOS_AIN0_gc, ADC_MUXPOS_AIN1_gc, ADC_MUXPOS_AIN2_gc,
		ADC_MUXPOS_AIN3_gc, ADC_MUXPOS_AIN4_gc, ADC_MUXPOS_AIN5_gc,
		ADC_MUXPOS_AIN6_gc, ADC_MUXPOS_AIN7_gc, ADC_MUXPOS_AIN8_gc,
		ADC_MUXPOS_AIN9_gc, ADC_MUXPOS_AIN10_gc, ADC_MUXPOS_AIN11_gc
	};
	
	/* Vérification si l'entrée est configurée */
	if (ts_channelsCaract[ui8_instanceNumber].b_enabled) {
		
		/* Détermination du registre ADC en fonction de l'instance */
		if (ts_channelsCaract[ui8_instanceNumber].en_instance == ADC_INSTANCE_0) {
			pui8_muxPos = &ADC0.MUXPOS;
		} else if (ts_channelsCaract[ui8_instanceNumber].en_instance == ADC_INSTANCE_1) {
			pui8_muxPos = &ADC1.MUXPOS;
		} else {
			en_codeError = DRV_ADC_ERROR_INSTANCE_UNDEFINED;
		}

		/* Configuration du canal ADC si nécessaire */
		if (en_codeError == DRV_ADC_ERROR_NO_ERROR) {
			if (*pui8_muxPos != ui8_muxPosValues[ts_channelsCaract[ui8_instanceNumber].en_channel]) {
				*pui8_muxPos = ui8_muxPosValues[ts_channelsCaract[ui8_instanceNumber].en_channel];
			} else {/* Nothing to do - Le multiplexeur est déjà configuré sur la bonne entrée */}
		} else {/* Nothing to do - Erreur, code mis à jour */}
	} else {
		en_codeError = DRV_ADC_ERROR_CHANNEL_NOT_USED;
	}
	
	return en_codeError;
}

/*!==========================================================================+*/
/*+
 *  \brief      
 *  \param[in]  none
 *  \param[out] none
 *  \return     none
 *  \author     Valentin DOMINIAK
 *  \date       Creation: 10/2023
 *  \remarks
 */
/*+==========================================================================+*/
DRV_ADC_ERROR en_drvADC_getADCValue(const UINT8 ui8_instanceNumber, UINT16 *pui16_value)
{
	DRV_ADC_ERROR	en_codeError	= DRV_ADC_ERROR_NO_ERROR;
	UINT32			ui32_timeout	= 0;

	if(pui16_value != NULL) {

		switch(ts_channelsCaract[ui8_instanceNumber].en_instance){
			case ADC_INSTANCE_0:
				/* Lancement de la conversion sur l'ADC0 */
				ADC0.COMMAND = ADC_STCONV_bm;
		
				/* Tant que la conversion n'est pas terminée */
				while(!(ADC0.INTFLAGS & ADC_RESRDY_bm) && (ui32_timeout < ADC_READING_TIMEOUT)){ui32_timeout++;};
		
				/* Vérification du timeout */
				if (ui32_timeout < ADC_READING_TIMEOUT) {
					/* La conversion est terminée, remise à 0 du flag l'indiquant */
					ADC0.INTFLAGS = ADC_RESRDY_bm;
				
					/* Récupération du résultat de la conversion, RES = 1023*Vin/Vref */
					*pui16_value = ADC0.RES;
				} else {
					en_codeError = DRV_ADC_ERROR_READING_TIMEOUT;
				}
				break;
			case ADC_INSTANCE_1:
				/* Lancement de la conversion sur l'ADC1 */
				ADC1.COMMAND = ADC_STCONV_bm;
					
				/* Tant que la conversion n'est pas terminée */
				while(!(ADC1.INTFLAGS & ADC_RESRDY_bm) && (ui32_timeout < ADC_READING_TIMEOUT)){ui32_timeout++;};
					
				/* Vérification du timeout */
				if (ui32_timeout < ADC_READING_TIMEOUT) {
					/* La conversion est terminée, remise à 0 du flag l'indiquant */
					ADC1.INTFLAGS = ADC_RESRDY_bm;
									
					/* Récupération du résultat de la conversion, RES = 1023*Vin/Vref */
					*pui16_value = ADC1.RES;
				} else {
					en_codeError = DRV_ADC_ERROR_READING_TIMEOUT;	
				}
				break;
			default:
				en_codeError = DRV_ADC_ERROR_INSTANCE_UNDEFINED;
				break;
		}
	} else {
		/* Erreur, pointeur nul */
		en_codeError = DRV_ADC_ERROR_NULL_PTR;
	}
	
	return en_codeError;
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
void v_drvADC_initializationStructChannelsCaract(void)
{
	UINT8 ui8_channelNumber = 0;
	
	/* Initialisation de la structure globale des configs ADC */
	for(ui8_channelNumber = 0 ; ui8_channelNumber < MAX_ADC_INPUTS ; ui8_channelNumber++){
		ts_channelsCaract[ui8_channelNumber].en_channel		= ADC_CHANNEL_UNDEFINED;
		ts_channelsCaract[ui8_channelNumber].en_instance	= ADC_INSTANCE_UNDEFINED;
		ts_channelsCaract[ui8_channelNumber].b_enabled		= false;
	}
}