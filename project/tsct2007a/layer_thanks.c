/**
*       @file
*               layer_thanks.c
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2017.12.05 <br>
*               author: dyhwang <br>
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
#include "tsctcommon.h"
#include "tsctcfg.h"

//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------
#define THANKS_TIMEOUT	20 // sec

//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------
static ITUBackground* sLtBackground;
static ITUSprite* sThanksSprite;
static ITUButton* sokThankButton;

static ITUText* sthanks1Text;
static ITUText* sthanks2Text;
// static ITUText* sthanks3Text;
// static ITUText* sthanks4Text;
		
		
static bool gotoStartLayer;
//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------
static void LayerThanksGotoStartLayer(void)
{	
	if(gotoStartLayer)
	{
		gotoStartLayer = false;
		if( (shmDataAppInfo.app_order == APP_ORDER_FINISH) || (shmDataAppInfo.app_order == APP_ORDER_FINISH))
		{
			GotoStartLayer();		
		}	
	}	
}

bool ThanksOkOnPress(ITUWidget* widget, char* param)
{	
	LayerThanksGotoStartLayer();
	return true;
}
//extern bool sCh1CPMonRun ;
bool ThanksOnEnter(ITUWidget* widget, char* param){

	// ControlPilotSetListener(CstGetActiveCh(), NULL);
	//sCh1CPMonRun = false;
	usleep(500);
	ControlPilotDisablePower(CstGetActiveCh());
	// CstPlayAudioMsg(AUDIO_THANKYOU);
	AudioPlay("A:/sounds/ThnakU.wav", NULL);

	CtLogRed("Enter thanks layer..\n");

	ChannelType activeCh = CstGetUserActiveChannel();	
	shmDataAppInfo.app_order = APP_ORDER_FINISH;
	gotoStartLayer = true;
	// TopGotoThanksStep(); ///mod

	if (!sLtBackground)
    {
        sLtBackground = ituSceneFindWidget(&theScene, "thanksBackground");
        assert(sLtBackground);

		 sThanksSprite = ituSceneFindWidget(&theScene, "thanksSprite");
		 assert(sThanksSprite);

		sokThankButton = ituSceneFindWidget(&theScene, "okThankButton");
		assert(sokThankButton);	
		
		sthanks1Text = ituSceneFindWidget(&theScene, "thanks1Text");
		assert(sthanks1Text);	

		sthanks2Text = ituSceneFindWidget(&theScene, "thanks2Text");
		assert(sthanks2Text);			
	
    }
	
	ituSpritePlay(sThanksSprite, 0);

	TopSetTimer(20, LayerThanksGotoStartLayer);
	usleep(300*1000);
	TopHomeBtnVisible(true);
    return true;
}

bool ThanksOnLeave(ITUWidget* widget, char* param)
{
	CtLogRed("Leave thanks layer\n");
	gotoStartLayer = false;
	TopCloseTimer();
	ituSpriteStop(sThanksSprite);
    return true;
}


void ThanksReset(void)
{
	sLtBackground = NULL;
}


