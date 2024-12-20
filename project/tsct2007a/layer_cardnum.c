/**
*       @file
*               layer_cardnum.c
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2017.12.08 <br>
*               author: dyhwang <br>
*               description: <br>
*/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "SDL/SDL.h"
#include "lwip/ip.h"
#include "ite/itp.h"
#include "scene.h"
#include "ctrlboard.h"
#include "tsctcommon.h"

//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------
static ITUBackground* sLcnBackground;
static ITUBackground* sNumKeypadBackground;

static ITUTextBox* sCardNum1TextBox;
static ITUTextBox* sCardNum2TextBox;
static ITUTextBox* sCardNum3TextBox;
static ITUTextBox* sCardNum4TextBox;

static bool GotoCardNumStart;

//static ITUButton* sokCardNumButton;

/*
static char sCardNum1[4];
static char sCardNum2[4];
static char sCardNum3[4];
static char sCardNum4[4];
*/
static char sCardNum[16];

static int sNumCount = 0;

//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------
static void CardWaitTimeout111(void)
{
	CtLogRed(" CardWaitTimeout111 =====");	
	CardNumStartLayer();	
} 

void CardNumStartLayer(void)
{
	if(GotoCardNumStart)
	{
		GotoCardNumStart = false;
		GotoStartLayer();	
	}
}

static void ResetNumber()
{
	int i;

	for (i = 0; i < 16; i++)
		sCardNum[i] = ' ';
	sNumCount = 0;
	CtLogRed("Check sCardNum");
	//ituTextSetString
	
	ituTextBoxSetString(sCardNum1TextBox, "");
	CtLogRed("Check sCardNum1TextBox");
	usleep(100*1000);
	ituTextBoxSetString(sCardNum2TextBox, "");
	CtLogRed("Check sCardNum2TextBox");
	usleep(100*1000);
	ituTextBoxSetString(sCardNum3TextBox, "");
	CtLogRed("Check sCardNum3TextBox");
	usleep(100*1000);
	ituTextBoxSetString(sCardNum4TextBox, "");
	CtLogRed("Check sCardNum4TextBox");
	usleep(100*1000);	
}

static void UpdateNumber()
{
	char buf[5];
	
	sprintf(buf, "%c%c%c%c", sCardNum[0], sCardNum[1], sCardNum[2], sCardNum[3]);
	ituTextBoxSetString(sCardNum1TextBox, buf);
	
	sprintf(buf, "%c%c%c%c", sCardNum[4], sCardNum[5], sCardNum[6], sCardNum[7]);
	ituTextBoxSetString(sCardNum2TextBox, buf);

	sprintf(buf, "%c%c%c%c", sCardNum[8], sCardNum[9], sCardNum[10], sCardNum[11]);
	ituTextBoxSetString(sCardNum3TextBox, buf);

	sprintf(buf, "%c%c%c%c", sCardNum[12], sCardNum[13], sCardNum[14], sCardNum[15]);
	ituTextBoxSetString(sCardNum4TextBox, buf);
}

static void NumInputListener(char c)
{
	int i = 0;	

	TopSetTimer(60, CardWaitTimeout111);

	if (c != 'd' && c != 'r') // number
	{
		if (sNumCount < 16)
		{
			#if 0 // fill from last way
			for (i = 0; i < 15; i++)
				sCardNum[i] = sCardNum[i+1];
			
			sCardNum[15] = c;
			sNumCount++;
			
			#else // fill from first way
			sCardNum[sNumCount++] = c;
			#endif
		}
	}
	else if(c == 'd') // delete (back space)
	{
		if (sNumCount > 0)
		{
			#if 0 // fill from last way
			for (i = 15; i > 0; i--)
				sCardNum[i] = sCardNum[i-1];

			sCardNum[0] = ' ';			
			sNumCount--;
			
			#else // fill from first way
			sCardNum[--sNumCount] = ' ';
			#endif
		}
	}
	else if(c == 'r')
	{
		while(sNumCount > 0)
		{
			sCardNum[--sNumCount] = ' ';
		}
	}
	UpdateNumber();		
}

bool CardNumCancelOnPress(ITUWidget* widget, char* param)
{
	ituLayerGoto(ituSceneFindWidget(&theScene, "authUserLayer"));
	return true;
}



bool CardNumOkOnPress(ITUWidget* widget, char* param)
{
	if(theConfig.ConfirmSelect == USER_AUTH_NET)
	{

		int i;

		ChannelType activeCh = CstGetUserActiveChannel();		
		if(shmDataAppInfo.app_order == APP_ORDER_CARD_READER && ((sNumCount == 10) || (sNumCount == 14) || (sNumCount == 16)))
		{
			for(i=sNumCount;i<16;i++)
			{
				sCardNum[i] = '/';
			}
			memcpy(shmDataAppInfo.card_no , sCardNum , 16);
			

			ituLayerGoto(ituSceneFindWidget(&theScene, "CardWaitLayer"));

		}
	}
	else if(theConfig.ConfirmSelect == USER_AUTH_PASSWORD)
	{
		ituLayerGoto(ituSceneFindWidget(&theScene, "connectLayer"));
	}
	
		return true;
}

bool CardNumOnEnter(ITUWidget* widget, char* param)
{	
	ChannelType activeCh = CstGetUserActiveChannel();
	int var = (int)shmDataAppInfo.auth_type[0];
	
	AudioPlay("A:/sounds/inputCardNum.wav", NULL);

	GotoCardNumStart = true;
	
	shmDataAppInfo.app_order = APP_ORDER_CARD_READER;
	shmDataAppInfo.charge_request_type = 0x01;						
	
	if (!sLcnBackground)
	{
		sLcnBackground = ituSceneFindWidget(&theScene, "cardNumBackground");
		assert(sLcnBackground);

		sNumKeypadBackground = ituSceneFindWidget(&theScene, "numKeypadBackground");
		assert(sNumKeypadBackground);

		//sokCardNumButton = ituSceneFindWidget(&theScene, "okCardNumButton");
		//assert(sokCardNumButton);			
		
		sCardNum1TextBox = ituSceneFindWidget(&theScene, "cardNum1TextBox");
		assert(sCardNum1TextBox);

		sCardNum2TextBox = ituSceneFindWidget(&theScene, "cardNum2TextBox");
		assert(sCardNum2TextBox);

		sCardNum3TextBox = ituSceneFindWidget(&theScene, "cardNum3TextBox");
		assert(sCardNum3TextBox);

		sCardNum4TextBox = ituSceneFindWidget(&theScene, "cardNum4TextBox");
		assert(sCardNum4TextBox);		
		
	}
	CtLogRed("Enter cardnum layer.. auth_type[%d-%d]", activeCh, var);
	ResetNumber();
	//CtLogRed("Check ResetNumber()");
	//ituWidgetSetVisible(sLcnBackground, true);
	ituWidgetSetVisible(sNumKeypadBackground, true);
	//CtLogRed("Check sNumKeypadBackground");
	NumKeypadSetInputListener(NumInputListener);
	//CtLogRed("Check NumInputListener");	
	TopSetTimer(60, CardWaitTimeout111);

	//TopHomeBtnVisible(true);
	TopBackBtnVisible(false);

    return true;
}


bool CardNumOnLeave(ITUWidget* widget, char* param)
{	
	ChannelType activeCh = CstGetUserActiveChannel();
	int var = (int)shmDataAppInfo.auth_type[0];
	
	CtLogRed("Leave cardnum layer\n");
	GotoCardNumStart = false;
	ituWidgetSetVisible(sNumKeypadBackground, false);
	
	//ituWidgetSetVisible(sLcnBackground, false);

	AudioStop();

	TopStopStepAnimation();
	TopCloseTimer();
	NumKeypadSetInputListener(NULL);

	usleep(100*1000);	// for Enter Main

    return true;
}

void CardNumReset(void)
{
	sLcnBackground = NULL;
}

