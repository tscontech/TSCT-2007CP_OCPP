/**
*       @file
*               layer_charge.c
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2018.12.18 <br>
*               author: dyhwang <br>
*				modify: dsAhn <br>
*               description: <br>
*/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "SDL/SDL.h"
#include "scene.h"
#include "ctrlboard.h"
#include <time.h>
#include "ite/itp.h"
#include "cststring.h"
#include "tsctcfg.h"

//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------
#define UNIT_MS     33
#define LIMIT_OV_VOLT	26400		// +20%
#define LIMIT_UD_VOLT	19800		// -10%
#define LIMIT_OV_CURR	3500		// +10%
//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------
static ITUBackground* sLcgBackground;
static ITUBackground* sback_Power;
static ITUBackground* sback_Unit;
static ITUBackground* sback_Curr;
static ITUBackground* sback_Time;

static ITUSprite* sChargeSprite;
//static ITUText* sReadyText;
//static ITUText* seffectiveCurrentText;
static ITUIcon* sreadyTextIcon;
static ITUIcon* schargingTextIcon;
static ITUIcon* sreadyChgGageIcon;
static ITUButton* sStopChargeButton;


static ITUText* sEffectiveCurrentText;
static ITUText* sEnergyUsedText;
static ITUText* sChargeTimeText;
static ITUText* sUnitPricetxt;
static ITUText* susertxt;
static ITUText* sMemberTypetxt;

//static ITUText* seffectiveCurrenttxt1;
static ITUText* sEffectiveCurrentText1;

static ITUText* schargeTimeText;
//static ITUText* schargeTimeTitleText;


static bool bLayerChargeFlg = false;
static bool sCharging = false;
static bool sChargingInfocheck = true;
static pthread_t sChargeMonitoringTask;
static pthread_t sChargeFaultMonitoringTask;
static bool sDLsChargeMonitoring = false;
static bool sDLsChargeFaultMonitoring = false;

struct timeval stv;
struct timeval etv;   
long chargeStart;
long chargeEnd;
long sChargeTime;
int charge_stop_btnState[2];
static int sCh1CurrentZeroTime = 0;

bool chargecomp_stop; 	// Car Send Charging Stop to Charger

extern bool bAmiErrChk;

//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------
static void StartCharge(void);

int hour=0, minute=0, second=0;
int charge_price = 0;
int charge_price_ori = 0;
uint32_t charge_watt = 0;
int iUnitprice = 0;
int iUnitprice_ori = 0;
static bool StartTimeCheck;
static int pricepollCount = 0;

static bool sChargeSpritebool;//, true);
static bool sReadyTextbool;//, true);
static bool sStopChargeButtonbool;//, true);
		
//static bool seffectiveCurrenttxt1bool;//, true);
static bool sEffectiveCurrentText1bool;//, true);
		
// static bool stimeIconbool;//, true);
static bool schargeTimeTextbool;//, true);
//static bool schargeTimeTitleTextbool;//, true);

static bool sChargeSpritePlaybool;

static bool bChkCardRxFlg;

char card_no2[21], PId_no2[21];

static void GotoNextLayerOnCharge(void)
{
	if(bLayerChargeFlg){
		ituLayerGoto(ituSceneFindWidget(&theScene, "ch2FinishLayer"));
		bLayerChargeFlg = false;
	}
}

int charge_time_price(bool credit)
{
	int n_price = DEFAULT_UNITPRICE;
	unsigned short var_int = 0;
	time_t time = CstGetTime();
	struct tm *tm = localtime(&time);
	/*
	if(!credit)
	{
		if(shmDataIfInfo.connect_status)
		{
			/////////////  단가 계산
			n_price = shmDataIfInfo.unit_cost[shmDataAppInfo.member_type][tm->tm_hour][0] <<24;
			n_price |= shmDataIfInfo.unit_cost[shmDataAppInfo.member_type][tm->tm_hour][1] <<16;
			n_price |= shmDataIfInfo.unit_cost[shmDataAppInfo.member_type][tm->tm_hour][2] <<8;
			n_price |= shmDataIfInfo.unit_cost[shmDataAppInfo.member_type][tm->tm_hour][3];
		}	
		
	}
else // creait card logic 
	{
		if(shmDataIfInfo.connect_status)
		{
		n_price = shmDataIfInfo.unit_cost[shmDataAppInfo.member_type+1][tm->tm_hour][0] <<24;
		n_price |= shmDataIfInfo.unit_cost[shmDataAppInfo.member_type+1][tm->tm_hour][1] <<16;
		n_price |= shmDataIfInfo.unit_cost[shmDataAppInfo.member_type+1][tm->tm_hour][2] <<8;
		n_price |= shmDataIfInfo.unit_cost[shmDataAppInfo.member_type+1][tm->tm_hour][3];
		   
		n_price = n_price / 100; // only straffic _dsAn 190411
		}
			
	}
*/
//	memcpy(shmDataAppInfo.charge_price , shmDataIfInfo.unit_cost[shmDataAppInfo.member_type][tm->tm_hour] ,sizeof(shmDataAppInfo.charge_price ));
	return shmDataIfInfo.OCPP_iUnitprice *100;
//	return n_price;
}

int charge_time_oriprice(void)
{
	int n_price = 100;
	time_t time = CstGetTime();
	struct tm *tm = localtime(&time);
	/*
	if(shmDataIfInfo.connect_status)
	{
		/////////////  ???? 계산
		n_price = shmDataIfInfo.unit_cost[shmDataAppInfo.member_type][tm->tm_hour][0] <<24;
		n_price |= shmDataIfInfo.unit_cost[shmDataAppInfo.member_type][tm->tm_hour][1] <<16;
		n_price |= shmDataIfInfo.unit_cost[shmDataAppInfo.member_type][tm->tm_hour][2] <<8;
		n_price |= shmDataIfInfo.unit_cost[shmDataAppInfo.member_type][tm->tm_hour][3];
	}
	*/	
	/*if(n_price < 1)
		n_price = 100;*/
		return shmDataIfInfo.OCPP_iUnitprice *100;
	//return n_price;
}

static void WHMListenerOnCharge(int ch, float current, float volt, uint32_t energy)
{
	char buf[32];	
	
	if(sChargingInfocheck)
	{
		if(current < 100.0)
		{
			memset(buf, 0, 32);
			sprintf(buf, "%.1f A", current);
			ituTextSetString(sEffectiveCurrentText1, buf);
		}
	}

	if (gAmiStartWatt == -1)
	{		
		printf(" sCh1StartEnergy == -1 ch1 energy when start %d0Wh\n", energy);
		gAmiStartWatt = (float)energy;
		gAmiChargeWatt = (float)energy;
	}
	else
	{
		gAmiChargeWatt = (float)energy;
	}

	CheckCurrentIsZero(CH1, &sCh1CurrentZeroTime, sChargeTime,  current);
}

static void TimeOutChargeStartWait(void)
{	
	ShowWhmErrorDialogBox(ERR_CHARGE);
	GotoNextLayerOnCharge();
}

static void CardReaderListenerOnCharging(char *data, int size)
{
	if ( theConfig.ConfirmSelect == USER_AUTH_NET)
	{	
		printf("[ch]card uid: ");

		for (int i = 0; i < size; i++) 
		{
			printf("%c ", *(data+i));
		}

		printf("card size:[%d]\r\n",size);

		if(size > sizeof(CsConfigVal.scnd_card_no))
			return;

		memset(CsConfigVal.scnd_card_no, '0', size);
		memcpy(CsConfigVal.scnd_card_no , data , size);
		memset(&(CsConfigVal.scnd_card_no[size]), '\0', 1);

		if(!memcmp(shmDataAppInfo.card_no, data, size))
		{
			shmDataAppInfo.charge_comp_status = END_CARD;
			printf("충전 종료  memcmp(shmDataAppInfo.card_no, data, size)");
			TSCT_ChargingStop();
		}
		else
		{
			if(PId_no2[0] != '\0')
			{
				CsConfigVal.bReqAuthNo = 2;
				printf("[with PID] Unknown Card Number : ");
				for(int j = 0; j<size; j++){
					if(!(j%4)) printf(" ");
					printf("%c", *(data+j));
				}
				printf("\r\n");
			}
			else
			{
				printf("[without PID] Unknown Card Number : ");
				for(int j = 0; j<size; j++){
					if(!(j%4)) printf(" ");
					printf("%c", *(data+j));
				}
				printf("\r\n");
			}
		}	
	}
}

static void* sChargeMonitoringTaskFuntion(void* arg)
{
	char title[32];
	char buf[32];
	bool var = ((int)shmDataAppInfo.auth_type[0] == PW_INPUT_CARNUM) ? 1 : 0;
	unsigned int kwch1 = 0;
	unsigned int kwch1_2 = 0;

	ChannelType activeCh = CstGetUserActiveChannel();	
	
	// in case of Cable type charger.
	if((theConfig.devtype == BC_TYPE) || (theConfig.devtype == HBC_TYPE) \ 
	|| (theConfig.devtype == C_TYPE) || (theConfig.devtype == HC_TYPE))
		TopSetTimer(100, TimeOutChargeStartWait);
	
	while(sDLsChargeMonitoring)
	{
		sleep(1);
		if(shmDataAppInfo.app_order == APP_ORDER_CHARGING && sCharging)
		{			    
			gettimeofday(&etv, NULL);

			chargeStart = stv.tv_sec;
			chargeEnd = etv.tv_sec;	
			sChargeTime = chargeEnd - chargeStart;
				
			hour = sChargeTime / 3600;
			minute = (sChargeTime % 3600) / 60;
			second = sChargeTime % 60;
							
			pricepollCount %= 8;
			if (pricepollCount == 0)	
			{	
				iUnitprice =  charge_time_price(var);
				if(!var)	iUnitprice_ori =  charge_time_oriprice();
				else iUnitprice_ori =  charge_time_price(var);
			}								   
		   charge_watt =  gAmiChargeWatt - gAmiStartWatt;

			// printf("\n\n======= ch%d energy used: %d -> %.2f (%.2f)\n", bDevChannel, gAmiStartWatt[CH1], gAmiChargeWatt[CH1], charge_watt);
			// printf("\n\n======= ch%d energy used: %d -> %d (%d)\n", bDevChannel, gAmiStartWatt, gAmiChargeWatt, charge_watt);  LOG OFF
		    //if(charge_watt > 1000.0) continue; // 간혹 ??력??계??값이 ????경우??while??리턴??킨?? 

			if(charge_watt < 0)		charge_watt = 0;	
			
			//charge_price = (charge_watt * 1000) * iUnitprice/1000.0f; 
			charge_price = charge_watt * iUnitprice / 100; 	//
			//charge_price_ori = (charge_watt * 1000) * iUnitprice_ori/1000.0f; 
			charge_price_ori = charge_watt * iUnitprice_ori / 100; 
				
			// printf("========== price: %d,  price_ori: %d===========\n\n", charge_price, charge_price_ori);  LOG OFF
			
			unsigned int kwch1 = 0;
			unsigned int kwch1_2 = 0;
			if(charge_watt)
			{
				//kwch1 = charge_watt * 100; 
				kwch1 = charge_watt;
				//kwch1_2 = gAmiChargeWatt[CH1] * 100; 
				// When Start Charge No Count Watt
				//kwch1_2 = gAmiChargeWatt[CH1]; 
			}
			kwch1_2 = gAmiChargeWatt; 
				
			char temp[6+1] = "";
			sprintf(temp, "%02d%02d%02d", hour, minute, second);
			
			shmDataAppInfo.charge_provide_time[0] = temp[0] << 4;
			shmDataAppInfo.charge_provide_time[0] |= (temp[1] &= 0x0F);
			shmDataAppInfo.charge_provide_time[1] = temp[2] << 4;;
			shmDataAppInfo.charge_provide_time[1] |= (temp[3] &= 0x0F);
			shmDataAppInfo.charge_provide_time[2] = temp[4] << 4;
			shmDataAppInfo.charge_provide_time[2] |= (temp[5] &= 0x0F);

			ValueOrderFourByte(shmDataAppInfo.eqp_watt, kwch1_2);
			ValueOrderFourByte(shmDataAppInfo.charge_watt, kwch1);
			ValueOrderFourByte(shmDataAppInfo.charge_money, charge_price);
			ValueOrderFourByte(shmDataAppInfo.sOri_price, charge_price_ori ); 

			CstSetUsedEnergy(charge_watt, sChargeTime);
			
			pricepollCount++;
		}

				
		memset(buf, 0, 32);
		sprintf(buf, "%d.%02d %s", iUnitprice/100, iUnitprice%100, STR_PRICE_WON);
		ituTextSetString(sUnitPricetxt, buf);

		memset(buf, 0, 32);
		sprintf(buf, "%d %s", charge_price/100, STR_PRICE_WON); ///mod (추후 프로토콜 전송 값 확인)
		ituTextSetString(sEffectiveCurrentText, buf);
		//충전??금

		memset(buf, 0, 32);
		sprintf(buf, "%.2f kWh",(float)charge_watt/100.0f ); ///mod
		ituTextSetString(sEnergyUsedText, buf);
		//충전??
		
		if(sChargingInfocheck)
		{
			memset(buf, 0, 32);
			sprintf(buf, "%02d:%02d", hour, minute);
			ituTextSetString(sChargeTimeText, buf);				
		}

		if(/*bChkCardRxFlg && */(CsConfigVal.bReqAuthNo == false)){
			if(!memcmp(PId_no2, CsConfigVal.parentId, sizeof(CsConfigVal.parentId)))
			{
				if(PId_no2[0] != '\0')
				{
					// shmDataAppInfo.charge_comp_status = END_CARD;
					memcpy(shmDataAppInfo.card_no, CsConfigVal.scnd_card_no, sizeof(shmDataAppInfo.card_no));
					memcpy(StopTsConfig.IdTag, CsConfigVal.scnd_card_no, sizeof(shmDataAppInfo.card_no));
					TSCT_ChargingStop();
				}
			}

			bChkCardRxFlg = false;

			RequestPollingStart();
		}
		/*
		if(ServerCallError)
		{
			StopCharge();
			ituLayerGoto(ituSceneFindWidget(&theScene, "ch2FinishLayer"));
		}
*/
		if(startTsQ.faultChargFlg)
		{
			// shmDataAppInfo.charge_comp_status = END_CARD;
			if(CfgKeyVal[14].CfgKeyDataInt)
			{
				TSCT_ChargingStop();
			}
			else
			{
				SetCpStatus(CP_STATUS_CODE_SUSPENDEDEVSE, 1);
			}
		}


		if(	SeccTxData.status_fault & (1<<SECC_STAT_STOP)) {
			CtLogRed("Charge Stop by SECC [%lu]", SeccTxData.status_fault);
			//ShowWhmErrorDialogBox(ERR_CHARGE);
			TSCT_ChargingStop();
		}		

		if(theConfig.OperationMode == OP_FREE_MODE \
		&& theConfig.FreeChargingTime != 0	\
		&& sChargeTime/60 > theConfig.FreeChargingTime)	
		{
			TSCT_ChargingStop();
		}
			

		if(CsConfigVal.bReqRmtStopTSFlg){
			// CsConfigVal.bReqRmtStopTSFlg = false;
			TSCT_ChargingStop();
		}	
	}
	sChargeMonitoringTask = 0;
}
void TSCT_ChargingStop()
{
	printf("[TSCT_ChargingStop] Charging Stop %d\r\n",shmDataAppInfo.charge_comp_status);
	sChCharging = false;
	//CstSetUserActiveChannel(CH1);
	// ControlPilotSetListener(bDevChannel, NULL);
	LEDStopBlink();	
	MagneticContactorOff();
	usleep(300*1000);
	StopPwm(CH1);
	WattHourMeterStopMonitoring(CH1);
	shmDataAppInfo.app_order = APP_ORDER_CHARGING_STOP;						
	GotoNextLayerOnCharge();
}
static void ChargeFaultMonitoringTaskFuntion(void *arg)
{
	uint8_t Fault_Count[3] = {0,};						// [0]:Over Current, [1]:Over Voltage, [2]:Under Voltage
	uint16_t chk_Current = 0, chk_Volt = 0;
	while(sDLsChargeFaultMonitoring)
	{
		sleep(1);
		// Sever Connection Check

		// AMI Connection Check
		if(bAmiErrChk && !CstGetEpqStatus(CST_COM_WHM)){		
			CstSetEpqStatus(CST_COM_WHM, false);
			ShowWhmErrorDialogBox(ERR_AMI_DISCON);				
		}
		else	CstSetEpqStatus(CST_COM_WHM, true);

		chk_Current = TSCTGetAMICurrent();
		chk_Volt = TSCTGetAMIVolt();

		if(chk_Current > LIMIT_OV_CURR)	Fault_Count[0]++;	
		else	Fault_Count[0] = 0;
		if(chk_Volt > LIMIT_OV_VOLT)	Fault_Count[1]++;	
		else	Fault_Count[1] = 0;
		if(chk_Volt < LIMIT_UD_VOLT)	Fault_Count[2]++;	
		else	Fault_Count[2] = 0;
		
		if(Fault_Count[0] > 10){ 
			CtLogRed("Charge Over Current Fault!!!!!!!!!!!!!!!!!!!");
			Fault_Count[0] = 0;
			Fault_Count[1] = 0;
			Fault_Count[2] = 0;
			shmDataAppInfo.charge_comp_status = END_ERR;
			CstSetEpqStatus(CST_OUT_OVER_CURRENT, false);
			ShowWhmErrorDialogBox(ERR_OV_CURT);
			charger_errcode = ERR_CODE_OV_CURT;
			SetCpStatus(CP_STATUS_CODE_FAULT, bDevChannel+1);
		}
		if(Fault_Count[1] > 10){ 
			CtLogRed("Charge Over Voltage Fault!!!!!!!!!!!!!!!!!!!");		
			Fault_Count[0] = 0;
			Fault_Count[1] = 0;
			Fault_Count[2] = 0;
			shmDataAppInfo.charge_comp_status = END_ERR;
			CstSetEpqStatus(CST_OUT_OVER_VOLTAGE, false);
			ShowWhmErrorDialogBox(ERR_OV_VOLT);
			charger_errcode = ERR_CODE_OV_VOLT;
			SetCpStatus(CP_STATUS_CODE_FAULT, bDevChannel+1);
		}
		if(Fault_Count[2] > 10){ 
			CtLogRed("Charge Under Voltage Fault!!!!!!!!!!!!!!!!!!!");
			Fault_Count[0] = 0;
			Fault_Count[1] = 0;
			Fault_Count[2] = 0;
			shmDataAppInfo.charge_comp_status = END_ERR;
			CstSetEpqStatus(CST_IN_UNDER_VOLTAGE, false);
			ShowWhmErrorDialogBox(ERR_UD_VOLT);
			charger_errcode = ERR_CODE_UD_VOLT;
			SetCpStatus(CP_STATUS_CODE_FAULT, bDevChannel+1);
		}
		// CtLogYellow("ChargeFault Cnt : %d / Current : %d, Volt : %d ", Fault_Count, chk_Current, chk_Volt); LOG OFF
	}
	sChargeFaultMonitoringTask = 0;
}

static void StartCharge(void)
{
	ChannelType activeCh = CstGetUserActiveChannel();
	// if(sReadyTextbool)
	// {
	// 	sReadyTextbool = false;
	// 	//ituWidgetSetVisible(sReadyText, false);
	// 	ituWidgetSetVisible(sreadyTextIcon, false);
	// 	ituWidgetSetVisible(sreadyChgGageIcon, false);
	// 	ituWidgetSetVisible(schargingTextIcon, true);
	// 	ituWidgetSetVisible(sEffectiveCurrentText, true);				
	// }	
	// ituSpritePlay(sChargeSprite, CH1);	
	// sChargeSpritePlaybool = true;
	
	// LEDStartBlink();
	MagneticContactorOn();
	usleep(300*1000);
	sCharging = true;	
	
	if((theConfig.devtype == BC_TYPE)|| (theConfig.devtype == HBC_TYPE)||
		(theConfig.devtype == C_TYPE) || (theConfig.devtype == HC_TYPE) )
		TopCloseTimer();
}

void StopCharge(void)
{
	printf("Stop Charge~~~~\r\n");
	MagneticContactorOff();
	
	ScreenOnScenario(); // _dsAn 200228
	
	ituSpriteStop(sChargeSprite);
	sChargeSpritePlaybool = false;
	if(sReadyTextbool) 	{
		sReadyTextbool = false;
		//ituWidgetSetVisible(sReadyText, false);		
		ituWidgetSetVisible(sreadyTextIcon, false);
		ituWidgetSetVisible(sreadyChgGageIcon, false);
		//ituWidgetSetVisible(schargingTextIcon, true);		
	}	
	ituSpriteGoto(sChargeSprite, 0);
	LEDStopBlink();
	TopHomeBtnVisible(false);
	usleep(300*1000);
	StopPwm(bDevChannel);	
	sCharging = false;	
	usleep(300*1000);
}

static void UpdateStartGui()
{	
	if(!sChargeSpritebool)
	{
		sChargeSpritebool = true;
		ituWidgetSetVisible(sChargeSprite, true);			
	}	
			
	if(!sStopChargeButtonbool)
	{
		sStopChargeButtonbool = true;
		ituWidgetSetVisible(sStopChargeButton, true);			
	}		

}

void UpdateStopGui()
{
	ituSpriteStop(sChargeSprite);
	sChargeSpritePlaybool = false;
	if(sChargeSpritebool)
	{
		sChargeSpritebool = false;
		ituWidgetSetVisible(sChargeSprite, false);			
	}	
	if(sStopChargeButtonbool)
	{
		sStopChargeButtonbool = false;
		ituWidgetSetVisible(sStopChargeButton, false);			
	}		

	//ituTextSetString(sEffectiveCurrentText1, "0.0 A");

	GotoNextLayerOnCharge();
}

static void CPListenerOnCharge(int ch, unsigned char nAdcValue, CPVoltage voltage)
{
	if(EmgControl) return;
	
	switch(voltage)
	{
		case CP_VOLTAGE_UNKNOWN:
			chargecomp_stop = false;
		    break;

		case CP_VOLTAGE_12V:
			if(CfgKeyVal[13].CfgKeyDataInt){
				ScreenOnScenario();
				chargecomp_stop = false;			
				shmDataAppInfo.app_order = APP_ORDER_CHARGING_STOP;
				shmDataAppInfo.charge_comp_status = END_UNPLUG;
				CtLogRed("Charge Stop by Disconnect");
				StopCharge();
				UpdateStopGui(); 
				sleep(1); ///mod
			}
			else{
				// if (sCharging){ 	
					if(CstGetMcstatus()){	
						MagneticContactorOff();	
						StartStopCharging(1, ch);
						chargecomp_stop = true;
						SetCpStatus(CP_STATUS_CODE_SUSPENDEDEV,bDevChannel+1);
					}
				// }
			}
			if(sCharging){
				GotoNextLayerOnCharge();
			}
			else{
				TimeOutChargeStartWait();
			}
			break;
			
		case CP_VOLTAGE_9V:
			if (sCharging)
			{ 	
				if(CstGetMcstatus())
				{	
					MagneticContactorOff();	
					StartStopCharging(1, ch);
					chargecomp_stop = true;
				}
			}
			break;

		case CP_VOLTAGE_6V:
			if(!EmgControl && !CsConfigVal.bReqRmtStopTSFlg)
			{
				if (!sCharging && (charge_stop_btnState[bDevChannel] == 0)) 
				{
					// if(sChCharging == false)				
						
					if(StartTimeCheck)
					{
						AudioPlay("A:/sounds/startCharge.wav", NULL);
						ituWidgetSetVisible(susertxt, false);
						StartTimeCheck = false;
						shmDataAppInfo.app_order = APP_ORDER_CHARGING;
						gettimeofday(&stv, NULL);
						SetCpStatus(CP_STATUS_CODE_CHARGING, bDevChannel+1);  
						
						ituSpritePlay(sChargeSprite, CH1);	
						sChargeSpritePlaybool = true;
					}		
			
					shmDataAppInfo.app_order = APP_ORDER_CHARGING;
					StartCharge();
					UpdateStartGui();				
				}
				else
				{		
					if(!CstGetMcstatus() && (charge_stop_btnState[bDevChannel] == 0))	
					{
						chargecomp_stop = false;
						MagneticContactorOn();
						StartStopCharging(0, ch);
					}
				}
			}
			break;			
	}
}

static void ChargeStopOnDialog(void)
{
	if (sCharging) 
	{		
		charge_stop_btnState[bDevChannel] = 1;
	    shmDataAppInfo.app_order = APP_ORDER_CHARGING_STOP;
		CtLogRed("Charge Stop by Button");
		StopCharge();
		WattHourMeterStopMonitoring(bDevChannel);
		shmDataAppInfo.charge_comp_status = END_BTN;
	}
	
	TopCloseTimer();
	UpdateStopGui();
}

bool InfoAddOnPress(ITUWidget* widget, char* param)
{	
	// ituWidgetSetVisible(seffectiveCurrenticon1, true);
	//ituWidgetSetVisible(seffectiveCurrenttxt1, true);
	ituWidgetSetVisible(sEffectiveCurrentText1, true);

	// ituWidgetSetVisible(stimeIcon, true);
	ituWidgetSetVisible(schargeTimeText, true);
	//ituWidgetSetVisible(schargeTimeTitleText, true);
	return true;
}

bool ChargeStopOnPress(ITUWidget* widget, char* param)
{	
	if(sReadyTextbool)
	{
		sReadyTextbool = false;
		//ituWidgetSetVisible(sReadyText, false);		
		ituWidgetSetVisible(sreadyTextIcon, false);
		ituWidgetSetVisible(sreadyChgGageIcon, false);
		//ituWidgetSetVisible(schargingTextIcon, true);		
	}	
	OkCancelDialogSetOkListener(ChargeStopOnDialog, NULL);	
	OkCancelDialogShow();
	
	return true;
}

bool ChargeOnEnter(ITUWidget* widget, char* param)
{
	char buf[32];
	stime = 0;
	ChannelType activeCh = CstGetUserActiveChannel();	
	CtLogRed("	[%d]  Enter charge layer..==========\n", bDevChannel);

	if(theConfig.OperationMode != OP_CHECK_MODE)
		SeccTxData.status_fault |= 1<<SECC_STAT_CHARG;

	bLayerChargeFlg = true;

	sChCharging = true;

	StartTimeCheck = true;
	hour=0; minute=0; second=0;
	sCharging = false;
	sChargeTime = 0;
	sCh1CurrentZeroTime = 0;
	gAmiChargeWatt = -1;
	gAmiStartWatt = -1;
	charge_stop_btnState[bDevChannel] = 0;
	CstSetUsedEnergy(0.0, 0);

	theConfig.chargingstatus |= (1<<(MAX_CONECTOR_ID+1));

	StopTsConfig.Connector_No = bDevChannel;

	StopTsConfig.TrId = CsConfigVal.bTrId[bDevChannel+1];

	GetDateTime(StopTsConfig.Time_Stamp);

	StopTsConfig.MeterStop_Val = 0;

	for(int i =0;i<4;i++)
		StopTsConfig.MeterStop_Val += (uint32_t)(shmDataAppInfo.eqp_watt[i] << ((3-i)*8));

	memcpy(StopTsConfig.IdTag, shmDataAppInfo.card_no, sizeof(shmDataAppInfo.card_no));
	memset(&(StopTsConfig.IdTag[16]), '\0', 1);

	StopTsConfig.Stop_Reason = STOP_REASON_POWER;


	memset(shmDataAppInfo.charge_provide_time, 0x00, sizeof(shmDataAppInfo.charge_provide_time));
	memset(shmDataAppInfo.charge_watt, 0x00, sizeof(shmDataAppInfo.charge_watt));
	memset(shmDataAppInfo.charge_money, 0x00, sizeof(shmDataAppInfo.charge_money));
	
	shmDataAppInfo.app_order = APP_ORDER_CHARGE_READY;

	if (!sLcgBackground)
	{
		sLcgBackground = ituSceneFindWidget(&theScene, "chargeBackground");
		assert(sLcgBackground);

		sChargeSprite = ituSceneFindWidget(&theScene, "chargeSprite");
		assert(sChargeSprite);

		sback_Power = ituSceneFindWidget(&theScene, "back_Power");
		assert(sback_Power);
		sback_Unit = ituSceneFindWidget(&theScene, "back_Unit");
		assert(sback_Unit);
		sback_Curr = ituSceneFindWidget(&theScene, "back_Curr");
		assert(sback_Curr);
		sback_Time = ituSceneFindWidget(&theScene, "back_Time");
		assert(sback_Time);

		schargingTextIcon = ituSceneFindWidget(&theScene, "chargingTextIcon");
		sreadyTextIcon = ituSceneFindWidget(&theScene, "readyTextIcon");
		sreadyChgGageIcon = ituSceneFindWidget(&theScene, "readyChgGageIcon");

		sEffectiveCurrentText = ituSceneFindWidget(&theScene, "effectiveCurrentText");
		assert(sEffectiveCurrentText);					
	
		sUnitPricetxt = ituSceneFindWidget(&theScene, "UnitPricetxt");
		assert(sUnitPricetxt);		
		sEnergyUsedText = ituSceneFindWidget(&theScene, "energyUsedText");
		assert(sEnergyUsedText);
		sChargeTimeText = ituSceneFindWidget(&theScene, "chargeTimeText");
		assert(sChargeTimeText);
		sStopChargeButton = ituSceneFindWidget(&theScene, "stopChargeButton");
		assert(sStopChargeButton);		
		susertxt = ituSceneFindWidget(&theScene, "usertxt");
		assert(susertxt);		
		
		sEffectiveCurrentText1 = ituSceneFindWidget(&theScene, "effectiveCurrentText1");
		assert(sEffectiveCurrentText1);
		sMemberTypetxt = ituSceneFindWidget(&theScene, "MemberTypetxt");
						
	}
	iUnitprice = charge_time_price(0);
		
	memset(buf, 0, 32);
	sprintf(buf, "%d.%02d %s", iUnitprice/100, iUnitprice%100, STR_PRICE_WON);
	ituTextSetString(sUnitPricetxt, buf);
	charge_price = 0;
	
	memset(buf, 0, 32);
	sprintf(buf, "%d %s", charge_price/100, STR_PRICE_WON);
	ituTextSetString(sEffectiveCurrentText, buf);

	ituTextSetString(sEffectiveCurrentText1, "0.0 A");
	
	//충전??금
	charge_watt = 0;
	memset(buf, 0, 32);
	sprintf(buf, "%.2f kWh",(float)charge_watt/100.0f); ///mod
	ituTextSetString(sEnergyUsedText, buf);
	ituTextSetString(sChargeTimeText, "00:00");	

	if(shmDataAppInfo.member_type == MEM_MEBER)
	{
		ituTextSetString(sMemberTypetxt, "태성콘텍 회원");
	}
	else if (shmDataAppInfo.member_type == MEM_ROAM)
	{
		ituTextSetString(sMemberTypetxt, "로밍 회원");
	}
	else if (shmDataAppInfo.member_type == MEM_GUEST)
	{
		ituTextSetString(sMemberTypetxt, "비회원");
	}
	else if (shmDataAppInfo.member_type == MEM_VIP)
	{
		ituTextSetString(sMemberTypetxt, "VIP 회원");
	}
	else if (shmDataAppInfo.member_type == MEM_NON)
	{
		ituTextSetString(sMemberTypetxt, "");
	}
	
	
	ControlPilotSetListener(bDevChannel, CPListenerOnCharge);

	StartPwm(bDevChannel);
	WattHourMeterStartMonitoring(activeCh, WHMListenerOnCharge);

	// memcpy(card_no2, shmDataAppInfo.card_no, sizeof(shmDataAppInfo.card_no));
	// memset(&(card_no2[16]), '\0', 1);
	memcpy(PId_no2, CsConfigVal.parentId, sizeof(CsConfigVal.parentId));
	memset(CsConfigVal.scnd_card_no,'\0', sizeof(CsConfigVal.scnd_card_no));
	memset(CsConfigVal.parentId, '\0', sizeof(CsConfigVal.parentId));

	//AudioPlay("A:/sounds/startCharge.wav", NULL);
	
	bChkCardRxFlg = false;
	CardReaderStartMonitoring(CardReaderListenerOnCharging);
	
	if(!sChargeSpritebool)
	{
		sChargeSpritebool = true;
		ituWidgetSetVisible(sChargeSprite, true);			
	}	
		
	if(!sStopChargeButtonbool)
	{
		sStopChargeButtonbool = true;
		ituWidgetSetVisible(sStopChargeButton, true);			
	}		
	// if(!sReadyTextbool)
	// {
	// 	sReadyTextbool = true;
	// 	//ituWidgetSetVisible(sReadyText, true);
	// 	ituWidgetSetVisible(sreadyChgGageIcon, true);
	// 	ituWidgetSetVisible(sreadyTextIcon, true);
	// 	ituWidgetSetVisible(schargingTextIcon, false);
	// 	ituWidgetSetVisible(sEffectiveCurrentText, false);
	// }

	// if(sReadyTextbool)
	// {
		// sReadyTextbool = false;
		//ituWidgetSetVisible(sReadyText, false);
		ituWidgetSetVisible(sreadyTextIcon, false);
		ituWidgetSetVisible(sreadyChgGageIcon, false);
		ituWidgetSetVisible(schargingTextIcon, true);
		ituWidgetSetVisible(sEffectiveCurrentText, true);	
		ituWidgetSetVisible(susertxt, true);
	// }	
	//ituSpritePlay(sChargeSprite, CH1);	
	//sChargeSpritePlaybool = true;
	
	LEDStartBlink();

	TopHomeBtnVisible(false);

	sChargingInfocheck = true;
	// ituWidgetSetVisible(seffectiveCurrenticon1, true);
	//ituWidgetSetVisible(seffectiveCurrenttxt1, true);
	//ituWidgetSetVisible(sEffectiveCurrentText1, true);

	// ituWidgetSetVisible(stimeIcon, true);
	//ituWidgetSetVisible(schargeTimeText, true);
	//ituWidgetSetVisible(schargeTimeTitleText, true);
	
	if (sChargeMonitoringTask == 0)
	{
		sDLsChargeMonitoring = true;
		pthread_create(&sChargeMonitoringTask, NULL, sChargeMonitoringTaskFuntion, NULL);
		pthread_detach(sChargeMonitoringTask);
	}

	if (sChargeFaultMonitoringTask == 0)
	{
		sDLsChargeFaultMonitoring = true;
		pthread_create(&sChargeFaultMonitoringTask, NULL, ChargeFaultMonitoringTaskFuntion, NULL);
		pthread_detach(sChargeFaultMonitoringTask);
	}
	
	if(EmgControl && shmDataAppInfo.app_order <= APP_ORDER_CHARGING)
	{
		UpdateStopGui();

	}

	ConfigSave();

    return true;
}

bool ChargeOnLeave(ITUWidget* widget, char* param)
{	
	char buf[8]={0x30,};

	SeccTxData.status_fault &= ~(1<<SECC_STAT_CHARG);
	SeccTxData.status_fault |= 1<<SECC_STAT_STOP;

	sprintf(buf, "0%s", STR_PRICE_WON);
	ChannelType activeCh = CstGetUserActiveChannel();
	
	sChCharging = false;

	StopTsConfig.Connector_No = bDevChannel;

	StopTsConfig.TrId = CsConfigVal.bTrId[bDevChannel+1];

	GetDateTime(StopTsConfig.Time_Stamp);

	StopTsConfig.MeterStop_Val = 0;

	for(int i =0;i<4;i++)
		StopTsConfig.MeterStop_Val += (uint32_t)(shmDataAppInfo.eqp_watt[i] << ((3-i)*8));

	memcpy(StopTsConfig.IdTag, shmDataAppInfo.card_no, sizeof(shmDataAppInfo.card_no));
	memset(&(StopTsConfig.IdTag[16]), '\0', 1);

	/*
	ituTextSetString(sEffectiveCurrentText1, "0.0 A");
	ituTextSetString(sUnitPricetxt, buf);
	ituTextSetString(sEffectiveCurrentText, buf);
	ituTextSetString(sEnergyUsedText, "0.00 kWh");
	ituTextSetString(sChargeTimeText, "00:00");
	*/

	// ControlPilotSetListener(bDevChannel, NULL);

	chargecomp_stop = false;

	if(!sleepOn1chCheck)
	{		
		sCharging = false;
		TopStopStepAnimation();	 

		StopCharge();		
		usleep(100*1000);
		printf("ChargeOnLeave :: sleepOn1chCheck %d \n", sleepOn1chCheck);
		
		WattHourMeterStopMonitoring(activeCh);
		if(sChargeSpritePlaybool)
		{
			sChargeSpritePlaybool = false;
			ituSpriteStop(sChargeSprite);
			ituSpriteGoto(sChargeSprite, 0);
		}
		
		if(sChargeSpritebool)
		{
			sChargeSpritebool = false;
			ituWidgetSetVisible(sChargeSprite, false);			
		}	
		sDLsChargeMonitoring = false;	
		sDLsChargeFaultMonitoring = false;
	}
	if(EmgControl)
	{
		printf("ChargeOnLeave :: EmgControl %d \n", EmgControl);

		TopStopStepAnimation();	 
		sCharging = false;
		WattHourMeterStopMonitoring(activeCh);
		if(sChargeSpritePlaybool)
		{
			sChargeSpritePlaybool = false;
			ituSpriteStop(sChargeSprite);
			ituSpriteGoto(sChargeSprite, 0);
		}
		if(sChargeSpritebool)
		{
			sChargeSpritebool = false;
			ituWidgetSetVisible(sChargeSprite, false);			
		}
		
		sDLsChargeMonitoring = false;		
		sDLsChargeFaultMonitoring = false;

		// CsConfigVal.bReqRmtStopTSFlg = false;
	}

	CsConfigVal.bReqAuthNo = false;
	CardReaderStopMonitoring();

	usleep(200*1000);
	CtLogRed("Leave charge layer\n");	
	return true;
}

void ChargeReset(void)
{
	sLcgBackground = NULL;
}


