/*!
 * \file 	drvUSART.cpp
 * \brief 	
 * \author 	Valentin DOMINIAK
 * \date	02/2024
 * \warning
 * MIT License
 * This software is provided "as is", without warranty of any kind. The authors
 * shall not be liable for any claims, damages, or other liability arising from
 * the use of the software. See the LICENSE file for more details.
 * \version 	0.1 (02/2024)
*/

/*!==========================================================================+*/
// FICHIERS HEADER
/*+==========================================================================+*/
#include <avr/io.h>
#include "drvUSART.h"

/*!==========================================================================+*/
// DEFINES
/*+==========================================================================+*/
#define USART0_BAUD_RATE(BAUD_RATE) ((float)(10000000 * 64 / (16 * (float)BAUD_RATE)) + 0.5)

/*!==========================================================================+*/
// PROTOTYPES
/*+==========================================================================+*/
#ifdef DEBUG_MOD_LOG
	INT i_drvUSART_write(char character, FILE *stream);
#endif

/*!==========================================================================+*/
// VARIABLES GLOBALES
/*+==========================================================================+*/
#ifdef DEBUG_MOD_LOG
	FILE USART_0_stream = FDEV_SETUP_STREAM(i_drvUSART_write, NULL, _FDEV_SETUP_WRITE);
#endif

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
void v_drvUSART_initialization(void)
{
	// Configuration à 9600 bauds
	USART0.BAUD = (uint16_t)USART0_BAUD_RATE(115200);

	// Activation de RX et TX
	USART0.CTRLB = 0 << USART_MPCM_bp | 0 << USART_ODME_bp | 1 << USART_RXEN_bp | USART_RXMODE_NORMAL_gc | 0 << USART_SFDEN_bp | 1 << USART_TXEN_bp;

	// Fonctionnment en mode debug
	USART0.DBGCTRL = 1 << USART_DBGRUN_bp;

	#ifdef DEBUG_MOD_LOG
		// Définition des sorties texte en UART
		stdout = &USART_0_stream;
	#endif
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
#ifdef DEBUG_MOD_LOG
	INT i_drvUSART_write(char character, FILE *stream)
	{
		while (!(USART0.STATUS & USART_DREIF_bm));
		
		USART0.TXDATAL = character;
		
		return 0;
	}
#endif

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
void v_drvUSART_writeChar(char c_character)
{
	while (!(USART0.STATUS & USART_DREIF_bm));
	
	USART0.TXDATAL = c_character;
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
void v_drvUSART_writeString(const char *pc_str) {
	
	while (!(USART0.STATUS & USART_DREIF_bm));
	
	while (*pc_str) {
		USART0.TXDATAL = *pc_str++;
	}
}