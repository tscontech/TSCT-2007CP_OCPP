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


//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------
static ITUBackground* spBackground;
static ITUBackground* sNumKeypadBackground;

static ITUTextBox* spassNum1TextBox;
static ITUTextBox* spassNum2TextBox;
static ITUTextBox* spassNum3TextBox;
static ITUTextBox* spassNum4TextBox;

static ITUButton* sokloginButton;
static char spCardNum[5];
static int spNumCount = 0;

//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------

static void ResetPassNumber()
{
	int i;
	
	for (i = 0; i < 4; i++)
		spCardNum[i] = ' ';
	spNumCount = 0;
	
	ituTextBoxSetString(spassNum1TextBox, "");
	ituTextBoxSetString(spassNum2TextBox, "");
	ituTextBoxSetString(spassNum3TextBox, "");
	ituTextBoxSetString(spassNum4TextBox, "");
}

static void UpdatepNumber()
{
	char buf[8];
	 
	 if(spNumCount == 0)
	 {
		sprintf(buf, "%c", ' ');
		ituTextBoxSetString(spassNum1TextBox, buf);
		
		sprintf(buf, "%c", ' ');
		ituTextBoxSetString(spassNum2TextBox, buf);
		
		sprintf(buf, "%c", ' ');
		ituTextBoxSetString(spassNum3TextBox, buf);
		
		sprintf(buf, "%c", ' ');
		ituTextBoxSetString(spassNum4TextBox, buf);
	 }
	 else if(spNumCount == 1)
	 {
		sprintf(buf, "%c", '*');
		ituTextBoxSetString(spassNum1TextBox, buf);
		
		sprintf(buf, "%c", ' ');
		ituTextBoxSetString(spassNum2TextBox, buf);
		
		sprintf(buf, "%c", ' ');
		ituTextBoxSetString(spassNum3TextBox, buf);
		
		sprintf(buf, "%c", ' ');
		ituTextBoxSetString(spassNum4TextBox, buf);
	 }
	 else if(spNumCount == 2)
	 {
		sprintf(buf, "%c", '*');
		ituTextBoxSetString(spassNum1TextBox, buf);
		
		sprintf(buf, "%c", '*');
		ituTextBoxSetString(spassNum2TextBox, buf);
		
		sprintf(buf, "%c", ' ');
		ituTextBoxSetString(spassNum3TextBox, buf);
		
		sprintf(buf, "%c", ' ');
		ituTextBoxSetString(spassNum4TextBox, buf);

	 }
	 else if(spNumCount == 3)
	 {
		sprintf(buf, "%c", '*');
		ituTextBoxSetString(spassNum1TextBox, buf);
		
		sprintf(buf, "%c", '*');
		ituTextBoxSetString(spassNum2TextBox, buf);
		
		sprintf(buf, "%c", '*');
		ituTextBoxSetString(spassNum3TextBox, buf);
		
		sprintf(buf, "%c", ' ');
		ituTextBoxSetString(spassNum4TextBox, buf);

	 }
	 else if(spNumCount == 4)
	 {
		sprintf(buf, "%c", '*');
		ituTextBoxSetString(spassNum1TextBox, buf);
		
		sprintf(buf, "%c", '*');
		ituTextBoxSetString(spassNum2TextBox, buf);
		
		sprintf(buf, "%c", '*');
		ituTextBoxSetString(spassNum3TextBox, buf);
		
		sprintf(buf, "%c", '*');
		ituTextBoxSetString(spassNum4TextBox, buf);
	 }
}


static void NumInputListener(char c)
{
	
	if (c != 'd' && c != 'r') // number
	{
		if (spNumCount < 4)		spCardNum[spNumCount++] = c;		
	}
	else if(c == 'd') // delete (back space)
	{
		if (spNumCount > 0)		spCardNum[--spNumCount] = ' ';		
	}
	else if(c == 'r')
	{
		while (spNumCount > 0)	{spCardNum[--spNumCount] = ' ';}
	}

	UpdatepNumber();	
	
}

bool AdminLoginOkOnPress(ITUWidget* widget, char* param)
{	
	if(spCardNum[0] == theConfig.adminpassword[0] && spCardNum[1] == theConfig.adminpassword[1] && spCardNum[2] == theConfig.adminpassword[2] && spCardNum[3] == theConfig.adminpassword[3]) 
	{
		ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetIpLayer"));
	}
	else if(spCardNum[0] == '0' && spCardNum[1] == '1' && spCardNum[2] == '3' && spCardNum[3] == '4')
	{
		//ituLayerGoto(ituSceneFindWidget(&theScene, "QualifTestLayer"));
	}
	else
	{
		int i=0;		
		for (i = 0; i < 4; i++)
			spCardNum[i] = ' ';
			
		spNumCount = 0;
		
		ituTextBoxSetString(spassNum1TextBox, "");
		ituTextBoxSetString(spassNum2TextBox, "");
		ituTextBoxSetString(spassNum3TextBox, "");
		ituTextBoxSetString(spassNum4TextBox, "");
	}
    return true;
}

bool AdminLoginCancelOnPress(ITUWidget* widget, char* param)
{
	AdminSetupMenuExit(false);
	return true;
}

bool PassNumOnEnter(ITUWidget* widget, char* param)
{
	CtLogRed("PassNumOnEnter :: Enter Passnum layer.....\n");
	bGloAdminStatus = true;
	if (!spBackground)
	{		
        spBackground = ituSceneFindWidget(&theScene, "passNumBackground");
        assert(spBackground);

		sNumKeypadBackground = ituSceneFindWidget(&theScene, "numKeypadBackground");
		assert(sNumKeypadBackground);

		spassNum1TextBox = ituSceneFindWidget(&theScene, "passNum1TextBox");
		assert(spassNum1TextBox);
		
		spassNum2TextBox = ituSceneFindWidget(&theScene, "passNum2TextBox");
		assert(spassNum2TextBox);
		
		spassNum3TextBox = ituSceneFindWidget(&theScene, "passNum3TextBox");
		assert(spassNum3TextBox);

		spassNum4TextBox = ituSceneFindWidget(&theScene, "passNum4TextBox");
		assert(spassNum4TextBox);
		
		sokloginButton = ituSceneFindWidget(&theScene, "okloginButton");
		assert(sokloginButton);	
	}
	
	ResetPassNumber();	
	ituWidgetSetVisible(sokloginButton, true);
	ituWidgetSetVisible(sNumKeypadBackground, true);
	NumKeypadSetInputListener(NumInputListener);

	TopSetTimer(30, GotoStartLayer);

	TopHomeBtnVisible(false);

    return true;
}

bool PassNumOnLeave(ITUWidget* widget, char* param)
{
	bGloAdminStatus = false;
	TopCloseTimer();
	NumKeypadSetInputListener(NULL);
	ituWidgetSetVisible(sNumKeypadBackground, false);
	
    return true;
}
void PassNumReset(void)
{
	spBackground = NULL;
}
