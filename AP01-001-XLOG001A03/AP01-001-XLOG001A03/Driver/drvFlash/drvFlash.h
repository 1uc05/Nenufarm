/*!
 * \file 	drvFlash.h
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

#ifndef DRVFLASH_H_
#define DRVFLASH_H_

/*!==========================================================================+*/
// FICHIERS HEADER
/*+==========================================================================+*/
#include "definitionTypes.h"

/*!==========================================================================+*/
// DEFINES
/*+==========================================================================+*/

/*!==========================================================================+*/
// STRUCTURES ET ENUMERATIONS
/*+==========================================================================+*/
typedef enum {
	DRV_FLASH_ERROR_NO_ERROR			= 0,
	DRV_FLASH_ERROR_READ_EEPROM_BUSY	= -1,
}DRV_FLASH_ERROR;


/*!==========================================================================+*/
// PROTOTYPES
/*+==========================================================================+*/
void	v_drvFlash_initialization		(void);
UINT8	ui8_drvFlash_readEEPROMByte		(UINT16 ui16_readAdress);
void	v_drvFlash_writeEEPROMByte		(UINT16 ui16_writeAdress, const UINT8 ui8_data);
void	v_drvFlash_readEEPROMBlock		(UINT16 ui16_readAdress, UINT8 *pui8_data, const UINT8 ui8_number);
void	v_drvFlash_writeEEPROMBlock		(UINT16 ui16_writeAdress, UINT8 *pui8_data, const UINT8 ui8_number);
BOOL	b_drvFlash_getEEPROMReadyFlag	(void);

#endif /* DRVFLASH_H_ */