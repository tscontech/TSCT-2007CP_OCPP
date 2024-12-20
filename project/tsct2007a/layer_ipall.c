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


static ITUBackground* sLiBackground;


static ITUBackground* sgatewaydialogBackground;
static ITUTextBox* sgateway1txt;

static ITUBackground* sserveripdialogBackground;
static ITUTextBox* sserverip1txt;

static ITUBackground* ssubnetdialogBackground;
static ITUTextBox* ssubnetmask1txt;

static ITUBackground* schargeripdialogBackground;
static ITUTextBox* schargerip1txt;
static ITUTextBox* schargerip2txt;
static ITUTextBox* schargerip3txt;
static ITUTextBox* schargerip4txt;

static ITUBackground* sportdialogBackground;
static ITUTextBox* sserverporttxt;

static ITUBackground* sMacAddressBackground;

static ITUTextBox* sMacaddresstxt;

static ITUBackground* snum1KeypadBackground;
static sNumkeyInputListener saInputListener = NULL;

static ITUBackground* sIpKeypad;

static ITUButton* sIpBtn_1;
static ITUButton* sIpBtn_2;
static ITUButton* sIpBtn_3;
static ITUButton* sIpBtn_4;
static ITUButton* sIpBtn_5;
static ITUButton* sIpBtn_6;
static ITUButton* sIpBtn_7;
static ITUButton* sIpBtn_8;
static ITUButton* sIpBtn_9;
static ITUButton* sIpBtn_10;
static ITUButton* sIpBtn_11;
static ITUButton* sIpBtn_12;
static ITUButton* sIpBtn_13;
static ITUButton* sIpBtn_14;
static ITUButton* sIpBtn_15;
static ITUButton* sIpBtn_16;

char IpKeyArr[3][15] = 
{ /// 0   1   2   3   4   5   6   7   8   9   10  11  12  13  14
	{'1','2','3','4','5','6','7','8','9','0','a','b','c','d','e'},
	{'f','g','h','i','j','k','l','m','n','o','p','q','r','s','t'},
	{'u','v','w','x','y','x','.',':','!','@','#','/','%','&','-'}
};

char IpKeyArrC[3][15] = 
{
	{'1','2','3','4','5','6','7','8','9','0','A','B','C','D','E'},
	{'F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T'},
	{'U','V','W','X','Y','Z','.',',','!','@','#','$','%','&','*'}
};


static char sIp[MAX_ADDRESS] = {' ',};

static char sNum[16];
static char sIpNum[40];
static int sCount = 0;

static int keypad_count = 0;

static bool bShiftKey = false;
static int nIpKeyType = 0;

void NumKeypadSetInputListenergate(sNumkeyInputListener listener)
{
	saInputListener = listener;
}
static void ResetNumbergate()
{
	memset(sNum,0x00,sizeof(sNum));
	
	strncpy(sNum,theConfig.gw,sizeof(sNum));
	ituTextSetString(sgateway1txt, sNum);
	
	sCount = strlen(theConfig.gw);
}
static void UpdateNumbergate()
{
	char buf[16];
	
	strncpy(buf, sNum, sizeof(sNum));
	ituTextSetString(sgateway1txt, buf);
}

static void NumInputListenergate(char c)
{
	if (c != 'd') // number
	{
		if (sCount < 15)
		{
			sNum[sCount++] = c;
		}
	}
	else // delete (back space)
	{
		if (sCount > 0)
		{
			sNum[--sCount] = ' ';
			//sCount--;
		}
	}
	UpdateNumbergate();		
}


static void ResetNumbermac()
{
	memset(sIp,0x00,sizeof(sIp));

	sprintf(sIp, "%s", theConfig.chargermac);

	printf("mac : %s, sIp ; %s\r\n", theConfig.chargermac, sIp);

	ituTextBoxSetString(sMacaddresstxt, sIp);	
}

static void ResetNumberserverip()
{
	memset(sIp,0x00,sizeof(sIp));

	sprintf(sIp, "%s", theConfig.serverip);
	
	printf("serverip : %s, sIp ; %s\r\n", theConfig.serverip, sIp);

	ituTextSetString(sserverip1txt, sIp);
}

void NumKeypadSetInputListenersubnet(sNumkeyInputListener listener)
{
	saInputListener = listener;
}
static void ResetNumbersubnet()
{
	memset(sNum,0x00,sizeof(sNum));
	
	strncpy(sNum,theConfig.netmask,sizeof(sNum));
	ituTextSetString(ssubnetmask1txt, sNum);
	
	sCount = strlen(theConfig.netmask);
}
static void UpdateNumbersubnet()
{
	char buf[16];
	
	strncpy(buf, sNum, sizeof(sNum));
	ituTextSetString(ssubnetmask1txt, buf);

	//sprintf(buf, "%c%c%c", sNum[0], sNum[1], sNum[2]);
	//ituTextBoxSetString(ssubnetmask1txt, buf);
}
static void NumInputListenersubnet(char c)
{
	if (c != 'd') // number
	{
		if (sCount < 15)
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
	UpdateNumbersubnet();		
}


void NumKeypadSetInputListenerchargerip(sNumkeyInputListener listener)
{
	saInputListener = listener;
}
static void ResetNumberchargerip()
{
	memset(sNum,0x00,sizeof(sNum));
	
	strncpy(sNum,theConfig.ipaddr,sizeof(sNum));
	ituTextSetString(schargerip1txt, theConfig.ipaddr);
	
	sCount = strlen(theConfig.ipaddr);
}
static void UpdateNumberchargerip()
{
	char buf[16];
	
	strncpy(buf, sNum, sizeof(sNum));
	ituTextSetString(schargerip1txt, buf);
}
static void NumInputListenerchargerip(char c)
{

	if (c != 'd') // number
	{
		if (sCount < 15)
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
	UpdateNumberchargerip();		
}


void NumKeypadSetInputListenersport(sNumkeyInputListener listener)
{
	saInputListener = listener;
}
static void ResetNumbersport()
{
	memset(sNum,' ',sizeof(sNum));
	sprintf(sNum, "%d", theConfig.serverport);
	//strncpy(sNum,theConfig.serverport,sizeof(sNum));
	ituTextSetString(sserverporttxt, sNum);
	
	sCount = strlen(sNum);
	/*
	sCount = 6;
	
	char buf[32];	
	memset(sNum,' ',sizeof(sNum));
	sprintf(buf, "%d", theConfig.serverport);
	ituTextBoxSetString(sserverporttxt, buf);
	sNum[0]=buf[0];sNum[1]=buf[1];sNum[2]=buf[2];
	sNum[3]=buf[3];sNum[4]=buf[4];sNum[5]=buf[5];
	*/
}
static void UpdateNumbersport()
{
	char buf[32];
	
	sprintf(buf, "%s", sNum);
	ituTextBoxSetString(sserverporttxt, buf);
}
static void NumInputListenersport(char c)
{
	if (c != 'd') // number
	{
		if (sCount < 6)
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
	UpdateNumbersport();		
}


static void FindIpAllDialogWidget(void)
{	
	if (!sLiBackground)
	{
		sLiBackground = ituSceneFindWidget(&theScene, "IpAllLayerBackground");
		assert(sLiBackground);
		
		sgatewaydialogBackground = ituSceneFindWidget(&theScene, "gatewaydialogBackground");
		assert(sgatewaydialogBackground);

		sgateway1txt = ituSceneFindWidget(&theScene, "gateway1txt");
		assert(sgateway1txt);	
		
		sserveripdialogBackground = ituSceneFindWidget(&theScene, "serveripdialogBackground");
		assert(sserveripdialogBackground);

		sserverip1txt = ituSceneFindWidget(&theScene, "serverip1txt");
		assert(sserverip1txt);
				
		ssubnetdialogBackground = ituSceneFindWidget(&theScene, "subnetdialogBackground");
		assert(ssubnetdialogBackground);

		ssubnetmask1txt = ituSceneFindWidget(&theScene, "subnetmask1txt");
		assert(ssubnetmask1txt);
				
		schargeripdialogBackground = ituSceneFindWidget(&theScene, "chargeripdialogBackground");
		assert(schargeripdialogBackground);

		schargerip1txt = ituSceneFindWidget(&theScene, "chargerip1txt");
		assert(schargerip1txt);
				
		sportdialogBackground = ituSceneFindWidget(&theScene, "portdialogBackground");
		assert(sportdialogBackground);
		sserverporttxt = ituSceneFindWidget(&theScene, "serverporttxt");
		assert(sserverporttxt);
		
		sMacAddressBackground = ituSceneFindWidget(&theScene, "MacAddressBackground");
		assert(sMacAddressBackground);
		sMacaddresstxt = ituSceneFindWidget(&theScene, "Macaddresstxt");
		assert(sMacaddresstxt);
	}	
}

static char *GetCharIpKeyPad(char num)
{
	printf(" GetCharKeyPad \r\n");

	if(	bShiftKey && ((nIpKeyType==1)|| ((nIpKeyType==2)&&(num < 12))))
	{
		return IpKeyArrC[nIpKeyType][num];  // Capital Alphabat...
	}
	else
	{
		return IpKeyArr[nIpKeyType][num];
	}	
}

static void IpInfoUpdate(int num)
{
	int length = 0;
	char sTxtTemp[MAX_ADDRESS];

	memset(sTxtTemp, 0x0, MAX_ADDRESS);
	
		length = strlen(sIp);
		if((num ==17))  // del key
		{	
			if(length > 0)
				sIp[length-1] = '\0';
		}
		else
		{			
			sprintf(&sIp[length], "%c", GetCharIpKeyPad(num));
			sIp[length+1] = '\0';			
		}
		
		if(strlen(sIp) >= 0)		
		{	
			// set IP Text		
			if(setipselect == 1){
				ituTextSetString(sserverip1txt, " ");
				sprintf(sTxtTemp, sIp, strlen(sIp)+1);
				ituTextSetString(sserverip1txt, sTxtTemp);
			}
			// set Mac Text
			else{
				ituTextSetString(sMacaddresstxt, " ");
				sprintf(sTxtTemp, sIp, strlen(sIp)+1);
				ituTextSetString(sMacaddresstxt, sTxtTemp);
			}
		}

	
	printf("FtpInfoUpdate :: length %d, %s  \n",  length, sTxtTemp);
}

void IpKeypadUpdate(void)
{
	char* buf[2] = {};
	int i;
	
	for(i=0; i<15; i++)
	{
		if(	bShiftKey && ((nIpKeyType==1)|| ((nIpKeyType==2)&&(i < 12))))
		{	
			sprintf(buf, "%c", IpKeyArrC[nIpKeyType][i]);	
		}
		else
		{
			sprintf(buf, "%c", IpKeyArr[nIpKeyType][i]);
		}
		
		switch(i)
		{
			case 0:		ituButtonSetString(sIpBtn_1, buf);			break;
			case 1: 	ituButtonSetString(sIpBtn_2, buf);			break;
			case 2: 	ituButtonSetString(sIpBtn_3, buf);			break;
			case 3:		ituButtonSetString(sIpBtn_4, buf);			break;
			case 4:		ituButtonSetString(sIpBtn_5, buf);			break;
			case 5:		ituButtonSetString(sIpBtn_6, buf);			break;
			case 6: 	ituButtonSetString(sIpBtn_7, buf);			break;
			case 7: 	ituButtonSetString(sIpBtn_8, buf);			break;
			case 8: 	ituButtonSetString(sIpBtn_9, buf);			break;
			case 9: 	ituButtonSetString(sIpBtn_10, buf);			break;
			case 10: 	ituButtonSetString(sIpBtn_11, buf);			break;
			case 11: 	ituButtonSetString(sIpBtn_12, buf);			break;
			case 12: 	ituButtonSetString(sIpBtn_13, buf);			break;
			case 13: 	ituButtonSetString(sIpBtn_14, buf);			break;
			case 14: 	ituButtonSetString(sIpBtn_15, buf);			break;
		}				
	}
	CtLogCyan("Kepad Upadate %d-%d",bShiftKey, nIpKeyType);
}

bool ipAllLayer_Enter(ITUWidget* widget, char* param)
{
	switch(setipselect)
	{
		case 1:
		{//SetNetFuncOnPress
			
			FindIpAllDialogWidget();
	
			ituWidgetSetVisible(sLiBackground, true);
			ituWidgetSetVisible(sserveripdialogBackground, true);

			if(!sIpKeypad)
			{
				sIpKeypad = ituSceneFindWidget(&theScene, "IpKeypad");
				assert(sIpKeypad);

				sIpBtn_1 = ituSceneFindWidget(&theScene, "IpBtn_1");
				assert(sIpBtn_1);

				sIpBtn_2 = ituSceneFindWidget(&theScene, "IpBtn_2");
				assert(sIpBtn_2);

				sIpBtn_3 = ituSceneFindWidget(&theScene, "IpBtn_3");
				assert(sIpBtn_3);

				sIpBtn_4 = ituSceneFindWidget(&theScene, "IpBtn_4");
				assert(sIpBtn_4);

				sIpBtn_5 = ituSceneFindWidget(&theScene, "IpBtn_5");
				assert(sIpBtn_5);

				sIpBtn_6 = ituSceneFindWidget(&theScene, "IpBtn_6");
				assert(sIpBtn_6);

				sIpBtn_7 = ituSceneFindWidget(&theScene, "IpBtn_7");
				assert(sIpBtn_7);

				sIpBtn_8 = ituSceneFindWidget(&theScene, "IpBtn_8");
				assert(sIpBtn_8);

				sIpBtn_9 = ituSceneFindWidget(&theScene, "IpBtn_9");
				assert(sIpBtn_9);

				sIpBtn_10 = ituSceneFindWidget(&theScene, "IpBtn_10");
				assert(sIpBtn_10);

				sIpBtn_11 = ituSceneFindWidget(&theScene, "IpBtn_11");
				assert(sIpBtn_11);

				sIpBtn_12 = ituSceneFindWidget(&theScene, "IpBtn_12");
				assert(sIpBtn_12);

				sIpBtn_13 = ituSceneFindWidget(&theScene, "IpBtn_13");
				assert(sIpBtn_13);

				sIpBtn_14 = ituSceneFindWidget(&theScene, "IpBtn_14");
				assert(sIpBtn_14);

				sIpBtn_15 = ituSceneFindWidget(&theScene, "IpBtn_15");
				assert(sIpBtn_15);
			}
			IpKeypadUpdate();
			ResetNumberserverip();			
			ituWidgetSetVisible(sIpKeypad, true);
			break;
		}
		case 2:
		{//SetNetFuncOnPress
			
			FindIpAllDialogWidget();

			ituWidgetSetVisible(sLiBackground, true);
			ituWidgetSetVisible(sportdialogBackground, true);
			//ituWidgetSetVisible(sIpKeypad, false);
			if(!snum1KeypadBackground)
			{
				snum1KeypadBackground = ituSceneFindWidget(&theScene, "num1KeypadBackground");
				assert(snum1KeypadBackground);
			}
			ResetNumbersport();
			ituWidgetSetVisible(snum1KeypadBackground, true);
			NumKeypadSetInputListenersport(NumInputListenersport);
			
			break;
		}
		case 3:
		{//SetNetFuncOnPress
		
			FindIpAllDialogWidget();
			
			ituWidgetSetVisible(sLiBackground, true);
			ituWidgetSetVisible(schargeripdialogBackground, true);
			 if(!snum1KeypadBackground)
			 {
				snum1KeypadBackground = ituSceneFindWidget(&theScene, "num1KeypadBackground");
				assert(snum1KeypadBackground);
			 }		
			 //ituWidgetSetVisible(sIpKeypad, false);
			ResetNumberchargerip();		
			ituWidgetSetVisible(snum1KeypadBackground, true);
			NumKeypadSetInputListenerchargerip(NumInputListenerchargerip);
			
			break;
		}
		case 4:
		{//SetNetFuncOnPress
			FindIpAllDialogWidget();	
	
			ituWidgetSetVisible(sLiBackground, true);
			ituWidgetSetVisible(sgatewaydialogBackground, true);			
			 if(!snum1KeypadBackground)
			 {
				snum1KeypadBackground = ituSceneFindWidget(&theScene, "num1KeypadBackground");
				assert(snum1KeypadBackground);
			 }
			 //ituWidgetSetVisible(sIpKeypad, false);
			ResetNumbergate();
			ituWidgetSetVisible(snum1KeypadBackground, true);
			NumKeypadSetInputListenergate(NumInputListenergate);
			
			break;
		}
		case 5:
		{//SetNetFuncOnPress
			FindIpAllDialogWidget();
	
			ituWidgetSetVisible(sLiBackground, true);
			ituWidgetSetVisible(ssubnetdialogBackground, true);
			if(!snum1KeypadBackground)
			{
				snum1KeypadBackground = ituSceneFindWidget(&theScene, "num1KeypadBackground");
				assert(snum1KeypadBackground);
			}			
			ResetNumbersubnet();
			ituWidgetSetVisible(snum1KeypadBackground, true);
			NumKeypadSetInputListenersubnet(NumInputListenersubnet);
			
			break;
		}
		case 6:
		{//SetNetFuncOnPress
			
			FindIpAllDialogWidget();
	
			ituWidgetSetVisible(sLiBackground, true);
			ituWidgetSetVisible(sMacAddressBackground, true);

			if(!sIpKeypad)
			{
				sIpKeypad = ituSceneFindWidget(&theScene, "IpKeypad");
				assert(sIpKeypad);

				sIpBtn_1 = ituSceneFindWidget(&theScene, "IpBtn_1");
				assert(sIpBtn_1);

				sIpBtn_2 = ituSceneFindWidget(&theScene, "IpBtn_2");
				assert(sIpBtn_2);

				sIpBtn_3 = ituSceneFindWidget(&theScene, "IpBtn_3");
				assert(sIpBtn_3);

				sIpBtn_4 = ituSceneFindWidget(&theScene, "IpBtn_4");
				assert(sIpBtn_4);

				sIpBtn_5 = ituSceneFindWidget(&theScene, "IpBtn_5");
				assert(sIpBtn_5);

				sIpBtn_6 = ituSceneFindWidget(&theScene, "IpBtn_6");
				assert(sIpBtn_6);

				sIpBtn_7 = ituSceneFindWidget(&theScene, "IpBtn_7");
				assert(sIpBtn_7);

				sIpBtn_8 = ituSceneFindWidget(&theScene, "IpBtn_8");
				assert(sIpBtn_8);

				sIpBtn_9 = ituSceneFindWidget(&theScene, "IpBtn_9");
				assert(sIpBtn_9);

				sIpBtn_10 = ituSceneFindWidget(&theScene, "IpBtn_10");
				assert(sIpBtn_10);

				sIpBtn_11 = ituSceneFindWidget(&theScene, "IpBtn_11");
				assert(sIpBtn_11);

				sIpBtn_12 = ituSceneFindWidget(&theScene, "IpBtn_12");
				assert(sIpBtn_12);

				sIpBtn_13 = ituSceneFindWidget(&theScene, "IpBtn_13");
				assert(sIpBtn_13);

				sIpBtn_14 = ituSceneFindWidget(&theScene, "IpBtn_14");
				assert(sIpBtn_14);

				sIpBtn_15 = ituSceneFindWidget(&theScene, "IpBtn_15");
				assert(sIpBtn_15);
			}
			IpKeypadUpdate();
			ResetNumbermac();			
			ituWidgetSetVisible(sIpKeypad, true);
			break;
		}
		
	}
	
    return true;
}

bool ipAllLayer_Leave(ITUWidget* widget, char* param)
{	
	switch(setipselect)
	{
		case 1:
		{//SetNetFuncOnPress

			ituWidgetSetVisible(sLiBackground, false);
			ituWidgetSetVisible(sserveripdialogBackground, false);
			ituWidgetSetVisible(sIpKeypad, false);				
			break;
		}
		case 2:
		{//SetNetFuncOnPress

			ituWidgetSetVisible(sLiBackground, false);
			ituWidgetSetVisible(sportdialogBackground, false);
			ituWidgetSetVisible(snum1KeypadBackground, false);
			NumKeypadSetInputListenersport(NULL);
			
			break;
		}
		case 3:
		{//SetNetFuncOnPress
		
			ituWidgetSetVisible(sLiBackground, false);
			ituWidgetSetVisible(schargeripdialogBackground, false);	
			ituWidgetSetVisible(snum1KeypadBackground, false);
			NumKeypadSetInputListenerchargerip(NULL);			
			break;
		}
		case 4:
		{//SetNetFuncOnPress
			ituWidgetSetVisible(sLiBackground, false);
			ituWidgetSetVisible(sgatewaydialogBackground, false);			
			ituWidgetSetVisible(snum1KeypadBackground, false);				
			NumKeypadSetInputListenergate(NULL);			
			break;
		}
		case 5:
		{//SetNetFuncOnPress

			ituWidgetSetVisible(sLiBackground, false);
			ituWidgetSetVisible(ssubnetdialogBackground, false);			
			ituWidgetSetVisible(snum1KeypadBackground, false);
			NumKeypadSetInputListenersubnet(NULL);			
			break;
		}		
		case 6:	//	Set Mac Address
		{
			ituWidgetSetVisible(sLiBackground, false);
			ituWidgetSetVisible(sMacAddressBackground, false);
			ituWidgetSetVisible(sIpKeypad, false);
			break;
		}
	}	
	setipselect = 0;
    return true;
}

bool exit1Layer(ITUWidget* widget, char* param)
{
	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetIpLayer"));		
    return true;
}

bool aNumKeypadDelOnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('d');
    return true;
}

bool aNumKeypad0OnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('0');
    return true;
}

bool aNumKeypad9OnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('9');
    return true;
}

bool aNumKeypad8OnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('8');
    return true;
}

bool aNumKeypad7OnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('7');
    return true;
}

bool aNumKeypad6OnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('6');
    return true;
}

bool aNumKeypad5OnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('5');
    return true;
}

bool aNumKeypad4OnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('4');
    return true;
}

bool aNumKeypad3OnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('3');
    return true;
}

bool aNumKeypad2OnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('2');
    return true;
}

bool aNumKeypad1OnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('1');
    return true;
}

bool aNumKeypadDotOnPress(ITUWidget* widget, char* param)
{
	if (saInputListener)
		(*saInputListener)('.');
    return true;
}


bool serverportOkOnPress(ITUWidget* widget, char* param)
{
	theConfig.serverport = atoi(sNum);        

	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetIpLayer"));
    return true;
}
bool chargeripsetOkOnPress(ITUWidget* widget, char* param)
{
	memset(theConfig.ipaddr,' ',sizeof(theConfig.ipaddr));	
	sprintf(theConfig.ipaddr,sNum);
	/*
	theConfig.ipaddr[0]=sNum[0];theConfig.ipaddr[1]=sNum[1];
	theConfig.ipaddr[2]=sNum[2];theConfig.ipaddr[3]='.';
	theConfig.ipaddr[4]=sNum[3];theConfig.ipaddr[5]=sNum[4];
	theConfig.ipaddr[6]=sNum[5];theConfig.ipaddr[7]='.';
	theConfig.ipaddr[8]=sNum[6];theConfig.ipaddr[9]=sNum[7];
	theConfig.ipaddr[10]=sNum[8];theConfig.ipaddr[11]='.';
	theConfig.ipaddr[12]=sNum[9];theConfig.ipaddr[13]=sNum[10];
	theConfig.ipaddr[14]=sNum[11];
	*/
	memset(sNum,0x00,sizeof(sNum));
	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetIpLayer"));
    return true;
}
bool subnetmaskOkOnPress(ITUWidget* widget, char* param)
{
	memset(theConfig.netmask,' ',sizeof(theConfig.netmask));	
	sprintf(theConfig.netmask,sNum);
	/*
	theConfig.netmask[0]=sNum[0];theConfig.netmask[1]=sNum[1];
	theConfig.netmask[2]=sNum[2];theConfig.netmask[3]='.';
	theConfig.netmask[4]=sNum[3];theConfig.netmask[5]=sNum[4];
	theConfig.netmask[6]=sNum[5];theConfig.netmask[7]='.';
	theConfig.netmask[8]=sNum[6];theConfig.netmask[9]=sNum[7];
	theConfig.netmask[10]=sNum[8];theConfig.netmask[11]='.';
	theConfig.netmask[12]=sNum[9];theConfig.netmask[13]=sNum[10];
	theConfig.netmask[14]=sNum[11];
	*/
	memset(sNum,0x00,sizeof(sNum));
	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetIpLayer"));
    return true;
}
bool ServeripOkOnPress(ITUWidget* widget, char* param)
{
	memcpy(theConfig.serverip, sIp, sizeof(sIp));

	memset(sIp,0x00,sizeof(sIp));
	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetIpLayer"));
    return true;
}
bool gatewayOkOnPress(ITUWidget* widget, char* param)
{
	memset(theConfig.gw,' ',sizeof(theConfig.gw));	
	sprintf(theConfig.gw,sNum);
	/*
	theConfig.gw[0]=sNum[0];theConfig.gw[1]=sNum[1];
	theConfig.gw[2]=sNum[2];theConfig.gw[3]='.';
	theConfig.gw[4]=sNum[3];theConfig.gw[5]=sNum[4];
	theConfig.gw[6]=sNum[5];theConfig.gw[7]='.';
	theConfig.gw[8]=sNum[6];theConfig.gw[9]=sNum[7];
	theConfig.gw[10]=sNum[8];theConfig.gw[11]='.';
	theConfig.gw[12]=sNum[9];theConfig.gw[13]=sNum[10];
	theConfig.gw[14]=sNum[11];
	*/
	memset(sNum,0x00,sizeof(sNum));
	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetIpLayer"));
    return true;
}

bool MacAddressOkOnPress(ITUWidget* widget, char* param)
{
	memcpy(theConfig.chargermac, sIp, sizeof(theConfig.chargermac));
	memset(sIp,0x00,sizeof(sIp));

	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetIpLayer"));
    return true;
}

bool serverportcancelOnPress(ITUWidget* widget, char* param)
{
	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetIpLayer"));
    return true;
}
bool ServeripOkCancelPress(ITUWidget* widget, char* param)
{
	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetIpLayer"));
    return true;
}

bool chargeripsetCancelOnPress(ITUWidget* widget, char* param)
{
	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetIpLayer"));
    return true;
}

bool subnetmaskCancelOnPress(ITUWidget* widget, char* param)
{
	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetIpLayer"));
    return true;
}

bool ChargerGateWayCancelOnPress(ITUWidget* widget, char* param)
{
	ITULayer* layer = ituSceneFindWidget(&theScene, "AdminSetIpLayer");
	ituLayerGoto(layer);
    return true;
}

bool MacAddressCancelOnPress(ITUWidget* widget, char* param)
{
	ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetIpLayer"));
    return true;
}

bool IpKeypadOnPress(ITUWidget* widget, char* param)
{
	int Num = atoi(param);
	CtLogCyan("Keypad Input %d", Num);
	switch(Num)
	{
		case 0:		case 1:		case 2:		case 3:
		case 4:		case 5:		case 6:		case 7:
		case 8: 	case 9: 	case 10: 	case 11:
		case 12: 	case 13: 	case 14:	case 17:
			IpInfoUpdate(Num);
			break;
			
		case 15:  /// Shift Key		
			bShiftKey = !bShiftKey;
			
			if(nIpKeyType > 0)		IpKeypadUpdate();			
			break;
		
		case 16:  // Change Key Pad Charactor..		
			{
				nIpKeyType++;	
				if(nIpKeyType > 2)		nIpKeyType = 0;
				IpKeypadUpdate();					
			}
			break;
	}
    return true;
}