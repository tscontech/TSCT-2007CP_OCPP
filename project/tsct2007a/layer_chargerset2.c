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


static ITUBackground* sBackground1;

static ITUBackground* sGpsLatitudeBackground;
static ITUTextBox* sGpsLatitudetxt;

static ITUBackground* sGpsLongitudeBackground;
static ITUTextBox* sGpsLongitudettxt;

static ITUBackground* sChargerinitBackground;

static ITUBackground* sConfirmSelectBackground;
static ITUText* schargerselecttxt;

static ITUBackground* snum3KeypadBackground;
static sNumkeyInputListener saInputListener = NULL;

static char sNum[18];
static int sCount = 0;

void NumKeypadSetInputListenerstla(sNumkeyInputListener listener)
{
	saInputListener = listener;	
}

static void ResetNumberstla()
{
	char buf[8];

	sCount = 8;	
	memset(sNum,0x00,sizeof(sNum));
	memcpy(sNum, theConfig.gpslat, 8);
	
	snprintf(buf, "%s", theConfig.gpslat, 8);
	ituTextBoxSetString(sGpsLatitudetxt, buf);
}

static void UpdateNumberstla()
{
	char buf[8];
	snprintf(buf, "%s", sNum, 8);
	ituTextBoxSetString(sGpsLatitudetxt, buf);	
}

static void NumInputListenerstla(char c)
{
	if (c != 'd') // number
	{
		if (sCount < 8)
		{
			sNum[sCount++] = c;
		}
	}
	else // delete (back space)
	{
		if (sCount > 0)
		{
			sNum[--sCount] = ' ';
		}
	}
	UpdateNumberstla();		
}

void NumKeypadSetInputListenerstlo(sNumkeyInputListener listener)
{
	saInputListener = listener;
}

static void ResetNumberstlo()
{
	sCount = 9;	
	memset(sNum,0x00,sizeof(sNum));
	char buf[16];
	
	sNum[0]=theConfig.gpslon[0];  sNum[1]=theConfig.gpslon[1];  sNum[2]=theConfig.gpslon[2];
	sNum[3]=theConfig.gpslon[3];  sNum[4]=theConfig.gpslon[4];  sNum[5]=theConfig.gpslon[5];
	sNum[6]=theConfig.gpslon[6];  sNum[7]=theConfig.gpslon[7];  sNum[8]=theConfig.gpslon[8];
	
	snprintf(buf, "%s", theConfig.gpslon, 9);
	ituTextBoxSetString(sGpsLongitudettxt, buf);
}

static void UpdateNumberstlo()
{
	char buf[32];
	sprintf(buf, "%c%c%c%c%c%c%c%c%c", sNum[0], sNum[1], sNum[2] ,sNum[3], sNum[4], sNum[5] ,sNum[6],sNum[7],sNum[8]);
	ituTextBoxSetString(sGpsLongitudettxt, buf);
	
}

static void NumInputListenerstlo(char c)
{
	if (c != 'd') // number
	{
		if (sCount < 9)
		{
			sNum[sCount++] = c;
		}
	}
	else // delete (back space)
	{
		if (sCount > 0)
		{
			sNum[--sCount] = ' ';
		}
	}
	UpdateNumberstlo();		
}

void NumKeypadSetInputListenerstsel(sNumkeyInputListener listener)
{
	saInputListener = listener;
}
static void ResetNumbersel()
{
	sCount = 1;
	
	char buf[10];	
	memset(sNum,' ',sizeof(sNum));
	sprintf(buf, "%d", theConfig.ConfirmSelect);	
	ituTextSetString(schargerselecttxt, buf);
	sNum[0]=buf[0];

}
static void UpdateNumbersel()
{
	char buf[10];
	sprintf(buf, "%c", sNum[0]);
	ituTextSetString(schargerselecttxt, buf);
}

static void NumInputListenersel(char c)
{
	if (c != 'd') // number
	{
		if (sCount < 1)
		{
			sNum[sCount++] = c;
		}
	}
	else // delete (back space)
	{
		if (sCount > 0)
		{
			sNum[--sCount] = ' ';
		}
	}
	UpdateNumbersel();		
}

static void FindIpAllDialogWidget2(void)
{
	
	if (!sBackground1)
	{
		 sBackground1 = ituSceneFindWidget(&theScene, "ChargetLayer2Background");
		 assert(sBackground1);
		
		 sGpsLatitudeBackground = ituSceneFindWidget(&theScene, "GpsLatitudeBackground");
		 assert(sGpsLatitudeBackground);
		 sGpsLatitudetxt = ituSceneFindWidget(&theScene, "GpsLatitudetxt");
		 assert(sGpsLatitudetxt);
		
		 sGpsLongitudeBackground = ituSceneFindWidget(&theScene, "GpsLongitudeBackground");
		 assert(sGpsLongitudeBackground);
		 sGpsLongitudettxt = ituSceneFindWidget(&theScene, "GpsLongitudettxt");
		 assert(sGpsLongitudettxt);

		 sChargerinitBackground = ituSceneFindWidget(&theScene, "ChargerinitBackground");
		 assert(sChargerinitBackground);
	
		 sConfirmSelectBackground = ituSceneFindWidget(&theScene, "ConfirmSelectBackground");
		 assert(sConfirmSelectBackground);
		 schargerselecttxt = ituSceneFindWidget(&theScene, "chargerselecttxt");
		 assert(schargerselecttxt);
		
		 snum3KeypadBackground = ituSceneFindWidget(&theScene, "num3KeypadBackground");
		 assert(snum3KeypadBackground);
	}	
}

bool layer_charger2setEnter(ITUWidget* widget, char* param)
{	
	printf("layer_charger2setEnter [%d] \n",adminsetchargerselect2);
	switch(adminsetchargerselect2)
	{
		case 1:
		{//SetNetFuncOnPress
			FindIpAllDialogWidget2();
			ituWidgetSetVisible(sBackground1, true);
			ituWidgetSetVisible(sGpsLatitudeBackground, true);			
			 if(!snum3KeypadBackground)
			 {
				snum3KeypadBackground = ituSceneFindWidget(&theScene, "num3KeypadBackground");
				assert(snum3KeypadBackground);
			 }
		
			ResetNumberstla();
			ituWidgetSetVisible(snum3KeypadBackground, true);
			NumKeypadSetInputListenerstla(NumInputListenerstla);
			break;
		}
		
		case 2:
		{//SetNetFuncOnPress
			
			FindIpAllDialogWidget2();			
			ituWidgetSetVisible(sBackground1, true);
			ituWidgetSetVisible(sGpsLongitudeBackground, true);			
			 if(!snum3KeypadBackground)
			 {
				snum3KeypadBackground = ituSceneFindWidget(&theScene, "num3KeypadBackground");
				assert(snum3KeypadBackground);
			 }
			ResetNumberstlo();
			ituWidgetSetVisible(snum3KeypadBackground, true);
			NumKeypadSetInputListenerstlo(NumInputListenerstlo);
			break;
		}
		case 3:
		{//SetNetFuncOnPress
			
			FindIpAllDialogWidget2();			
			ituWidgetSetVisible(sBackground1, true);
			ituWidgetSetVisible(sChargerinitBackground, true);	
			break;
		}
		case 5:
		{	//SetNetFuncOnPress		
			FindIpAllDialogWidget2();			
			ituWidgetSetVisible(sBackground1, true);
			ituWidgetSetVisible(sConfirmSelectBackground, true);			
			 if(!snum3KeypadBackground)
			 {
				snum3KeypadBackground = ituSceneFindWidget(&theScene, "num3KeypadBackground");
				assert(snum3KeypadBackground);
			 }
			ResetNumbersel();
			ituWidgetSetVisible(snum3KeypadBackground, true);
			NumKeypadSetInputListenerstsel(NumInputListenersel);
			break;
		}
	}
    return true;
}

bool Layer_charger2setLeave(ITUWidget* widget, char* param)
{
	switch(adminsetchargerselect2)
	{
		case 1:
		{//SetNetFuncOnPress

			ituWidgetSetVisible(sBackground1, false);
			ituWidgetSetVisible(sGpsLatitudeBackground, false);
			ituWidgetSetVisible(snum3KeypadBackground, false);			
			NumKeypadSetInputListenerstla(NULL);
			break;
		}
		case 2:
		{//SetNetFuncOnPress

			ituWidgetSetVisible(sBackground1, false);
			ituWidgetSetVisible(sGpsLongitudeBackground, false);
			ituWidgetSetVisible(snum3KeypadBackground, false);
			NumKeypadSetInputListenerstlo(NULL);			
			break;
		}
		case 3:
		{//SetNetFuncOnPress
		
			ituWidgetSetVisible(sBackground1, false);
			ituWidgetSetVisible(sChargerinitBackground, false);	
		
			
			break;
		}
		case 5:
		{//SetNetFuncOnPress

			ituWidgetSetVisible(sBackground1, false);
			ituWidgetSetVisible(sConfirmSelectBackground, false);			
			ituWidgetSetVisible(snum3KeypadBackground, false);				
			NumKeypadSetInputListenerstsel(NULL);			
			break;
		}		
	}

	adminsetchargerselect2 = 0;
    return true;
}

bool exit3Layer(ITUWidget* widget, char* param)
{
	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetChargerLayer"));
    return true;
}


bool cNumKeypaddelOnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('d');
    return true;
}

bool cNumKeypadjjumOnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('.');
    return true;
}

bool cNumKeypad0OnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('0');
    return true;
}

bool cNumKeypad9OnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('9');
    return true;
}

bool cNumKeypad8OnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('8');
    return true;
}

bool cNumKeypad7OnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('7');
    return true;
}

bool cNumKeypad6OnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('6');
    return true;
}

bool cNumKeypad5OnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('5');
    return true;
}

bool cNumKeypad4OnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('4');
    return true;
}

bool cNumKeypad3OnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('3');
    return true;
}

bool cNumKeypad2OnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('2');
    return true;
}

bool cNumKeypad1OnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('1');
    return true;
}

bool cNumKeypadFOnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('F');
    return true;
}

bool cNumKeypadEOnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('E');
    return true;
}

bool cNumKeypadDOnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('D');
    return true;
}

bool cNumKeypadCOnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('C');
    return true;
}

bool cNumKeypadBOnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('B');
    return true;
}

bool cNumKeypadAOnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('A');
    return true;
}

bool ChargerSelectCancelOnPress(ITUWidget* widget, char* param)
{
	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetChargerLayer"));
    return true;
}

bool ChargerSelectOkOnPress(ITUWidget* widget, char* param)
{
   theConfig.ConfirmSelect = atoi(sNum); 
	memset(sNum,0x00,sizeof(sNum));
	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetChargerLayer"));
    return true;
}

bool SystemInitcancelOnPress(ITUWidget* widget, char* param)
{
	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetCharger2Layer"));
    return true;
}

bool SystemInitOkOnPress(ITUWidget* widget, char* param)
{
#ifdef SAMWONFA_CD_TERMINAL
	SmwonfaSetAutoRead();
#else
    // CardReaderSetAutoRead();	
#endif
	ConfigInitSave();	
	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetCharger2Layer"));
    return true;
}

bool GpsLongitudetCancelOnPress(ITUWidget* widget, char* param)
{
	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetCharger2Layer"));
    return true;
}

bool GpsLongitudetOkOnPress(ITUWidget* widget, char* param)
{
	memcpy(theConfig.gpslon, sNum, 10);
    memset(sNum,0x00,sizeof(sNum));
	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetCharger2Layer"));
    return true;
}

bool GpsLatitudeCancelOnPress(ITUWidget* widget, char* param)
{
	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetCharger2Layer"));
    return true;
}

bool GpsLatitudeOkOnPress(ITUWidget* widget, char* param)
{
	memcpy(theConfig.gpslat, sNum, 9);
	memset(sNum,0x00,sizeof(sNum));
	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetCharger2Layer"));
    return true;
}

