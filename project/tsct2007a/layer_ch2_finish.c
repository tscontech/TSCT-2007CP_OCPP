/**
*       @file
*               layer_ch2_finish.c
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2017.12.28 <br>
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


//-----------------------------------------------------------------------

// MACRO

//-----------------------------------------------------------------------

//-----------------------------------------------------------------------

// Local Variable

//-----------------------------------------------------------------------

static ITUBackground* sLc2Background;
static ITUText* sEnergyUsedText;
static ITUText* sChargeTimeText;
static ITUText* sChargePriceText;

static CPVoltage sVoltage = CP_VOLTAGE_UNKNOWN;

extern int charge_price;
extern uint32_t charge_watt;

//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------

static void CPListenerOnFinish(int ch, unsigned char nAdcValue, CPVoltage voltage)
{
	sVoltage = voltage;
	
	switch(sVoltage)
	{
		case CP_VOLTAGE_12V:
			sleep(2);			// for Send CP finish status when disconnect cable
			GotoStartLayer();	
			break;
			
		case CP_VOLTAGE_9V:
		case CP_VOLTAGE_6V:
			break;			
	}
}

bool Ch2FinishConfirmOnPress(ITUWidget* widget, char* param)
{
	ituLayerGoto(ituSceneFindWidget(&theScene, "disconnectLayer"));	
}

bool Ch2FinishOnEnter(ITUWidget* widget, char* param)
{
	int hour, minute, second;
	float energy; int time;
	char buf[32] = {" ",};

	shmDataAppInfo.app_order = APP_ORDER_CHARGING_STOP;

	AudioPlay("A:/sounds/endCharge.wav", NULL);
	
	CtLogRed("Enter ch2 finish layer..\n");

	ControlPilotSetListener(bDevChannel, CPListenerOnFinish);

	if (!sLc2Background)
    {
        sLc2Background = ituSceneFindWidget(&theScene, "ch2FinishBackground");
        assert(sLc2Background);

		sEnergyUsedText = ituSceneFindWidget(&theScene, "ch2EnergyUsedText");
		assert(sEnergyUsedText);
		sChargeTimeText = ituSceneFindWidget(&theScene, "ch2ChargeTimeText");
		assert(sChargeTimeText);
		sChargePriceText = ituSceneFindWidget(&theScene, "ChargePriceText");
		assert(sChargePriceText);
    }

	CstGetUsedEnergy(&energy, &time);	

	
	sprintf(buf, "%.2f kWh", (float)charge_watt/100.0f);
	ituTextSetString(sEnergyUsedText, buf);
	
	hour = time / 3600;
	minute = (time % 3600) / 60;
	second = time % 60;

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%02d:%02d:%02d", hour, minute, second);
	ituTextSetString(sChargeTimeText, buf);		
	
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d 원", charge_price/100);
	ituTextSetString(sChargePriceText, buf);

	if(startTsQ.faultChargFlg && CfgKeyVal[14].CfgKeyDataInt)
	{
		StopTsConfig.Stop_Reason = STOP_REASON_DEAUTH;
	}
	else
	{
		StopTsConfig.Stop_Reason = STOP_REASON_NONE;
	}

	CsConfigVal.bReqStopTsFlg = true;

	sleep(1);	//for Send CP FInish STatus msg	to CSMS

    return true;
}



bool Ch2FinishOnLeave(ITUWidget* widget, char* param)
{
	CtLogRed("Leave ch2 finish layer\n");
	AudioStop();
	TopStopStepAnimation();	
	// CsConfigVal.bReqStopTsFlg = false;
	CsConfigVal.bReqRmtStopTSFlg = false;
    return true;
}

void Ch2FinishReset(void)
{
	sLc2Background = NULL;
}
