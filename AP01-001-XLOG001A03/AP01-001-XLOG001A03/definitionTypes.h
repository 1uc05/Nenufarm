/*!
 * \file 	definitionTypes.h
 * \brief 	
 * \author 	Valentin DOMINIAK
 * \date 	10/2023
 * \warning
 * MIT License
 * This software is provided "as is", without warranty of any kind. The authors
 * shall not be liable for any claims, damages, or other liability arising from
 * the use of the software. See the LICENSE file for more details.
 * \version 	0.1 (10/2023)
*/

#ifndef DEFINITIONTYPES_H_
#define DEFINITIONTYPES_H_

/*!==========================================================================+*/
// FICHIERS HEADER
/*+==========================================================================+*/
#include <stdbool.h>

/*!==========================================================================+*/
// DEFINES
/*+==========================================================================+*/
typedef bool                BOOL;
typedef unsigned char		BYTE;
typedef unsigned short int	WORD;
typedef unsigned long		DWORD;
typedef unsigned long long	QWORD;
typedef signed char			CHAR;
typedef signed short int	SHORT;
typedef signed long			LONG;
typedef signed long long	LONGLONG;
typedef void                VOID;
typedef char                CHAR8;
typedef unsigned char       UCHAR8;
typedef signed int          INT;
typedef signed char         INT8;
typedef signed short int    INT16;
typedef signed long int     INT32;
typedef signed long long    INT64;
typedef unsigned int        UINT;
typedef unsigned char       UINT8;
typedef unsigned short int  UINT16;
typedef unsigned long int   UINT32;
typedef unsigned long long  UINT64;

#define ENABLE							true
#define DISABLE							false

#define MAX_CPU_FREQUENCY				20000000	/* Fréquence de fonctionnement CPU maximale de l'ATTINY3217 */
#define CONF_CPU_FREQUENCY				10000000	/* Fréquence de fonctionnement CPU sélectionnée */

/****************************************************/
/*					SOFT VERSION					*/
#define SOFT_VERSION					"0.11"
/*													*/
/****************************************************/

/****************************************************/
/*					BAC VERSION						*/
//#define BAC_VERSION_1
#define BAC_VERSION_2
/*													*/
/****************************************************/

/* Fréquence de rotation de la pompe définie pour le niveau bas en fonction de la version du bac */
#ifdef BAC_VERSION_1
	#define LOW_LEVEL_FREQUENCY			56
#elif defined BAC_VERSION_2
	#define LOW_LEVEL_FREQUENCY			70
#endif

#define LOW_LEVEL_FREQUENCY_VALUE	(UINT32)((CONF_CPU_FREQUENCY/8)/LOW_LEVEL_FREQUENCY)	/* RPM de la pompe pour une fréquence de LOW_LEVEL_FREQUENCY */	

/* Variables de debug */
//#define DEBUG_MOD_NO_FLASH			1			/* Désactive l'écriture en flash */
//#define DEBUG_MOD_NO_ADC				1			/* Désactive l'init et la lecture ADC pour AquaLight */
//#define DEBUG_MOD_NO_RPM				1			/* TODO - Désactive l'init et la lecture RPM pour Pump */
//#define DEBUG_MOD_LOG					1			/* Active l'écriture de LOG en UART */
//#define DEBUG_MOD_SHORT_DAY			1			/* Les temps sont divisés par 100 sauf pompe car trop court */
//#define DEBUG_MOD_NO_BRICK			1			/* Le produit ne se brick pas au démarrage */

/*!==========================================================================+*/
// STRUCTURES ET ENUMERATIONS
/*+==========================================================================+*/
typedef enum
{
	NO_EVENT,
	START_PUMP_HIGH_LVL,
	STOP_PUMP_HIGH_LVL,
	START_PLANTLIGHT_CYCLE,
	STOP_PLANTLIGHT_CYCLE,
	START_AQUALIGHT_CYCLE,
	STOP_AQUALIGHT_CYCLE,
	ON_MANUAL_LIGHT_MODE,
	OFF_MANUAL_LIGHT_MODE,
} SCHEDULED_EVENT;

typedef enum {
	H12_LIGHTING,
	H14_LIGHTING,
} PLANTLIGHT_CONFIG_TIME;

typedef enum {
	H0_LIGHTING,
	H6_LIGHTING,
	H8_LIGHTING,
	MANUAL_LIGHTING,
} AQUALIGHT_CONFIG_TIME;

typedef enum {
	LUMEN_20,
	LUMEN_25,
	LUMEN_30,
	LUMEN_35,
} AQUALIGHT_CONFIG_INTENSITY;

struct AQUALIGHT_ADC_VALUE {
	UINT16						ui16_adc20LumValue;
	UINT16						ui16_adc25LumValue;
	UINT16						ui16_adc30LumValue;
	UINT16						ui16_adc35LumValue;
};

struct SETTINGS {
	BOOL						b_hasAlreadyProgrammed;
	UINT32						ui32_currentTime;
	UINT32						ui32_stopTimePlantLigh;
	UINT32						ui32_stopTimeAquaLight;
	UINT32						ui32_startTimeManualAquaLight;
	PLANTLIGHT_CONFIG_TIME		en_timeModePlantLight;
	AQUALIGHT_CONFIG_TIME		en_timeModeAquaLight;
	AQUALIGHT_CONFIG_INTENSITY	en_intensityAquaLight;
	UINT16						ui16_pumpFrequency;
};

struct BLINK_CONFIG {
	UINT8			ui8_nbOfBlink;
	UINT8			ui8_firstPhaseDC;
	UINT8			ui8_secondPhaseDC;
	UINT16			ui16_offsetTime;
	UINT16			ui16_firstPhaseTime;
	UINT16			ui16_secondPhaseTime;
	UINT16			ui16_totalPeriodeTime;
};

#ifdef DEBUG_MOD_LOG
	#include <stdio.h>
	#define LOG(fmt, ...) printf(fmt "\r", ##__VA_ARGS__)
	//#define LOG(fmt, ...) printf("%s - " fmt "\r", __FILE__, ##__VA_ARGS__)
#else
	#define LOG(...){}
#endif

/*!==========================================================================+*/
// AFFICHAGE ERREURS, WARNINGS ET CONFIGURATIONS
/*+==========================================================================+*/
/* Fonctions d'écriture dans la console de compilation */
#define STRINGIFY(x) #x
#define SHOW_WARNING(msg) _Pragma(STRINGIFY(GCC warning msg))
#define SHOW_ERROR(msg) _Pragma(STRINGIFY(GCC error msg))
#define SHOW_MESSAGE(msg) _Pragma(STRINGIFY(message(msg)))

/* Les deux versions ou aucune version du bac n'a été choisie */
#if !defined(BAC_VERSION_1) ^ defined(BAC_VERSION_2)
	SHOW_ERROR ("LES DEUX OU AUCUNE VERSION DE BAC A ETE CHOISIE")
#endif

/* Par sécurité, le compilateur émet un warning si un mode de debug a été sélectionné */
#if defined(DEBUG_MOD_NO_FLASH) || defined(DEBUG_MOD_NO_ADC) || defined(DEBUG_MOD_NO_RPM) || defined(DEBUG_MOD_LOG) || defined(DEBUG_MOD_SHORT_DAY) || defined(DEBUG_MOD_NO_BRICK)
	SHOW_WARNING("UN MODE DE DEBUG A ETE CHOISI")
#endif

/* Affichage de la configuration de la compilation */
#if defined(BAC_VERSION_1)
	SHOW_MESSAGE("DATE : " __DATE__ " / TIME : " __TIME__ " / SOFT : V" SOFT_VERSION " / BAC : indA")
#elif defined(BAC_VERSION_2)
	SHOW_MESSAGE("DATE : " __DATE__ " / TIME : " __TIME__ " / SOFT : V" SOFT_VERSION " / BAC : indB")
#endif

#endif /* DEFINITIONTYPES_H_ */