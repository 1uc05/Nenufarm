/*!
 * \file 	main.cpp
 * \brief 	
 * \author 	NENUFARM
 * \date	01/2024
 * \warning
 * MIT License
 * This software is provided "as is", without warranty of any kind. The authors
 * shall not be liable for any claims, damages, or other liability arising from
 * the use of the software. See the LICENSE file for more details.
 * \version 	0.7 (06/2024)
*/

#include "StartManager.h"
#include "HandlerClient.h"
#include "HandlerProd.h"
#include "drvUSART.h"

/* Variable globale pour utilisation en IT */
volatile static BOOL	b_msRefreshFlag			= false;
volatile static BOOL	b_secRefreshFlag		= false;
volatile static BOOL	b_isRpmRead				= false;
volatile static UINT16	ui16_rpmCounterValue	= 0;
HandlerClient			Ha_handlerClient;
Button					Bu_Button;

void v_runHandlerProd(Memory *pMe_memory, Button *pBu_button, PlantLight *pPl_plantLight, AquaLight *pAl_aquaLight, Pump *pPu_pump);
void v_runHandlerExpo(Memory *pMe_memory, Button *pBu_button, PlantLight *pPl_plantLight, AquaLight *pAl_aquaLight, Pump *pPu_pump);

int main(void)
{
	BOOL			b_isTimeoutChoseSW	= false;
	StartManager	SM_startManager;
	Memory			Me_memory;
	PlantLight		Pl_plantLight;
	AquaLight		Al_aquaLight;
	Pump			Pu_pump;

#ifdef DEBUG_MOD_LOG
	v_drvUSART_initialization();
#endif
	
	/* Initialisation produit */
	SM_startManager.v_StartManager_initialization(&Bu_Button, &Pl_plantLight, &Al_aquaLight, &Pu_pump);

	LOG("Start Main");

	/* Choix du soft */
	while (!b_isTimeoutChoseSW) {
		SM_startManager.v_StartManagerr_everyTime();

		if(b_msRefreshFlag) {
			b_msRefreshFlag = false;
			b_isTimeoutChoseSW = SM_startManager.b_StartManager_isTimeout();
			SM_startManager.v_StartManagerr_everyMilSecond();
		} else {/* Nothing to do - Délais ms pas encore atteint */}
	}

	/* Récupération du choix du soft */
	const SOFT_MODE en_softChosen = SM_startManager.en_startManager_getSoftChoice();
	
	LOG("	Soft choisi : %d", en_softChosen);
	
	/* Lancement de la boucle de soft correpondante */
	switch(en_softChosen) {
		case CLIENT_SOFT:
			Ha_handlerClient.v_Handler_initialization(&Me_memory, &Bu_Button, &Pl_plantLight, &Al_aquaLight, &Pu_pump);
			while(Ha_handlerClient.b_Handler_isBricked());
			break;
			
		case PROD_SOFT:
			v_runHandlerProd(&Me_memory, &Bu_Button, &Pl_plantLight, &Al_aquaLight, &Pu_pump);
			break;
			
		case EXPO_SOFT:
			v_runHandlerExpo(&Me_memory, &Bu_Button, &Pl_plantLight, &Al_aquaLight, &Pu_pump);
			break;
	}
		
	/* Boucle de HandlerClient */
	while (1) {
		
		Ha_handlerClient.v_Handler_everyTime();
		
		if(b_secRefreshFlag) {
			b_secRefreshFlag = false;
			Ha_handlerClient.v_Handler_everySecond();
		} else {/* Nothing to do - En attente d'une sec */}
		
		if(b_msRefreshFlag) {
			b_msRefreshFlag = false;
			Ha_handlerClient.v_Handler_everyMilSecond(ui16_rpmCounterValue, b_isRpmRead);
			b_isRpmRead = false;
		} else {/* Nothing to do - En attente d'une ms */}
	}
}

void v_runHandlerProd(Memory *pMe_memory, Button *pBu_button, PlantLight *pPl_plantLight, AquaLight *pAl_aquaLight, Pump *pPu_pump)
{
	HandlerProd Ha_handlerProd;
	Ha_handlerProd.v_Handler_initialization(pMe_memory, pBu_button, pPl_plantLight, pAl_aquaLight, pPu_pump);
	
	while (1) {
		Ha_handlerProd.v_Handler_everyTime();
		
		if(b_msRefreshFlag) {
			b_msRefreshFlag = false;
			Ha_handlerProd.v_Handler_everyMilSecond(ui16_rpmCounterValue, b_isRpmRead);
			b_isRpmRead = false;
		} else {/* Nothing to do - En attente d'une ms */}
	}
}

void v_runHandlerExpo(Memory *pMe_memory, Button *pBu_button, PlantLight *pPl_plantLight, AquaLight *pAl_aquaLight, Pump *pPu_pump)
{
	//HandlerExpo Ha_handlerExpo;
	//Ha_handlerExpo.v_Handler_initialization(pMe_memory, pBu_button, pPl_plantLight, pAl_aquaLight, pPu_pump);
	//while (1) {
//
		//Ha_handlerExpo.v_Handler_everyTime();
		//
		//if(b_secRefreshFlag) {
			//b_secRefreshFlag = false;
			//Ha_handlerExpo.v_Handler_everySecond();
		//} else {/* Nothing to do - En attente d'une sec */}
		//
		//if(b_msRefreshFlag) {
			//b_msRefreshFlag = false;
			//Ha_handlerExpo.v_Handler_everyMilSecond(ui16_rpmCounterValue, b_isRpmRead);
			//b_isRpmRead = false;
		//} else {/* Nothing to do - En attente d'une ms */}
	//}
}

/* Interruption toutes les milisec */
ISR(TCB0_INT_vect)
{
	/* Flag 1ms passé, remise à 0 du flag d'interruption */
	Bu_Button.v_Button_millisRefreshButton();
	b_msRefreshFlag = true;
	TCB0.INTFLAGS = TCB_CAPT_bm;
}

/* Interruption sur clock externe (non utilisée) */
ISR(RTC_CNT_vect)
{
	/* Remise à 0 du flag d'interruption */
	RTC.INTFLAGS = RTC_CMP_bm;
}

/* Interruption Quartz externe - Déclenche toutes les sec */
ISR(RTC_PIT_vect)
{
	b_secRefreshFlag = true;

	/* Remise à 0 du flag d'interruption */
	RTC.PITINTFLAGS = RTC_PI_bm;
}

/* Interruption sur front montant - Déclenche à chaque lecture frequence frpm = (fclk/8)/ui16_rpmCounterValue) */
ISR(TCB1_INT_vect)
{	
	if(b_isRpmRead == false) {
		if (TCB1.CCMP != 0) {
			ui16_rpmCounterValue = TCB1.CCMP;
			b_isRpmRead = true;
		} else {/* Nothing to do - Premier démarrage ou redemarrage compteur */} 
	} else {/* Nothing to do - L'ancienne valeur lue n'a pas encore été traitée */}

	/* Remise à 0 du flag d'interruption */
	TCB1.INTFLAGS = TCB_CAPT_bm;
}


/* Interruption problème alim */
ISR(BOD_VLM_vect)
{

}

/* Interruption sur le port A */
ISR(PORTA_PORT_vect)
{
	/* S'il ya eu une IT sur le port A et pin 4 */
	if(PORTA.INTFLAGS & PIN4_bm)
	{
		Ha_handlerClient.v_Handler_saveTimeInMemory();
		
		/* Remise à 0 du flag d'interruption */
		PORTA.INTFLAGS &= PIN4_bm;
	} else {/* Nothing to do - Pas de perte de tension */}
}