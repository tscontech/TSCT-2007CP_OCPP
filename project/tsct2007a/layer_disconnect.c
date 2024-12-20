/**
*       @file
*               layer_disconnect.c
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
#include "cststring.h"
#include "tsctcfg.h"


//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------

#define CLOSE_WAIT_TIMEOUT		30
#define DLMON_TASK_DELAY		500 // ms

//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------

static ITUBackground* sLdBackground;
static ITUSprite* sDisconnectSpriteB;
// static ITUSprite* sDisconnectSpriteC;

// static ITUText* sDisconnectText;

static CPVoltage sVoltage = CP_VOLTAGE_UNKNOWN;
static pthread_t sDLMonitoringTask;
static bool sDLMonitoringRunning = false;

static CPVoltage sVoltage1 = CP_VOLTAGE_UNKNOWN;

static bool sDLMonitoringcheck[2] = {false, false};
static bool gotoThankbool;




//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------
static void GotoMainLayer(void)
{
	printf("goto Main layer..\n");	
	ChannelType activeCh = CstGetUserActiveChannel();	
	
	gotoThankbool = false;
	// bmakepacket_g1 = true;
	TopCloseTimer();	
	TopStopStepAnimation();

	shmDataAppInfo.app_order = APP_ORDER_FINISH;
	sDLMonitoringRunning = false;
	// ControlPilotSetListener(CH1, NULL);
	// ControlPilotSetListener(CH2, NULL);

	ituSpriteStop(sDisconnectSpriteB);
	ituWidgetSetVisible(sDisconnectSpriteB, false);

	GotoStartLayer();
}

void TimeOutPageOut1(void)
{	
	usleep(500*1000);
	GotoMainLayer();
}

static void CPListenerOnDisconnect(int ch, unsigned char nAdcValue, CPVoltage voltage)
{
	sVoltage = voltage;
	
	switch(sVoltage)
	{
		case CP_VOLTAGE_12V:
			GotoMainLayer();	
			break;
			
		case CP_VOLTAGE_9V:
		case CP_VOLTAGE_6V:
			break;			
	}
}
static void CPListenerOnDisconnect1(int ch, unsigned char nAdcValue, CPVoltage voltage)
{
	sVoltage1 = voltage;
	
	switch(sVoltage1)
	{
		case CP_VOLTAGE_12V:

			{ 
				if(TwoChOnlyEmg)
				{
					sDLMonitoringcheck[CH2] = true;							
					if(sDLMonitoringcheck[CH1])		GotoMainLayer();	//GotoThanksLayer(true);					
				}
			}
			break;
			
		case CP_VOLTAGE_9V:
		case CP_VOLTAGE_6V:
			break;			
	}
}

bool DisconnectOnEnter(ITUWidget* widget, char* param)
{
	AudioPlay("A:/sounds/hangConnector.wav", NULL);
	
	CtLogRed("Enter disconnect layer..\n");
	gotoThankbool = true;
	sleepOn1chCheck = false;
	// bmakepacket_g1 = false;

	ScreenOnScenario();

	ChannelType activeCh = CstGetUserActiveChannel();	
	shmDataAppInfo.app_order = APP_ORDER_CONNECT_OUT;		
	
	if (!sLdBackground)
    {
        sLdBackground = ituSceneFindWidget(&theScene, "disconnectBackground");
        assert(sLdBackground);

		sDisconnectSpriteB = ituSceneFindWidget(&theScene, "disconnectSpriteB");
		assert(sDisconnectSpriteB);
    }	

	ituWidgetSetVisible(sDisconnectSpriteB, true);
	ituSpritePlay(sDisconnectSpriteB, CH1);
			
	printf( "\n\n\n================DisconnectOnEnter ChannelType activeCh = CstGetUserActiveChannel(); [%d] \n", CstGetActiveCh());

	ControlPilotSetListener(0, CPListenerOnDisconnect);

	if(!EmgControl) {
		TopSetTimer(90, TimeOutPageOut1);
		TopHomeBtnVisible(false);
	}

	
    return true;
}

bool DisconnectOnLeave(ITUWidget* widget, char* param)
{
	CtLogRed("Leave disconnect layer..\n");
	AudioStop();
	usleep(500*1000);	// for Enter Main

   	return true;
}


void DisconnectReset(void)
{
	sLdBackground = NULL;
}


