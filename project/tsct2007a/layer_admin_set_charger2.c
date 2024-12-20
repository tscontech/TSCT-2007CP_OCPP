/*       @brief
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

static ITUBackground* sBackground1_;

static ITUText* sLatitudePtxt;
static ITUText* sLongitudetxt;
static ITUText* sMacaddrtxt;

bool adminsetcharger2Enter(ITUWidget* widget, char* param)
{
	adminsetchargerselect2 = 0;
	bGloAdminStatus = true;
	if (!sBackground1_)
	{
		sBackground1_ = ituSceneFindWidget(&theScene, "AdminSetIp2Background");
		assert(sBackground1_);
		
		sLatitudePtxt = ituSceneFindWidget(&theScene, "LatitudePtxt");
		assert(sLatitudePtxt);
		
		sLongitudetxt = ituSceneFindWidget(&theScene, "Longitudetxt");
		assert(sLongitudetxt);
		
		sMacaddrtxt = ituSceneFindWidget(&theScene, "Macaddrtxt");
		assert(sMacaddrtxt);

	}
	
	ituWidgetSetVisible(sBackground1_,true);
	
	char buf[32];
	snprintf(buf, "%s", theConfig.gpslat,8);
	ituTextSetString(sLatitudePtxt, buf);

	snprintf(buf, "%s", theConfig.gpslon,9);
	ituTextSetString(sLongitudetxt, buf);

	sprintf(buf, "%s", theConfig.chargermac,17);
	ituTextSetString(sMacaddrtxt, buf);

    return true;
}

bool adminsetcharger2Leave(ITUWidget* widget, char* param)
{
    ituWidgetSetVisible(sBackground1_,false);	
    return true;
}

bool exitbLayer(ITUWidget* widget, char* param)
{
	AdminSetupMenuExit(true);
    return true;
}

bool SetCharger2SubMenu(ITUWidget* widget, char* param)
{
	ChangeSetupSubMenu(atoi(param));
    return true;
}
