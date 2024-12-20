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
#include "cststring.h"
#include "tsctcfg.h"

static ITUBackground* sBackground2;

//static ITUButton *sChargerId2btn;
static ITUText* sStationIdtxt;
static ITUText* sdevidtxt1;
//static ITUText* sChargerId22txt;
static ITUText* sLcdOnOffTimetxt;
static ITUText* sPassWordChangetxt;
static ITUText* sSoundtxt;
static ITUText* sDevertxt;
static ITUText* sModetxt;

// static ITUBackground* ssetting_popup1;
// static ITUBackground* ssetting_popup2;

// static ITUText* spage_text1;
// static ITUText* spage_text2;

// 2 page admin 
// static ITUText* sLatitudePtxt;
// static ITUText* sLongitudetxt;
// static ITUText* sMacaddrtxt;
// static ITUText* sChargerIDtxt;
// static ITUText* sChargerIDtxt2;
// static ITUText* sdevidtxt2;


bool adminsetchargerEnter(ITUWidget* widget, char* param)
{
	adminsetchargerselect1 = 0;	
	adminsetchargerselect2 = 0;
	bGloAdminStatus = true;
	
	if (!sBackground2)
	{
		sBackground2 = ituSceneFindWidget(&theScene, "AdminSetchargerBackground");
		assert(sBackground2);

		sStationIdtxt = ituSceneFindWidget(&theScene, "StationIdtxt");
		assert(sStationIdtxt);
		
		sdevidtxt1 = ituSceneFindWidget(&theScene, "devidtxt1");
		assert(sdevidtxt1);

		//sChargerId22txt = ituSceneFindWidget(&theScene, "ChargerId22txt");
		//assert(sChargerId22txt);

		//sChargerId2btn = ituSceneFindWidget(&theScene, "ChargerId2btn");
        //assert(sChargerId2btn);
			
		sLcdOnOffTimetxt = ituSceneFindWidget(&theScene, "LcdOnOffTimetxt");
		assert(sLcdOnOffTimetxt);
		
		sPassWordChangetxt = ituSceneFindWidget(&theScene, "PassWordChangetxt");
		assert(sPassWordChangetxt);

		sSoundtxt = ituSceneFindWidget(&theScene, "Soundtxt");
		assert(sSoundtxt);		

		sDevertxt = ituSceneFindWidget(&theScene, "Devertxt");
		assert(sDevertxt);	
		
		sModetxt = ituSceneFindWidget(&theScene, "Modetxt");
		assert(sModetxt);	

		// sChargerIDtxt = ituSceneFindWidget(&theScene, "ChargerIDtxt");
		// assert(sChargerIDtxt);	

		// sChargerIDtxt2 = ituSceneFindWidget(&theScene, "ChargerIDtxt2");
		// assert(sChargerIDtxt2);	

		// sdevidtxt2 = ituSceneFindWidget(&theScene, "devidtxt2");
		// assert(sdevidtxt2);	
	}
	
	ituWidgetSetVisible(sBackground2,true);	
	//ituWidgetSetVisible(sChargerId2btn, false);

	// if((theConfig.devtype == BB_TYPE)||(theConfig.devtype == BC2_TYPE))
	// {
	// 	ituWidgetSetVisible(sChargerIDtxt2, true);
	// 	ituWidgetSetVisible(sdevidtxt2, true);
	// }
	// else
	// {
	// 	ituWidgetSetVisible(sChargerIDtxt2, false);
	// 	ituWidgetSetVisible(sdevidtxt2, false);
	// }

	char buf[9]; char buf1[13];
	snprintf(buf, "%s", theConfig.siteid,8);

	ituTextSetString(sStationIdtxt, buf);
	
	memset(buf,0x00,sizeof(buf));
	sprintf(buf, "%c%c", theConfig.devid1[0], theConfig.devid1[1]);
	ituTextSetString(sdevidtxt1, buf);

	memset(buf,0x00,sizeof(buf));
	sprintf(buf, "%02d", theConfig.audiolevel);
	ituTextSetString(sSoundtxt, buf);

	memset(buf1,0x00,sizeof(buf1));
	sprintf(buf1, TSCT_DEVVERSION);
	ituTextSetString(sDevertxt, buf1);

	if(theConfig.OperationMode == OP_CHECK_MODE)
		ituTextSetString(sModetxt, "점검");
	else if(theConfig.OperationMode == OP_FREE_MODE)
		ituTextSetString(sModetxt, "무료");
	else	
		ituTextSetString(sModetxt, "일반");

	// if((theConfig.devtype == BB_TYPE)||(theConfig.devtype == BC2_TYPE))
	// {
	// 	memset(buf,0x00,sizeof(buf));
	// 	sprintf(buf, "%c%c", theConfig.devid2[0], theConfig.devid2[1]);
	// 	ituTextSetString(sdevidtxt2, buf);
	// }
#if 0 //protocol version
	memset(buf,0x00,sizeof(buf));
	sprintf(buf, "%s", "Protocol Ver");
	ituTextSetString(sAdminSetChargerLayer_txt_proto, buf);

	ituTextSetString(sChargerId22txt, SW_VERSION);
#else
	//ituWidgetSetVisible(sChargerId22txt, false);
#endif

	memset(buf,0x00,sizeof(buf));	
	sprintf(buf, "%d%s", theConfig.screensaver_time, STR_TIME_MIN);	
	ituTextSetString(sLcdOnOffTimetxt, buf);	
	
	memset(buf,0x00,sizeof(buf));
	snprintf(buf, "%s", theConfig.adminpassword, 4);
	ituTextSetString(sPassWordChangetxt, buf);
 
	return true;
}

bool adminsetchargerLeave(ITUWidget* widget, char* param)
{
	ituWidgetSetVisible(sBackground2,false);	
    return true;
}

bool SelectDevSetFunc(ITUWidget* widget, char* param)
{
	adminsetchargerselect1 = atoi(param);
	printf("SelectDevSetFunc :: %d \n", adminsetchargerselect1);
	ituLayerGoto(ituSceneFindWidget(&theScene, "ChargerLayer"));	
    return true;
}

bool SetCharger2SelectItem(ITUWidget* widget, char* param)
{	
	adminsetchargerselect2 = atoi(param);
   ituLayerGoto(ituSceneFindWidget(&theScene, "Charger2Layer"));
    return true;
}


bool exitaLayer(ITUWidget* widget, char* param)
{
	AdminSetupMenuExit(true);
    return true;
}

bool SetChargerSubMenu(ITUWidget* widget, char* param)
{
	ChangeSetupSubMenu(atoi(param));
    return true;
}
