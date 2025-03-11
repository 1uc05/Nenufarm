/*!
 * \file 	drvHardware.cpp
 * \brief 	
 * \author 	Valentin DOMINIAK
 * \date	10/2023
 * \warning
 * MIT License
 * This software is provided "as is", without warranty of any kind. The authors
 * shall not be liable for any claims, damages, or other liability arising from
 * the use of the software. See the LICENSE file for more details.
 * \version 	0.1 (10/2023)
*/

/*!==========================================================================+*/
// FICHIERS HEADER
/*+==========================================================================+*/
#include "drvHardware.h"

/*!==========================================================================+*/
// DEFINES
/*+==========================================================================+*/
#if CONF_CPU_FREQUENCY <= (MAX_CPU_FREQUENCY/64)
	#define OSC_PRESCALER 64
	#define OSC_PRESCALER_REGISTER_VALUE CLKCTRL_PDIV_64X_gc
#elif CONF_CPU_FREQUENCY <= (MAX_CPU_FREQUENCY/48)
	#define OSC_PRESCALER 48
	#define OSC_PRESCALER_REGISTER_VALUE CLKCTRL_PDIV_48X_gc
#elif CONF_CPU_FREQUENCY <= (MAX_CPU_FREQUENCY/32)
	#define OSC_PRESCALER 32
	#define OSC_PRESCALER_REGISTER_VALUE CLKCTRL_PDIV_32X_gc
#elif CONF_CPU_FREQUENCY <= (MAX_CPU_FREQUENCY/24)
	#define OSC_PRESCALER 24
	#define OSC_PRESCALER_REGISTER_VALUE CLKCTRL_PDIV_24X_gc
#elif CONF_CPU_FREQUENCY <= (MAX_CPU_FREQUENCY/16)
	#define OSC_PRESCALER 16
	#define OSC_PRESCALER_REGISTER_VALUE CLKCTRL_PDIV_16X_gc
#elif CONF_CPU_FREQUENCY <= (MAX_CPU_FREQUENCY/12)
	#define OSC_PRESCALER 12
	#define OSC_PRESCALER_REGISTER_VALUE CLKCTRL_PDIV_12X_gc
#elif CONF_CPU_FREQUENCY <= (MAX_CPU_FREQUENCY/10)
	#define OSC_PRESCALER 10
	#define OSC_PRESCALER_REGISTER_VALUE CLKCTRL_PDIV_10X_gc
#elif CONF_CPU_FREQUENCY <= (MAX_CPU_FREQUENCY/8)
	#define OSC_PRESCALER 8
	#define OSC_PRESCALER_REGISTER_VALUE CLKCTRL_PDIV_8X_gc
#elif CONF_CPU_FREQUENCY <= (MAX_CPU_FREQUENCY/6)
	#define OSC_PRESCALER 6
	#define OSC_PRESCALER_REGISTER_VALUE CLKCTRL_PDIV_6X_gc
#elif CONF_CPU_FREQUENCY <= (MAX_CPU_FREQUENCY/4)
	#define OSC_PRESCALER 4
	#define OSC_PRESCALER_REGISTER_VALUE CLKCTRL_PDIV_4X_gc
#elif CONF_CPU_FREQUENCY <= (MAX_CPU_FREQUENCY/2)
	#define OSC_PRESCALER 2
	#define OSC_PRESCALER_REGISTER_VALUE CLKCTRL_PDIV_2X_gc
#elif CONF_CPU_FREQUENCY <= (MAX_CPU_FREQUENCY)
	#define OSC_PRESCALER 1
#else
	#error "Wrong CPU frequency configuration, set #define CONF_CPU_FREQUENCY to the maximum value of MAX_CPU_FREQUENCY"
#endif

/*!==========================================================================+*/
// VARIABLES GLOBALES
/*+==========================================================================+*/

/*!==========================================================================+*/
// PROTOTYPES
/*+==========================================================================+*/
void v_drvHardware_portsInitialization		(void);
void v_drvHardware_clocksInitialization		(void);

/*!==========================================================================+*/
/*+
 *  \brief      Initialisation de base du µC, ports, horloges, IT, ...
 *  \param[in]  none
 *  \param[out] none
 *  \return     none
 *  \author     Valentin DOMINIAK
 *  \date       Creation: 10/2023
 *  \remarks
 */
/*+==========================================================================+*/
void v_drvHardware_initialization(void)
{
	v_drvHardware_portsInitialization();
	v_drvHardware_clocksInitialization();
	
	// Activation des interruptions globales
	sei();
}

/*!==========================================================================+*/
/*+
 *  \brief     Choix de la direction d'une IO
 *  \param[in] en_port			GPIO_PORTA = PORTA
 *								GPIO_PORTB = PORTB
 *								GPIO_PORTC = PORTC
 *  \param[in] ui8_pin			Numéro de l'IO
 *  \param[in] en_direction		PORT_DIRECION_INPUT		= IO en entrée
 *								PORT_DIRECION_OUTPUT	= IO en sortie
 *								PORT_DIRECION_OFF		= Non utilisé
 *  \param[out] none
 *  \return     none
 *  \author     Valentin DOMINIAK
 *  \date       Creation: 10/2023
 *  \remarks
 */
/*+==========================================================================+*/
void v_drvHardware_setPinDirection(const GPIO_PORT en_port, const UINT8 ui8_pin, const PORT_DIRECTION en_direction)
{
	switch(en_port)
	{
		case GPIO_PORTA :
			switch (en_direction) {
				case PORT_DIRECION_INPUT:
					VPORTA.DIR &= ~(1 << ui8_pin);
					break;
				case PORT_DIRECION_OUTPUT:
					VPORTA.DIR |= (1 << ui8_pin);
					break;
				case PORT_DIRECION_OFF:
					*((UINT8 *)&PORTA + 0x10 + ui8_pin) |= 1 << PORT_PULLUPEN_bp;
					break;
				default:
					break;
			}
			break;
		case GPIO_PORTB :
			switch (en_direction) {
				case PORT_DIRECION_INPUT:
					VPORTB.DIR &= ~(1 << ui8_pin);
					break;
				case PORT_DIRECION_OUTPUT:
					VPORTB.DIR |= (1 << ui8_pin);
					break;
				case PORT_DIRECION_OFF:
					*((UINT8 *)&PORTB + 0x10 + ui8_pin) |= 1 << PORT_PULLUPEN_bp;
					break;
				default:
					break;
			}
			break;
		case GPIO_PORTC :
			switch (en_direction) {
				case PORT_DIRECION_INPUT:
					VPORTC.DIR &= ~(1 << ui8_pin);
					break;
				case PORT_DIRECION_OUTPUT:
					VPORTC.DIR |= (1 << ui8_pin);
					break;
				case PORT_DIRECION_OFF:
					*((UINT8 *)&PORTC + 0x10 + ui8_pin) |= 1 << PORT_PULLUPEN_bp;
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}

/*!==========================================================================+*/
/*+
 *  \brief     Configuration d'une IO
 *  \param[in] en_port			GPIO_PORTA = PORTA
 *								GPIO_PORTB = PORTB
 *								GPIO_PORTC = PORTC
 *  \param[in] ui8_pin			Numéro de l'IO
 *  \param[in] en_configuration	PORT_ISC_INTDISABLE_gc    = Pas d'interruption, le buffer digital d'entrée est activé
 *								PORT_ISC_BOTHEDGES_gc     = Interruption activée sur front montant/descendant
 *								PORT_ISC_RISING_gc        = Interruption activée sur front montant
 *								PORT_ISC_FALLING_gc       = Interruption activée sur front descendant
 *								PORT_ISC_INPUT_DISABLE_gc = Pas d'interruption, pas de buffer digital d'entrée
 *								PORT_ISC_LEVEL_gc         = Interruption activée sur un niveau bas
 *  \param[out] none
 *  \return     none
 *  \author     Valentin DOMINIAK
 *  \date       Creation: 10/2023
 *  \remarks
 */
/*+==========================================================================+*/
void v_drvHardware_setPinConfiguration(const GPIO_PORT en_port, const UINT8 ui8_pin, const PORT_ISC_t en_configuration)
{
	volatile UINT8 *pui8_portPinCtrl;

	switch(en_port)
	{
		case GPIO_PORTA :
			pui8_portPinCtrl = ((UINT8 *)&PORTA + 0x10 + ui8_pin);
			*pui8_portPinCtrl = (*pui8_portPinCtrl & ~PORT_ISC_gm) | en_configuration;
			break;
		case GPIO_PORTB :
			pui8_portPinCtrl = ((UINT8 *)&PORTB + 0x10 + ui8_pin);
			*pui8_portPinCtrl = (*pui8_portPinCtrl & ~PORT_ISC_gm) | en_configuration;
			break;
		case GPIO_PORTC :
			pui8_portPinCtrl = ((UINT8 *)&PORTC + 0x10 + ui8_pin);
			*pui8_portPinCtrl = (*pui8_portPinCtrl & ~PORT_ISC_gm) | en_configuration;
			break;
		default:
			break;
	}
}

/*!==========================================================================+*/
/*+
 *  \brief     Initialisation d'une IO
 *  \param[in] en_port			GPIO_PORTA = PORTA
 *								GPIO_PORTB = PORTB
 *								GPIO_PORTC = PORTC
 *  \param[in] ui8_pin			Numéro de l'IO
 *  \param[in] en_pullMode		PORT_PULL_OFF = Pas de forçage à l'état haut
 *								PORT_PULL_UP  = Activation d'une pull-up
 *  \param[out] none
 *  \return     none
 *  \author     Valentin DOMINIAK
 *  \date       Creation: 10/2023
 *  \remarks
 */
/*+==========================================================================+*/
void v_drvHardware_setPinPullMode(const GPIO_PORT en_port, const UINT8 ui8_pin, const PORT_PULL_MODE en_pullMode)
{
	volatile UINT8 *pui8_portPinCtrl;

	switch(en_port)
	{
		case GPIO_PORTA :
			pui8_portPinCtrl = ((UINT8 *)&PORTA + 0x10 + ui8_pin);

			if (en_pullMode == PORT_PULL_MODE_UP) {
				*pui8_portPinCtrl |= PORT_PULLUPEN_bm;
			} else if (en_pullMode == PORT_PULL_MODE_OFF) {
				*pui8_portPinCtrl &= ~PORT_PULLUPEN_bm;
			}
			break;
		case GPIO_PORTB :
			pui8_portPinCtrl = ((UINT8 *)&PORTB + 0x10 + ui8_pin);

			if (en_pullMode == PORT_PULL_MODE_UP) {
				*pui8_portPinCtrl |= PORT_PULLUPEN_bm;
			} else if (en_pullMode == PORT_PULL_MODE_OFF) {
				*pui8_portPinCtrl &= ~PORT_PULLUPEN_bm;
			}
			break;
		case GPIO_PORTC :
			pui8_portPinCtrl = ((UINT8 *)&PORTC + 0x10 + ui8_pin);

			if (en_pullMode == PORT_PULL_MODE_UP) {
				*pui8_portPinCtrl |= PORT_PULLUPEN_bm;
			} else if (en_pullMode == PORT_PULL_MODE_OFF) {
				*pui8_portPinCtrl &= ~PORT_PULLUPEN_bm;
			}
			break;
		default:
			break;
	}
}

/*!==========================================================================+*/
/*+
 *  \brief     Changement d'état d'une IO
 *  \param[in] en_port			GPIO_PORTA = PORTA
 *								GPIO_PORTB = PORTB
 *								GPIO_PORTC = PORTC
 *  \param[in] ui8_pin			Numéro de l'IO
 *  \param[in] b_value			true	= IO à l'état haut
 *								false	= IO à l'état bas
 *  \param[out] none
 *  \return     none
 *  \author     Valentin DOMINIAK
 *  \date       Creation: 10/2023
 *  \remarks
 */
/*+==========================================================================+*/
void v_drvHardware_setPinValue(const GPIO_PORT en_port, const UINT8 ui8_pin, const BOOL b_value)
{
	switch(en_port)
	{
		case GPIO_PORTA :
			if (b_value == true) {
				VPORTA.OUT |= (1 << ui8_pin);
			} else {
				VPORTA.OUT &= ~(1 << ui8_pin);
			}
			break;
		case GPIO_PORTB :
			if (b_value == true) {
				VPORTB.OUT |= (1 << ui8_pin);
			} else {
				VPORTB.OUT &= ~(1 << ui8_pin);
			}
			break;
		case GPIO_PORTC :
			if (b_value == true) {
				VPORTC.OUT |= (1 << ui8_pin);
			} else {
				VPORTC.OUT &= ~(1 << ui8_pin);
			}
			break;
		default:
			break;
	}
}

/*!==========================================================================+*/
/*+
 *  \brief     Changement d'état d'une IO
 *  \param[in]	en_port			GPIO_PORTA = PORTA
 *								GPIO_PORTB = PORTB
 *								GPIO_PORTC = PORTC
 *  \param[in]	ui8_pin			Numéro de l'IO
 *  \param[out]	none				
 *  \return						true	= IO à l'état haut
 *								false	= IO à l'état bas
 *  \author     Valentin DOMINIAK
 *  \date       Creation: 10/2023
 *  \remarks
 */
/*+==========================================================================+*/
BOOL b_drvHardware_getPinValue(const GPIO_PORT en_port, const UINT8 ui8_pin)
{
	BOOL b_value = false;
	
	switch(en_port)
	{
		case GPIO_PORTA :
			b_value = VPORTA.IN & (1 << ui8_pin);
		break;
		case GPIO_PORTB :
			b_value = VPORTB.IN & (1 << ui8_pin);
		break;
		case GPIO_PORTC :
			b_value = VPORTC.IN & (1 << ui8_pin);
		break;
		default:
		break;
	}
	
	return b_value;
}

/*!==========================================================================+*/
/*+
 *  \brief      Initialisation des ports
 *  \param[in]  none
 *  \param[out] none
 *  \return     none
 *  \author     Valentin DOMINIAK
 *  \date       Creation: 10/2023
 *  \remarks
 */
/*+==========================================================================+*/
void v_drvHardware_portsInitialization(void)
{
	// Pull-up sur les pins non utilisées pour limiter la consommation PORTA
	v_drvHardware_setPinPullMode(GPIO_PORTA, 0, PORT_PULL_MODE_UP);
	v_drvHardware_setPinPullMode(GPIO_PORTA, 1, PORT_PULL_MODE_UP);
	v_drvHardware_setPinPullMode(GPIO_PORTA, 2, PORT_PULL_MODE_UP);
	//v_drvHardware_setPinPullMode(GPIO_PORTA, 3, PORT_PULL_MODE_UP);
	//v_drvHardware_setPinPullMode(GPIO_PORTA, 4, PORT_PULL_MODE_UP);
	//v_drvHardware_setPinPullMode(GPIO_PORTA, 5, PORT_PULL_MODE_UP);
	v_drvHardware_setPinPullMode(GPIO_PORTA, 6, PORT_PULL_MODE_UP);
	v_drvHardware_setPinPullMode(GPIO_PORTA, 7, PORT_PULL_MODE_UP);
	
	// PORTB
	//v_drvHardware_setPinPullMode(GPIO_PORTB, 0, PORT_PULL_MODE_UP);
	//v_drvHardware_setPinPullMode(GPIO_PORTB, 1, PORT_PULL_MODE_UP);
	v_drvHardware_setPinPullMode(GPIO_PORTB, 2, PORT_PULL_MODE_UP);
	v_drvHardware_setPinPullMode(GPIO_PORTB, 3, PORT_PULL_MODE_UP);
	//v_drvHardware_setPinPullMode(GPIO_PORTB, 4, PORT_PULL_MODE_UP);
	v_drvHardware_setPinPullMode(GPIO_PORTB, 5, PORT_PULL_MODE_UP);
	v_drvHardware_setPinPullMode(GPIO_PORTB, 6, PORT_PULL_MODE_UP);
	v_drvHardware_setPinPullMode(GPIO_PORTB, 7, PORT_PULL_MODE_UP);
	
	// PORTC
	//v_drvHardware_setPinPullMode(GPIO_PORTC, 0, PORT_PULL_MODE_UP);
	//v_drvHardware_setPinPullMode(GPIO_PORTC, 1, PORT_PULL_MODE_UP);
	v_drvHardware_setPinPullMode(GPIO_PORTC, 2, PORT_PULL_MODE_UP);
	//v_drvHardware_setPinPullMode(GPIO_PORTC, 3, PORT_PULL_MODE_UP);
	//_drvHardware_setPinPullMode(GPIO_PORTC, 4, PORT_PULL_MODE_UP);
	//v_drvHardware_setPinPullMode(GPIO_PORTC, 5, PORT_PULL_MODE_UP);
	
	
	// RPM pompe PA3 en entrée, pas de pull-up
	v_drvHardware_setPinDirection(GPIO_PORTA, 3, PORT_DIRECION_INPUT);
	v_drvHardware_setPinPullMode(GPIO_PORTA, 3, PORT_PULL_MODE_OFF);
	
	// PWM aquarium PC4 en sortie à l'état bas par défaut
	v_drvHardware_setPinValue(GPIO_PORTC, 4, false);
	v_drvHardware_setPinDirection(GPIO_PORTC, 4, PORT_DIRECION_OUTPUT);

	// PWM plantes PB4 en sortie à l'état bas par défaut
	v_drvHardware_setPinValue(GPIO_PORTB, 4, false);
	v_drvHardware_setPinDirection(GPIO_PORTB, 4, PORT_DIRECION_OUTPUT);
		
	// PWM pompe PC5 en sortie à l'état bas par défaut
	v_drvHardware_setPinValue(GPIO_PORTC, 5, false);
	v_drvHardware_setPinDirection(GPIO_PORTC, 5, PORT_DIRECION_OUTPUT);
	
	// ADC drain aqua PC3, pas d'interruption, pas de buffer digital d'entrée (PORT_ISC_INPUT_DISABLE_gc), pas de pull-up
	v_drvHardware_setPinConfiguration(GPIO_PORTC, 3, PORT_ISC_INPUT_DISABLE_gc);
	v_drvHardware_setPinPullMode(GPIO_PORTC, 3, PORT_PULL_MODE_OFF);
	
	// PTC aquarium PA5, pas d'interruption, pas de buffer digital d'entrée (PORT_ISC_INPUT_DISABLE_gc), pas de pull-up
	v_drvHardware_setPinConfiguration(GPIO_PORTA, 5, PORT_ISC_INPUT_DISABLE_gc);
	v_drvHardware_setPinPullMode(GPIO_PORTA, 5, PORT_PULL_MODE_OFF);
		
	// PTC plantes PC1, pas d'interruption, pas de buffer digital d'entrée (PORT_ISC_INPUT_DISABLE_gc), pas de pull-up
	v_drvHardware_setPinConfiguration(GPIO_PORTC, 1, PORT_ISC_INPUT_DISABLE_gc);
	v_drvHardware_setPinPullMode(GPIO_PORTC, 1, PORT_PULL_MODE_OFF);
	
	// PGOOD 12VDC PA4 en entrée, pas de pull-up, interruption sur front descendant
	v_drvHardware_setPinDirection(GPIO_PORTA, 4, PORT_DIRECION_INPUT);
	v_drvHardware_setPinPullMode(GPIO_PORTA, 4, PORT_PULL_MODE_OFF);
	v_drvHardware_setPinConfiguration(GPIO_PORTA, 4, PORT_ISC_FALLING_gc);
	
	// EN TESTA PC0 en entrée, pas de pull-up
	v_drvHardware_setPinDirection(GPIO_PORTC, 0, PORT_DIRECION_INPUT);
	v_drvHardware_setPinPullMode(GPIO_PORTC, 0, PORT_PULL_MODE_OFF);
	
	// SCL TESTA PB0 à configurer
	v_drvHardware_setPinPullMode(GPIO_PORTB, 0, PORT_PULL_MODE_UP);
	
	// SDA TESTA PB1 à configurer
	v_drvHardware_setPinPullMode(GPIO_PORTB, 1, PORT_PULL_MODE_UP);
	
	// Remapping PWM aquarium PC4 sur TCA0 WO4
	PORTMUX.CTRLC |= PORTMUX_TCA04_bm;
		
	// Remapping PWM plantes PB4 sur TCA0 WO1
	PORTMUX.CTRLC |= PORTMUX_TCA01_bm;

	// Remapping PWM pompe PC5 sur TCA0 WO5
	PORTMUX.CTRLC |= PORTMUX_TCA05_bm;
	
#ifdef DEBUG_MOD_LOG
	// Debug UART RX
	v_drvHardware_setPinDirection(GPIO_PORTA, 2, PORT_DIRECION_INPUT);
	v_drvHardware_setPinPullMode(GPIO_PORTA, 2, PORT_PULL_MODE_OFF);
	PORTMUX.CTRLB |= PORTMUX_USART0_bm;
		
	// Debug UART TX
	v_drvHardware_setPinValue(GPIO_PORTA, 1, false);
	v_drvHardware_setPinDirection(GPIO_PORTA, 1, PORT_DIRECION_OUTPUT);
	PORTMUX.CTRLB |= PORTMUX_USART0_bm;
#endif
}

/*!==========================================================================+*/
/*+
 *  \brief      Initialisation des horloges
 *  \param[in]  none
 *  \param[out] none
 *  \return     none
 *  \author     Valentin DOMINIAK
 *  \date       Creation: 10/2023
 *  \remarks
 */
/*+==========================================================================+*/
void v_drvHardware_clocksInitialization(void)
{
	// Oscillateur externe 32.768kHz configuration 1k cycles de temps d'activation (CSUT = 0), activé (ENABLE = 1), selection du crystal externe (SEL = 0)
	_PROTECTED_WRITE(CLKCTRL.XOSC32KCTRLA, CLKCTRL_CSUT_1K_gc  | 1 << CLKCTRL_ENABLE_bp | 0 << CLKCTRL_RUNSTDBY_bp | 0 << CLKCTRL_SEL_bp);
	 
	// La valeur OSC_PRESCALER_REGISTER_VALUE est placée dans le registre concerné MCLKCTRLB et activation de la division
	#ifdef OSC_PRESCALER_REGISTER_VALUE
		_PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, OSC_PRESCALER_REGISTER_VALUE | 1 << CLKCTRL_PEN_bp);
	#else
		// Par défaut, pas de division de la clock
	#endif
	
	// Oscillateur interne de 20MHz
	_PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_OSC20M_gc);

	// Blocage du registre d'horloge contre toute modification
	_PROTECTED_WRITE(CLKCTRL.MCLKLOCK, 1 << CLKCTRL_LOCKEN_bp);
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
UINT32 ui32_drvHardware_getCPUClockFrequency(void)
{
	return (UINT32)(MAX_CPU_FREQUENCY / OSC_PRESCALER);
}