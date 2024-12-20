/**
*       @file
*               layer_user_auth.c
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2019.05.21 <br>
*               author: dsAn <br>
*               description: modify <br>
*/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "SDL/SDL.h"
#include "scene.h"
#include "ctrlboard.h"

#include "ite/itp.h"

// #include "audio_mgr.h"
// #include <pthread.h>
// #include <sys/ioctl.h>
// #include "i2s/i2s.h"


//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------
#define CARD_WAIT_TIMEOUT	30
//#define CARD_WAIT_TIMEOUT 10
//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------
static ITUBackground* sBackground;

static ITUButton* sMemberCardBtn;
static ITUButton* sMemberNumBtn;
static ITUButton* sMemberQrBtn;

bool AuthUserOnEnter(ITUWidget* widget, char* param)
{
	// CstPlayAudioMsg(AUDIO_SELECTAUTHTYPE);

	AudioPlay("A:/sounds/selectAuthtype.wav", NULL);

    // const char *filename = "a:/sounds/opening.wav";
    // AudioPlayMusic(filename,NULL);

	// char filepath[256];
	// smtkAudioMgrSetVolume(theConfig.keylevel);
	// strcpy(filepath, CFG_PRIVATE_DRIVE ":/sounds/");
	// strcat(filepath, "opening.wav");
	// AudioPlay(filepath, NULL);

	printf("\n-----audio played-----\n");

	CtLogRed("Enter auth_user layer..\n");
	
	ChannelType activeCh = CstGetUserActiveChannel();
	
	shmDataIfInfo.card_auth = CARD_AUTH_WAIT;
	shmDataAppInfo.app_order = APP_ORDER_AUTH_METHOD; 
	shmDataAppInfo.payment_type = 0x01; 
	
	TopGotoAuthUserStep();

    if (!sBackground)
    {
        sBackground = ituSceneFindWidget(&theScene, "authCardBackground");
        assert(sBackground);

        sMemberCardBtn = ituSceneFindWidget(&theScene, "MemberCardBtn");
        assert(sMemberCardBtn);
		
        sMemberNumBtn = ituSceneFindWidget(&theScene, "MemberNumBtn");
        assert(sMemberNumBtn);		

		sMemberQrBtn = ituSceneFindWidget(&theScene, "MemberQrBtn");
		assert(sMemberQrBtn);
    }

	// memset(shmDataAppInfo.card_no, 0x00, sizeof(shmDataAppInfo.card_no));

	ituWidgetSetVisible(sMemberCardBtn, true);

	ituWidgetSetVisible(sMemberNumBtn, true);

	ituWidgetSetVisible(sMemberQrBtn, true);

	//TopHomeBtnVisible(true);
	TopBackBtnVisible(true);

	TopSetTimer(CARD_WAIT_TIMEOUT, GotoStartLayer);	
	//sleep(10);

	//FtpIMGUpdate_func();

    return true;
}


bool AuthUserOnLeave(ITUWidget* widget, char* param)
{
	CtLogRed("Leave auth_user layer\n");
	TopCloseTimer();	
	TopStopStepAnimation();

	usleep(100*1000);	// for Enter Main

    return true;
}

bool AuthTypeSelectOnPress(ITUWidget* widget, char* param)
{
	ChannelType activeCh = CstGetUserActiveChannel();
	int nSel = atoi(param);

	if(!GetMeterCon())
	{
		ShowWhmErrorDialogBox(ERR_AMI_DISCON);
		return true;
	}
	// if(!GetServerCon())
	// {
	// 	ShowWhmErrorDialogBox(ERR_SERVER_DISCON);
	// 	return true;
	// }

	switch(nSel)
	{
		case 1:
			shmDataAppInfo.auth_type[0] = INPUT_CARDNUM;
			ituLayerGoto(ituSceneFindWidget(&theScene, "cardNumLayer"));
			break;
	}

	return true;
}

bool MemCardOnPress(ITUWidget* widget, char* param)
{
	ChannelType activeCh = CstGetUserActiveChannel();
	shmDataAppInfo.auth_type[0] = TOUCH_CARD;
	
	if(!GetMeterCon())
	{
		ShowWhmErrorDialogBox(ERR_AMI_DISCON);
		return true;
	}
	// if(!GetServerCon())
	// {
	// 	ShowWhmErrorDialogBox(ERR_SERVER_DISCON);
	// 	return true;
	// }

	ituLayerGoto(ituSceneFindWidget(&theScene, "rfidCardAuthLayer"));
	return true;
}

bool RemoteAuthOnPress(ITUWidget* widget, char* param)
{
	// ChannelType activeCh = CstGetUserActiveChannel();
	// //shmDataAppInfo.auth_type[0] = SERVER_AUTH;
	// shmDataAppInfo.auth_type[0] = CONNECT_AUTH;
	
	// if(!GetMeterCon())
	// {
	// 	ShowWhmErrorDialogBox(ERR_AMI_DISCON);
	// 	return true;
	// }
	// if(!GetServerCon())
	// {
	// 	ShowWhmErrorDialogBox(ERR_SERVER_DISCON);
	// 	return true;
	// }

	// ituLayerGoto(ituSceneFindWidget(&theScene, "connectLayer"));
	return true;
}

void AuthUserReset(void)
{
	sBackground = NULL;
}

