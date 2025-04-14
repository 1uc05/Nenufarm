/*!
 * \file 		drvUSART.h
 * \brief
 * \author      Valentin DOMINIAK
 * \date        01/2024
 * \warning
 * Copyright (C) 2025 NENUFARM - See the file LICENSE for copying permission.
 */

#ifndef DRVUSART_H_
#define DRVUSART_H_

#ifdef __cplusplus
	extern "C" {
#endif // __cplusplus

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

/*!==========================================================================+*/
// PROTOTYPES
/*+==========================================================================+*/
void v_drvUSART_initialization	(void);
void v_drvUSART_writeChar		(char c_character);
void v_drvUSART_writeString		(const char *pc_str);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif /* DRVUSART_H_ */

