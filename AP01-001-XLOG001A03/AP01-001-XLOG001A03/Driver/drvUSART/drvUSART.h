/*!
 * \file 	drvUSART.h
 * \brief 	
 * \author 	Valentin DOMINIAK
 * \date 	02/2024
 * \warning
 * MIT License
 * This software is provided "as is", without warranty of any kind. The authors
 * shall not be liable for any claims, damages, or other liability arising from
 * the use of the software. See the LICENSE file for more details.
 * \version 	0.1 (02/2024)
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