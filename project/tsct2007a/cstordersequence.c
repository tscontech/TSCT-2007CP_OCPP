/**
*       @file
*               cstordersequence.c
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2018.01.05 <br>
*               author: bmlee <br>
*               description: <br>
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "SDL/SDL.h"
#include "ctrlboard.h"
#include "tsctcommon.h"
#include "scene.h"
#include "cstordersequence.h"
#include "cststring.h"
#include "tsctcfg.h"

// 각종 ??러 체크 ????동??을 방?? ??기 ??해 
// 모든 ??퀀??????작구간??가지????어????다.
//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------
#define MON_TASK_DELAY		500 // ms
//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------


static pthread_t sOrderSequenceMonitoringTask;
static bool sDLsOrderSequenceMonitoring = false;

bool bEmbCheckShowHide;

int EmgCount = 0;
extern bool chargecomp_stop;
static bool oldPressed = false;			// Ture : Idle / False : EMG Button En

//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------
static void EMBListenerOnCharge12(bool pressed)
{
	if(bGloAdminStatus)	return;
	
	if(EmgCount++ < 20)	return;			

	if (pressed == oldPressed)
	{
		if (pressed)
		{
			CsConfigVal.bReqEmgBtnFlg = true;
			EmgControl = true;
			ScreenOnScenario();	

			CstSetEpqStatus(CST_EMG_SW, !EmgControl);
			shmDataAppInfo.charge_comp_status = END_EMG;
			
			printf("\r\nEmergencyDialogShow(STEP_START)\r\n");
			EmergencyDialogShow(STEP_START);

			sChCharging = false;

			LEDStopBlink();	
			MagneticContactorOff();
			StopPwm(CH1);
			WattHourMeterStopMonitoring(CH1);		
						
			if(shmDataAppInfo.app_order >= APP_ORDER_CHARGE_INPUT && shmDataAppInfo.app_order <= APP_ORDER_CHARGING_STOP)
			{		
				printf("charging EMG\r\n");						
				shmDataAppInfo.app_order = APP_ORDER_CHARGE_END;						
				ituLayerGoto(ituSceneFindWidget(&theScene, "ch2FinishLayer"));
			}
			else if (shmDataAppInfo.app_order >= APP_ORDER_WAIT && shmDataAppInfo.app_order <= APP_ORDER_PAYMENT_METHOD)
			{			
				printf("idle EMG\r\n");				
				shmDataAppInfo.app_order = APP_ORDER_WAIT;
				GotoStartLayer();	
			}
			else if(shmDataAppInfo.app_order == APP_ORDER_FINISH)
			{	
				shmDataAppInfo.app_order = APP_ORDER_WAIT;
				GotoStartLayer();	
			}			
		}
		else
		{
			CsConfigVal.bReqEmgBtnFlg = false;
			EmgControl = false;
			EmergencyDialogHide();			
			CstSetEpqStatus(CST_EMG_SW, !EmgControl);
		}
		oldPressed = !pressed;
	}
}

static void* sOrderSequenceMonitoringTaskFuntion(void* arg)
{
	while(sDLsOrderSequenceMonitoring)
	{
		usleep(200*1000);	
		
		// for LED Control, when idle status
		if(!EmgControl && !bGloAdminStatus && (screenOff == false))
		{
			if(shmDataAppInfo.app_order < APP_ORDER_AUTH_METHOD) LEDOff();	
		}	
			
		if(chargecomp_stop)
		{
			if((evc_get_time() - stime) > 59) 	// 1min CP Level 6V Charging
			{
				shmDataAppInfo.app_order = APP_ORDER_CHARGING_STOP;
				StopCharge();
				UpdateStopGui(); 
				shmDataAppInfo.charge_comp_status = END_CAR;	// Charging Stop Req from EV					

				chargecomp_stop = false;
			}
			else
			{
				printf("[SM3 ch1] Wait StopCharging =%d\n", (evc_get_time() - stime));
			}

			sleep(1);
		}
	}
	sDLsOrderSequenceMonitoring = false;
	sOrderSequenceMonitoringTask = 0;
}

void OderSequenceInit(void)
{	
	bFTPupgrade = FTP_FW_UD_IDLE;

	EmgControl = false;
	bErrorcheck1ShowHide = false;
	bErrorcheck2ShowHide = false;		
	bEmbCheckShowHide = false;
	
	EmergencyButtonStartMonitoring(EMBListenerOnCharge12);
	
	if (sOrderSequenceMonitoringTask == 0)
	{
		sDLsOrderSequenceMonitoring = true;
		pthread_create(&sOrderSequenceMonitoringTask, NULL, sOrderSequenceMonitoringTaskFuntion, NULL);
		pthread_detach(sOrderSequenceMonitoringTask);
	}		
	printf("[Debug] OderSequenceInit..  Start  \n");	
	
}

void exitOrderSequenceMonitoringTask()
{
	sDLsOrderSequenceMonitoring = false;
}

