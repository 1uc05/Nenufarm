/*!
 * \file 		drvRPM.cpp
 * \brief
 * \author      Valentin DOMINIAK
 * \date        01/2024
 * \warning
 * Copyright (C) 2025 NENUFARM - See the file LICENSE for copying permission.
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
	// Mode asynchrone d?sactiv?, pin en entr?e sans ?tat initial, mode mesure de fr?quence
	TCB1.CTRLB = 0 << TCB_ASYNC_bp | 0 << TCB_CCMPEN_bp | 0 << TCB_CCMPINIT_bp | TCB_CNTMODE_FRQ_gc;
	
	// Fonctionnement en mode debug
	TCB1.DBGCTRL = 1 << TCB_DBGRUN_bp;

	// Activation des ?venements (CAPTEI), interruption front positif sur reinitialisation et red?marrage du compteur (EDGE), pas de filtre pour le bruit (FILTER)
	TCB1.EVCTRL = 1 << TCB_CAPTEI_bp| 0 << TCB_EDGE_bp | 0 << TCB_FILTER_bp; 
	
	// Activation de l'IT lorsque le registre de capture est charg? et que le compteur red?marre, flag remis ? 0 lors de la lecture du registre de capture CCMP
	TCB1.INTCTRL = 1 << TCB_CAPT_bp;
	
	// Activation d'un ?venement asynchrone venant de la pin PA3 (EVSYS_ASYNCCH0_PORTA_PIN3_gc)
	EVSYS.ASYNCCH0 = EVSYS_ASYNCCH0_PORTA_PIN3_gc;
	
	// Assignation de l'?v?nement TCB1 (ASYNCUSER11) sur le channel asynchrone 0 configur? au dessus (EVSYS_ASYNCUSER11_ASYNCCH0_gc)
	EVSYS.ASYNCUSER11 = EVSYS_ASYNCUSER11_ASYNCCH0_gc;
	
	// Remise ? z?ro de la fr?quence lue
	TCB1.CCMP = 0;
		
	// Utilisation de la clock de TCA de la clock (CLKTCA de 1.25MHz), activation du timer (ENABLE), ne fonctionne pas en standby (RUNSTDBY), pas de redemarrage lorsque le compteur red?marre ou qu'il y a un overflow (SYNCUPD)
	TCB1.CTRLA = TCB_CLKSEL_CLKTCA_gc | 1 << TCB_ENABLE_bp  | 0 << TCB_RUNSTDBY_bp | 0 << TCB_SYNCUPD_bp;
	
	// Remise ? 0 du flag d'interruption
	TCB1.INTFLAGS = TCB_CAPT_bm;
}

