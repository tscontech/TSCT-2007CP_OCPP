/**
*       @file
*               layer_top.c
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

//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------

struct timeval sttv;
struct timeval ettv; 
int TopStart1;
int TopeEnd1;
int  totalvalue;
int oldtotalvalue;

//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------
static ITUBackground* sToBackground;

// static ITUIcon* sAuthIcon;
// static ITUIcon* sAuthStep2Icon;

// static ITUIcon* sConnectStep1Icon;
// static ITUIcon* sConnectIcon;
// static ITUIcon* sConnectStep2Icon;

// static ITUIcon* sChargeStep1Icon;
// static ITUIcon* sChargeIcon;
// static ITUIcon* sChargeStep2Icon;

// static ITUIcon* sDisconnectStep1Icon;
// static ITUIcon* sDisconnectIcon;
// static ITUIcon* sDisconnectStep2Icon;

// static ITUText* stopYearText;
// static ITUText* stopMonthText;
// static ITUText* stopDayText;
// static ITUText* stopHourText;
// static ITUText* stopMinuteText;
// static ITUText* stopColon;
// static ITUText* stopDot1;
// static ITUText* stopDot2;

static ITUText* sTimerText;
static ITUIcon* scountdownIcon;
int sTimerCount , time__Count = 1;
static bool sTimerCounting;
static TopTimerTimeoutListener sTimeoutListener;

static pthread_t sTopStatusTask;

short tmyear_,tmmon_,tmmday_,tmhour_,tmmin_;
static pthread_t sTopMonitoringTask;
static bool sDLsTopMonitoring = false;
char __temp__[12];
static ITUButton* shome_btn;
static ITUButton* sback_btn;

static ITUIcon* sMeterErrIcon;
static ITUIcon* sMeterIcon;
static ITUIcon* sServerErrIcon;
static ITUIcon* sServerIcon;
static ITUIcon* sNetErrIcon;
static ITUIcon* sNetIcon;

static ITUIcon* smainlogo_icon;

static ITUText* sTopChargeIDText;
bool lastSeccConSetVal = false;
bool lastServerConSetVal = true;		// Current Status

unsigned char serdiscon_cnt = 0;

bool lastMeterConSetVal = true;			// Current Status

extern bool bAmiErrChk;
extern bool bPlcConn;
struct timeval Errortv_top;
struct timeval Nowtv_top;
long NetErrorTime_top;
bool NetError_Flg = true;

//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------

static void GetTimeToString(void);

static void FindTopWidget(void)
{
	if (!sToBackground)
	{
		printf("FindTopWidget----------------\r\n");
        sToBackground = ituSceneFindWidget(&theScene, "topBackground");
		assert(sToBackground);

		sTimerText = ituSceneFindWidget(&theScene, "countdownText");
		assert(sTimerText);

		scountdownIcon = ituSceneFindWidget(&theScene, "countdownIcon");
		assert(scountdownIcon);

		GetTimeToString();

		shome_btn = ituSceneFindWidget(&theScene, "home_btn");
		assert(shome_btn);
		sMeterErrIcon = ituSceneFindWidget(&theScene, "MeterErrIcon");
		assert(sMeterErrIcon);
		sMeterIcon = ituSceneFindWidget(&theScene, "MeterIcon");
		assert(sMeterIcon);
		sServerErrIcon = ituSceneFindWidget(&theScene, "ServerErrIcon");
		assert(sServerErrIcon);
		sServerIcon = ituSceneFindWidget(&theScene, "ServerIcon");
		assert(sServerIcon);
		sNetErrIcon = ituSceneFindWidget(&theScene, "NetErrIcon");
		assert(sNetErrIcon);
		sNetIcon = ituSceneFindWidget(&theScene, "NetIcon");
		assert(sNetIcon);

		smainlogo_icon = ituSceneFindWidget(&theScene, "mainlogo_icon");
		assert(smainlogo_icon);

		sTopChargeIDText = ituSceneFindWidget(&theScene, "TopChargeIDText");
		assert(sTopChargeIDText);
	}	
}

void TopWidgetVisable(bool flag)
{
	ituWidgetSetVisible(sMeterErrIcon, flag);
	ituWidgetSetVisible(sMeterIcon, flag);
	ituWidgetSetVisible(sServerErrIcon, flag);
	ituWidgetSetVisible(sServerIcon, flag);
	ituWidgetSetVisible(smainlogo_icon, flag);
	ituWidgetSetVisible(sNetErrIcon, flag);
	ituWidgetSetVisible(sNetIcon, flag);

	ituWidgetSetVisible(sTopChargeIDText, flag);
}

void TopTimerWidgetMove(int pos_x, int pos_y)
{
	ituWidgetSetX(sTimerText, pos_x);
	ituWidgetSetY(sTimerText, pos_y);

	ituWidgetSetVisible(scountdownIcon, false);
}

bool GetServerCon(void)
{
	return ((theConfig.ConfirmSelect != USER_AUTH_NET) || \
	((shmDataIfInfo.connect_status) && (bConnect) && !ServerCallError));
}

bool GetMeterCon(void)
{
	return !bAmiErrChk;
}

bool GetPlcCon(void)
{
	return !bPlcConn;
}

void SetNetCon(void)
{
	bool bSet = TSCT_NetworkIsReady();

	if(sToBackground == NULL || lastSeccConSetVal == bSet)	return;

	lastSeccConSetVal = bSet;

	if(bSet)
	{
		ituWidgetSetVisible((ITUWidget*)sNetErrIcon, false);
		ituWidgetSetVisible((ITUWidget*)sNetIcon, true);
		NetError_Flg = true;
	}
	else
	{
		ituWidgetSetVisible((ITUWidget*)sNetErrIcon, true);
		ituWidgetSetVisible((ITUWidget*)sNetIcon, false);
	}
}

void SetServerCon(void)
{
	bool bSet = GetServerCon();
	if(sToBackground == NULL || lastServerConSetVal == bSet){
		serdiscon_cnt = 0;
		return;
	}

	else if(lastServerConSetVal != bSet){
		serdiscon_cnt++;
		if(serdiscon_cnt>3)
			lastServerConSetVal = bSet;
		else 
			return;
	}


	if(bSet)
	{
		ituWidgetSetVisible((ITUWidget*)sServerErrIcon, false);
		ituWidgetSetVisible((ITUWidget*)sServerIcon, true);
	}
	else
	{
		ituWidgetSetVisible((ITUWidget*)sServerErrIcon, true);
		ituWidgetSetVisible((ITUWidget*)sServerIcon, false);
	}
}

void SetMeterCon(void)
{
	bool bSet = GetMeterCon();
	if(sToBackground == NULL || lastMeterConSetVal == bSet) return;

	lastMeterConSetVal = bSet;

	if(bSet)
	{
		ituWidgetSetVisible((ITUWidget*)sMeterErrIcon, false);
		ituWidgetSetVisible((ITUWidget*)sMeterIcon, true);
	}
	else
	{
		ituWidgetSetVisible((ITUWidget*)sMeterErrIcon, true);
		ituWidgetSetVisible((ITUWidget*)sMeterIcon, false);
	}
}

void homeBtn_press(void) {
	ChannelType activeCh = CstGetUserActiveChannel();
	
	GotoStartLayer();	
	usleep(100*1000);			
	// ControlPilotDisablePower(activeCh);
	// usleep(100*1000);
}

void backBtn_press(void) {
	switch (shmDataAppInfo.app_order)
	{
	case APP_ORDER_WAIT :
		GotoStartLayer();
		break;

	case APP_ORDER_AUTH_METHOD :
		ituLayerGoto(ituSceneFindWidget(&theScene, "ch2Layer"));
		break;

	case APP_ORDER_CARD_READER :
		ituLayerGoto(ituSceneFindWidget(&theScene, "authUserLayer"));
		break;	
	
	default:
		break;
	}
}

void TopHomeBtnVisible(bool flag)
{
	ituWidgetSetVisible(shome_btn, flag);
}	

void TopBackBtnVisible(bool flag)
{
	ituWidgetSetVisible(sback_btn, flag);
}

void TopCounterVisible(bool flag)
{
	printf("Top Counter Set Visable\r\n");
	FindTopWidget();
	ituWidgetSetVisible(sToBackground, flag);
	ituWidgetSetVisible(sTimerText, flag);
	//ituWidgetSetVisible(scountdownIcon, flag);
}	

void TopGotoAuthUserStep(void)
{
	ChannelType activeCh = CstGetUserActiveChannel();
	
	if(shmDataAppInfo.app_order < APP_ORDER_CUSTOMER_AUTH) ituWidgetSetVisible(shome_btn, true);
	else ituWidgetSetVisible(shome_btn, false);
}


void TopGotoConnectStep(void)
{
	// ituWidgetSetVisible(sConnectStep2Icon, true);
	ituWidgetSetVisible(shome_btn, false);
}

void TopStopStepAnimation(void)
{

}

/* Add for solving a Time Display Bug when Card Auth Menu	20190114 KT Lee */
static void GetTimeToString(void)
{
	time_t time = CstGetTime();
	struct tm *tm = localtime(&time);
	char _temp_[12]={0x30,};
	
	sprintf(_temp_, "%d", tm->tm_year+1900 ); 
	// ituTextSetString(stopYearText,_temp_);
	
	memset(_temp_, 0x30, 12);
	sprintf(_temp_, "%02d", tm->tm_mon+1); 
	// ituTextSetString(stopMonthText,_temp_);
	
	memset(_temp_, 0x30, 12);
	sprintf(_temp_, "%02d", tm->tm_mday );	
	// ituTextSetString(stopDayText,_temp_);
	
	memset(_temp_, 0x30, 12);
	sprintf(_temp_, "%02d", tm->tm_hour );	
	// ituTextSetString(stopHourText,_temp_);

	memset(_temp_, 0x30, 12);	
	sprintf(_temp_, "%02d", tm->tm_min );	
	// ituTextSetString(stopMinuteText,_temp_);

	//printf(" 7>> %04d-%02d-%02d:%02d.%02d.%02d \n", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);	
}

static void* sTopMonitoringTaskFuntion(void* arg)
{	
	while(sDLsTopMonitoring)
	{
		usleep( 1000*100 );	
		time__Count %= 32;

		if(time__Count == 20)
		{
		    /* Add for solving a Time Display Bug when Card Auth Menu	20190114 KT Lee */
			GetTimeToString();			
		}
		time__Count++;
		if(sDLsTopMonitoring && (time__Count != 0))

			if (sTimerCounting)
			{				
				gettimeofday(&ettv, NULL);			
				TopStart1 = sttv.tv_sec;
				TopeEnd1 = ettv.tv_sec;				
				totalvalue = TopeEnd1 - TopStart1;
		
				char buf[16];
				char buf1[16];
				
				if(oldtotalvalue != totalvalue)
				{
					sTimerCount--;

					sprintf(buf, "%d", sTimerCount);				
					memcpy(buf1,buf,sizeof(buf));
					ituTextSetString(sTimerText, buf1);						

				}
				oldtotalvalue = totalvalue;			
				
				if (sTimerCount < 1)	{
					ituTextSetString(sTimerText, " ");
					ituWidgetSetVisible(scountdownIcon, false);
					sDLsTopMonitoring = false;
					if (sTimeoutListener != NULL)
						(*sTimeoutListener)();
				}
			} 	else {
					sDLsTopMonitoring = false;
				}
	}
	sTopMonitoringTask = 0;	

}

static void* sTopStatusTaskFuntion(void* arg)
{
	//CtLogRed("[TopStatusTaskFuntion] start ");
	bool golayer_flg;
	long nowT_top = 0;
	long Errorminute_top = 0;
	while(1)
	{
		sleep(1);
		SetServerCon();
		SetMeterCon();
		SetNetCon();
		if(bConnect)
		{
			NetError_Flg = true;
		}
		
		if((!bConnect || !TSCT_NetworkIsReady()) && NetError_Flg)
		{
			NetError_Flg = false;
			gettimeofday(&Errortv_top, NULL);
			NetErrorTime_top = Errortv_top.tv_sec;
			CtLogRed("NetError_Flg  : %d : %d", NetErrorTime_top, Errortv_top.tv_sec);
		}

		if(!NetError_Flg && theConfig.ConfirmSelect == USER_AUTH_NET)
		{
			gettimeofday(&Nowtv_top, NULL);
			/*
			nowT_top = (Nowtv_top.tv_sec % 3600) / 60;
			Errorminute_top = (NetErrorTime_top % 3600) / 60;
			CheckT = nowT_top - Errorminute_top;
*/
			nowT_top = Nowtv_top.tv_sec - NetErrorTime_top;
			Errorminute_top = (nowT_top % 3600) / 60;
			
			printf(" Top net error : %d: %d \n", nowT_top, Errorminute_top);

			if(Errorminute_top > 5)
			{
				CtLogRed("NetError stop : %d", Errorminute_top);
				if(shmDataAppInfo.app_order != APP_ORDER_CHARGING)
				{
					ConfigSave();
					printf("\n\n soon reset....\n\n");
					usleep(200*1000);
					custom_reboot();
				}
			}
		}

		if(CsConfigVal.bReqRmtStartTsNo){
			bDevChannel = CsConfigVal.bReqRmtStartTsNo - 1;
			shmDataAppInfo.auth_type[0] = SERVER_AUTH;
			shmDataAppInfo.app_order = APP_ORDER_REMOTE_CHECK;
			sleep(2);
			ituLayerGoto(ituSceneFindWidget(&theScene, "CardWaitLayer"));
		}

		if(CsConfigVal.diagLogReqStep == DIAG_STAT_UPLOAD)
		{
			CsConfigVal.diagLogReqStep = DIAG_STAT_UPLOADING;

			sleep(2);	//wait Send Msg to CSMS

			if(FtpLogUpload_func())	// upload fail
				CsConfigVal.diagLogReqStep = DIAG_STAT_UPLOADFAIL;

			else	// upload success
				CsConfigVal.diagLogReqStep = DIAG_STAT_UPLOADED;
		}

		if(fwUpdateVals.updateStep == UPDATE_STEP_CHKDATE)
		{
			if(CheckUpdateFwDate())
			{
				theConfig.chargingstatus = 0;
				SetCpStatus(CP_STATUS_CODE_UNAVAIL ,0);
				SetCpStatus(CP_STATUS_CODE_UNAVAIL ,1);

				sleep(2);

				fwUpdateVals.updateStep = UPDATE_STEP_DOWNLOAD;

				sleep(2);

				memcpy(theConfig.ftpDns, fwUpdateVals.fwUpdateUrl, sizeof(fwUpdateVals.fwUpdateUrl));

				FtpFwUpdate_func();

				if(!checkUpdate())	
				{
					// fwUpdateVals.updateStep = UPDATE_STEP_DOWNLOADED;

					printf("\n\n soon reset....\n\n");

					fwUpdateVals.updateStep = UPDATE_STEP_INSTALED;

					usleep(2000*1000);

					ConfigSave();

					usleep(2000*1000);

					custom_reboot();
					
					while(1);
				}
				else
				{
					// fwUpdateVals.updateStep = UPDATE_STEP_DOWNLOADFAIL;

					clearUpdate();	// Set the Upadate Status
	
					usleep(2000*1000);

					ConfigSave();

					usleep(2000*1000);

					custom_reboot();
					
					while(1);
				}
			}
		}

		if(CsConfigVal.bReqResetNo){
			if(GetCpStatus(bDevChannel+1) == CP_STATUS_CODE_CHARGING || GetCpStatus(bDevChannel+1) == CP_STATUS_CODE_PREPARE){
				shmDataAppInfo.charge_comp_status = END_SERVER;
				TSCT_ChargingStop();
				sleep(1);
			}
			else{
				printf("\n\n soon reset....\n\n");
				
				ConfigSave();

				sleep(5);

				custom_reboot();

				while(1);
			}
		}
	}
}

void TopPauseTimer(void)
{
	sTimerCounting = false;
}
void TopResumeTimer(void)
{
	sTimerCounting = true;
}
void TopSetTimer(int count, TopTimerTimeoutListener listener)
{
	char buf[32];

	sTimerCount = count;
	sTimeoutListener = listener;
	sTimerCounting = true;
	gettimeofday(&sttv, NULL);
	printf("TopSetTimer == sTimerId = SDL_AddTimer(1000, TopUpdateTimer, NULL) \n");
	sprintf(buf, "%d", count);
	ituTextSetString(sTimerText, buf);
	ituWidgetSetVisible(scountdownIcon, true);
	sDLsTopMonitoring = true;
	if (sTopMonitoringTask == 0)
	{
		pthread_create(&sTopMonitoringTask, NULL, sTopMonitoringTaskFuntion, NULL);
		pthread_detach(sTopMonitoringTask);
	}		
}

void TopCloseTimer(void)
{
	ituTextSetString(sTimerText, ' ');	
	ituWidgetSetVisible(scountdownIcon, false);
	sDLsTopMonitoring = false;
	sTimeoutListener = NULL;
}

void TopInitLayer(void)
{
	unsigned char Buf[64];

	if (!sToBackground)
    {
        sToBackground = ituSceneFindWidget(&theScene, "topBackground");

		sTimerText = ituSceneFindWidget(&theScene, "countdownText");
		scountdownIcon = ituSceneFindWidget(&theScene, "countdownIcon");

		GetTimeToString();

		shome_btn = ituSceneFindWidget(&theScene, "home_btn");
		sback_btn = ituSceneFindWidget(&theScene, "back_btn");

		sMeterErrIcon = ituSceneFindWidget(&theScene, "MeterErrIcon");
		sMeterIcon = ituSceneFindWidget(&theScene, "MeterIcon");
		sServerErrIcon = ituSceneFindWidget(&theScene, "ServerErrIcon");
		sServerIcon = ituSceneFindWidget(&theScene, "ServerIcon");
		sNetErrIcon = ituSceneFindWidget(&theScene, "NetErrIcon");
		sNetIcon = ituSceneFindWidget(&theScene, "NetIcon");	

		smainlogo_icon = ituSceneFindWidget(&theScene, "mainlogo_icon");

		sTopChargeIDText = ituSceneFindWidget(&theScene, "TopChargeIDText");
    }
	memset(Buf, 0x00, sizeof(Buf));

	if(theConfig.OperationMode == OP_FREE_MODE)
		sprintf(Buf, "ID %s[%s] 무료",theConfig.siteid, theConfig.devid1);
	else if (theConfig.OperationMode == OP_CHECK_MODE)
		sprintf(Buf, "ID %s[%s] 점검",theConfig.siteid, theConfig.devid1);
	else
		sprintf(Buf, "ID %s [%s]",theConfig.siteid, theConfig.devid1);
	ituTextSetString(sTopChargeIDText, Buf);

	ituTextSetString(sTimerText, " ");
	ituWidgetSetVisible(scountdownIcon, false);

	ituWidgetSetVisible(smainlogo_icon, true);

	sTopMonitoringTask = 0;
	sDLsTopMonitoring = false;

	if (sTopStatusTask == NULL)
	{
		pthread_create(&sTopStatusTask, NULL, sTopStatusTaskFuntion, NULL);
		pthread_detach(sTopStatusTask);
	}	
}


void TopReset(void)
{
	sToBackground = NULL;
}

