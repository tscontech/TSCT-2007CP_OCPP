/**
*       @file
*               layer_admin_login.c
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2018.01.10 <br>
*               author: bmlee <br>
*               description: <br>
*/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "scene.h"
#include "ctrlboard.h"
#include "tsctcfg.h"

static int nCurSubMenu = 0;
extern int charge_stop_btnState[2];

void GotoStartLayer(void)
{
	char *nextLayer = "mainLayer";
	ituLayerGoto(ituSceneFindWidget(&theScene, nextLayer));
}

void GotoChargeLayer(void)
{
	printf("\r\n[GotoChargeLayer] Enter the Function\r\n");
	char *nextLayer = ((theConfig.devtype == BB_TYPE)||(theConfig.devtype == BC2_TYPE))? "mainLayer":"chargeLayer";
	ituLayerGoto(ituSceneFindWidget(&theScene, nextLayer));
}

void AdminSetupMenuExit(bool bSaveCfg)
{
	if(bSaveCfg){	
		LEDOn();

		bGloAdminStatus = true;

		ResetEthernet();

		ConfigSave();

		nCurSubMenu = 0;
		bGloAdminStatus = false;
		bAdminExit = true;
		
		printf("\n\n soon reset....\n\n");
		
		sleep(2);

		TSCT_NetworkExit();

		printf("end....\n");
		custom_reboot();
		
		while(1);
	}
	
	GotoStartLayer();
}

bool ChangeSetupSubMenu(int nSelect)
{
	printf("ChangeSetupSubMenu :: selected item : %d => %d \n", nCurSubMenu, nSelect);

	if(nCurSubMenu == nSelect)		return;
	
	switch(nSelect)		
	{	
		case 0:
			ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetIpLayer"));
			break;
			
		case 1:
			ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetChargerLayer"));
			break;
			
		case 2:
			ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetCharger2Layer"));
			break;
			
		case 3:
			ituLayerGoto(ituSceneFindWidget(&theScene, "AdminChargerTestLayer"));
			break;
			
		case 4:			
			ituLayerGoto(ituSceneFindWidget(&theScene, "AdminChargerListLayer"));
			break;
			
		defualt:				
			break;	
	}

	nCurSubMenu = nSelect;
}

void CstPlayAudioMsg(int nAudioMsg)
{
#ifdef AUDIO_SUPPORT 
	mp3Testflag = true;
	mp3TestSelect = nAudioMsg;
#endif
}

void CheckCurrentIsZero(int ch, int *tCurZeroTime, int tChargedTime, float nCurrent)
{
	bool common_type;

	common_type = bDevChannel;
	if((theConfig.devtype == BB_TYPE) || (theConfig.devtype == BC2_TYPE)) common_type = ch;
	
	if(nCurrent < 0.01 && (shmDataAppInfo.app_order == APP_ORDER_CHARGING) && CstGetMcstatus())
	{
		if( *tCurZeroTime == 0 )		*tCurZeroTime = tChargedTime;
		else
		{
			int tStartZeroTimeMin = (int)(*tCurZeroTime%3600) / 60;
			int tCurrentZeroTimeMin = (int)(tChargedTime%3600) / 60;
			int tZeroCurrentPastTime =  tCurrentZeroTimeMin - tStartZeroTimeMin;

			printf("[%d]CheckCurrentIsZero :: Zero Current Charging? <<< S:%d, C:%d, P:%d >>>\n ",\
				ch, tStartZeroTimeMin, tCurrentZeroTimeMin, tZeroCurrentPastTime);
			
			if( tZeroCurrentPastTime < 0 )		tZeroCurrentPastTime = 0;
			
			if(tZeroCurrentPastTime > 14) // Charging ends when it is 0.0A for 15 minutes
			{				
			
				charge_stop_btnState[common_type] = 2;	
				if((theConfig.devtype == BB_TYPE) || (theConfig.devtype == BC2_TYPE))	StopCharge2ch(ch);
				// FOR TEST BUILD 2021.12.15	
				else								StopCharge();
				
				WattHourMeterStopMonitoring(ch);		
			
				// FOR TEST BUILD 2021.12.15	
				if((theConfig.devtype != BB_TYPE) && (theConfig.devtype != BC2_TYPE))	UpdateStopGui();
				

				shmDataAppInfo.app_order = APP_ORDER_CHARGING_STOP;
				shmDataAppInfo.charge_comp_status = END_ERR;
			}
		}
	}
     else
		*tCurZeroTime = 0;
}

void CstGotoAuthOrNextLayer(void)
{
	if(theConfig.ConfirmSelect == USER_AUTH_NONE)
	{			
		ituLayerGoto(ituSceneFindWidget(&theScene, "connectLayer"));		
	}
	else 
	{
		ituLayerGoto(ituSceneFindWidget(&theScene, "authUserLayer"));	
	}
}

int CstGetActiveCh(void)
{
	if(theConfig.devtype == BC_TYPE || theConfig.devtype == HBC_TYPE)
	{
		return (int)bDevChannel;
	}
	else
	{
		return (int)CstGetUserActiveChannel();
	}
}
