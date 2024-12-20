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


static ITUBackground* sLctBackground;
 
bool adminChargerTestEnter(ITUWidget* widget, char* param)
{
	bGloAdminStatus = true;
	adminsettestselect = 0;
	if (!sLctBackground)
	{
		sLctBackground = ituSceneFindWidget(&theScene, "AdminSetTestBackground");
		assert(sLctBackground);
	}
	
	ituWidgetSetVisible(sLctBackground,true);

    return true;
}

bool adminChargerTestLeave(ITUWidget* widget, char* param)
{
	ituWidgetSetVisible(sLctBackground,false);
	
    return true;
}

bool AdminSetTestSelectItem(ITUWidget* widget, char* param)
{
	
	adminsettestselect = atoi(param);	
	ituLayerGoto(ituSceneFindWidget(&theScene, "ChargerTestLayer"));
    return true;
}

bool exitcLayer(ITUWidget* widget, char* param)
{
	AdminSetupMenuExit(true);
    return true;
}

bool ChargerTestSubMenu(ITUWidget* widget, char* param)
{
	
	ChangeSetupSubMenu(atoi(param));
    return true;
}
