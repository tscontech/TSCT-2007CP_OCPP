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
#include "lwip/ip.h"
#include "ite/itp.h"

#include "SDL/SDL.h"

//////ping 
#include "lwip/opt.h"
#include "cstping.h"
#include "lwip/mem.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/timers.h"
#include "lwip/inet_chksum.h"
/////ping

static SDL_TimerID sTimerAdmintest;

bool Amiteststart1;
bool Amiteststart2;
bool PingTeststart;
bool bMc1Check1;
bool bMc2Check1;
bool bDoor1Check1;
bool bDoor2Check1;
bool bLed1Check1;
bool bBackLightCheck;
bool pressed1;
bool temprfButtonCheck;

static ITUBackground* sLatBackground;

static ITUBackground* sRfReader_Background;
static ITUTextBox* sRfStatustxtbx;
static ITUTextBox* sAmi1Statustxtbx;
static ITUTextBox* sAmi2Statustxtbx;
static ITUButton *sAmi2Startbtn;
static ITUText *sPingStatustxtbx;

static ITUBackground* sI_oTestBackground;
static ITUText* sAdminMc1txt;

static ITUText* sAdminLed1txt;
static ITUText* sstat_led1;
static ITUText* sAdminMc2txt;

static ITUButton *sbtn_mc2;

static ITUText* sTestLayer_PwrTxt;

static pthread_t GetAMIVMonitorTask;

bool bGetAMIVMonitorTaskFlg = false;

extern bool bAmiErrChk;

void adminTimeout1(void)
{
	AdminSetupMenuExit(true);
}

static Uint32 AdminSetTestTimer(Uint32 interval, void *param)
{
	SDL_RemoveTimer(sTimerAdmintest);
	
	bool open, close;
		bool bMc1Check11;
		bool bMc2Check11;
	if(adminsettestselect == 1)
	{
		if(rfredercheck == false && temprfButtonCheck == true)
		{
			ituTextSetString(sRfStatustxtbx, "OK");
			temprfButtonCheck = false;
		}
		else if(temprfButtonCheck == true && rfredercheck == true)
		{
			ituTextSetString(sRfStatustxtbx, "FAIL");
			temprfButtonCheck = false;
		}

		if(!bAmiErrChk && Amiteststart2)
		 	ituTextSetString(sAmi2Statustxtbx, "OK");
		else if(Amiteststart2)ituTextSetString(sAmi2Statustxtbx, "FAIL");
		else ituTextSetString(sAmi2Statustxtbx, "");

		if(!bAmiErrChk && Amiteststart1)
			ituTextSetString(sAmi1Statustxtbx, "OK");
		else if(Amiteststart1)ituTextSetString(sAmi1Statustxtbx, "FAIL");
		else ituTextSetString(sAmi1Statustxtbx, "");		
		
		//ping
		if(PingTeststart == true && pingtestcheck == true)
		{
			ituTextSetString(sPingStatustxtbx, "OK");
			PingTeststart = false;
		}
		else if(PingTeststart == true && pingtestcheck == false)
		{
			ituTextSetString(sPingStatustxtbx, "FAIL");
			PingTeststart = false;
		}
	}
	
	if(adminsettestselect == 2)
	{	
		ithGpioSetMode(GPIO_EMBUTTON, ITH_GPIO_MODE0);
		ithGpioSetIn(GPIO_EMBUTTON);    
		pressed1 = !ithGpioGet(GPIO_EMBUTTON);

	}
	sTimerAdmintest = SDL_AddTimer(400, AdminSetTestTimer, NULL);	
	return(interval);
}

static void FindIpAllDialogWidget4(void)
{	
	if (!sLatBackground)
	{
		sLatBackground = ituSceneFindWidget(&theScene, "ChargetTestLayerBackground");
		assert(sLatBackground);
		
		sRfReader_Background = ituSceneFindWidget(&theScene, "RfReader_Background");
		assert(sRfReader_Background);
		sRfStatustxtbx = ituSceneFindWidget(&theScene, "RfStatustxtbx");
		assert(sRfStatustxtbx);		
		sAmi1Statustxtbx = ituSceneFindWidget(&theScene, "Ami1Statustxtbx");
		assert(sAmi1Statustxtbx);	
		sAmi2Statustxtbx = ituSceneFindWidget(&theScene, "Ami2Statustxtbx");
		assert(sAmi2Statustxtbx);
		sPingStatustxtbx = ituSceneFindWidget(&theScene, "PingStatustxtbx");
		assert(sPingStatustxtbx);		
		
		sAmi2Startbtn = ituSceneFindWidget(&theScene, "Ami2Startbtn");
		assert(sAmi2Startbtn);
				
		sI_oTestBackground = ituSceneFindWidget(&theScene, "I_oTestBackground");
		assert(sI_oTestBackground);

		sAdminMc1txt = ituSceneFindWidget(&theScene, "AdminMc1txt");
		assert(sAdminMc1txt);		
	
		sAdminLed1txt = ituSceneFindWidget(&theScene, "AdminLed1txt");
		assert(sAdminLed1txt);	
		sstat_led1 = ituSceneFindWidget(&theScene, "stat_led1");
		assert(sstat_led1);	
		sAdminMc2txt = ituSceneFindWidget(&theScene, "AdminMc2txt");
		assert(sAdminMc2txt);

		sbtn_mc2 = ituSceneFindWidget(&theScene, "btn_mc2");
		assert(sbtn_mc2);

		sTestLayer_PwrTxt = ituSceneFindWidget(&theScene, "TestLayer_PwrTxt");
		assert(sTestLayer_PwrTxt);	
	}
}

static void* GetAMIVMonitorTaskFuntion(void* arg)
{
	char temp[32] = {0,};
	int AMIValue_H, AMIValue_L;;

	while(bGetAMIVMonitorTaskFlg)
	{
		AMIValue_H = GetAMIValue() / 100;
		AMIValue_L = GetAMIValue() % 100;

		memset(temp, 0, 32); 
		sprintf(temp, "%d.%02d kWh", AMIValue_H, AMIValue_L);

		ituTextSetString(sTestLayer_PwrTxt, temp);	
		usleep(500*1000);
	}
}

bool AdmintestEnter(ITUWidget* widget, char* param)
{	
	switch(adminsettestselect)
	{		
		case 1:
		{//
			FindIpAllDialogWidget4();	
			ituWidgetSetVisible(sLatBackground, true);
			ituWidgetSetVisible(sRfReader_Background, true);
			
			ituTextSetString(sRfStatustxtbx, "");
			ituTextSetString(sAmi1Statustxtbx, "");
			ituTextSetString(sPingStatustxtbx, "");
			
			
			ituWidgetSetVisible(sRfStatustxtbx, true);
			ituWidgetSetVisible(sAmi1Statustxtbx, true);
			ituWidgetSetVisible(sPingStatustxtbx, true);
						
			ituTextSetString(sAmi2Statustxtbx, "");			
			ituWidgetSetVisible(sAmi2Statustxtbx, true);
			ituWidgetSetVisible(sAmi2Startbtn, true);	

			break;
		}
		case 2:
		{//			
			FindIpAllDialogWidget4();
			ituWidgetSetVisible(sLatBackground, true);
			ituWidgetSetVisible(sI_oTestBackground, true);		
		
			ituWidgetSetVisible(sAdminMc1txt, true);

			ituWidgetSetVisible(sAdminMc2txt, true);

			break;
		}
		
	}
	if(GetAMIVMonitorTask == NULL){
		bGetAMIVMonitorTaskFlg = true;
		pthread_create(&GetAMIVMonitorTask, NULL, GetAMIVMonitorTaskFuntion, NULL);
		pthread_detach(GetAMIVMonitorTask);
	}

	ituTextSetString(sRfStatustxtbx, "");

	TopCloseTimer();

	sTimerAdmintest = SDL_AddTimer(1000, AdminSetTestTimer, NULL);
    return true;
}

bool AdmintestLeave(ITUWidget* widget, char* param)
{
	SDL_RemoveTimer(sTimerAdmintest);
	
	MagneticContactorOff();

	bGetAMIVMonitorTaskFlg = false;

	Amiteststart1 = false;
	Amiteststart2 = false;
	bMc1Check1 = false;
	bMc2Check1 = false;

	bDoor1Check1 = false;
	bDoor2Check1 = false;
	bLed1Check1 = false;
	
	pressed1 = false;
	temprfButtonCheck = false;
	PingTeststart = false;
	pingtestcheck = false;
	switch(adminsettestselect)
	{
		case 1:
		{//
			ituWidgetSetVisible(sLatBackground, false);
			ituWidgetSetVisible(sRfReader_Background, false);
			ituWidgetSetVisible(sRfStatustxtbx, false);
			ituWidgetSetVisible(sAmi1Statustxtbx, false);
			ituWidgetSetVisible(sAmi2Statustxtbx, false);
			ituWidgetSetVisible(sPingStatustxtbx, false);	

			break;
		}
		case 2:
		{//			
			ituWidgetSetVisible(sLatBackground, false);
			ituWidgetSetVisible(sI_oTestBackground, false);
			
			ituWidgetSetVisible(sAdminMc1txt, false);

			ituWidgetSetVisible(sAdminMc2txt, false);

			break;
		}
	}
	adminsettestselect = 0;

	TopSetTimer(180, adminTimeout1);

    return true;
}
static void CardReaderListenerOnAuthUser1(char *data, int size)
{
	if(size > 0)
		ituTextSetString(sRfStatustxtbx, "O K");	
#ifdef SAMWONFA_CD_TERMINAL
	SamwonfaStopMonitoring();
#else	
	CardReaderStopMonitoring();	
#endif
}

bool exit4Layer(ITUWidget* widget, char* param)
{
	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminChargerTestLayer"));
    return true;
}

bool FullTestStartOnPress(ITUWidget* widget, char* param)
{
    return true;
}

bool EmgteststartOnPress(ITUWidget* widget, char* param)
{
    return true;
}
bool Mc1teststartOnPress(ITUWidget* widget, char* param)
{
	if(!bMc1Check1 && !bMc2Check1)
	{	
		ituTextSetString(sAdminMc1txt, "ON");	
		MagneticContactorOn();
		bMc1Check1 = true;
	}
	else	
	{
		ituTextSetString(sAdminMc1txt, "OFF");	
		MagneticContactorOff();	
		bMc1Check1 = false;;	
	}
    return true;
}

bool Led1teststartOnPress(ITUWidget* widget, char* param)
{
	if(!bLed1Check1)
	{	
		ituTextSetString(sstat_led1, "ON");			
		ituTextSetString(sAdminLed1txt, "ON");
		LEDOn();
		bLed1Check1 = true;
	}
	else	
	{
		ituTextSetString(sstat_led1, "OFF");
		ituTextSetString(sAdminLed1txt, "OFF");		
		LEDOff();			
		bLed1Check1 = false;;	
	}
    return true;
}

bool Door1teststartOnPress(ITUWidget* widget, char* param)
{
	// ituTextSetString(sAdminDoor1txt, "ON");	
	// DoorLockOpen(CH1);	
	return true;
}

bool Mc2teststartOnPress(ITUWidget* widget, char* param)
{

    return true;
}

bool Door2teststartOnPress(ITUWidget* widget, char* param)
{
	// ituTextSetString(sAdminDoor2txt, "ON");	
	// DoorLockOpen(CH2);
    return true;
}


bool Ami2Test_OnPress(ITUWidget* widget, char* param)
{//why Fail ????
	Amiteststart2 = true;
	// WattHourMeterCheck(1);
    return true;
}


bool Ami1Test_OnPress(ITUWidget* widget, char* param)
{
	Amiteststart1 = true;
	// WattHourMeterCheck(0);
    return true;
}

bool PingTest_OnPress(ITUWidget* widget, char* param)
{
#ifndef EN_CERTI
	if(iteEthGetLink())		ping_init();
	else{
		pingtestcheck = false;		
	}
#endif	
	PingTeststart = true;
	
    return true;
}

bool RfReaderTest_OnPress(ITUWidget* widget, char* param)
{
	if(RFIDCardReaderCheck())
		temprfButtonCheck = false;
	else
		temprfButtonCheck = true;

    return true;
}

bool BacklightOff_OnPress(ITUWidget* widget, char* param)
{
	if(!bBackLightCheck){
		BacklightOff();
		bBackLightCheck = true;
	}
	else{
		BacklightOn();
		bBackLightCheck = false;
	}
	//ioctl(ITP_DEVICE_BACKLIGHT, ITP_IOCTL_OFF, NULL);
    return true;
}