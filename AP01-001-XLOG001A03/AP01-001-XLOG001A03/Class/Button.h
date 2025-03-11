/*!
 * \file 	Button.h
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

#ifndef __BUTTON_H__
#define __BUTTON_H__

/*!==========================================================================+*/
// FICHIERS HEADER
/*+==========================================================================+*/
#include "definitionTypes.h"

/*!==========================================================================+*/
// DEFINES
/*+==========================================================================+*/
#define AQUARIUM_SENSOR_NODE	0		// Numéro d'instance du bouton de l'aquarium dans le drvPTC
#define PLANT_SENSOR_NODE		1		// Numéro d'instance du bouton des plantes dans le drvPTC

#define	SUSPEND_SENSOR			0
#define RESUME_SENSOR			1

/*!==========================================================================+*/
// STRUCTURES ET ENUMERATIONS
/*+==========================================================================+*/
typedef enum {
	BUTTON_EVENT_NO_EVENT,
	BUTTON_EVENT_AQUA_SIMPLE_PUSH,
	BUTTON_EVENT_AQUA_SHORT_PUSH,
	BUTTON_EVENT_AQUA_LONG_PUSH,
	BUTTON_EVENT_PLANT_SIMPLE_PUSH,
	BUTTON_EVENT_PLANT_SHORT_PUSH,
	BUTTON_EVENT_PLANT_LONG_PUSH,
	BUTTON_EVENT_AQUA_PLANT_SIMPLE_PUSH,
	BUTTON_EVENT_AQUA_PLANT_SHORT_PUSH,
	BUTTON_EVENT_AQUA_PLANT_LONG_PUSH,
} BUTTON_EVENT;

typedef enum {
	KEY_IDLE,
	PLANT_DOUBLE_KEY_TIMEOUT,
	AQUA_DOUBLE_KEY_TIMEOUT,
	PLANT_KEY_PUSHED,
	AQUA_KEY_PUSHED,
	AQUA_PLANT_KEY_PUSHED,
	KEY_RELEASED,
} KEY_STATE;

class Button
{
//variables
public:
protected:
private:
	BOOL		m_b_isInitialized	= false;
	BOOL		m_b_timerIsRunning	= false;
	UINT32		m_ui32_buttonTimer	= 0;
	KEY_STATE	m_en_lastKeyStatus	= KEY_IDLE;
//functions
public:
					Button();
					~Button();
	void			v_Button_initialization();
	void			v_Button_millisRefreshButton();
	BUTTON_EVENT	en_Button_getEvent();
	void			v_Button_setSensorState(UINT8 ui8_sensorNode, BOOL b_sensorState);
protected:
private:
	Button( const Button &c );
	Button& operator=( const Button &c );

}; //Button

#endif //__BUTTON_H__
