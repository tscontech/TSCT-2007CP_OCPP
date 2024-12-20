/**
*       @file
*               layer_admin_login.c
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2018.01.10 <br>
*               author: bmlee <br>
*               description: <br>
*/

#include <sys/times.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <unistd.h>
#include <stdbool.h>

#include <assert.h>
#include "SDL/SDL.h"
#include "ctrlboard.h"
#include "scene.h"
#include "cststring.h"
#include "tsctcfg.h" 


static ITUBackground* sBackground2_;


static ITUBackground* sstationiddialogBackground;
static ITUTextBox* schargerstationtxt;

static ITUBackground* sChargerId1Background;
static ITUTextBox* schargerid1txt;
static ITUTextBox* schargerid2txt;

static ITUBackground* sSoundVolumBackground;
static ITUTextBox* sSoundVol_txt;
static ITUIcon* sVolumeset_box_icon;


static ITUBackground* slcdOnOffdialogBackground;
static ITUTextBox* sTextBox22;


static ITUBackground* sPassWordChangeground;
static ITUTextBox* sOldPassChangetxt;
static ITUTextBox* sNewPassChangetxt;

static ITUBackground* sOperationModeBackground;
static ITUTextBox* sChargeModeChangetxt;
static ITUTextBox* sFreeModeChangetxt;
static ITUTextBox* sCheckModeChangetxt;

static ITUTextBox* syyyyTxt;
static ITUTextBox* smmTxt;
static ITUTextBox* sddTxt;
static ITUTextBox* shhTxt;
static ITUTextBox* smm2Txt;
static ITUTextBox* sssTxt;

static ITUButton* sButton_1;
static ITUButton* sButton_2;
static ITUButton* sButton_3;
static ITUButton* sButton_4;
static ITUButton* sButton_5;
static ITUButton* sButton_6;
static ITUButton* sButton_7;
static ITUButton* sButton_8;
static ITUButton* sButton_9;
static ITUButton* sButton_10;
static ITUButton* sButton_11;
static ITUButton* sButton_12;
static ITUButton* sButton_13;
static ITUButton* sButton_14;
static ITUButton* sButton_15;
static ITUButton* sButton_16;

static ITUIcon* sIcon224;
static ITUIcon* sIcon13;

static ITUText* sDevIDTitle1;
static ITUText* sDevIDTitle2;

static ITUBackground* sTargetSocBackground;
static ITUTextBox* sTargetSoc_SocDigitTxt;

ITUButton *sEnKeyBtn[16];

static ITUBackground* snum2KeypadBackground;
static ITUBackground* sEnKeypadCom;

static sNumkeyInputListener saInputListener = NULL;

bool SleeptimeSelectControl;
bool timeSelectControl;
static char sNum[12];
static char sNumDev[12];
static char sNumDevDev[13];

static unsigned char sCount = 0;
static unsigned char sCountDev = 0;
static unsigned char sCountDevDev = 0;
static unsigned char MaxDigit = 0;

static int keypad_count = 0;

char sLNum[12];
static char keyarr[3][16] = 
{
	{'1','2','3','4','5','6','7','8','9','0','A','B','C','D','E','F'},
	{'G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V'},
	{'W','X','Y','Z',}
};

char sNumKeypadChar[11] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'd'};

long _rtcSec = 0;

char __hour[2];
char __min[2];
char __year[4];
char __mon[2];
char __day[2];				
char __sec[2];

static unsigned char nSelect = 0xFF;

void KeypadUpdate(void);

static void ResetNumbermode1();
static void ResetNumbermode2();

// Common Function +++++++++++++++++++

static void FindIpAllDialogWidget1(void)
{	
	if (!sBackground2_)
	{
		sBackground2_ = ituSceneFindWidget(&theScene, "ChargetLayerBackground");
		assert(sBackground2_);
		
		sstationiddialogBackground = ituSceneFindWidget(&theScene, "stationiddialogBackground");
		assert(sstationiddialogBackground);
		schargerstationtxt = ituSceneFindWidget(&theScene, "chargerstationtxt");
		assert(schargerstationtxt);

		sChargerId1Background = ituSceneFindWidget(&theScene, "ChargerId1Background");
		assert(sChargerId1Background);
		schargerid1txt = ituSceneFindWidget(&theScene, "chargerid1txt");
		assert(schargerid1txt);
		sIcon224 = ituSceneFindWidget(&theScene, "Icon224");
		assert(sIcon224);
		schargerid2txt = ituSceneFindWidget(&theScene, "chargerid2txt");
		assert(schargerid2txt);
		sIcon13 = ituSceneFindWidget(&theScene, "Icon13");
		assert(sIcon13);
		
		sSoundVolumBackground = ituSceneFindWidget(&theScene, "SoundVolumBackground");
		assert(sSoundVolumBackground);
		sSoundVol_txt = ituSceneFindWidget(&theScene, "SoundVol_txt");
		assert(sSoundVol_txt);	
		sVolumeset_box_icon = ituSceneFindWidget(&theScene, "Volumeset_box_icon");
		assert(sVolumeset_box_icon);			

		slcdOnOffdialogBackground = ituSceneFindWidget(&theScene, "lcdOnOffdialogBackground");
		assert(slcdOnOffdialogBackground);
		sTextBox22 = ituSceneFindWidget(&theScene, "TextBox22");
		assert(sTextBox22);

		sPassWordChangeground = ituSceneFindWidget(&theScene, "PassWordChangeground");
		assert(sPassWordChangeground);
		
		sOldPassChangetxt = ituSceneFindWidget(&theScene, "OldPassChangetxt");
		assert(sOldPassChangetxt);

		sNewPassChangetxt = ituSceneFindWidget(&theScene, "NewPassChangetxt");
		assert(sNewPassChangetxt);
		
		syyyyTxt = ituSceneFindWidget(&theScene, "yyyyTxt");
		assert(syyyyTxt);
		
		smmTxt = ituSceneFindWidget(&theScene, "mmTxt");
		assert(smmTxt);
		
		sddTxt = ituSceneFindWidget(&theScene, "ddTxt");
		assert(sddTxt);
		
		shhTxt = ituSceneFindWidget(&theScene, "hhTxt");
		assert(shhTxt);
		
		smm2Txt = ituSceneFindWidget(&theScene, "mm2Txt");
		assert(smm2Txt);
		
		sssTxt = ituSceneFindWidget(&theScene, "ssTxt");
		assert(sssTxt);
		
		snum2KeypadBackground = ituSceneFindWidget(&theScene, "num2KeypadBackground");
		assert(snum2KeypadBackground);

		sEnKeypadCom = ituSceneFindWidget(&theScene, "EnKeypadCom");
		assert(sEnKeypadCom);

		sButton_1 = ituSceneFindWidget(&theScene, "Button_1");
		assert(sButton_1);
		sEnKeyBtn[0] = sButton_1;

		sButton_2 = ituSceneFindWidget(&theScene, "Button_2");
		assert(sButton_2);
		sEnKeyBtn[1] = sButton_2;

		sButton_3 = ituSceneFindWidget(&theScene, "Button_3");
		assert(sButton_3);
		sEnKeyBtn[2] = sButton_3;

		sButton_4 = ituSceneFindWidget(&theScene, "Button_4");
		assert(sButton_4);
		sEnKeyBtn[3] = sButton_4;

		sButton_5 = ituSceneFindWidget(&theScene, "Button_5");
		assert(sButton_5);
		sEnKeyBtn[4] = sButton_5;

		sButton_6 = ituSceneFindWidget(&theScene, "Button_6");
		assert(sButton_6);
		sEnKeyBtn[5] = sButton_6;

		sButton_7 = ituSceneFindWidget(&theScene, "Button_7");
		assert(sButton_7);
		sEnKeyBtn[6] = sButton_7;

		sButton_8 = ituSceneFindWidget(&theScene, "Button_8");
		assert(sButton_8);
		sEnKeyBtn[7] = sButton_8;

		sButton_9 = ituSceneFindWidget(&theScene, "Button_9");
		assert(sButton_9);
		sEnKeyBtn[8] = sButton_9;

		sButton_10 = ituSceneFindWidget(&theScene, "Button_10");
		assert(sButton_10);
		sEnKeyBtn[9] = sButton_10;

		sButton_11 = ituSceneFindWidget(&theScene, "Button_11");
		assert(sButton_11);
		sEnKeyBtn[10] = sButton_11;

		sButton_12 = ituSceneFindWidget(&theScene, "Button_12");
		assert(sButton_12);
		sEnKeyBtn[11] = sButton_12;

		sButton_13 = ituSceneFindWidget(&theScene, "Button_13");
		assert(sButton_13);
		sEnKeyBtn[12] = sButton_13;

		sButton_14 = ituSceneFindWidget(&theScene, "Button_14");
		assert(sButton_14);
		sEnKeyBtn[13] = sButton_14;

		sButton_15 = ituSceneFindWidget(&theScene, "Button_15");
		assert(sButton_15);
		sEnKeyBtn[14] = sButton_15;

		sButton_16 = ituSceneFindWidget(&theScene, "Button_16");
		assert(sButton_16);		
		sEnKeyBtn[15] = sButton_16;		

		sDevIDTitle1 = ituSceneFindWidget(&theScene, "DevIDTitle1");
		assert(sDevIDTitle1);
		sDevIDTitle2 = ituSceneFindWidget(&theScene, "DevIDTitle2");
		assert(sDevIDTitle2);

		sOperationModeBackground = ituSceneFindWidget(&theScene, "OperationModeBackground");
		sChargeModeChangetxt = ituSceneFindWidget(&theScene, "ChargeModeChangetxt");
		sFreeModeChangetxt = ituSceneFindWidget(&theScene, "FreeModeChangetxt");
		sCheckModeChangetxt = ituSceneFindWidget(&theScene, "CheckModeChangetxt");
		
		sTargetSocBackground = ituSceneFindWidget(&theScene, "TargetSocBackground");

		sTargetSoc_SocDigitTxt = ituSceneFindWidget(&theScene, "TargetSoc_SocDigitTxt");
	}	
}

static void BackToPeviouseStep(void)
{	
	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetChargerLayer"));
}

static void SetInputListener(sNumkeyInputListener listener)
{
	saInputListener = listener;
}

static char GetItemNumberDigit(void)
{
	char ret = 0;
	
	if(adminsetchargerselect1 == 1)
	{
		ret =  8;
	}
	else if((adminsetchargerselect1 == 2)||(adminsetchargerselect1 == 3))
	{
		ret = 2;
	}
	else if(adminsetchargerselect1 == 5)
	{
		ret = 4;
	}	
	printf("GetItemNumberDigit :: [%d] => [%d] \n", adminsetchargerselect1, ret);
	return ret;
}

static void EnglishKeyPadInit(void)
{
	if(!sEnKeypadCom)
	{
		sEnKeypadCom = ituSceneFindWidget(&theScene, "EnKeypadCom");
		assert(sEnKeypadCom);
	}
	
	ituWidgetSetVisible(sEnKeypadCom, true);
}

static void Num2KeyPadInit(void)
{
	if(!snum2KeypadBackground)
	{
		snum2KeypadBackground = ituSceneFindWidget(&theScene, "num2KeypadBackground");
		assert(snum2KeypadBackground);
	}
	ituWidgetSetVisible(snum2KeypadBackground, true);
}

void TextColorSetBlue(ITUTextBox* tb)
{
	ituSetColor(&tb->text.widget.color, 255, 0, 158, 234);
	ituWidgetSetDirty(&tb->text.widget, true);
}

void TextColorSetWhite(ITUTextBox* tb)
{
	ituSetColor(&tb->text.widget.color, 255, 255, 255, 255);
	ituWidgetSetDirty(&tb->text.widget, true);
}

// Reset UI Setting Value +++++++++++++++++++++
static void ResetNumberstid()
{
	sCount = 8;
	
	memset(sNum,0x00,sizeof(sNum));
	char buf[32];
	
	sNum[0]=theConfig.siteid[0];  sNum[1]=theConfig.siteid[1];  sNum[2]=theConfig.siteid[2];
	sNum[3]=theConfig.siteid[3];  sNum[4]=theConfig.siteid[4];  sNum[5]=theConfig.siteid[5];
	sNum[6]=theConfig.siteid[6];  sNum[7]=theConfig.siteid[7];
	
	sprintf(buf, "%c%c%c%c%c%c%c%c", theConfig.siteid[0], theConfig.siteid[1], theConfig.siteid[2]
	                               , theConfig.siteid[3], theConfig.siteid[4], theConfig.siteid[5]
								   , theConfig.siteid[6], theConfig.siteid[7]);
	ituTextBoxSetString(schargerstationtxt, buf);
}

static void ResetNumbersound()
{
	sCount = 2;	
	memset(sNum,0x00,sizeof(sNum));
	char buf[32];

	sNum[0]=theConfig.audiolevel/10 + '0';  sNum[1]=theConfig.audiolevel%10 + '0';
	
	sprintf(buf, "%c%c", sNum[0], sNum[1]);
	ituTextBoxSetString(sSoundVol_txt, buf);
}

static void ResetNumberid1()
{
	sCount = 2;	
	sCountDev = 2;
	memset(sNum,0x00,sizeof(sNum));
	memset(sNumDev,0x00,sizeof(sNumDev));
	char buf[32];
	char bufId[32];
	
	sNum[0]=theConfig.devid1[0];  sNum[1]=theConfig.devid1[1];
	sNumDev[0]=theConfig.devid2[0];  sNumDev[1]=theConfig.devid2[1];  
	
	sprintf(buf, "%c%c", theConfig.devid1[0], theConfig.devid1[1]);
	sprintf(bufId, "%c%c", theConfig.devid2[0], theConfig.devid2[1]);
	ituTextBoxSetString(schargerid1txt, buf);
	ituTextBoxSetString(schargerid2txt, bufId);
}

static void ResetNumberlcd()
{
	sCount = 3;
	
	char buf[32];	
	memset(sNum,' ',sizeof(sNum));
	sprintf(buf, "%d", theConfig.screensaver_time);	
	strcat(buf, STR_TIME_MIN);
	
	ituTextBoxSetString(sTextBox22, buf);
	
	if(theConfig.screensaver_time > 99)
	{
		sNum[0]=buf[0];sNum[1]=buf[1];sNum[2]=buf[2];
	}
	else if(theConfig.screensaver_time > 9)
	{
		sNum[0]=buf[0];sNum[1]=buf[1];
	}
	else if(theConfig.screensaver_time < 10)
	{
		sNum[0]=buf[0];
	}
}

static void ResetNumbermode1()
{
	sCount = 1;	
	memset(sNum,0x00,sizeof(sNum));

	char buf[2];
	
	sNum[0]=theConfig.OperationMode + '0';
	
	sprintf(buf, "%c", sNum[0]);
	ituTextBoxSetString(sChargeModeChangetxt, buf);
}

static void ResetNumbermode2()
{
	sCountDev = 2;
	memset(sNumDev,0x00,sizeof(sNumDev));

	char bufDev[3];
	
	sNumDev[0]=theConfig.FreeChargingTime/10 + '0';  sNumDev[1]=theConfig.FreeChargingTime%10 + '0';  
	
	sprintf(bufDev, "%c%c", sNumDev[0], sNumDev[1]);
	ituTextBoxSetString(sFreeModeChangetxt, bufDev);
}

static void ResetNumbermode3()
{
	char bufDev[13];

	sCountDevDev = 12;
	memset(sNumDevDev,0x00,sizeof(sNumDevDev));

	memcpy(sNumDevDev, theConfig.chkModeMac, sizeof(theConfig.chkModeMac));
		
	memcpy(bufDev, sNumDevDev, sizeof(sNumDevDev));
	
	memset(&bufDev[12], NULL, 1);

	ituTextBoxSetString(sCheckModeChangetxt, bufDev);
}

static void ResetNumberpass()
{
	sCount = 4;
	
	memset(sNum,0x00,sizeof(sNum));
	char buf[32];
	
	sNum[0]=theConfig.adminpassword[0];  sNum[1]=theConfig.adminpassword[1];  
	sNum[2]=theConfig.adminpassword[2];  sNum[3]=theConfig.adminpassword[3];  
	
	sprintf(buf, "%c%c%c%c", theConfig.adminpassword[0], theConfig.adminpassword[1], theConfig.adminpassword[2], theConfig.adminpassword[3]);
	ituTextBoxSetString(sNewPassChangetxt, buf);
	ituTextBoxSetString(sOldPassChangetxt, buf);
}

static void ResetNumberTSoc()
{
	char buf[32];

	memset(sNum,0x00,sizeof(sNum));

	sCount = 3;

	// if(theConfig.targetSoc > 99)
	// {
		// sCount = 3;
		sNum[0] = (theConfig.targetSoc / 100) %10;
		sNum[1] = (theConfig.targetSoc / 10) % 10;
		sNum[2] = (theConfig.targetSoc / 1) % 10;
	// }
	// else if(theConfig.targetSoc > 9)
	// {
	// 	// sCount = 2;

	// }
	// else 
	// 	sCount = 1;
	
	sprintf(buf, "%c%c%c", sNum[0], sNum[1], sNum[2]);
	
	ituTextBoxSetString(sTargetSoc_SocDigitTxt, buf);
}

// Update +++++++++++++++++++++++++++++++++++
static void UpdateDateTimeNumber(void)
{
	char buf[32];

	if(nSelect == 0)
	{
		sprintf(buf, "%c%c%c%c", sNum[0], sNum[1], sNum[2] ,sNum[3]);

		__year[0] = sNum[0];
		__year[1] = sNum[1];
		__year[2] = sNum[2];
		__year[3] = sNum[3];
	
		ituTextBoxSetString(syyyyTxt, buf);
	}
	else if(nSelect == 1)
	{
		sprintf(buf, "%c%c", sNum[0], sNum[1]);
		if(sCount < 1)
		{
			__mon[1] = sNum[0];
			__mon[0] = 0x30;
		}
		else
		{
			__mon[0] = sNum[0];
			__mon[1] = sNum[1];
		}
		ituTextBoxSetString(smmTxt, buf);
	}
	else if(nSelect == 2)
	{
		sprintf(buf, "%c%c", sNum[0], sNum[1]);
		if(sCount < 1)
		{
			__day[1] = sNum[0];
			__day[0] = 0x30;
		}
		else
		{
			__day[0] = sNum[0];
			__day[1] = sNum[1];
		}
		ituTextBoxSetString(sddTxt, buf);
	}
	else if(nSelect == 3)
	{
		sprintf(buf, "%c%c", sNum[0], sNum[1]);
		if(sCount < 1)
		{
			__hour[1] = sNum[0];
			__hour[0] = 0x30;
		}
		else
		{
			__hour[0] = sNum[0];
			__hour[1] = sNum[1];
		}
		ituTextBoxSetString(shhTxt, buf);
	}
	else if(nSelect == 4)
	{
		sprintf(buf, "%c%c", sNum[0], sNum[1]);	
		if(sCount < 1)
		{
			__min[1] = sNum[0];
			__min[0] = 0x30;
		}
		else
		{
			__min[0] = sNum[0];
			__min[1] = sNum[1];
		}
		ituTextBoxSetString(smm2Txt, buf);
	}
	else if(nSelect == 5)
	{
		sprintf(buf, "%c%c", sNum[0], sNum[1]);		
		if(sCount < 1)
		{
			__sec[1] = sNum[0];
			__sec[0] = 0x30;
		}
		else
		{
			__sec[0] = sNum[0];
			__sec[1] = sNum[1];
		}
		ituTextBoxSetString(sssTxt, buf);
	}
}

static void UpdateNumberstid()
{
	char buf[32];
	sprintf(buf, "%c%c%c%c%c%c%c%c", sNum[0], sNum[1], sNum[2] ,sNum[3], sNum[4], sNum[5] ,sNum[6],sNum[7]);
	ituTextBoxSetString(schargerstationtxt, buf);	
}

static void UpdateNumberCpId(char nID)
{
	char buf[32];

	if(nID == 1)
	{
		sprintf(buf, "%c%c", sNum[0], sNum[1]);
		ituTextBoxSetString(schargerid1txt, buf);	
	}
	else if(nID == 2)
	{
		sprintf(buf, "%c%c", sNumDev[0], sNumDev[1]);
		ituTextBoxSetString(schargerid2txt, buf);	
	}
}

static void UpdateNumberSound(void)
{
	char buf[32];

	sprintf(buf, "%c%c", sNum[0], sNum[1]);
	ituTextBoxSetString(sSoundVol_txt, buf);	
}

static void UpdateNumberlcd()
{
	SleeptimeSelectControl = true;
	char buf[32];
	
	if(sNum[1] == ' ')
		sprintf(buf, "%c", sNum[0]);
	else
		sprintf(buf, "%c%c", sNum[0], sNum[1]);
	
	strcat(buf, STR_TIME_MIN);
	
	memcpy(sLNum,&sNum,sizeof(sLNum));
	ituTextBoxSetString(sTextBox22, buf);
}

static void UpdateNumbermode1()
{
	char buf[2];

	sprintf(buf, "%c", sNum[0]);
	
	ituTextBoxSetString(sChargeModeChangetxt, buf);
}

static void UpdateNumbermode2()
{
	char bufDev[3];

	sprintf(bufDev, "%c%c", sNumDev[0], sNumDev[1]);

	ituTextBoxSetString(sFreeModeChangetxt, bufDev);
}

static void UpdateNumbermode3()
{
	char bufDev[13];

	memcpy(bufDev, sNumDevDev, sizeof(sNumDevDev));

	memset(&bufDev[12], NULL, 1);

	CtLogGreen("chkModeMac buf : %s", bufDev);

	ituTextBoxSetString(sCheckModeChangetxt, bufDev);
}

static void UpdateNumberpass()
{
	char buf[32];
	sprintf(buf, "%c%c%c%c", sNum[0], sNum[1], sNum[2], sNum[3]);
	ituTextBoxSetString(sNewPassChangetxt, buf);	
}

static void UpdateNumberTSoc()
{
	char buf[32];
	sprintf(buf, "%c%c%c", sNum[0], sNum[1], sNum[2]);
	ituTextBoxSetString(sTargetSoc_SocDigitTxt, buf);	
}

// Num Input Listener +++++++++++++++++++++++++++++++++++
static void NumKeyDateInputListener(char c)
{
	if (c != 'd') // number
	{
		if (sCount < MaxDigit)		sNum[sCount++] = c;		
	}
	else // delete (back space)
	{
		if (sCount > 0)		sNum[--sCount] = ' ';
	}
	UpdateDateTimeNumber();		
}

static void NumInputListenerstid(char c)
{
	char nMaxNum = GetItemNumberDigit();

	printf("max digit [%d]  NumInputListenerstid = [%c] \n", nMaxNum, c);

	if (c != 'd') // number
	{
		if (sCount < nMaxNum)		sNum[sCount++] = c;				
	}
	else // delete (back space)
	{
		if (sCount > 0)		sNum[--sCount] = ' ';
	}
	UpdateNumberstid();		
}

static void NumInputListenerid1(char c)
{
	if (c != 'd') // number
	{
		if (sCount < 2)		sNum[sCount++] = c;		
	}
	else // delete (back space)
	{
		if (sCount > 0)		sNum[--sCount] = ' ';
	}
	UpdateNumberCpId(1);		
}

static void NumInputListenersound(char c)
{
	if (c != 'd') // number
	{
		if (sCount < 2)		sNum[sCount++] = c;		
	}
	else // delete (back space)
	{
		if (sCount > 0)		sNum[--sCount] = ' ';
	}
	UpdateNumberSound();		
}

static void NumInputListenerid2(char c)
{
	if (c != 'd') // number
	{
		if (sCountDev < 2)		sNumDev[sCountDev++] = c;		
	}
	else // delete (back space)
	{
		if (sCountDev > 0)		sNumDev[--sCountDev] = ' ';		
	}
	UpdateNumberCpId(2);		
}

static void NumInputListenerlcd(char c)
{
	if (c != 'd') // number
	{
		if (sCount < 2)		sNum[sCount++] = c;		
	}
	else // delete (back space)
	{
		if (sCount > 0)		sNum[--sCount] = ' ';		
	}
	UpdateNumberlcd();		
}

static void NumInputListenermode1(char c)
{
	if (c != 'd') // number
	{
		if (sCount < 1)		sNum[sCount++] = c;		
	}
	
	else // delete (back space)
	{
		if (sCount > 0)		sNum[--sCount] = ' ';		
	}
	UpdateNumbermode1();		
}

static void NumInputListenermode2(char c)
{
	if (c != 'd') // number
	{
		if (sCountDev < 2)		sNumDev[sCountDev++] = c;		
	}
	
	else // delete (back space)
	{
		if (sCountDev > 0)		sNumDev[--sCountDev] = ' ';		
	}
	UpdateNumbermode2();		
}

static void NumInputListenermode3(char c)
{
	if (c != 'd') // number
	{
		if (sCountDevDev < 12)		sNumDevDev[sCountDevDev++] = c;		
	}
	
	else // delete (back space)
	{
		if (sCountDevDev > 0)		sNumDevDev[--sCountDevDev] = ' ';		
	}
	UpdateNumbermode3();		
}

static void NumInputListenerpass(char c)
{
	if (c != 'd') // number
	{
		if (sCount < 4)		sNum[sCount++] = c;		
	}
	
	else // delete (back space)
	{
		if (sCount > 0)		sNum[--sCount] = ' ';		
	}
	UpdateNumberpass();		
}

static void NumInputListenerTSoc(char c)
{
	if (c != 'd') // number
	{
		if (sCount < 3)		sNum[sCount++] = c;		
	}
	
	else // delete (back space)
	{
		if (sCount > 0)		sNum[--sCount] = ' ';		
	}
	UpdateNumberTSoc();		
}

// Select Button Function on UI	+++++++++++++++++++++++++++

bool SelectDateTimeSet(ITUWidget* widget, char* param)
{
	nSelect = atoi(param);

	timeSelectControl = true;

	switch(nSelect)
	{
		case 0:			sCount = 4;		    break;
		case 1:			sCount = 2;		    break;
		case 2:			sCount = 2;		    break;
		case 3:			sCount = 2;		    break;
		case 4:			sCount = 2;		    break;
		case 5:			sCount = 2;		    break;
	}
	
	MaxDigit = sCount;

	sCount =0;

	memset(sNum,0x00,sizeof(sNum));

	SetInputListener(NumKeyDateInputListener);
	return true;
}

bool DevIdOnePress(ITUWidget* widget, char* param)
{
	TextColorSetBlue(schargerid1txt);
	TextColorSetWhite(schargerid2txt);
	SetInputListener(NumInputListenerid1);
}

bool DevIdTwoPress(ITUWidget* widget, char* param)
{
	TextColorSetWhite(schargerid1txt);
	TextColorSetBlue(schargerid2txt);
	SetInputListener(NumInputListenerid2);
}

bool SelSoundSetPress(ITUWidget* widget, char* param)
{
	SetInputListener(NumInputListenersound);
}

bool SelectLcdTimeSetPress(ITUWidget* widget, char* param)
{
	ResetNumberlcd();
	SetInputListener(NumInputListenerlcd);
	return true;
}

bool SelectModeSetPress(ITUWidget* widget, char* param)
{
	sCount = 1;
	//ResetNumbermode1();
	SetInputListener(NumInputListenermode1);
	return true;
}

bool SelectFreeTimeSetPress(ITUWidget* widget, char* param)
{
	sCount = 2;
	//ResetNumbermode2();
	SetInputListener(NumInputListenermode2);
	return true;
}

bool SelectEvccMacSetPress(ITUWidget* widget, char* param)
{
	sCount = 12;
	//ResetNumbermode2();
	SetInputListener(NumInputListenermode3);
	return true;
}

bool SelectTargetSocSetPress(ITUWidget* widget, char* param)
{
	sCount = 3;
	//ResetNumbermode2();
	SetInputListener(NumInputListenerTSoc);
	return true;
}

// Select Button Function on UI	----------------------------

// Layer Enter / Leave ++++++++++++++++++++++++++++++++

bool layer_chargersetEnter(ITUWidget* widget, char* param)
{
	timeSelectControl = false;
	switch(adminsetchargerselect1)
	{
		case 1:
		{//stationid input
			keypad_count = 0;			
			FindIpAllDialogWidget1();		
			ituWidgetSetVisible(sBackground2_, true);
			ituWidgetSetVisible(sstationiddialogBackground, true);			

			ResetNumberstid();			
			Num2KeyPadInit(); //EnglishKeyPadInit();			
			SetInputListener(NumInputListenerstid);
			
			KeypadUpdate();
			break;
		}
		
		case 2:
		{//charger ID 1			
			FindIpAllDialogWidget1();			
			ituWidgetSetVisible(sBackground2_, true);
			ituWidgetSetVisible(sChargerId1Background, true);				
			ResetNumberid1();			
			if((theConfig.devtype == BB_TYPE)||(theConfig.devtype == BC2_TYPE))
			{
				ituWidgetSetVisible(schargerid2txt, true);
				ituWidgetSetVisible(sIcon13, true);
				ituWidgetSetVisible(sDevIDTitle2, true);
				TextColorSetBlue(schargerid1txt);
				TextColorSetWhite(schargerid2txt);
			}
			else
			{
				ituWidgetSetVisible(schargerid2txt, false);
				ituWidgetSetVisible(sIcon13, false);
				ituWidgetSetVisible(sDevIDTitle2, false);
			}
			Num2KeyPadInit(); //EnglishKeyPadInit();			
			SetInputListener(NumInputListenerid1);			
			KeypadUpdate();
			break;
		}
		case 3:
		{//Sound Volume Set
			
			FindIpAllDialogWidget1();			
			ituWidgetSetVisible(sBackground2_, true);
			ituWidgetSetVisible(sSoundVolumBackground, true);		
			
			ResetNumbersound();
			Num2KeyPadInit();
			//EnglishKeyPadInit();
			SetInputListener(NumInputListenersound);			
			KeypadUpdate();
			break;
		}
	
		case 4:
		{//blackout time and RTC Set
			SleeptimeSelectControl = false;
			FindIpAllDialogWidget1();
			
			ituWidgetSetVisible(sBackground2_, true);
			ituWidgetSetVisible(slcdOnOffdialogBackground, true);	
			ituWidgetSetVisible(syyyyTxt, true);
			ituWidgetSetVisible(smmTxt, true);			
			ituWidgetSetVisible(sddTxt, true);		
			ituWidgetSetVisible(shhTxt, true);
			ituWidgetSetVisible(smm2Txt, true); 		
			ituWidgetSetVisible(sssTxt, true);		
													
			time_t time = CstGetTime();
			struct tm *tm = localtime(&time);

			sprintf(__year, "%d", tm->tm_year+1900); 
			ituTextSetString(syyyyTxt, __year); 
			
			sprintf(__mon, "%02d", tm->tm_mon+1);			
			ituTextSetString(smmTxt, __mon);
		
			sprintf(__day, "%02d", tm->tm_mday);			
			ituTextSetString(sddTxt, __day);	
			
			sprintf(__hour, "%02d", tm->tm_hour);			
			ituTextSetString(shhTxt, __hour);	
			
			sprintf(__min, "%02d", tm->tm_min);
			ituTextSetString(smm2Txt, __min); 
			
			sprintf(__sec, "%02d", tm->tm_sec);
			ituTextSetString(sssTxt, __sec);	
			
			char buf[16]={0x30,};
			
			sprintf(buf, "%02d", theConfig.screensaver_time);		
						strcat(buf, STR_TIME_MIN);

			ituTextBoxSetString(sTextBox22, buf);
			Num2KeyPadInit();
			SetInputListener(NumInputListenerlcd);
			KeypadUpdate();
			//printf(" 1>> %04d-%02d-%02d:%02d.%02d.%02d \n", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);			
			break;
		}
		case 5:			//Password input			
			FindIpAllDialogWidget1();
			
			ituWidgetSetVisible(sBackground2_, true);
			ituWidgetSetVisible(sPassWordChangeground, true);		
			
			ResetNumberpass();
			Num2KeyPadInit();
			SetInputListener(NumInputListenerpass);
		break;

		case 6:		//Charger Mode input			
			FindIpAllDialogWidget1();
			
			ituWidgetSetVisible(sBackground2_, true);
			ituWidgetSetVisible(sOperationModeBackground, true);		
			
			ResetNumbermode1();
			ResetNumbermode2();
			ResetNumbermode3();
			// Num2KeyPadInit();
			EnglishKeyPadInit();
			SetInputListener(NumInputListenermode1);
		break;

		case 7:		//Charger SOC input			
			FindIpAllDialogWidget1();
			
			ituWidgetSetVisible(sBackground2_, true);
			ituWidgetSetVisible(sTargetSocBackground, true);		
			
			ResetNumberTSoc();
			Num2KeyPadInit();
			SetInputListener(NumInputListenerTSoc);
		break;
	}
	
    return true;
}

bool layer_chargersetLeave(ITUWidget* widget, char* param)
{
	switch(adminsetchargerselect1)
	{
		case 1:
		{
			ituWidgetSetVisible(sBackground2_, false);
			ituWidgetSetVisible(sstationiddialogBackground, false);
			ituWidgetSetVisible(sEnKeypadCom, false);			
			SetInputListener(NULL);
			break;
		}
		case 2:
		{

			ituWidgetSetVisible(sBackground2_, false);
			ituWidgetSetVisible(sChargerId1Background, false);
			ituWidgetSetVisible(sEnKeypadCom, false);
			SetInputListener(NULL);			
			break;
		}
		case 3:
		{		
			ituWidgetSetVisible(sBackground2_, false);
			ituWidgetSetVisible(sSoundVolumBackground, false);	
			ituWidgetSetVisible(sEnKeypadCom, false);
			SetInputListener(NULL);		
			break;
		}
		case 4:
		{
			ituWidgetSetVisible(sBackground2_, false);
			ituWidgetSetVisible(slcdOnOffdialogBackground, false);			
			ituWidgetSetVisible(snum2KeypadBackground, false);		

			ituWidgetSetVisible(syyyyTxt, false);
			ituWidgetSetVisible(smmTxt, false);			
			ituWidgetSetVisible(sddTxt, false);		
			ituWidgetSetVisible(shhTxt, false);
			ituWidgetSetVisible(smm2Txt, false);			
			ituWidgetSetVisible(sssTxt, false);		

			SetInputListener(NULL);			
			break;
		}
		case 5:
		{
			ituWidgetSetVisible(sBackground2_, false);
			ituWidgetSetVisible(sPassWordChangeground, false);			
			ituWidgetSetVisible(snum2KeypadBackground, false);
			SetInputListener(NULL);			
			break;
		}
		case 6:
		{
			ituWidgetSetVisible(sBackground2_, false);
			ituWidgetSetVisible(sOperationModeBackground, false);			
			ituWidgetSetVisible(sEnKeypadCom, false);
			SetInputListener(NULL);			
			break;
		}
	}
	
	adminsetchargerselect1 = 0;
	
    return true;
}

// Layer Enter / Leave -------------------------------

// Kepad Press +++++++++++++++++++++++++++++++++++++++
void KeypadUpdate(void)
{
	char* buf[2] = {};
	int i;
	
	for(i=0; i<16;i++)
	{
		sprintf(buf, "%c", keyarr[keypad_count][i]);	
		ituButtonSetString(sEnKeyBtn[i], buf);
	}
}

bool KeypadOnPress(ITUWidget* widget, char* param)
{
	int switch_param_int = atoi(param);

	printf("switch_param_int = %d", switch_param_int);
	
	if(saInputListener)(*saInputListener)(keyarr[keypad_count][switch_param_int]);

	return true;
}

bool KeypadOnPress_next(ITUWidget* widget, char* param)
{
	// if(keypad_count == 2) 	keypad_count = 0;
	// else keypad_count += 1;
	
	// KeypadUpdate();	
	
	return true;
}

bool KeypadOnPress_del(ITUWidget* widget, char* param)
{
	if (saInputListener)
			(*saInputListener)('d');
		return true;
}

bool bNumKeypad1OnPress(ITUWidget* widget, char* param)
{
	int num = atoi(param);
	printf("bNumKeypad1OnPress :: %d \n", num);	
	if (saInputListener)
	{
		(*saInputListener)(sNumKeypadChar[num]);
	}
    return true;
}

// OK Button Press ++++++++++++++++++++++++++++++++++++++

bool PassChangeOkOnPress(ITUWidget* widget, char* param)
{
	theConfig.adminpassword[0]=sNum[0];theConfig.adminpassword[1]=sNum[1];
	theConfig.adminpassword[2]=sNum[2];theConfig.adminpassword[3]=sNum[3];
	memset(sNum,0x00,sizeof(sNum));
	BackToPeviouseStep();
    return true;
}

bool lcdOnOffOkOnPress(ITUWidget* widget, char* param)
{
 ////leebm
 if(SleeptimeSelectControl)
 {
	 SleeptimeSelectControl = false;
	theConfig.screensaver_time = atoi(sLNum); 
 }
	
	if(timeSelectControl)
	{
		struct  tm tttt;
		
		admintimeStamp[0] = __year[0] << 4;
		admintimeStamp[0]|= (__year[1] &= 0x0F);
		admintimeStamp[1] = __year[2] << 4;
		admintimeStamp[1]|= (__year[3] &= 0x0F);		
		
		admintimeStamp[2] = __mon[0] << 4;
		admintimeStamp[2]|= (__mon[1] &= 0x0F);		
		
		admintimeStamp[3] = __day[0] << 4;
		admintimeStamp[3]|= (__day[1] &= 0x0F);		
		
		admintimeStamp[4] = __hour[0] << 4;
		admintimeStamp[4]|= (__hour[1] &= 0x0F);		
		
		admintimeStamp[5] = __min[0] << 4;
		admintimeStamp[5]|= (__min[1] &= 0x0F);		
		
		admintimeStamp[6] = __sec[0] << 4;
		admintimeStamp[6]|= (__sec[1] &= 0x0F);		

		printf(" time char %x %x %x %x %x %x %x \n " ,admintimeStamp[0],admintimeStamp[1],admintimeStamp[2],admintimeStamp[3] ,admintimeStamp[4],admintimeStamp[5],admintimeStamp[6]);	
		timeSelectControl = false;
		timeSelectControl_ = true;

	}
	ScreenInit();
	
	memset(sNum,0x00,sizeof(sNum));
	BackToPeviouseStep();
    return true;
}

bool chargerid2OkOnPress(ITUWidget* widget, char* param)
{
	if((sNum[0] == ' ') || (sNum[1] == ' '))
		theConfig.audiolevel = 70;
	else
		theConfig.audiolevel = (((sNum[0] - '0') > 0) ? (sNum[0] - '0') : 0) * 10 + (sNum[1] - '0');
	
	if (theConfig.audiolevel < 0 ) theConfig.audiolevel = 70;

	memset(sNum,0x00,sizeof(sNum));
	BackToPeviouseStep();	
    return true;
}

bool chargerid1OkOnPress(ITUWidget* widget, char* param)
{
	theConfig.devid1[0]=sNum[0];	theConfig.devid1[1]=sNum[1];

	theConfig.devid2[0]=sNumDev[0];	theConfig.devid2[1]=sNumDev[1];

	memset(sNum,0x00,sizeof(sNum));
	BackToPeviouseStep();	
    return true;
}

bool stationIdOkOnPress(ITUWidget* widget, char* param)
{	
	theConfig.siteid[0]=sNum[0];theConfig.siteid[1]=sNum[1];
	theConfig.siteid[2]=sNum[2];theConfig.siteid[3]=sNum[3];
	theConfig.siteid[4]=sNum[4];theConfig.siteid[5]=sNum[5];
	theConfig.siteid[6]=sNum[6];theConfig.siteid[7]=sNum[7];
	memset(sNum,0x00,sizeof(sNum));
	BackToPeviouseStep();	
    return true;
}

bool ModeOkOnPress(ITUWidget* widget, char* param)
{	
	if(sNum[0]>='0' && sNum[0] < '3')
		theConfig.OperationMode = sNum[0] - '0';
	else
		theConfig.OperationMode = NORMAL_MODE;

	theConfig.FreeChargingTime = (((sNumDev[0] - '0') > 0) ? (sNumDev[0] - '0') : 0) * 10 + (((sNumDev[1] - '0') > 0) ? (sNumDev[1] - '0') : 0);

	memcpy(theConfig.chkModeMac, sNumDevDev, sizeof(sNumDevDev));

	memset(&theConfig.chkModeMac[12], NULL, 1);

	CtLogGreen("chkModeMac : %s", theConfig.chkModeMac);

	memset(sNum,0x00,sizeof(sNum));
	BackToPeviouseStep();	
    return true;
}

bool TargetSocOkOnPress(ITUWidget* widget, char* param)
{	
	if((sNum[0] >= '0') && (sNum[0] <= '9')) 
	{
		if((sNum[1] >= '0') && (sNum[1] <= '9')) 
		{
			if(sNum[2] == '0') 
			{
				if((sNum[0] == '1') && (sNum[1] == '0'))
				{
					theConfig.targetSoc = 100;
				}
				else
				{
					// theConfig.targetSoc = 100;
				}
			}
			else if(sNum[2] != ' ')
			{
				// theConfig.targetSoc = 100;
			}
			else
			{
				theConfig.targetSoc = (sNum[0] - '0') * 10 + sNum[1] - '0';
			}
		}
		else if(sNum[1] != ' ')
		{
			// theConfig.targetSoc = 100;
		}
		else 
		{
			theConfig.targetSoc = sNum[0] - '0';
		}
	}

	else {
		// theConfig.targetSoc = 100;
	}

	memset(sNum,0x00,sizeof(sNum));
	BackToPeviouseStep();	
    return true;
}


// Cancel Botton Press +++++++++++++++++++++++++++++++++++

bool CancelONpress(ITUWidget* widget, char* param)
{
	BackToPeviouseStep();
    return true;
}

bool exit2Layer(ITUWidget* widget, char* param)
{	
	BackToPeviouseStep();
    return true;
}
