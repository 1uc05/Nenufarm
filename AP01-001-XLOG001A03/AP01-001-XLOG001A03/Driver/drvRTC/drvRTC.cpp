/*!
 * \file 	drvRTC.cpp
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
#include "drvRTC.h"

/*!==========================================================================+*/
// DEFINES
/*+==========================================================================+*/

/*!==========================================================================+*/
// VARIABLES GLOBALES
/*+==========================================================================+*/

/*!==========================================================================+*/
// PROTOTYPES
/*+==========================================================================+*/

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
void v_drvRTC_initialization(void)
{
	// Attente que tous les registres soient synchronisés
	while (RTC.STATUS > 0) {}

	// Activation du timer RTC, pas de division de l'horloge
	RTC.CTRLA = RTC_PRESCALER_DIV1_gc | 1 << RTC_RTCEN_bp | 0 << RTC_RUNSTDBY_bp;

	// Choix de la clock externe 32.768kHz (TOSC32K)
	RTC.CLKSEL = RTC_CLKSEL_TOSC32K_gc;

	// Fonctionnement du timer en mode debug
	RTC.DBGCTRL = 1 << RTC_DBGRUN_bp;
	
	// Activation d'une interruption toutes les ms (1.007ms) pour le drvPTC, pas d'interruption sur l'overflow
	//RTC.INTCTRL = 1 << RTC_CMP_bp | 0 << RTC_OVF_bp;
		
	// Attente que tous les registres soient synchronisés
	while (RTC.PITSTATUS > 0) {}
	
	// Activation d'une interruption périodique toute les 32768/32768 = 1sec
	//RTC.PITCTRLA = RTC_PERIOD_CYC1024_gc | 1 << RTC_PITEN_bp;	//Pour debug: temps accéléré x32 (45minutes la journée)
	RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc | 1 << RTC_PITEN_bp; 

	// Fonctionnement de l'interruption en mode debug
	RTC.PITDBGCTRL = 0 << RTC_DBGRUN_bp;

	// Activation de l'interruption périodique
	RTC.PITINTCTRL = 1 << RTC_PI_bp;
}