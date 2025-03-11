/* 
* Memory.cpp
*
* Created: 23/01/2024 17:13:33
* Author: LucasTeissier
*/

#include "Memory.h"
#include "drvFlash.h"

/* Adresses flash ecrasable */
#define HAS_ALREADY_STARTED_ADDR			0x00										/* 0  - 0x00 */
#define CURRENT_TIME_ADDR					HAS_ALREADY_STARTED_ADDR			+ 0x04	/* 4  - 0x04 */
#define STOP_TIME_PLANTLIGHT_ADDR			CURRENT_TIME_ADDR					+ 0x04	/* 8  - 0x08 */
#define STOP_TIME_AQUALIGHT_ADDR			STOP_TIME_PLANTLIGHT_ADDR			+ 0x04	/* 12 - 0x0C */
#define TIME_MODE_PLANTLIGHT_ADDR			STOP_TIME_AQUALIGHT_ADDR			+ 0x04	/* 16 - 0x10 */
#define TIME_MODE_AQUALIGHT_ADDR			TIME_MODE_PLANTLIGHT_ADDR			+ 0x04	/* 20 - 0x14 */
#define INTENSITY_AQUALIGHT_ADDR			TIME_MODE_AQUALIGHT_ADDR			+ 0x04	/* 24 - 0x18 */
#define START_TIME_MANUAL_AQUALIGHT_ADDR	INTENSITY_AQUALIGHT_ADDR			+ 0x04	/* 28 - 0x1C */
#define ADC_20LUM_VALUE_ADDR				START_TIME_MANUAL_AQUALIGHT_ADDR	+ 0x04	/* 32 - 0x20 */
#define ADC_25LUM_VALUE_ADDR				ADC_20LUM_VALUE_ADDR				+ 0x04	/* 36 - 0x24 */
#define ADC_30LUM_VALUE_ADDR				ADC_25LUM_VALUE_ADDR				+ 0x04	/* 40 - 0x28 */
#define ADC_35LUM_VALUE_ADDR				ADC_30LUM_VALUE_ADDR				+ 0x04	/* 44 - 0x2C */

#define CRITICAL_ERROR_ADDR					ADC_35LUM_VALUE_ADDR				+ 0x04	/* 48 - 0x30 */
#define LAST_VALUE_NO_PROTECTED_ADDR		CRITICAL_ERROR_ADDR					+ 0x04	/* 52 - 0x34 */

/* Adresses flash non ecrasé */
#define PUMP_FREQUENCY_ADDR					LAST_VALUE_NO_PROTECTED_ADDR		+ 0x04	/* 56 - 0x38 */
#define HAS_ALREADY_CONFIG_PUMP_ADDR		PUMP_FREQUENCY_ADDR					+ 0x04	/* 60 - 0x3C */
#define ADC_BRICK_MODE						HAS_ALREADY_CONFIG_PUMP_ADDR		+ 0x04	/* 64 - 0x40 */

/* Réglages temps par défaut, en s */
#ifndef DEBUG_MOD_SHORT_DAY
#define DEFAULT_HOUR_START_PLANTLIGHT	0				/* S'allume à N			0				*/
#define DEFAULT_HOUR_STOP_PLANTLIGHT	50400			/* S'eteind à N+14h		60*60*12		*/
#define DEFAULT_HOUR_START_AQUALIGHT	0				/* S'allume à N			0				*/
#define DEFAULT_HOUR_STOP_AQUALIGHT		21600			/* S'eteind à N+6h		60*60*6			*/
#define DEFAULT_HOUR_START_PUMP			72000			/* S'allume à N+19h		60*60*19		*/
#define DEFAULT_HOUR_STOP_PUMP			72300			/* S'eteind à N+19h05	60*60*19+5*60	*/
#else
#define	DEFAULT_HOUR_START_PLANTLIGHT	0				/* S'allume à N			0				*/
#define	DEFAULT_HOUR_STOP_PLANTLIGHT	504				/* S'eteind à N+14h		60*60*12		*/
#define	DEFAULT_HOUR_START_AQUALIGHT	0				/* S'allume à N			0				*/
#define	DEFAULT_HOUR_STOP_AQUALIGHT		216				/* S'eteind à N+6h		60*60*6			*/
#define	DEFAULT_HOUR_START_PUMP			200				/* S'allume à N+19h		60*60*20		*/
#define	DEFAULT_HOUR_STOP_PUMP			350				/* S'eteind à N+19h05	60*60*20+5*60	*/
#endif

/* Valeurs fixes inscrites en flash */
#define IS_ALREADY_STARTED_VALUE			0x071220
#define IS_ALREADY_CONFIG_PUMP_VALUE		0x012345
#define FIRST_CRITICAL_ERROR_VALUE			0x69		/* Valeur à inscrire si 1ere erreur critique - testé lorsqu'une erreur critique se déclanche */
#define BRICK_CRITICAL_ERROR_VALUE			0x96		/* Valeur à inscrire si une erreur critique est déja arrivée - testé au démarrage */
#define BRICK_MODE_DISABLE					0x31		/* Valeur si le mode brickage est désactivé - testé au démarrage */

/* General */
#define FOUR_BYTE							4
#define ZERO								0
#define MIN_SPEED_PUMP_FREQ			(UINT16)(80000)		/* DC à 15 */

typedef union {
    UINT32  ui32_value;
    UINT8   tui8_arrayValue[4];
} FLASH_VALUE;

// default constructor
Memory::Memory()
{
	v_drvFlash_initialization();
} //Memory

// default destructor
Memory::~Memory()
{
} //~Memory

const BOOL Memory::b_Memory_isBricked() const
{
	FLASH_VALUE u_isBricked;

	v_drvFlash_readEEPROMBlock(CRITICAL_ERROR_ADDR, u_isBricked.tui8_arrayValue, FOUR_BYTE);

	return (u_isBricked.ui32_value == BRICK_CRITICAL_ERROR_VALUE) ? true : false;
}

void Memory::v_Memory_loadSettings(SETTINGS *st_settings)
{
	FLASH_VALUE u_isSettingSaved;
	FLASH_VALUE u_currentTime;
	FLASH_VALUE u_stopTimePlantLight;
	FLASH_VALUE u_stopTimeAquaLight;
	FLASH_VALUE u_timeModePlantLight;
	FLASH_VALUE u_timeModeAquaLight;
	FLASH_VALUE u_intensityAquaLight;
	FLASH_VALUE u_startTimeManualAquaLight;
	FLASH_VALUE u_pumpfrequency;

	v_drvFlash_readEEPROMBlock(HAS_ALREADY_STARTED_ADDR, u_isSettingSaved.tui8_arrayValue, FOUR_BYTE);

	/* Le soft a déja été lancé, on récupère les paramètres enregistrés */
	if(u_isSettingSaved.ui32_value == IS_ALREADY_STARTED_VALUE) {
		st_settings->b_hasAlreadyProgrammed = true;
		
		LOG("Recuperation des reglages en Flash");
		v_drvFlash_readEEPROMBlock(CURRENT_TIME_ADDR, u_currentTime.tui8_arrayValue, FOUR_BYTE);
		v_drvFlash_readEEPROMBlock(STOP_TIME_PLANTLIGHT_ADDR, u_stopTimePlantLight.tui8_arrayValue, FOUR_BYTE);
		v_drvFlash_readEEPROMBlock(STOP_TIME_AQUALIGHT_ADDR, u_stopTimeAquaLight.tui8_arrayValue, FOUR_BYTE);
		v_drvFlash_readEEPROMBlock(TIME_MODE_PLANTLIGHT_ADDR, u_timeModePlantLight.tui8_arrayValue, FOUR_BYTE);
		v_drvFlash_readEEPROMBlock(TIME_MODE_AQUALIGHT_ADDR, u_timeModeAquaLight.tui8_arrayValue, FOUR_BYTE);
		v_drvFlash_readEEPROMBlock(INTENSITY_AQUALIGHT_ADDR, u_intensityAquaLight.tui8_arrayValue, FOUR_BYTE);
		v_drvFlash_readEEPROMBlock(START_TIME_MANUAL_AQUALIGHT_ADDR, u_startTimeManualAquaLight.tui8_arrayValue, FOUR_BYTE);
		v_drvFlash_readEEPROMBlock(PUMP_FREQUENCY_ADDR, u_pumpfrequency.tui8_arrayValue, FOUR_BYTE);
		
		/* Sauvegarde des valeurs en attribut pour faire une comparaison avant une écriture Flash d'un changement de setting */
		m_ui32_currentTime			= u_currentTime.ui32_value;
		m_ui32_stopTimePlantLight	= u_stopTimePlantLight.ui32_value;
		m_ui32_stopTimeAquaLight	= u_stopTimeAquaLight.ui32_value;
		m_en_timeModePlantLight		= (PLANTLIGHT_CONFIG_TIME)(u_timeModePlantLight.ui32_value);
		m_en_timeModeAquaLight		= (AQUALIGHT_CONFIG_TIME)(u_timeModeAquaLight.ui32_value);
		m_en_intensityAquaLight		= (AQUALIGHT_CONFIG_INTENSITY)(u_intensityAquaLight.ui32_value);
	} else {		
	/* Le soft n'a jamais été lancé, réglage par défaut */
		st_settings->b_hasAlreadyProgrammed = false;
		
		LOG("1er lancement du soft, reglage par défaut et enregistrement en Flash");
		m_ui32_currentTime			= 0;
		m_ui32_stopTimePlantLight	= DEFAULT_HOUR_STOP_PLANTLIGHT;
		m_ui32_stopTimeAquaLight	= DEFAULT_HOUR_STOP_AQUALIGHT;
		m_en_timeModePlantLight		= H14_LIGHTING;
		m_en_timeModeAquaLight		= H6_LIGHTING;
		m_en_intensityAquaLight		= LUMEN_25;
		/* Variable non gardé en attribut car pas de controle necessaire à l'écriture */
		u_startTimeManualAquaLight.ui32_value = 0;
		u_pumpfrequency.ui32_value	= ui32_Memory_getPumpFrequency();
		
		/* Sauvegarde en mémoire des réglages par défaut en cas d'un reboot sans passer par une sauvegarde (sans passer par l'IT) */
		v_Memory_svgFourByte(CURRENT_TIME_ADDR, 0);
		v_Memory_svgFourByte(STOP_TIME_PLANTLIGHT_ADDR, m_ui32_stopTimePlantLight);
		v_Memory_svgFourByte(STOP_TIME_AQUALIGHT_ADDR, m_ui32_stopTimeAquaLight);
		v_Memory_svgFourByte(TIME_MODE_PLANTLIGHT_ADDR, m_en_timeModePlantLight);
		v_Memory_svgFourByte(TIME_MODE_AQUALIGHT_ADDR, m_en_timeModeAquaLight);
		v_Memory_svgFourByte(INTENSITY_AQUALIGHT_ADDR, m_en_intensityAquaLight);
		v_Memory_svgFourByte(START_TIME_MANUAL_AQUALIGHT_ADDR, 0);
		v_Memory_svgFourByte(PUMP_FREQUENCY_ADDR, u_pumpfrequency.ui32_value);
		v_Memory_svgFourByte(HAS_ALREADY_CONFIG_PUMP_ADDR, IS_ALREADY_CONFIG_PUMP_VALUE);

		/* Marque en mémoire que le soft a déjà été lancé. Au prochain démarrage, les settings seront récup en Flash */
		//v_Memory_markAsAlreadyStarted(); Déplacé dans le Handler, après la config des valeurs d'ADC
	}
	
	st_settings->ui32_currentTime = m_ui32_currentTime;
	st_settings->ui32_stopTimePlantLigh = m_ui32_stopTimePlantLight;
	st_settings->ui32_stopTimeAquaLight = m_ui32_stopTimeAquaLight;
	st_settings->en_timeModePlantLight = m_en_timeModePlantLight;
	st_settings->en_timeModeAquaLight = m_en_timeModeAquaLight;
	st_settings->en_intensityAquaLight = m_en_intensityAquaLight;
	st_settings->ui32_startTimeManualAquaLight = u_startTimeManualAquaLight.ui32_value;
	st_settings->ui16_pumpFrequency = (UINT16)(u_pumpfrequency.ui32_value);
}

void Memory::v_Memory_saveCurrentTime(const UINT32 ui32_currentTime) const
{
	v_Memory_svgFourByte(CURRENT_TIME_ADDR, ui32_currentTime);
}

void Memory::v_Memory_saveStopTimePlantLight(const UINT32 ui32_stopTimePlantLight)
{
	if(ui32_stopTimePlantLight != m_ui32_stopTimePlantLight) {
		m_ui32_stopTimePlantLight = ui32_stopTimePlantLight;
		v_Memory_svgFourByte(STOP_TIME_PLANTLIGHT_ADDR, ui32_stopTimePlantLight);
	} else {/* Nothing to do - La valeur n'a pas changé */}
}

void Memory::v_Memory_saveStopTimeAquaLight(const UINT32 ui32_stopTimeAquaLight)
{
	if(ui32_stopTimeAquaLight != m_ui32_stopTimeAquaLight) {
		m_ui32_stopTimeAquaLight = ui32_stopTimeAquaLight;
		v_Memory_svgFourByte(STOP_TIME_AQUALIGHT_ADDR, ui32_stopTimeAquaLight);
	} else {/* Nothing to do - La valeur n'a pas changé */}
}

void Memory::v_Memory_saveTimeModePlantLight(const PLANTLIGHT_CONFIG_TIME en_timeModePlantLight)
{
	if(en_timeModePlantLight != m_en_timeModePlantLight) {
		m_en_timeModePlantLight = en_timeModePlantLight;
		v_Memory_svgFourByte(TIME_MODE_PLANTLIGHT_ADDR, (UINT32)(en_timeModePlantLight));
	} else {/* Nothing to do - La valeur n'a pas changé */}
}

void Memory::v_Memory_saveTimeModeAquaLight(const AQUALIGHT_CONFIG_TIME en_timeModeAquaLight)
{
	if(en_timeModeAquaLight != m_en_timeModeAquaLight) {
		m_en_timeModeAquaLight = en_timeModeAquaLight;
		v_Memory_svgFourByte(TIME_MODE_AQUALIGHT_ADDR, (UINT32)(en_timeModeAquaLight));
	} else {/* Nothing to do - La valeur n'a pas changé */}
}

void Memory::v_Memory_saveIntensityAquaLight(const AQUALIGHT_CONFIG_INTENSITY en_intensityAquaLight)
{
	if(en_intensityAquaLight != m_en_intensityAquaLight) {
		m_en_intensityAquaLight = en_intensityAquaLight;
		v_Memory_svgFourByte(INTENSITY_AQUALIGHT_ADDR, (UINT32)(en_intensityAquaLight));
	} else {/* Nothing to do - La valeur n'a pas changé */}
}

void Memory::v_Memory_saveManualModeAquaLight(const UINT32 ui32_startTime, const UINT32 ui32_stopTime)
{
	v_Memory_svgFourByte(START_TIME_MANUAL_AQUALIGHT_ADDR, (UINT32)(ui32_startTime));
	v_Memory_svgFourByte(STOP_TIME_AQUALIGHT_ADDR, (UINT32)(ui32_stopTime));
}

void Memory::v_Memory_clearMemory() const
{
	LOG("Suppression parametre memoire non protege");
	for(UINT8 ui8_itAddrMemory(0); ui8_itAddrMemory < LAST_VALUE_NO_PROTECTED_ADDR; ++ui8_itAddrMemory) {
		v_drvFlash_writeEEPROMByte(ui8_itAddrMemory, ZERO);
	}
	
	while(b_drvFlash_getEEPROMReadyFlag());
}

void Memory::v_Memory_markAsAlreadyStarted() const
{
	FLASH_VALUE u_value;
	
	u_value.ui32_value = IS_ALREADY_STARTED_VALUE;
	
	v_drvFlash_writeEEPROMBlock(HAS_ALREADY_STARTED_ADDR, u_value.tui8_arrayValue, FOUR_BYTE);
	
	while(b_drvFlash_getEEPROMReadyFlag());
}

BOOL Memory::b_Memory_isBrickedModeDisable() const
{
	return (ui8_drvFlash_readEEPROMByte(ADC_BRICK_MODE) == BRICK_MODE_DISABLE)
		? true : false;
}

void Memory::v_Memory_enableBrickMode() const
{
	v_Memory_svgFourByte(ADC_BRICK_MODE, 0);
}

void Memory::v_Memory_disableBrickMode() const
{
	v_Memory_svgFourByte(ADC_BRICK_MODE, BRICK_MODE_DISABLE);
}

const BOOL Memory::b_Memory_isCriticalErrorRecording() const
{
	return (ui8_drvFlash_readEEPROMByte(CRITICAL_ERROR_ADDR) == FIRST_CRITICAL_ERROR_VALUE)
		? true : false;
}

void Memory::v_Memory_writeFirstCriticalError() const
{
	v_Memory_svgFourByte(CRITICAL_ERROR_ADDR, FIRST_CRITICAL_ERROR_VALUE);
}

void Memory::v_Memory_writeBrickCriticalError() const
{
	v_Memory_svgFourByte(CRITICAL_ERROR_ADDR, BRICK_CRITICAL_ERROR_VALUE);
}

void Memory::v_Memory_clearFirstCriticalError() const
{
	v_Memory_svgFourByte(CRITICAL_ERROR_ADDR, 0);
}

UINT32 Memory::ui32_Memory_getPumpFrequency() const
{
	FLASH_VALUE u_pumpFrequency;
	FLASH_VALUE u_isAlreadyConfPumpValue;

	v_drvFlash_readEEPROMBlock(HAS_ALREADY_CONFIG_PUMP_ADDR, u_isAlreadyConfPumpValue.tui8_arrayValue, FOUR_BYTE);

	if(u_isAlreadyConfPumpValue.ui32_value == IS_ALREADY_CONFIG_PUMP_VALUE) {
		v_drvFlash_readEEPROMBlock(PUMP_FREQUENCY_ADDR, u_pumpFrequency.tui8_arrayValue, FOUR_BYTE);
	} else {
		u_pumpFrequency.ui32_value = LOW_LEVEL_FREQUENCY_VALUE;
	}

	return u_pumpFrequency.ui32_value;
}

void Memory::v_Memory_svgFourByte(const UINT8 ui8_adress, const UINT32 ui32_valueToSave) const
{
	FLASH_VALUE u_value;

	u_value.ui32_value = ui32_valueToSave;
	
	LOG("Ecriture flash - Addr %d / Value : %lu", ui8_adress, ui32_valueToSave);
	v_drvFlash_writeEEPROMBlock(ui8_adress, u_value.tui8_arrayValue, FOUR_BYTE);
	
	while(b_drvFlash_getEEPROMReadyFlag());
}

void Memory::v_Memory_saveSpeedPumpFreq(const UINT16 ui16_pumpFrequency) const
{
	if(ui16_pumpFrequency > MIN_SPEED_PUMP_FREQ) {
		/* Sauvegarde de la fréquence */
		v_Memory_svgFourByte(PUMP_FREQUENCY_ADDR, ui16_pumpFrequency);
		
		/* Marque en Flash que la fréqeunce a été configurée */
		v_Memory_svgFourByte(HAS_ALREADY_CONFIG_PUMP_ADDR, IS_ALREADY_CONFIG_PUMP_VALUE);
	} else {
		LOG("CodeError : Fréquence de la pompe trop faible, pas de svg Flash - Freq : %u", ui16_pumpFrequency);
	}
}

void Memory::v_Memory_clearFrequency() const
{
	v_Memory_svgFourByte(HAS_ALREADY_CONFIG_PUMP_ADDR, 0);
	v_Memory_svgFourByte(PUMP_FREQUENCY_ADDR, LOW_LEVEL_FREQUENCY_VALUE);
}

void Memory::v_Memory_getADCValues(AQUALIGHT_ADC_VALUE * pst_adcValues) const
{
	FLASH_VALUE u_adcLumValue = {0};

	v_drvFlash_readEEPROMBlock(ADC_20LUM_VALUE_ADDR, u_adcLumValue.tui8_arrayValue, FOUR_BYTE);
	pst_adcValues->ui16_adc20LumValue = (UINT16)(u_adcLumValue.ui32_value);
	
	v_drvFlash_readEEPROMBlock(ADC_25LUM_VALUE_ADDR, u_adcLumValue.tui8_arrayValue, FOUR_BYTE);
	pst_adcValues->ui16_adc25LumValue = (UINT16)(u_adcLumValue.ui32_value);
	
	v_drvFlash_readEEPROMBlock(ADC_30LUM_VALUE_ADDR, u_adcLumValue.tui8_arrayValue, FOUR_BYTE);
	pst_adcValues->ui16_adc30LumValue = (UINT16)(u_adcLumValue.ui32_value);
	
	v_drvFlash_readEEPROMBlock(ADC_35LUM_VALUE_ADDR, u_adcLumValue.tui8_arrayValue, FOUR_BYTE);
	pst_adcValues->ui16_adc35LumValue = (UINT16)(u_adcLumValue.ui32_value);
}

void Memory::v_Memory_saveADCValues(const AQUALIGHT_ADC_VALUE st_adcValues) const
{
	v_Memory_svgFourByte(ADC_20LUM_VALUE_ADDR, st_adcValues.ui16_adc20LumValue);
	v_Memory_svgFourByte(ADC_25LUM_VALUE_ADDR, st_adcValues.ui16_adc25LumValue);
	v_Memory_svgFourByte(ADC_30LUM_VALUE_ADDR, st_adcValues.ui16_adc30LumValue);
	v_Memory_svgFourByte(ADC_35LUM_VALUE_ADDR, st_adcValues.ui16_adc35LumValue);
}
