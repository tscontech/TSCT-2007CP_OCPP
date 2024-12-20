/**
*       @file
*               layer_FtpFwUpdate.c
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2019.04.12 <br>
*               author: ktlee <br>
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
#include "cststring.h"

static ITUBackground* sFtpFwUpdateBackground;
static ITUBackground* sFtpKeypad;
static ITUTextBox*	sFtpIptxt;
static ITUTextBox*	sFtpIdtxt;
static ITUTextBox*	sFtpPasswordtxt;

static ITUButton* sFtpBtn_1;
static ITUButton* sFtpBtn_2;
static ITUButton* sFtpBtn_3;
static ITUButton* sFtpBtn_4;
static ITUButton* sFtpBtn_5;
static ITUButton* sFtpBtn_6;
static ITUButton* sFtpBtn_7;
static ITUButton* sFtpBtn_8;
static ITUButton* sFtpBtn_9;
static ITUButton* sFtpBtn_10;
static ITUButton* sFtpBtn_11;
static ITUButton* sFtpBtn_12;
static ITUButton* sFtpBtn_13;
static ITUButton* sFtpBtn_14;
static ITUButton* sFtpBtn_15;

char FtpKeyArr[3][15] = 
{ /// 0   1   2   3   4   5   6   7   8   9   10  11  12  13  14
	{'1','2','3','4','5','6','7','8','9','0','a','b','c','d','e'},
	{'f','g','h','i','j','k','l','m','n','o','p','q','r','s','t'},
	{'u','v','w','x','y','x','.',',','!','@','#','$','%','&','*'}
};

char FtpKeyArrC[3][15] = 
{
	{'1','2','3','4','5','6','7','8','9','0','A','B','C','D','E'},
	{'F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T'},
	{'U','V','W','X','Y','Z','.',',','!','@','#','$','%','&','*'}
};

static bool bShiftKey = false;
static unsigned char nSelectInfo = 0;
static int nFtpKeyType = 0;

static char sFtpIp[40] = {' ',};
static char sFtpUserId[10] = {' ',};
static char sFtpPW[16] = {' ',};

static char *GetCharKeyPad(char num)
{
	printf(" GetCharKeyPad ");

	if(	bShiftKey && ((nFtpKeyType==1)|| ((nFtpKeyType==2)&&(num < 12))))
	{
		return FtpKeyArrC[nFtpKeyType][num];  // Capital Alphabat...
	}
	else
	{
		return FtpKeyArr[nFtpKeyType][num];
	}	
}

static bool CheckIpAddress(char * sIp) 
{
    char buf[16], *saveptr;
	unsigned char Temp0, Temp1, Temp2, Temp3;

    strcpy(buf, sIp);
    Temp0 = atoi(strtok_r(buf, ".", &saveptr));	
    Temp1 = atoi(strtok_r(NULL, ".", &saveptr));
    Temp2 = atoi(strtok_r(NULL, ".", &saveptr));
    Temp3 = atoi(strtok_r(NULL, " ", &saveptr));
	
	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> %d %d %d %d \n", Temp0, Temp1, Temp2, Temp3);
	
	if((Temp3 > 0) && (Temp0 > 0))
	{
		return true;
	}	
	else 
		return false;
}

static void FtpInfoUpdate(int num)
{
	int length = 0;
	char sTxtTemp[32];

	memset(sTxtTemp, 0x0, 32);

	if(nSelectInfo == 0)  // FTP IP
	{		
		//if((nFtpKeyType == 0)  && ((num <11)||(num==17)))  // number or '.' or del key...	
		//{		
			length = strlen(sFtpIp);
			if((num ==17))  // del key
			{	
				if(length > 0)
					sFtpIp[length-1] = '\0';
			}
			else
			{			
				sprintf(&sFtpIp[length], "%c", GetCharKeyPad(num));
				sFtpIp[length+1] = '\0';			
			}
			
			if(strlen(sFtpIp) > 0)		
			{			
				ituTextSetString(sFtpIptxt, STR_BLANK_STRING);
				sprintf(sTxtTemp, sFtpIp, strlen(sFtpIp)+1);
				ituTextSetString(sFtpIptxt, sTxtTemp);
			}
		//}
	}
	else  if(nSelectInfo == 1)  // FTP USER ID
	{
		length = strlen(sFtpUserId);
		if(num ==17)  // del key
		{
			if(length > 0)
				sFtpUserId[length-1] = '\0';
		}
		else
		{
			sprintf(&sFtpUserId[length], "%c", GetCharKeyPad(num));
			sFtpUserId[length+1] = '\0';
		}
		
		ituTextSetString(sFtpIdtxt, STR_BLANK_STRING);
		if(strlen(sFtpUserId) > 0)
		{
			sprintf(sTxtTemp, sFtpUserId, strlen(sFtpUserId)+1);
			ituTextSetString(sFtpIdtxt, sTxtTemp);
		}		
	}
	else if(nSelectInfo ==2)   // FTP Password
	{
		length = strlen(sFtpPW);
		if(num ==17)  // del key
		{
			if(length > 0)
				sFtpPW[length-1] = '\0';
		}
		else
		{
			sprintf(&sFtpPW[length], "%c", GetCharKeyPad(num));
			sFtpPW[length+1] = '\0';
		}
		
		ituTextSetString(sFtpPasswordtxt, STR_BLANK_STRING);
		if(strlen(sFtpPW) > 0)
		{		
			sprintf(sTxtTemp, sFtpPW, strlen(sFtpPW)+1);
			ituTextSetString(sFtpPasswordtxt, sTxtTemp);
		}		
	}
	
	printf("FtpInfoUpdate :: nSelectInfo[%d] length %d, %s  \n", nSelectInfo, length, sTxtTemp);
}


void FtpKeypadUpdate(void)
{
	char* buf[2] = {};
	int i;
	
	for(i=0; i<15; i++)
	{
		if(	bShiftKey && ((nFtpKeyType==1)|| ((nFtpKeyType==2)&&(i < 12))))
		{	
			sprintf(buf, "%c", FtpKeyArrC[nFtpKeyType][i]);	
		}
		else
		{
			sprintf(buf, "%c", FtpKeyArr[nFtpKeyType][i]);
		}
		
		switch(i)
		{
			case 0:		ituButtonSetString(sFtpBtn_1, buf);			break;
			case 1: 	ituButtonSetString(sFtpBtn_2, buf);			break;
			case 2: 	ituButtonSetString(sFtpBtn_3, buf);			break;
			case 3:		ituButtonSetString(sFtpBtn_4, buf);			break;
			case 4:		ituButtonSetString(sFtpBtn_5, buf);			break;
			case 5:		ituButtonSetString(sFtpBtn_6, buf);			break;
			case 6: 	ituButtonSetString(sFtpBtn_7, buf);			break;
			case 7: 	ituButtonSetString(sFtpBtn_8, buf);			break;
			case 8: 	ituButtonSetString(sFtpBtn_9, buf);			break;
			case 9: 	ituButtonSetString(sFtpBtn_10, buf);		break;
			case 10: 	ituButtonSetString(sFtpBtn_11, buf);		break;
			case 11: 	ituButtonSetString(sFtpBtn_12, buf);		break;
			case 12: 	ituButtonSetString(sFtpBtn_13, buf);		break;
			case 13: 	ituButtonSetString(sFtpBtn_14, buf);		break;
			case 14: 	ituButtonSetString(sFtpBtn_15, buf);		break;
		}				
	}
}

bool FtpFwUpdateEnter(ITUWidget* widget, char* param)
{
	if (!sFtpFwUpdateBackground)
	{
		sFtpFwUpdateBackground = ituSceneFindWidget(&theScene, "FtpFwUpdateBackground");
		assert(sFtpFwUpdateBackground);
		
		sFtpKeypad = ituSceneFindWidget(&theScene, "FtpKeypad");
		assert(sFtpKeypad);
		
		sFtpIptxt = ituSceneFindWidget(&theScene, "FtpIptxt");
		assert(sFtpIptxt);
		
		sFtpPasswordtxt = ituSceneFindWidget(&theScene, "FtpPasswordtxt");
		assert(sFtpPasswordtxt);
		
		sFtpIdtxt = ituSceneFindWidget(&theScene, "FtpIdtxt");
		assert(sFtpIdtxt);
		
		sFtpBtn_1 = ituSceneFindWidget(&theScene, "FtpBtn_1");
		assert(sFtpBtn_1);

		sFtpBtn_2 = ituSceneFindWidget(&theScene, "FtpBtn_2");
		assert(sFtpBtn_2);

		sFtpBtn_3 = ituSceneFindWidget(&theScene, "FtpBtn_3");
		assert(sFtpBtn_3);

		sFtpBtn_4 = ituSceneFindWidget(&theScene, "FtpBtn_4");
		assert(sFtpBtn_4);

		sFtpBtn_5 = ituSceneFindWidget(&theScene, "FtpBtn_5");
		assert(sFtpBtn_5);

		sFtpBtn_6 = ituSceneFindWidget(&theScene, "FtpBtn_6");
		assert(sFtpBtn_6);

		sFtpBtn_7 = ituSceneFindWidget(&theScene, "FtpBtn_7");
		assert(sFtpBtn_7);

		sFtpBtn_8 = ituSceneFindWidget(&theScene, "FtpBtn_8");
		assert(sFtpBtn_8);

		sFtpBtn_9 = ituSceneFindWidget(&theScene, "FtpBtn_9");
		assert(sFtpBtn_9);

		sFtpBtn_10 = ituSceneFindWidget(&theScene, "FtpBtn_10");
		assert(sFtpBtn_10);

		sFtpBtn_11 = ituSceneFindWidget(&theScene, "FtpBtn_11");
		assert(sFtpBtn_11);

		sFtpBtn_12 = ituSceneFindWidget(&theScene, "FtpBtn_12");
		assert(sFtpBtn_12);

		sFtpBtn_13 = ituSceneFindWidget(&theScene, "FtpBtn_13");
		assert(sFtpBtn_13);

		sFtpBtn_14 = ituSceneFindWidget(&theScene, "FtpBtn_14");
		assert(sFtpBtn_14);

		sFtpBtn_15 = ituSceneFindWidget(&theScene, "FtpBtn_15");
		assert(sFtpBtn_15);
	}

	ituWidgetSetVisible(sFtpFwUpdateBackground, true);
	FtpKeypadUpdate();
	ituWidgetSetVisible(sFtpKeypad, true);

	bGloAdminStatus = true;

	memset(sFtpIp, 0x0, sizeof(sFtpIp));
	memset(sFtpUserId, 0x0, sizeof(sFtpUserId));
	memset(sFtpPW, 0x0, sizeof(sFtpPW));
	
	//strcpy(sFtpIp, theConfig.ftpIp);
	strcpy(sFtpIp, theConfig.ftpDns);
	strcpy(sFtpUserId, theConfig.ftpId);
	strcpy(sFtpPW, theConfig.ftpPw);

	printf(" FtpFwUpdateEnter  \\\\\\\\\\\\\\\ \n");
	printf(" FTP IP: %s \n", sFtpIp);
	printf(" FTP USER ID: %s \n", sFtpUserId);
	printf(" FTP Password : %s \n", sFtpPW);

	if(strlen(sFtpIp)==0)		ituTextSetString(sFtpIptxt, STR_BLANK_STRING);
	else						ituTextSetString(sFtpIptxt, sFtpIp);
	
	if(strlen(sFtpPW)==0)		ituTextSetString(sFtpPasswordtxt, STR_BLANK_STRING);
	else						ituTextSetString(sFtpPasswordtxt, sFtpPW);
	
	if(strlen(sFtpUserId)==0)		ituTextSetString(sFtpIdtxt, STR_BLANK_STRING);
	else						ituTextSetString(sFtpIdtxt, sFtpUserId);

	nSelectInfo = 0;

	ituTextSetBackColor(sFtpIptxt, 150, 255, 255, 255);
	ituTextSetBackColor(sFtpIdtxt, 0, 255, 255, 255);
	ituTextSetBackColor(sFtpPasswordtxt, 0, 255, 255, 255);

	ituWidgetSetVisible(sFtpIptxt, true);
	ituWidgetSetVisible(sFtpIdtxt, true);
	ituWidgetSetVisible(sFtpPasswordtxt, true);
		
    return true;
}

bool FtpFwUpdateLeave(ITUWidget* widget, char* param)
{
	printf(" FtpFwUpdateLeave \n");

	ituWidgetSetVisible(sFtpFwUpdateBackground, false);
    return true;
}

bool FtpKeypadOnPress(ITUWidget* widget, char* param)
{
	int Num = atoi(param);

	switch(Num)
	{
		case 0:		case 1:		case 2:		case 3:
		case 4:		case 5:		case 6:		case 7:
		case 8: 	case 9: 	case 10: 	case 11:
		case 12: 	case 13: 	case 14:	case 17:
			FtpInfoUpdate(Num);
			break;
			
		case 15:  /// Shift Key		
			bShiftKey = !bShiftKey;
			
			if(nFtpKeyType > 0)		FtpKeypadUpdate();			
			break;
		
		case 16:  // Change Key Pad Charactor..		
			{
				nFtpKeyType++;	
				if(nFtpKeyType > 2)		nFtpKeyType = 0;
				FtpKeypadUpdate();					
			}
			break;
	}
    return true;
}

bool FtpFwUpdatOnPress(ITUWidget* widget, char* param)
{
	//memset(theConfig.ftpIp, 0x0, 16);
	memset(theConfig.ftpDns, 0x0, 40);
	memset(theConfig.ftpId, 0x0, 10);
	memset(theConfig.ftpPw, 0x0, 16);
	
	//strcpy(theConfig.ftpIp, sFtpIp);	
	//strcpy(theConfig.ftpDns, sFtpIp);
	strcpy(theConfig.ftpDns, sFtpIp);
	strcpy(theConfig.ftpId, sFtpUserId);
	strcpy(theConfig.ftpPw, sFtpPW);
	strcpy(theConfig.ftpPath, "/fw/2007a/ITEPKG03.PKG");

	//printf("FTP IP : %s, %s  \n", theConfig.ftpIp, sFtpIp);
	printf("FTP IP : %s, %s  \n", theConfig.ftpDns, sFtpIp);
	printf("FTP ID : %s, %s  \n", theConfig.ftpId, sFtpUserId);
	printf("FTP PW : %s, %s  \n", theConfig.ftpPw, sFtpPW);
	
	ConfigSave();

	bFTPupgrade = FTP_FW_UD_WAIT_CON;	
	
	ituWidgetSetVisible(sFtpFwUpdateBackground, false);
	
	FtpFwUpdate_func();

	//FtpIMGUpdate_func();

	// reset

	printf("\n\n soon reset....\n\n");

	usleep(2000*1000);

	custom_reboot();
	
	while(1);
	
	return true;			
}

bool FtpFwUpdatCancelOnPress(ITUWidget* widget, char* param)
{
		ituWidgetSetVisible(sFtpFwUpdateBackground, false);
		ituLayerGoto(ituSceneFindWidget(&theScene, "AdminSetIpLayer"));
    return true;
}

bool FtpInfoSelect(ITUWidget* widget, char* param)
{
	nSelectInfo = (unsigned char)atoi(param);	
	printf(" FtpInfoSelect [[ %d ]]\n", nSelectInfo);
	
	ituTextSetBackColor(sFtpIptxt, 0, 255, 255, 255);
	ituTextSetBackColor(sFtpIdtxt, 0, 255, 255, 255);
	ituTextSetBackColor(sFtpPasswordtxt, 0, 255, 255, 255);

	switch(nSelectInfo)
	{
		case 1:
			ituTextSetBackColor(sFtpIdtxt, 150, 200, 255, 255);	
			break;
			
		case 2:		
			ituTextSetBackColor(sFtpPasswordtxt, 150, 200, 255, 255);	
			break;
		
		default:		
		case 0:		
			ituTextSetBackColor(sFtpIptxt, 150, 200, 255, 255);	
			break;
	}	
}

void FtpLayerReset(void)
{
	sFtpFwUpdateBackground = NULL;
}

