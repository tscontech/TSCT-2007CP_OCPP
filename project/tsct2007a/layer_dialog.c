/**
*       @file
*               layer_dialog.c
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

#define CONNECT_ERR_TXT1	"충전기 연결 중 입니다."
#define CONNECT_ERR_TXT2	"잠시만 기다려 주십시오."

#define BILFAIL_ERR_TXT1	"통신이 불안정 합니다."
#define BILFAIL_ERR_TXT2	"잠시후 재시도 해주십시오."

#define REBOOT_ERR_TXT1		"충전기를 재부팅 합니다."
#define REBOOT_ERR_TXT2		"잠시만 기다려 주십시오."

#define FREEFAIL_ERR_TXT1	"무료충전모드 입니다."
#define FREEFAIL_ERR_TXT2	"회원카드 및 회원번호로 인증 바랍니다."

#define OKDIALOG_TIMEOUT 	20

//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------
static ITUBackground* sLdaBackground;
static ITUBackground* sEmergencyDialog;
static ITUText* sEmergencyDialogText1;
static ITUText* sEmergencyDialogText2;
static ITUText* sEmergencyDialogText3;

static ITUBackground* sOkCancelDialog;
//static ITUText* sokCancelDialogTitleText1;
static ITUIcon* sokCancelDialogTitleIcon1;

static ITUBackground* sOkDialog;
static ITUText* sOkDialogTitle;
static ITUText* sOkDialogTitleText1;
static ITUText* sOkDialogTitleText2;

static ITUText* sDialogCntText;
int sDTimerCount , Dtime__Count = 1;
static bool sDTimerCounting;

struct timeval dsttv;
struct timeval dettv;

int DTopStart1;
int DTopeEnd1;
int dtotalvalue;
int doldtotalvalue;

static DialogTimerTimeoutListener sDTimeoutListener;
static pthread_t sDialogMonitoringTask;
static bool sDLsDialogMonitoring = false;





// static ITUIcon* semergencyIcon;
// static ITUText* semergencyButtonDialogTitle;
static ITUIcon* semergencyButtonIcon;

typedef void (*OkCancelListener)(void);
static OkCancelListener sOkCancelDialogOkListener;
static OkCancelListener sOkCancelDialogCancelListener;

static OkCancelListener sOkDialogOkListener;
extern bool bAmiErrChk;
extern bool bPlcConn;



void OkDialogHide(int ch);

//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------

void DialogGotoStartLayer(void)
{
	OkDialogHide(CH1);
	OkDialogHide(CH2);
	char *nextLayer = "mainLayer";
	ituLayerGoto(ituSceneFindWidget(&theScene, nextLayer));
}

static void FindDialogWidget(void)
{
	if (!sLdaBackground)
	{
		sLdaBackground = ituSceneFindWidget(&theScene, "dialogBackground");
		assert(sLdaBackground);

		sEmergencyDialog = ituSceneFindWidget(&theScene, "emergencyDialogBackground");
		assert(sEmergencyDialog);

		sEmergencyDialogText1 = ituSceneFindWidget(&theScene, "emergencyDialogText1");
		assert(sEmergencyDialogText1);

		sEmergencyDialogText2 = ituSceneFindWidget(&theScene, "emergencyDialogText2");
		assert(sEmergencyDialogText2);
		
		sEmergencyDialogText3 = ituSceneFindWidget(&theScene, "emergencyDialogText3");
		assert(sEmergencyDialogText3);

		// semergencyIcon = ituSceneFindWidget(&theScene, "emergencyIcon");
		// assert(semergencyIcon);
		// semergencyButtonDialogTitle = ituSceneFindWidget(&theScene, "emergencyButtonDialogTitle");
		// assert(semergencyButtonDialogTitle);
		semergencyButtonIcon = ituSceneFindWidget(&theScene, "emergencyButtonIcon");
		assert(semergencyButtonIcon);


		sOkCancelDialog = ituSceneFindWidget(&theScene, "okCancelDialogBackground");
		assert(sOkCancelDialog);

		//sokCancelDialogTitleText1 = ituSceneFindWidget(&theScene, "okCancelDialogTitleText1");
		//assert(sokCancelDialogTitleText1);

		sokCancelDialogTitleIcon1 = ituSceneFindWidget(&theScene, "okCancelDialogTitleIcon1");
		assert(sokCancelDialogTitleIcon1);

		sOkDialog = ituSceneFindWidget(&theScene, "okDialogBackground");
		assert(sOkDialog);

		sOkDialogTitle = ituSceneFindWidget(&theScene, "okDialogTitle");
		assert(sOkDialogTitle);

		sOkDialogTitleText1 = ituSceneFindWidget(&theScene, "okDialogTitleText2");
		assert(sOkDialogTitleText1);

		sOkDialogTitleText2 = ituSceneFindWidget(&theScene, "okDialogTitleText1");
		assert(sOkDialogTitleText2);

		sDialogCntText = ituSceneFindWidget(&theScene, "DialogCntText");
		assert(sDialogCntText);
	}	
}

static void* sDialogMonitoringTaskFuntion(void* arg)
{	
	while(sDLsDialogMonitoring)
	{
		usleep( 1000*100 );	
		Dtime__Count %= 32;

		if(Dtime__Count == 20)
		{
		    /* Add for solving a Time Display Bug when Card Auth Menu	20190114 KT Lee */
			//GetTimeToString();			
		}
		Dtime__Count++;
		if(sDLsDialogMonitoring && (Dtime__Count != 0))

			if (sDTimerCounting)
			{				
				gettimeofday(&dettv, NULL);			
				DTopStart1 = dsttv.tv_sec;
				DTopeEnd1 = dettv.tv_sec;				
				dtotalvalue = DTopeEnd1 - DTopStart1;
		
				char buf[16];
				char buf1[16];
				
				if(doldtotalvalue != dtotalvalue)
				{
					sDTimerCount--;

					sprintf(buf, "%d", sDTimerCount);				
					memcpy(buf1,buf,sizeof(buf));
					ituTextSetString(sDialogCntText, buf1);						

				}
				doldtotalvalue = dtotalvalue;			
				
				if (sDTimerCount < 1)	{
					ituTextSetString(sDialogCntText, " ");
					sDLsDialogMonitoring = false;
					if (sOkDialogOkListener != NULL)
						(*sOkDialogOkListener)();
					sOkDialogOkListener = NULL;
					if(bErrorcheck1ShowHide)		OkDialogHide(CH1);
					if(bErrorcheck2ShowHide) 		OkDialogHide(CH2);						
				}
			} 	else {
					sDLsDialogMonitoring = false;
				}
	}
	sDialogMonitoringTask = 0;	

}

void DialogCloseTimer(void)
{
	ituTextSetString(sDialogCntText, ' ');	
	sDLsDialogMonitoring = false;
	sOkDialogOkListener = NULL;
}

void DialogSetTimer(int count)
{
	char buf[32];

	sprintf(buf, "%d", count);
	ituTextSetString(sDialogCntText, buf);

	sDTimerCount = count;
	//sDTimeoutListener = listener;
	sDTimerCounting = true;
	gettimeofday(&dsttv, NULL);
	printf("DialogSetTimer == sTimerId = SDL_AddTimer(1000, TopUpdateTimer, NULL) \n");
	sDLsDialogMonitoring = true;
	if (sDialogMonitoringTask == 0)
	{
		pthread_create(&sDialogMonitoringTask, NULL, sDialogMonitoringTaskFuntion, NULL);
		pthread_detach(sDialogMonitoringTask);
	}		
}

void EmergencyDialogShow(ChargeStep step)
{

	//if(bErrorcheck1ShowHide)	OkDialogHide(CH1);
	//if(bErrorcheck2ShowHide) 	OkDialogHide(CH2);

	TopPauseTimer();
	FindDialogWidget();
	switch (step)
	{
		case STEP_START:
		case STEP_AUTH_USER:
		case STEP_FINISH:
		case STEP_DISCONNECT:
		case STEP_THANKS:
		case STEP_REQ_CONNECT:
		case STEP_REQ_CLOSE:
		case STEP_CHARGE:
			ituTextSetString(sEmergencyDialogText1, STR_EMERGENCY_DIALOG_1);
			ituTextSetString(sEmergencyDialogText2, STR_EMERGENCY_DIALOG_2);
			ituTextSetString(sEmergencyDialogText3, STR_BLANK_STRING);
			break;

		case STEP_FTPFWUP:
			/// after error bit Check,
			// ituWidgetSetVisible(semergencyIcon, false);
			ituWidgetSetVisible(semergencyButtonIcon, false);
			// ituWidgetSetVisible(semergencyButtonDialogTitle, false);
			// ituTextSetString(semergencyButtonDialogTitle, STR_K1_UPDATE);
			// ituWidgetSetVisible(semergencyButtonDialogTitle, true);

			if(bFTPupgrade&FTP_FW_UD_ERR_MASK)
			{
				char temp[32] ={' ',};

				sprintf(temp, "%s [0x%x]", STR_SYSTEM_ERROR_01, bFTPupgrade);
				
				ituTextSetString(sEmergencyDialogText1, temp);
				ituTextSetString(sEmergencyDialogText2, STR_SYSTEM_RESET_01);
				ituTextSetString(sEmergencyDialogText3, STR_BLANK_STRING);
			}
			else if((bFTPupgrade&~FTP_FW_UD_ERR_MASK) == FTP_FW_UD_WAIT_CON)
			{
				ituTextSetString(sEmergencyDialogText1, STR_FTPFWUPDATE_WAIT_CON_01);
				ituTextSetString(sEmergencyDialogText2, STR_WAIT_MSG);
				ituTextSetString(sEmergencyDialogText3, STR_BLANK_STRING);
			}
			else if((bFTPupgrade&~FTP_FW_UD_ERR_MASK) == FTP_FW_UD_DOWNLOAD)
			{
				ituTextSetString(sEmergencyDialogText1, STR_FTPFWUPDATE_DOWNLOAD_01);
				ituTextSetString(sEmergencyDialogText2, STR_WAIT_MSG);
				ituTextSetString(sEmergencyDialogText3, STR_BLANK_STRING);
			}
			else if((bFTPupgrade&~FTP_FW_UD_ERR_MASK) == FTP_FW_UD_WRITE)
			{
				ituTextSetString(sEmergencyDialogText1, STR_K1_UPDATE_01);
				ituTextSetString(sEmergencyDialogText2, STR_WAIT_MSG);
				ituTextSetString(sEmergencyDialogText3, STR_BLANK_STRING);
			}
			else if((bFTPupgrade&~FTP_FW_UD_ERR_MASK) == FTP_FW_UD_WAIT_RST)
			{
				ituTextSetString(sEmergencyDialogText1, STR_SYSTEM_RESET_01);
				ituTextSetString(sEmergencyDialogText2, STR_WAIT_MSG);
				ituTextSetString(sEmergencyDialogText3, STR_BLANK_STRING);
			}
			printf(" EmergencyDialogShow \n");
			break;
			

		case STEP_REBOOT:
			/// after error bit Check,
			// ituWidgetSetVisible(semergencyIcon, false);
			ituWidgetSetVisible(semergencyButtonIcon, false);
			// ituWidgetSetVisible(semergencyButtonDialogTitle, false);

			ituTextSetString(sEmergencyDialogText1, STR_SYSTEM_RESET_01);
			ituTextSetString(sEmergencyDialogText2, STR_WAIT_MSG);
			ituTextSetString(sEmergencyDialogText3, STR_BLANK_STRING);
		
			printf(" EmergencyDialogShow : STEP_REBOOT \n");
			break;

		default: // start or finish??
			ituTextSetString(sEmergencyDialogText1, STR_BLANK_STRING);
			ituTextSetString(sEmergencyDialogText2, STR_BLANK_STRING);
			ituTextSetString(sEmergencyDialogText3, STR_BLANK_STRING);
			break;
	}

	ituWidgetSetVisible(sLdaBackground, true);
	ituWidgetSetVisible(sEmergencyDialog, true);

	ituWidgetSetVisible(sOkCancelDialog, false);
	ituWidgetSetVisible(sOkDialog, false);
	
	bEmbCheckShowHide = true;
}


void EmergencyDialogHide(void)
{
	FindDialogWidget();
	
	ituWidgetSetVisible(sLdaBackground, false);
	ituWidgetSetVisible(sEmergencyDialog, false);

	if(shmDataAppInfo.app_order == APP_ORDER_CONNECT_OUT) 
		TopSetTimer(90, TimeOutPageOut1);

	bEmbCheckShowHide = false;
}

void OkCancelDialogShow()
{
	FindDialogWidget();
	//ituWidgetSetVisible(sokCancelDialogTitleText1, false);
	ituWidgetSetVisible(sokCancelDialogTitleIcon1, true);
	ituWidgetSetVisible(sLdaBackground, true);
	ituWidgetSetVisible(sOkCancelDialog, true);
}

void OkCancelDialogShow_Cal()
{
	FindDialogWidget();
	ituWidgetSetVisible(sokCancelDialogTitleIcon1, true);
	//ituWidgetSetVisible(sokCancelDialogTitleText1, true);
	ituWidgetSetVisible(sLdaBackground, true);
	ituWidgetSetVisible(sOkCancelDialog, true);
}

void OkCancelDialogHide(void)
{
	FindDialogWidget();
	//ituWidgetSetVisible(sokCancelDialogTitleText1, false);
	ituWidgetSetVisible(sokCancelDialogTitleIcon1, false);
	ituWidgetSetVisible(sLdaBackground, false);
	ituWidgetSetVisible(sOkCancelDialog, false);
}

void OkCancelDialogSetOkListener(OkCancelListener oklistener, OkCancelListener cancellistener)
{
	sOkCancelDialogOkListener = oklistener;
	sOkCancelDialogCancelListener = cancellistener;	
}

bool OkCancelDialogOkOnPress(ITUWidget* widget, char* param)
{
	OkCancelDialogHide();

	if (sOkCancelDialogOkListener != NULL)
		(*sOkCancelDialogOkListener)();

	return true;
}

bool OkCancelDialogCancelOnPress(ITUWidget* widget, char* param)
{
	OkCancelDialogHide();

	if (sOkCancelDialogCancelListener != NULL)
		(*sOkCancelDialogCancelListener)();
	
	return true;
}

void OkDialogSetTitle(char *title, char *title_txt1, char *title_txt2)
{
	FindDialogWidget();
	ituTextSetString(sOkDialogTitle, title);
	ituTextSetString(sOkDialogTitleText1, title_txt1);
	ituTextSetString(sOkDialogTitleText2, title_txt2);
}

void OkDialogShow(bool bGfci, int ch)
{
	
	//printf("OkDialogShow : %d => %d, %d \n", bGfci, bErrorcheck1ShowHide, bErrorcheck2ShowHide);
	if(!EmgControl) // _dsAn 200228 EMB not pressed 
	{
		if( !bGfci )
		{
			if((theConfig.devtype == BC_TYPE || theConfig.devtype == HBC_TYPE))
			{
				bErrorcheck1ShowHide = true;
				bErrorcheck2ShowHide = true;
			}			
			else if(ch == CH1)		bErrorcheck1ShowHide = true;
			else if(ch == CH2)	bErrorcheck2ShowHide = true;			
		}
		
		FindDialogWidget();
		ituWidgetSetVisible(sLdaBackground, true);
		ituWidgetSetVisible(sOkDialog, true);
	}
}

void ShowWhmErrorDialogBox(unsigned short Ecode)
{
	char title[32], title_txt1[40], title_txt2[40];

	if(shmDataAppInfo.app_order == APP_ORDER_CHARGING)
		TSCT_ChargingStop();

	sprintf(title, "CODE %2d", Ecode);

	if(Ecode == PAY_ERR){
		if(theConfig.OperationMode == OP_FREE_MODE){
			sprintf(title_txt1, FREEFAIL_ERR_TXT1);
			sprintf(title_txt2, FREEFAIL_ERR_TXT2);
			OkDialogSetTitle(title, title_txt1, title_txt2);
			OkDialogShow(false, 0);
			DialogSetTimer(OKDIALOG_TIMEOUT);
			TopCloseTimer();
			GotoStartLayer();
			usleep(100*1000);	
		}else{
			sprintf(title_txt1, BILFAIL_ERR_TXT1);
			sprintf(title_txt2, BILFAIL_ERR_TXT2);
			OkDialogSetTitle(title, title_txt1, title_txt2);
			OkDialogShow(false, 0);
			DialogSetTimer(OKDIALOG_TIMEOUT);
			TopCloseTimer();
			GotoStartLayer();
			usleep(100*1000);
		}
		
		//DialogGotoStartLayer();
	} else if(Ecode == ERR_TOUCH || Ecode == ERR_RFID || Ecode == ERR_AMI) {
		sprintf(title_txt1, REBOOT_ERR_TXT1);
		sprintf(title_txt2, REBOOT_ERR_TXT2);
		OkDialogSetTitle(title, title_txt1, title_txt2);
		OkDialogShow(false, 0);
		DialogSetTimer(OKDIALOG_TIMEOUT);
	}	else {
		sprintf(title_txt1, CONNECT_ERR_TXT1);
		sprintf(title_txt2, CONNECT_ERR_TXT2);
		OkDialogSetTitle(title, title_txt1, title_txt2);
		OkDialogShow(false, 0);
		DialogSetTimer(OKDIALOG_TIMEOUT);
	}
	// OkDialogSetTitle(title, title_txt1, title_txt2);
	// OkDialogShow(false, 0);
	// DialogSetTimer(OKDIALOG_TIMEOUT);
}

void OkDialogHide(int ch)
{	
	if(!EmgControl) // _dsAn 200228 EMB not pressed 
	{
		if(ch == CH1 && bErrorcheck1ShowHide)
		{
			bErrorcheck1ShowHide = false;
			FindDialogWidget();
			ituWidgetSetVisible(sLdaBackground, false);
			ituWidgetSetVisible(sOkDialog, false);
		}
		
		if(ch == CH2 && bErrorcheck2ShowHide)
		{
			bErrorcheck2ShowHide = false;
			FindDialogWidget();
			ituWidgetSetVisible(sLdaBackground, false);
			ituWidgetSetVisible(sOkDialog, false);
		}
		
		if((theConfig.devtype == BB_TYPE) || (theConfig.devtype == BC2_TYPE))
		{
			if(bErrorcheck2ShowHide && bAmiErrChk)
				ShowWhmErrorDialogBox(ERR_AMI_DISCON);
		}
		
		if(bErrorcheck1ShowHide && bAmiErrChk)
			ShowWhmErrorDialogBox(ERR_AMI_DISCON);
	}
}


void OkDialogSetOkListener(OkCancelListener oklistener)
{
	sOkDialogOkListener = oklistener;
}


bool OkDialogOkOnPress(ITUWidget* widget, char* param)
{	
	if(bErrorcheck1ShowHide)		OkDialogHide(CH1);
	if(bErrorcheck2ShowHide) 		OkDialogHide(CH2);

	if (sOkDialogOkListener != NULL)	//	OkDialogSetOkListener Call X
		(*sOkDialogOkListener)();

	DialogCloseTimer();
	
	/*
	if(shmDataAppInfo.app_order == APP_ORDER_CHARGING){
		TSCT_ChargingStop();
	}
	*/

	return true;
}


void DialogReset(void)
{
	sLdaBackground = NULL;
}

