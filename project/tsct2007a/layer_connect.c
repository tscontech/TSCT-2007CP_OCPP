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
#define CONNECT_WAIT_TIMEOUT	90 // 180
#define CLOSE_WAIT_TIMEOUT		30
#define CLOSE2_WAIT_TIMEOUT		30 // 60

#define DLMON_TASK_DELAY		500 // ms


//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------
static ITUBackground* sLcBackground;
//static ITUText* sConnectTextBackground;
// static ITUText* sConnectText;
// static ITUSprite* sCloseSprite;

static ITUSprite* sConnectSpriteB;
// static ITUSprite* sConnectSpriteC;

// static ITUText* sCloseTextBackground;
// static ITUText* sCloseText;

static ITUButton* b_nextLayer2;


static ChargeStep sStep = STEP_REQ_CONNECT;
static CPVoltage sVoltage = CP_VOLTAGE_UNKNOWN;

static bool PlayConnectSprite_flag; 
// static bool PlayConnectSprite_door_flag; 
static bool  GotoConnectToStartLayer;
//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------
#if defined(SAMWONFA_CD_TERMINAL)	
void Prepay_cancelLayer(void)
{	
	ChannelType activeCh = CstGetUserActiveChannel();
	shmDataAppInfo.sPay_type = PRE_PAYMENT_CANCEL;
	ituLayerGoto(ituSceneFindWidget(&theScene, "creditInsertLayer"));
}
#endif

static void PlayConnectSprite(void)
{
	PlayConnectSprite_flag = true;
	ituWidgetSetVisible(sConnectSpriteB, true);
	ituSpritePlay(sConnectSpriteB, CH1);
	//ituWidgetSetVisible(sConnectTextBackground, true);	
}

static void StopConnectSprite(void)
{
	PlayConnectSprite_flag = false;
	ituSpriteStop(sConnectSpriteB);
	ituWidgetSetVisible(sConnectSpriteB, false);
	//ituWidgetSetVisible(sConnectTextBackground, false);	
}

static void TimeOutPageOut(void)
{	
	    ChannelType activeCh = CstGetUserActiveChannel();
		ituLayerGoto(ituSceneFindWidget(&theScene, "mainLayer"));
}

static void CPListenerOnConnect(int ch, unsigned char nAdcValue, CPVoltage voltage)
{
	sVoltage = voltage;
	ChannelType activeCh = CstGetUserActiveChannel();

	//CtLogGreen("++[%d %d] CPListenerOnConnect [[v: %d]] sStep: %d\n", activeCh, ch, sVoltage, sStep);
	
	switch (sVoltage)
	{
		case CP_VOLTAGE_12V:
			//CtLogGreen("12V sStep: %d\n", sStep);
			if (sStep == STEP_REQ_CLOSE) // case : 12V -> 9V -> 12V		
				if(!PlayConnectSprite_flag) PlayConnectSprite();	

			else 
			{			
				if (ituWidgetIsVisible(sConnectSpriteB) == false )
				{
					if(!PlayConnectSprite_flag) PlayConnectSprite();
				}
			}						
			sStep = STEP_REQ_CONNECT;
		break;
			
		case CP_VOLTAGE_9V:

			CsConfigVal.bReqStartTsNo = bDevChannel + 1;
			// while(GetCpStatus(bDevChannel+1) == CP_STATUS_CODE_PREPARE) 
			// {
			// 	// else if(CsConfigVal.bReqStartTsNo == (2*MAX_CONECTOR_ID + bDevChannel + 1))
			// 	// {
			// 	// 	printf("#########Receive Error Go to Main [%d]#######\r\n",CsConfigVal.bReqStartTsNo);
			// 	// 	if(PlayConnectSprite_flag) StopConnectSprite();	
			// 	// 	sStep = STEP_REQ_CLOSE;
			// 	// 	CsConfigVal.bReqStartTsNo = 0;
			// 	// 	TimeOutPageOut();				
			// 	// 	break;					
			// 	// }
			// 	printf("#########Waitting Ok Receive [%d]#######\r\n",CsConfigVal.bReqStartTsNo);
			// 	usleep(200*1000);
			// }
			// if(GetCpStatus(bDevChannel+1) == CP_STATUS_CODE_CHARGING)
			// {
				printf("#########Receive Ok Go to Charge [%d]#######\r\n",CsConfigVal.bReqStartTsNo);
				if(PlayConnectSprite_flag) StopConnectSprite();	
				sStep = STEP_REQ_CLOSE;
				// CsConfigVal.bReqStartTsNo = 0;
				GotoChargeLayer();
				// break;				
			// }					

		break;

		case CP_VOLTAGE_6V:
		break;	
	}
}

bool ConnectOnEnter(ITUWidget* widget, char* param)
{
	// CstPlayAudioMsg(AUDIO_TRYCONNECT);
	AudioPlay("A:/sounds/tryConnect.wav", NULL);

	// TopGotoConnectStep();

	sleepOn1chCheck = false;
	GotoConnectToStartLayer = true;
	
	ChannelType activeCh = CstGetUserActiveChannel();

	CtLogRed("[%d]  Enter Connect layer..==========\n", activeCh);
	
	shmDataAppInfo.app_order = APP_ORDER_CONNECTOR_SELECT;
		
    if (!sLcBackground)
    {
        sLcBackground = ituSceneFindWidget(&theScene, "connectBackground");
        assert(sLcBackground);

		sConnectSpriteB = ituSceneFindWidget(&theScene, "connectSpriteB");
		assert(sConnectSpriteB);
		
		///mod
		b_nextLayer2 = ituSceneFindWidget(&theScene, "b_nextLayer2");
        assert(b_nextLayer2);	
		ituWidgetSetVisible(b_nextLayer2, false);
    }
	
	PlayConnectSprite();
	// StopCloseSprite();
	sVoltage = CP_VOLTAGE_UNKNOWN;
	sStep = STEP_REQ_CONNECT;

	// initilize for Connectting Other Connector.
	// ControlPilotDisablePower(CH1);			
	// ControlPilotDisablePower(CH2);

	sleep(1);		

	ControlPilotEnablePower(CstGetActiveCh());
	sleep(1);

	LEDOn();

	TopSetTimer(CfgKeyVal[2].CfgKeyDataInt, TimeOutPageOut);

	ControlPilotSetListener(0, CPListenerOnConnect);

    return true;
}

bool ConnectOnLeave(ITUWidget* widget, char* param)
{
	TopCloseTimer();
	ChannelType activeCh = CstGetUserActiveChannel();
	CtLogRed("Leave Connect layer\n");	
	
	// ControlPilotSetListener(CstGetActiveCh(), NULL);
	
	TopStopStepAnimation();
	GotoConnectToStartLayer = false;
	StopConnectSprite();

	// CsConfigVal.bReqStartTsNo = 0;

	usleep(100*1000);	// for Enter Main

    return true;
}


void ConnectReset(void)
{
	sLcBackground = NULL;
}

bool B_nextLayer2(ITUWidget* widget, char* param)
{
	GotoChargeLayer();	
	return true;
}