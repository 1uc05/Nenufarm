/*!
 * \file 	drvRPM.cpp
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
#include "drvRPM.h"

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
void v_drvRPM_initialization(void)
{
	// Mode asynchrone désactivé, pin en entrée sans état initial, mode mesure de fréquence
	TCB1.CTRLB = 0 << TCB_ASYNC_bp | 0 << TCB_CCMPEN_bp | 0 << TCB_CCMPINIT_bp | TCB_CNTMODE_FRQ_gc;
	
	// Fonctionnement en mode debug
	TCB1.DBGCTRL = 1 << TCB_DBGRUN_bp;

	// Activation des évenements (CAPTEI), interruption front positif sur reinitialisation et redémarrage du compteur (EDGE), pas de filtre pour le bruit (FILTER)
	TCB1.EVCTRL = 1 << TCB_CAPTEI_bp| 0 << TCB_EDGE_bp | 0 << TCB_FILTER_bp; 
	
	// Activation de l'IT lorsque le registre de capture est chargé et que le compteur redémarre, flag remis à 0 lors de la lecture du registre de capture CCMP
	TCB1.INTCTRL = 1 << TCB_CAPT_bp;
	
	// Activation d'un évenement asynchrone venant de la pin PA3 (EVSYS_ASYNCCH0_PORTA_PIN3_gc)
	EVSYS.ASYNCCH0 = EVSYS_ASYNCCH0_PORTA_PIN3_gc;
	
	// Assignation de l'évènement TCB1 (ASYNCUSER11) sur le channel asynchrone 0 configuré au dessus (EVSYS_ASYNCUSER11_ASYNCCH0_gc)
	EVSYS.ASYNCUSER11 = EVSYS_ASYNCUSER11_ASYNCCH0_gc;
	
	// Remise à zéro de la fréquence lue
	TCB1.CCMP = 0;
		
	// Utilisation de la clock de TCA de la clock (CLKTCA de 1.25MHz), activation du timer (ENABLE), ne fonctionne pas en standby (RUNSTDBY), pas de redemarrage lorsque le compteur redémarre ou qu'il y a un overflow (SYNCUPD)
	TCB1.CTRLA = TCB_CLKSEL_CLKTCA_gc | 1 << TCB_ENABLE_bp  | 0 << TCB_RUNSTDBY_bp | 0 << TCB_SYNCUPD_bp;
	
	// Remise à 0 du flag d'interruption
	TCB1.INTFLAGS = TCB_CAPT_bm;
}