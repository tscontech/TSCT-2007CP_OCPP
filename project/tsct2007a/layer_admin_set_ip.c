#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lwip/ip.h"
#include "ite/itp.h"
#include "ctrlboard.h"
#include "scene.h"

static ITUBackground* sBackground;
static ITUText* sServerIPtxt;
static ITUText* sServerporttxt;
static ITUText* schargeriptxt;
static ITUText* schargergwtxt;
static ITUText* schargersmtxt;
static ITUText* schargermatxt;
static ITUText* sDhcptxt;

static bool bChangedNetInfo = false;

static bool bDHCPStatus = true;

static bool bTimeoutCnt = false;

void adminTimeout(void)
{
	AdminSetupMenuExit(true);
}


bool adminsetipEnter(ITUWidget* widget, char* param)
{
	bGloAdminStatus = true;
	setipselect = 0;
	if (!sBackground)
	{
		sBackground = ituSceneFindWidget(&theScene, "AdminSetIpBackground");
		assert(sBackground);
		
		sServerIPtxt = ituSceneFindWidget(&theScene, "ServerIPtxt");
		assert(sServerIPtxt);
		
		sServerporttxt = ituSceneFindWidget(&theScene, "Serverporttxt");
		assert(sServerporttxt);
		
		schargeriptxt = ituSceneFindWidget(&theScene, "chargeriptxt");
		assert(schargeriptxt);
		
		schargergwtxt = ituSceneFindWidget(&theScene, "chargergwtxt");
		assert(schargergwtxt);
		
		schargersmtxt = ituSceneFindWidget(&theScene, "chargersmtxt");
		assert(schargersmtxt);	

		schargermatxt = ituSceneFindWidget(&theScene, "Macaddrtxt");
		assert(schargermatxt);	
		
		sDhcptxt = ituSceneFindWidget(&theScene, "Dhcptxt");
		assert(sDhcptxt);	
	}
	
	ituWidgetSetVisible(sBackground,true);
	
	char buf[MAX_ADDRESS];
	char buf_mac[18];



	snprintf(buf, "%s", theConfig.serverip, sizeof(theConfig.serverip));
	ituTextSetString(sServerIPtxt, buf);
	
	memset(buf,0x00,sizeof(buf));
	sprintf(buf, "%d", theConfig.serverport);
	ituTextSetString(sServerporttxt, buf);
	
	memset(buf,0x00,sizeof(buf));
	snprintf(buf, "%s", theConfig.ipaddr, 15);
	ituTextSetString(schargeriptxt, buf);
	
	memset(buf,0x00,sizeof(buf));
	snprintf(buf, "%s", theConfig.gw, 15);
	ituTextSetString(schargergwtxt, buf);
	
	memset(buf,0x00,sizeof(buf));
	sprintf(buf, "%s", theConfig.netmask, 15);
	ituTextSetString(schargersmtxt, buf);

	memset(buf_mac,0x00,sizeof(buf_mac));
	sprintf(buf_mac, "%s", theConfig.chargermac, 17);
	ituTextSetString(schargermatxt, buf_mac);


	if(!theConfig.dhcp){
		sprintf(buf, "OFF");
		ituTextSetString(sDhcptxt, buf);
		bDHCPStatus = false;
		ituWidgetSetColor(schargeriptxt, 255, 78, 130, 174);
		ituWidgetSetColor(schargergwtxt, 255, 78, 130, 174);
		ituWidgetSetColor(schargersmtxt, 255, 78, 130, 174);

	}
	else{
		sprintf(buf, "ON");
		ituTextSetString(sDhcptxt, buf);
		bDHCPStatus = true;
		ituWidgetSetColor(schargeriptxt, 150, 255, 255, 255);
		ituWidgetSetColor(schargergwtxt, 150, 255, 255, 255);
		ituWidgetSetColor(schargersmtxt, 150, 255, 255, 255);
	}

	if(!bTimeoutCnt){
		printf("\r\n[TimeOut] Set Time Out Counter--------------%d\r\n",bTimeoutCnt);
		
		TopSetTimer(180, adminTimeout);
		TopTimerWidgetMove(690,390);
		TopCounterVisible(true);
		TopWidgetVisable(false);
		
		bTimeoutCnt = true;
	}

    return true;
}

bool adminsetipLeave(ITUWidget* widget, char* param)
{
	ituWidgetSetVisible(sBackground,false);
	//bGloAdminStatus = false;
	if(bChangedNetInfo)
	{
		bChangedNetInfo = false;
#ifndef EN_CERTI		
		NetworkReset();
#endif
	}
	
    return true;
}

bool SetNetFuncOnPress(ITUWidget* widget, char* param)
{
	setipselect = atoi(param);

	if(bDHCPStatus){	// DHCP ON
		if((setipselect > 2) && (setipselect < 6))
			return true;
	}

	if(( setipselect > 2) && (setipselect <= 6))
	{
		bChangedNetInfo = true;
	}
	
	ituLayerGoto(ituSceneFindWidget(&theScene, "IpAllLayer"));	
    return true;
}


bool exitLayer(ITUWidget* widget, char* param)
{
	AdminSetupMenuExit(true);
    return true;
}

bool SetIpSubMenu(ITUWidget* widget, char* param)
{
	ChangeSetupSubMenu(atoi(param));
    return true;
}

bool ToggleDHCP(ITUWidget* widget, char* param)
{
	char buf[16];
	if(bDHCPStatus){
		CtLogCyan("ToggleDHCP %d",bDHCPStatus);
		sprintf(buf, "OFF");
		ituTextSetString(sDhcptxt, buf);
		bDHCPStatus = false;
		theConfig.dhcp = false;
		ituWidgetSetColor(schargeriptxt, 255, 78, 130, 174);
		ituWidgetSetColor(schargergwtxt, 255, 78, 130, 174);
		ituWidgetSetColor(schargersmtxt, 255, 78, 130, 174);
	}
	else{
		CtLogCyan("ToggleDHCP %d",bDHCPStatus);
		sprintf(buf, "ON");
		ituTextSetString(sDhcptxt, buf);
		bDHCPStatus = true;
		theConfig.dhcp = true;
		ituWidgetSetColor(schargeriptxt, 150, 255, 255, 255);
		ituWidgetSetColor(schargergwtxt, 150, 255, 255, 255);
		ituWidgetSetColor(schargersmtxt, 150, 255, 255, 255);
	}
}