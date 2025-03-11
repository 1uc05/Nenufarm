/*!
 * \file 	drvFlash.cpp
 * \brief 	
 * \author 	Valentin DOMINIAK
 * \date	01/2024
 * \warning
 * MIT License
 * This software is provided "as is", without warranty of any kind. The authors
 * shall not be liable for any claims, damages, or other liability arising from
 * the use of the software. See the LICENSE file for more details.
 * \version 	0.1 (01/2024)
*/

/*!==========================================================================+*/
// FICHIERS HEADER
/*+==========================================================================+*/
#include <avr/io.h>
#include <string.h>
#include "drvFlash.h"

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
void v_drvFlash_initialization(void)
{
#ifndef DEBUG_MOD_NO_FLASH
	// Protection en écriture désactivée, boot lock désactivé
	NVMCTRL.CTRLB = 0 << NVMCTRL_APCWP_bp | 0 << NVMCTRL_BOOTLOCK_bp;
	
	// Pas d'interruption pour indiquer que la EEPROM est prête à une séquence de lecture ou écriture
	NVMCTRL.INTCTRL = 0 << NVMCTRL_EEREADY_bp;
#endif
}

/*!==========================================================================+*/
/*+
 *  \brief      Lecture d'un octet dans la EEPROM
 *  \param[in]  ui16_readAdress		Adresse de lecture
 *  \param[out]	none
 *  \return     Octet lu
 *  \author     Valentin DOMINIAK
 *  \date       Creation: 04/2024
 *  \remarks
 */
/*+==========================================================================+*/
UINT8 ui8_drvFlash_readEEPROMByte(UINT16 ui16_readAdress)
{
#ifdef DEBUG_MOD_NO_FLASH
	return 1;
#else
	return *(UINT8 *)(EEPROM_START + ui16_readAdress);
#endif
}

/*!==========================================================================+*/
/*+
 *  \brief      Ecriture d'un octet dans la EEPROM
 *  \param[in]  ui16_writeAdress	Adresse d'écriture
				ui8_data			Donnée à écrire
 *  \param[out] none
 *  \return     none
 *  \author     Valentin DOMINIAK
 *  \date       Creation: 04/2024
 *  \remarks
 */
/*+==========================================================================+*/
void v_drvFlash_writeEEPROMByte(UINT16 ui16_writeAdress, const UINT8 ui8_data)
{
#ifndef DEBUG_MOD_NO_FLASH
	// Attente que la EEPROM ne soit pas occupée
	while (NVMCTRL.STATUS & NVMCTRL_EEBUSY_bm);

	// Effacage de la mémoire tampon de la page
	_PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, NVMCTRL_CMD_PAGEBUFCLR_gc);

	// Ecriture de l'octet dans la mémoire tampon de la page
	*(UINT8 *)(EEPROM_START + ui16_writeAdress) = ui8_data;

	// Suppression de l'ancien octet et écriture du nouveau dans la EEPROM
	_PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, NVMCTRL_CMD_PAGEERASEWRITE_gc);
#endif
}

/*!==========================================================================+*/
/*+
 *  \brief      Lecture d'un block d'octets dans la EEPROM
 *  \param[in]  ui16_readAdress		Adresse de lecture
				ui8_number			Nombre d'octets à lire
 *  \param[out] pui8_data			Buffer des données lues
 *  \return     none
 *  \author     Valentin DOMINIAK
 *  \date       Creation: 04/2024
 *  \remarks
 */
/*+==========================================================================+*/
void v_drvFlash_readEEPROMBlock(UINT16 ui16_readAdress, UINT8 *pui8_data, const UINT8 ui8_number)
{
#ifndef DEBUG_MOD_NO_FLASH
	// Attente que la EEPROM ne soit pas occupée
	while (NVMCTRL.STATUS & NVMCTRL_EEBUSY_bm);
	
	memcpy(pui8_data, (UINT8 *)(EEPROM_START + ui16_readAdress), ui8_number);
#endif
}

/*!==========================================================================+*/
/*+
 *  \brief      Ecriture d'un block d'octets dans la EEPROM
 *  \param[in]  ui16_readAdress		Adresse de lecture
				pui8_data			Buffer des données à écrire
				ui8_number			Nombre d'octets à lire
 *  \param[out] none
 *  \return     none
 *  \author     Valentin DOMINIAK
 *  \date       Creation: 04/2024
 *  \remarks
 */
/*+==========================================================================+*/
void v_drvFlash_writeEEPROMBlock(UINT16 ui16_writeAdress, UINT8 *pui8_data, const UINT8 ui8_number)
{
#ifndef DEBUG_MOD_NO_FLASH
	UINT8 ui8_actualByteNumber		= ui8_number;
	UINT8 *pui8_actualWriteAdress	= (UINT8 *)(EEPROM_START + ui16_writeAdress);

	// Attente que la EEPROM ne soit pas occupée
	while (NVMCTRL.STATUS & NVMCTRL_EEBUSY_bm);
	
	// Effacage de la mémoire tampon de la page
	_PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, NVMCTRL_CMD_PAGEBUFCLR_gc);

	do {
		// Ecriture d'un octet dans le buffer de la page
		*pui8_actualWriteAdress++ = *pui8_data++;
		ui8_actualByteNumber--;
		
		// Si une page a été remplie ou que le dernier octet a été écrit
		if ((((uintptr_t)pui8_actualWriteAdress % EEPROM_PAGE_SIZE) == 0) || (ui8_actualByteNumber == 0)) {
			// Suppression des anciens octets et écriture des nouveaux
			_PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, NVMCTRL_CMD_PAGEERASEWRITE_gc);
		}
	} while (ui8_actualByteNumber != 0);	
#endif
}


/*!==========================================================================+*/
/*+
 *  \brief      Vérification si la EEPROM peut être écrite ou lue
 *  \param[in]  none
 *  \param[out] none
 *  \return     false :	EEPROM indisponible
				true :	EEPROM disponible
 *  \author     Valentin DOMINIAK
 *  \date       Creation: 04/2024
 *  \remarks
 */
/*+==========================================================================+*/
BOOL b_drvFlash_getEEPROMReadyFlag(void)
{
#ifdef DEBUG_MOD_NO_FLASH
	return 0;
#else
	return (NVMCTRL.STATUS & (NVMCTRL_EEBUSY_bm | NVMCTRL_FBUSY_bm));
#endif
}