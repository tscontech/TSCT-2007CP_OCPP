/**
*       @file
*               layer_rfidcard_auth.c
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2019.05.20 <br>
*               author: dsAn <br>
*               description: <br>
*/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "SDL/SDL.h"
#include "scene.h"
#include "ctrlboard.h"
#include "cstsamwonfa.h"

#include "ite/itp.h"

//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------
#define CARD_WAIT_TIMEOUT	30

//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------
static ITUBackground* srfidcardBackground;
static ITUSprite* sCardSprite;
static ITUButton* b_nextLayer;


static void GoCardWaitLayer(void)
{
	ituLayerGoto(ituSceneFindWidget(&theScene, "CardWaitLayer"));

}

static void CardReaderListenerOnAuthUser(char *data, int size) // straffic 190322 _dsAn
{
	int i;	
	int var;
	if ( theConfig.ConfirmSelect == USER_AUTH_NET)
	{
		ChannelType activeCh = CstGetUserActiveChannel();
	
		printf("[ch%d]card uid: ", activeCh);
		for (i = 0; i < size; i++)
			printf("%c ", *(data+i));
		printf("card size:[%d] ",size);
		printf("\n");
		if(shmDataAppInfo.app_order == APP_ORDER_CARD_READER)
		{	
			memset(shmDataAppInfo.card_no, '0',sizeof(shmDataAppInfo.card_no));
			memcpy(shmDataAppInfo.card_no , data , size);
			
			GoCardWaitLayer();
		}	
	}

	if(theConfig.ConfirmSelect == USER_AUTH_CARD)
	{		
		ituLayerGoto(ituSceneFindWidget(&theScene, "connectLayer"));		
	}
}

bool RfidCardOnEnter(ITUWidget* widget, char* param)
{
	CtLogRed("Enter RfidCard layer..\n");
	// CstPlayAudioMsg(AUDIO_RFIDCARD);
	AudioPlay("A:/sounds/attachRFcard.wav", NULL);
	ChannelType activeCh = CstGetUserActiveChannel();

	shmDataAppInfo.app_order = APP_ORDER_CARD_READER;
	shmDataAppInfo.charge_request_type = 0x01;	

    if (!srfidcardBackground)
    {
        srfidcardBackground = ituSceneFindWidget(&theScene, "rfidcardBackground");
        assert(srfidcardBackground);

        sCardSprite = ituSceneFindWidget(&theScene, "cardSprite");
        assert(sCardSprite);

		///mod
		
		b_nextLayer = ituSceneFindWidget(&theScene, "b_nextLayer");
		assert(b_nextLayer);	
		ituWidgetSetVisible(b_nextLayer, false);
			
    }
	if(theConfig.ConfirmSelect == USER_AUTH_NONE)	ituWidgetSetVisible(b_nextLayer, true);
	
	ituSpritePlay(sCardSprite, CH1);

	CardReaderStartMonitoring(CardReaderListenerOnAuthUser);
	
	TopSetTimer(CARD_WAIT_TIMEOUT, GotoStartLayer);
	
    return true;
}


bool RfidCardOnLeave(ITUWidget* widget, char* param)
{
	CtLogRed("RfidCard Leave \n");
	ituSpriteStop(sCardSprite);
	TopCloseTimer();

	CardReaderStopMonitoring();
	
	TopStopStepAnimation();

    return true;
}

void RfidCardReset(void)
{
	srfidcardBackground = NULL;
}

bool B_nextLayer(ITUWidget* widget, char* param)
{
	ituLayerGoto(ituSceneFindWidget(&theScene, "connectLayer"));	
	return true;
}
