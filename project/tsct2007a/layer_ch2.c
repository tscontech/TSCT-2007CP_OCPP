/**
*       @file
*               layer_ch2.c
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2017.12.26 <br>
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
#include "cststring.h"
#include "tsctcfg.h"
#include "tsctcommon.h"


//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------
#define REMOTE_TIMEOUT	30

//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------
static ITUBackground* sLcch2Background;

static ITUBackgroundButton* sCh1StartBkButton;
static ITUBackgroundButton* sCh2StartBkButton;
//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------

bool Ch2LeftStartOnPress(ITUWidget* widget, char* param)
{
	bDevChannel = CH1; // only used BC type 
	
	if(!EmgControl)
	{	
		if(!GetMeterCon())
		{
			ShowWhmErrorDialogBox(ERR_AMI_DISCON);
			return true;
		}
		if(!GetServerCon() && GetCpStatus(1) != CP_STATUS_CODE_AVAIL)
		{
			ShowWhmErrorDialogBox(ERR_SERVER_DISCON);
			return true;
		}
		
		if(!sleepOnCheck)
		{
			gAmiChargeWatt = 0.0;
			gAmiStartWatt = 0.0;
			gChargeWatt[CH1] = 0.0;		
			
			ControlPilotEnablePower(0);
			sleep(1);
			ControlPilotSetListener(0, CPListenerOnAhead);
			CstSetUserActiveChannel(CH1);
			CstGotoAuthOrNextLayer();
		}

		sleepOnCheck = false;
	}

	ithGpioSet(CST_GPIO_CP0);
    usleep(100);

	return true;
}

bool Ch2RightStartOnPress(ITUWidget* widget, char* param)
{
	// bDevChannel = CH2; // only used BC type 

	// printf("Ch2RightStartOnPress [%d] ", bDevChannel);
	
	// if(!EmgControl)
	// {
	// 	if(!GetMeterCon())
	// 	{	
	// 		ShowWhmErrorDialogBox(ERR_AMI_DISCON);
	// 		return true;
	// 	}
	// 	if(!GetServerCon() && GetCpStatus(2) != CP_STATUS_CODE_AVAIL)
	// 	{
	// 		ShowWhmErrorDialogBox(ERR_SERVER_DISCON);
	// 		return true;
	// 	}

	// 	if(!sleepOnCheck)
	// 	{
	// 		gAmiChargeWatt = 0.0;
	// 		gAmiStartWatt = 0.0;
	// 		gChargeWatt[bDevChannel] = 0.0;	

	// 		ControlPilotEnablePower(bDevChannel);
	// 		sleep(1);
	// 		ControlPilotSetListener(bDevChannel, CPListenerOnAhead);
	// 		CstSetUserActiveChannel(bDevChannel);
	// 		CstGotoAuthOrNextLayer();
	// 	} 
	// 	sleepOnCheck = false;
	// }
	return true;
}



bool Ch2OnEnter(ITUWidget* widget, char* param)
{
	CtLogRed("Enter ch2 layer..\n");
	
    if(!sLcch2Background)
    {
        sLcch2Background = ituSceneFindWidget(&theScene, "ch2Background");
        //assert(sLcch2Background);
		sCh1StartBkButton = ituSceneFindWidget(&theScene, "ch2LeftStartBkButton");
		//assert(sCh1StartBkButton);
		sCh2StartBkButton = ituSceneFindWidget(&theScene, "ch2RightStartBkButton");
		//assert(sCh2StartBkButton);
    }

	shmDataAppInfo.app_order = APP_ORDER_WAIT;

	TopHomeBtnVisible(true);
	TopBackBtnVisible(true);

	TopSetTimer(REMOTE_TIMEOUT, GotoStartLayer);

    return true;
}

bool Ch2OnLeave(ITUWidget* widget, char* param)
{		
	CtLogRed("Exit ch2 Layer");
	AudioStop();
	return true;
}