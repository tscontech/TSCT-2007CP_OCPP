#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "scene.h"
#include "ctrlboard.h"
#include "audio_mgr.h"

static ITUBackground* stypeSelBackground;

CHARGE_WAY sel_type = PRICE_TYPE; // 0 : charge type 'price', 1 : charge type 'Electric Energy', 2 : charge type 'Full Charge'

void AuthTypeSelOnPress(ITUWidget* widget, char* param)
{
	int var = atoi(param);	
	sel_type = var;
	
	if(sel_type == FULL_CHARGE) // FULL Charge
	{	
		char *s = 0;
		ChannelType activeCh = CstGetUserActiveChannel();	
		shmDataAppInfo.charge_request_type = 0x01;
		shmDataAppInfo.sPay_type = PRE_PAYMENT;
		ValueOrderFourByte(credit_Pay[0].chargeWatt, 0);
		ValueOrderFourByte(credit_Pay[0].chargePrice, 30000);
		
		ituLayerGoto(ituSceneFindWidget(&theScene, "creditInsertLayer"));
		
	}
	else ituLayerGoto(ituSceneFindWidget(&theScene, "InputAmountLayer"));

}

bool authTypeSelEnter(ITUWidget* widget, char* param)
{	
	// CstPlayAudioMsg(AUDIO_SELECTCHARGETYPE);
	AudioPlay("A:/sounds/selectChargetype.wav", NULL);

	ChannelType activeCh = CstGetUserActiveChannel();
	shmDataAppInfo.app_order = APP_ORDER_CARD_READER;
	
	CtLogRed("authTypeSelEnter..\n");

	if (!stypeSelBackground)
	{
		stypeSelBackground = ituSceneFindWidget(&theScene, "typeSelBackground");
		assert(stypeSelBackground);			
	}	
	
	TopSetTimer(60, GotoStartLayer);
	 return true;
}

bool authTypeSelLeave(ITUWidget* widget, char* param)
{
	AudioStop();

	TopCloseTimer();
	return true;
}

void authTypeSelReset(void)
{
	stypeSelBackground = NULL;
}


