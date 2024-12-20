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

static ITUBackground* sLclBackground;

static ITUText* sAdminVersiontxt;
static ITUText* sAdminModeltxt;
static ITUText*	sTextFwUpdate;
static ITUButton* sSetUserCardButton;

static ITUText* stext_setServer;
static ITUButton* sinstall_setServer;

static ITUText* sAdminModelTitleText;
static ITUText* sRegisteSetupPointText;

static ITUText* sInfoPrice1Text;
static ITUText* sInfoPrice2Text;
static ITUText* sInfoPrice3Text;

void press_Send_h1(void)
{
}

bool AdminchargerlistEnter(ITUWidget* widget, char* param)
{
	printf(" AdminchargerlistEnter ... \n");
	if (!sLclBackground)
	{
		sLclBackground = ituSceneFindWidget(&theScene, "AdminSetListBackground");
		assert(sLclBackground);
		
		sAdminVersiontxt = ituSceneFindWidget(&theScene, "AdminVersiontxt");
		assert(sAdminVersiontxt);

//		sAdminModelTitleText = ituSceneFindWidget(&theScene, "AdminModelTitleText");
//		assert(sAdminModelTitleText);

		sAdminModeltxt = ituSceneFindWidget(&theScene, "AdminModeltxt");
		assert(sAdminModeltxt);

//		sTextFwUpdate = ituSceneFindWidget(&theScene, "TextFwUpdate");
//		assert(sTextFwUpdate);

		stext_setServer = ituSceneFindWidget(&theScene, "text_setServer");
		assert(stext_setServer);

		sinstall_setServer = ituSceneFindWidget(&theScene, "install_setServer");
		assert(sinstall_setServer);
		
		sRegisteSetupPointText = ituSceneFindWidget(&theScene, "RegisteSetupPointText");
		assert(sRegisteSetupPointText);		

		sSetUserCardButton = ituSceneFindWidget(&theScene, "SetUserCardButton");
		assert(sSetUserCardButton);

		sInfoPrice1Text = ituSceneFindWidget(&theScene, "InfoPrice1Text");
		assert(sInfoPrice1Text);

		sInfoPrice2Text = ituSceneFindWidget(&theScene, "InfoPrice2Text");
		assert(sInfoPrice2Text);

		sInfoPrice3Text = ituSceneFindWidget(&theScene, "InfoPrice3Text");
		assert(sInfoPrice3Text);
	}
	
	ituWidgetSetVisible(sLclBackground,true);	

//	ituWidgetSetVisible(sAdminModelTitleText,true);	

	char info_price_buf[6];
	unsigned int info_price_tmp;

	if(theConfig.ConfirmSelect == USER_AUTH_NET){
		memset(info_price_buf, 0, sizeof(info_price_buf));
		info_price_tmp = FourByteOrder(&shmDataIfInfo.unit_cost[0][0][0]);
		if(info_price_tmp == 0)
			ituTextSetString(sInfoPrice1Text, "-");
		else{
			sprintf(info_price_buf, "%d", info_price_tmp);
			ituTextSetString(sInfoPrice1Text, info_price_buf);
		}
		
		memset(info_price_buf, 0, sizeof(info_price_buf));
		info_price_tmp = FourByteOrder(&shmDataIfInfo.unit_cost[1][0][0]);
		if(info_price_tmp == 0)
			ituTextSetString(sInfoPrice2Text, "-");
		else{
			sprintf(info_price_buf, "%d", info_price_tmp);
			ituTextSetString(sInfoPrice2Text, info_price_buf);
		}

		memset(info_price_buf, 0, sizeof(info_price_buf));
		info_price_tmp = FourByteOrder(&shmDataIfInfo.unit_cost[2][0][0]);
		if(info_price_tmp == 0)
			ituTextSetString(sInfoPrice3Text, "-");
		else{
			sprintf(info_price_buf, "%d", info_price_tmp);
			ituTextSetString(sInfoPrice3Text, info_price_buf);
		}
	}
	else{
		info_price_tmp = DEFAULT_UNITPRICE;
		memset(info_price_buf, 0, sizeof(info_price_buf));		
		sprintf(info_price_buf, "%d", info_price_tmp);
		ituTextSetString(sInfoPrice1Text, info_price_buf);
		ituTextSetString(sInfoPrice2Text, "-");
		ituTextSetString(sInfoPrice3Text, "-");
	}


	ituWidgetSetVisible(stext_setServer,false);	
	ituWidgetSetVisible(sinstall_setServer,false);	
	ituWidgetSetVisible(sRegisteSetupPointText,false);		
	ituWidgetSetVisible(sSetUserCardButton,false);	

	ituTextSetString(sAdminVersiontxt, SW_VERSION);
	//ituTextSetString(sAdminVersiontxt, CERTI_SW_VERSION); //certification version
	ituTextSetString(sAdminModeltxt, StrModelName);		
    return true;
}

bool AdminchargerlistLeave(ITUWidget* widget, char* param)
{
	ituWidgetSetVisible(sLclBackground,false);
    return true;
}

bool exitdLayer(ITUWidget* widget, char* param)
{
	AdminSetupMenuExit(true);
	return true;
}

bool ChargerListSubMenu(ITUWidget* widget, char* param)
{
	ChangeSetupSubMenu(atoi(param));
    return true;
}


static char nTouchCnt = 0;
bool ChkChangeDevType(ITUWidget* widget, char* param)
{
	nTouchCnt++;
	printf("ChkChangeDevType :: touch cnt %d \n", nTouchCnt);
	if(nTouchCnt > 4)
	{
		nTouchCnt = 0;	
		ituWidgetSetVisible(stext_setServer,false);	
		ituWidgetSetVisible(sinstall_setServer,false);	
		ituWidgetSetVisible(sRegisteSetupPointText,false);	

		ituWidgetSetVisible(sAdminVersiontxt,false);	
		ituWidgetSetVisible(sAdminModelTitleText,false);	
		ituWidgetSetVisible(sAdminModeltxt,false);	

		ituWidgetSetVisible(sTextFwUpdate,false);	
		ituWidgetSetVisible(sLclBackground,false);	
		
		ituLayerGoto(ituSceneFindWidget(&theScene, "ChangeDevType"));
	}
	return true;
}

void FtpFWupdate(ITUWidget* widget, char* param)
{
	ituLayerGoto(ituSceneFindWidget(&theScene, "FtpFwUpdate"));
}

void GoUserCardManage(ITUWidget* widget, char* param)
{
#if defined(sTraffic_Server)
	ituLayerGoto(ituSceneFindWidget(&theScene, "FtpFwUpdate"));
#endif
}

void CancelButtonOnPress(ITUWidget* widget, char* param)
{
	ituLayerGoto(ituSceneFindWidget(&theScene, "TestCancelLayer"));
}

