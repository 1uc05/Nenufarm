/*!
 * \file 	drvTimerSysteme.cpp
 * \brief 	
 * \author 	Valentin DOMINIAK
 * \date	12/2023
 * \warning
 * MIT License
 * This software is provided "as is", without warranty of any kind. The authors
 * shall not be liable for any claims, damages, or other liability arising from
 * the use of the software. See the LICENSE file for more details.engineered or used in any manner without prior written authorization from
 * NENUFARM.
 * \version 	0.1 (12/2023)
*/

/*!==========================================================================+*/
// FICHIERS HEADER
/*+==========================================================================+*/
#include <avr/io.h>
#include "drvTimerSysteme.h"

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
 *  \date       Creation: 12/2023
 *  \remarks
 */
/*+==========================================================================+*/
void v_drvTimerSysteme_initialization(void)
{
	// Valeur TOP du compteur configuré à 10000 (1 ms)
	TCB0.CCMP = 0x2710;

	// Le compteur démarre à 0
	TCB0.CNT = 0x0;

	// Mode asynchrone désactivé, pin en sortie désactivé, état initial pin désactivé, mode interruption periodique
	TCB0.CTRLB = 0 << TCB_ASYNC_bp | 0 << TCB_CCMPEN_bp | 0 << TCB_CCMPINIT_bp | TCB_CNTMODE_INT_gc;

	// Fonctionnement en mode debug
	TCB0.DBGCTRL = 0 << TCB_DBGRUN_bp;

	// Activation de l'interruption périodique
	TCB0.INTCTRL = 1 << TCB_CAPT_bp;

	// Pas de prescaler, activation du timer et fonctionnement en standby
	TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | 1 << TCB_ENABLE_bp | 1 << TCB_RUNSTDBY_bp | 0 << TCB_SYNCUPD_bp;
}