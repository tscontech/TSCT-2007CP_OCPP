/**
*       @file
*               cstcardreader.c
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2017.12.05 <br>
*               author: dyhwang <br>
*               description: <br>
*/
#include <string.h>
#include <unistd.h>
#include "ite/audio.h"
#include "audio_mgr.h"
#include "ctrlboard.h"
#include "tsctcommon.h"
#include "cstcardreader.h"
#include "tsctcfg.h"

#include "uart/uart.h"
#include "ite/ith.h"
#include "ite/itp.h"
#include "openrtos/queue.h"
#include <errno.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include "openrtos/FreeRTOS.h"
#include "openrtos/queue.h"
#include "ite/ith.h"
#include "ite/itp.h"
#include "uart/uart.h"
#include "uart/uart_intr.h"
#include "uart/uart_dma.h"
#include "uart/uart_fifo.h"

// #include "tsctsecc.h"


//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------
#define SECC_READ_232_TASK_DELAY		25 // ms
#define SECC_READ_ID_TASK_DELAY		50 // ms

#define CHRG_ID			0x01
#define CAN_ADDR_MAX 	30030
#define CAN_ADDR_MIN 	30000
#define FC_READ			0x04
#define FC_WRITE		0x10

#define MAX_LEN			90

//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------
static pthread_t sReqPlcTask;

static pthread_t sReadPlcTask;


uint16_t PlcRespWaitCnt = false;

bool SendChkFlg = false;

uint8_t charge_cnt = 0;

uint8_t vasReadCd = false;

uint16_t vasDataBuf[512];

bool bPlcConn = false;
size_t rebootCnt = 0;

//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------

static void* SeccRS232ReadTask(void* arg);

/**
 * @brief Little Endian Data => Big Endian Data (16bit) 
 */
static uint16_t Led2Bed16(uint16_t Led16)
{
	uint16_t tmp_u16;

	tmp_u16 = (Led16 >> 8) & 0xff;
	tmp_u16 += (Led16 << 8) & 0xff00;

	return tmp_u16;
}

static uint8_t MakeReadTx(uint16_t Addr16, uint16_t AddrLen16)
{
	SeccModbusReadTx.Id01 = CHRG_ID;
	SeccModbusReadTx.FuncCode = FC_READ;
	SeccModbusReadTx.ReqAddr = Led2Bed16(Addr16);
	SeccModbusReadTx.ReqAddrLen = Led2Bed16(AddrLen16);
	SeccModbusReadTx.Crc16 = Led2Bed16(TsctCrc16(&(SeccModbusReadTx.Id01), 6));	

	return 1;
}

static uint8_t MakeWriteTx(uint16_t Addr16, uint16_t AddrLen16)
{
	uint8_t crc_tmp16[20];
	
	SeccModbusWriteTx.Id01 = CHRG_ID;
	SeccModbusWriteTx.FuncCode = FC_WRITE;
	SeccModbusWriteTx.ReqAddr = Led2Bed16(Addr16);
	SeccModbusWriteTx.ReqAddrLen = Led2Bed16(AddrLen16);

	SeccModbusWriteTx.Datalen = (uint8_t)(2 * AddrLen16);

	memcpy(crc_tmp16, &(SeccModbusWriteTx.Id01), 7);
	memcpy(&crc_tmp16[7], SeccModbusWriteTx.Data16, SeccModbusWriteTx.Datalen);

	SeccModbusWriteTx.Crc16 = Led2Bed16(TsctCrc16(crc_tmp16, 7 + SeccModbusWriteTx.Datalen));

	return 2;	
}


static uint8_t ReadVasData(void)
{
	// for Read Count
	int32_t tmp16 = SeccRxData.vasMapCnt - 10 - ( MAX_LEN * (vasReadCd-1) );

	if( tmp16 > MAX_LEN ) {
		return MakeReadTx(32011 + ( MAX_LEN * (vasReadCd-1) ), MAX_LEN);
	}
	else {
		return MakeReadTx(32011 + ( MAX_LEN * (vasReadCd-1) ), tmp16);
	}
}

static uint8_t ChkVasData(void)
{
	return MakeReadTx(32001, 10);
}

static uint8_t ChckSeccStat(void)
{
	return MakeReadTx(30002, 6);
}

static uint8_t SendChrgReady(void)
{
	SeccModbusWriteTx.Data16[0] = Led2Bed16(SeccTxData.status_fault);

	return MakeWriteTx(40001, 1);
}

static uint8_t SendChrgPrm(void)
{
	SeccModbusWriteTx.Data16[0] = Led2Bed16(SeccTxData.MaxCurr);
	SeccModbusWriteTx.Data16[1] = Led2Bed16(SeccTxData.NormVolt);
	SeccModbusWriteTx.Data16[2] = Led2Bed16(SeccTxData.NormFreq);
	SeccModbusWriteTx.Data16[3] = Led2Bed16(SeccTxData.MaxPower);

	sleep(1);

	return MakeWriteTx(40027, 4);
}

uint8_t InitSecc(void)
{
	uint8_t crc_tmp16[20];

	switch (SeccInitStep){
		case SECC_INIT_STEP_NONE:	
		// 1. Check CSM Ready = True [30001 - 0]
			if(SeccRxData.ready == 0) {
				return MakeReadTx(30001, 1);
			}
			else {
				SeccInitStep = SECC_INIT_STEP_CSMR;
				SendChkFlg = true;
			}
		break;

		case SECC_INIT_STEP_CSMR :
		// 2. Send EVSE Ready = True [40001 - 0]
			if(SendChkFlg) {
				return SendChrgReady();				
			}
			else {
				SeccInitStep = SECC_INIT_STEP_EVSER;
				SendChkFlg = true;
			}
		break;

		case SECC_INIT_STEP_EVSER :
			if(SendChkFlg) {
				return SendChrgPrm();
			}
			else {
				SeccInitStep = SECC_INIT_STEP_CHRGPRM;
				SendChkFlg = true;
			}
		break;

		case SECC_INIT_STEP_CHRGPRM :
			if(SendChkFlg) {
				// SeccModbusWriteTx.Data16[0] = Led2Bed16(0);	// ISO15118
				// SeccModbusWriteTx.Data16[0] = Led2Bed16(64);	// 61851 Only
				// SeccModbusWriteTx.Data16[0] = Led2Bed16(128);	// ISO15118 with 61851 AC Fail
				SeccModbusWriteTx.Data16[0] = Led2Bed16(192);	// ISO15118 with 61851 AC Fail

				return MakeWriteTx(40006, 1);			
			}
			else {
				SeccInitStep = SECC_INIT_STEP_CHRGTYPE;
				SendChkFlg = true;
			}
		break;
			
		case SECC_INIT_STEP_CHRGTYPE :
			if(SendChkFlg) {
				SeccModbusWriteTx.Data16[0] = Led2Bed16(SeccTxData.payment);

				return MakeWriteTx(40007, 1);	
			}
			else {
				// SeccInitStep = SECC_INIT_STEP_PAYOPT;
				// SendChkFlg = true;
				SeccInitStep = SECC_INIT_STEP_INIT;
			}
		break;
			
		case SECC_INIT_STEP_PAYOPT :
			if(SendChkFlg) {
				SeccModbusWriteTx.Data16[0] = Led2Bed16(SeccTxData.status_fault);

				return MakeWriteTx(40001, 1);				
			}
			else {
				SeccInitStep = SECC_INIT_STEP_INIT;
				// SeccInitStep = SECC_INIT_STEP_CSMSTAT;
			}
		break;

		case SECC_INIT_STEP_INIT :
			if((SeccRxData.stcode == CSM_STAT_NONE) || (SeccRxData.secc_errcode != 0) /*|| (SeccRxData.pwmduty != 100) || (SeccRxData.pwmvoltage != 120) */) {
				return MakeReadTx(30002, 6);
			}
			else {
				// SeccInitStep = SECC_INIT_STEP_CSMERR;
				SendChkFlg = true;
				
				SeccInitStep = SECC_INIT_STEP_VASREQ;
			}

		break;

		case SECC_INIT_STEP_VASREQ :

			if(SendChkFlg) {

				SeccModbusWriteTx.Data16[0] = Led2Bed16(1);

				return MakeWriteTx(40009, 1);
			}
			else {
				SeccInitStep = SECC_INIT_STEP_VASREADREQ;
			}

		break;

		case SECC_INIT_STEP_VASREADREQ :
			SeccInitStep = SECC_INIT_STEP_VASCHK;
			return MakeReadTx(32002, 1);
		break;

		case SECC_INIT_STEP_VASCHK:
			if(!SeccRxData.vasEnable)
				SeccInitStep = SECC_INIT_STEP_VASREQ;
			else {
				CtLogGreen("Success Secc Initialize1111111111111!!");
				SeccInitStep = SECC_INIT_STEP_DONE;
			}
		break;			
	}


	return false;
}

uint8_t prcCnt;

uint8_t ChrgSecc(void)
{
	uint8_t crc_tmp16[20];

	if((SeccRxData.stcode >= CSM_STAT_STOPCHRG) && (SeccRxData.stcode <= CSM_STAT_NARMALSTOP)) 
	{
		SeccChrgStep = SECC_CHRG_STEP_NONE;

		SeccTxData.status_fault &= ~(1<<SECC_STAT_CHARG);
		SeccTxData.status_fault |= 1<<SECC_STAT_STOP;

		return ChckSeccStat();
	}

	// 사용자 충전 종료 시 PLC 모뎀에 충전종료 전달
	if((SeccChrgStep > SECC_CHRG_STEP_START) && (SeccTxData.status_fault & (1<<SECC_STAT_STOP))) 
	{
		SeccModbusWriteTx.Data16[0] = Led2Bed16(SeccTxData.status_fault);

		return MakeWriteTx(40001, 1);
	}

	if(SeccTxData.status_fault & (1<<SECC_STAT_CHARG)) 
	{
		switch (SeccChrgStep)
		{
			case SECC_CHRG_STEP_NONE :
				prcCnt = 0;

				SendChkFlg = false;

				if(SeccTxData.status_fault & (1<<SECC_STAT_CHARG)){
					SeccChrgStep = SECC_CHRG_STEP_START;
					SeccTxData.status_fault |= (1 << SECC_STAT_AUTHFIN);
					SeccTxData.status_fault |= (1 << SECC_STAT_CHRGPRMFIN);
					SendChkFlg = true;
				}


			break;

			case SECC_CHRG_STEP_REQID : 

				prcCnt ++;

				SeccChrgStep = SECC_CHRG_STEP_CHKID;

				return MakeReadTx(30061, 10);

			break;

			case SECC_CHRG_STEP_CHKID : 

				if(evccIdFlg || (prcCnt > 20)) {
					if(evccIdFlg) {
						CtLogCyan("Yes Check EVCC MAC %s", theConfig.chkModeMac);
					}
					else {
						CtLogCyan("No Check EVCC MAC %s", theConfig.chkModeMac);
					}
					if(SeccTxData.status_fault & (1<<SECC_STAT_CHARG)) {
						SeccChrgStep = SECC_CHRG_STEP_CHARGING;
					}
				}
				else {
					SeccChrgStep = SECC_CHRG_STEP_REQID;
				}

			break;

			case SECC_CHRG_STEP_START : 

				CtLogGreen("1. Check Send Flag %d", SendChkFlg);

				if(SendChkFlg) {

					SeccModbusWriteTx.Data16[0] = Led2Bed16(SeccTxData.status_fault);

					MakeWriteTx(40001, 1);

					CtLogGreen("1. Send Charge Control Ready!!");

					return 2;
				}
				else {
					CtLogBlue("1. Check Response of Send Charge Control Ready!!");
					SeccChrgStep = SECC_CHRG_STEP_HANDSH;
				}
			break;

			case SECC_CHRG_STEP_HANDSH:

				if(SeccRxData.stcode == CSM_STAT_CHRG){
					CtLogBlue("6. Wait Charging Start!!");
					if (SeccRxData.pwmvoltage >= 50 && SeccRxData.pwmvoltage <= 70){
						CtLogBlue("7. Charging Start!!");

						if(!strcmp(StrModelName, "TSCT-2007CO")) {
							CtLogCyan("Start Check EVCC MAC %s", theConfig.chkModeMac);
							SeccChrgStep = SECC_CHRG_STEP_REQID;
						}
						else {
							SeccChrgStep = SECC_CHRG_STEP_CHARGING;
						}
					}
				}
				else if(SeccRxData.stcode == CSM_STAT_AUTHCHECK) {
					// SeccTxData.status_fault |= 1 << SECC_STAT_AUTHFIN;
					CtLogBlue("4. Wait Auth Check!! %d", SeccTxData.status_fault);
					SeccChrgStep = SECC_CHRG_STEP_AUTHCHK;
					SendChkFlg = true;
					return ChckSeccStat();
				}
				else if(SeccRxData.stcode == CSM_STAT_CHARGPRMCHK) {
					CtLogBlue("5. Wait Charge Param Check!!");
					SeccChrgStep = SECC_CHRG_STEP_PRMCHK;
					SendChkFlg = true;
					return ChckSeccStat();
				}

				else if(SeccRxData.stcode == CSM_STAT_WAITHANDSHAKE) {
					CtLogBlue("2. Wait Handshake!!");
					return ChckSeccStat();
					// printf("Wait HandShaking\r\n");
				}
				else if(SeccRxData.stcode == CSM_STAT_SESSIONREADY) {
					CtLogBlue("3. Session Ready after Slac, Sdp, Sap!!");
					// printf("Wait HandShaking\r\n");
					return ChckSeccStat();
				}
				else if(SeccRxData.stcode < CSM_STAT_WAITHANDSHAKE) {
					SeccChrgStep = SECC_CHRG_STEP_START;
					SendChkFlg = true;
					return ChckSeccStat();
				}
				else if((SeccRxData.stcode >= CSM_STAT_STOPCHRG) && (SeccRxData.stcode <= CSM_STAT_NARMALSTOP)) 
				{
					SeccChrgStep = SECC_CHRG_STEP_NONE;

					SeccTxData.status_fault &= ~(1<<SECC_STAT_CHARG);
					SeccTxData.status_fault |= 1<<SECC_STAT_STOP;

					return ChckSeccStat();
				}
				else {
					CtLogBlue("0. Unknown Stat : %d!!", SeccRxData.stcode);
					return ChckSeccStat();
				}
			break;

			case SECC_CHRG_STEP_AUTHCHK :
				SeccTxData.status_fault |= (1 << SECC_STAT_AUTHFIN);
				if(SendChkFlg) {
					SeccModbusWriteTx.Id01 = CHRG_ID;
					SeccModbusWriteTx.FuncCode = FC_WRITE;
					SeccModbusWriteTx.ReqAddr = Led2Bed16(40001);
					SeccModbusWriteTx.ReqAddrLen = Led2Bed16(1);

					SeccModbusWriteTx.Datalen = 2;

					SeccModbusWriteTx.Data16[0] = Led2Bed16(SeccTxData.status_fault);

					memcpy(crc_tmp16, &(SeccModbusWriteTx.Id01), 7);
					memcpy(&crc_tmp16[7], SeccModbusWriteTx.Data16, SeccModbusWriteTx.Datalen);

					SeccModbusWriteTx.Crc16 = Led2Bed16(TsctCrc16(crc_tmp16, 9));
					return 2;
				}
				else {
					SeccChrgStep = SECC_CHRG_STEP_HANDSH;
				}
			break;
			
			case SECC_CHRG_STEP_PRMCHK :
				SeccTxData.status_fault |= (1 << SECC_STAT_CHRGPRMFIN);
				if(SendChkFlg) {
					SeccModbusWriteTx.Id01 = CHRG_ID;
					SeccModbusWriteTx.FuncCode = FC_WRITE;
					SeccModbusWriteTx.ReqAddr = Led2Bed16(40001);
					SeccModbusWriteTx.ReqAddrLen = Led2Bed16(1);

					SeccModbusWriteTx.Datalen = 2;

					SeccModbusWriteTx.Data16[0] = Led2Bed16(SeccTxData.status_fault);
					// SeccModbusWriteTx.Data16[0] = SeccTxData.status_fault;

					memcpy(crc_tmp16, &(SeccModbusWriteTx.Id01), 7);
					memcpy(&crc_tmp16[7], SeccModbusWriteTx.Data16, SeccModbusWriteTx.Datalen);

					SeccModbusWriteTx.Crc16 = Led2Bed16(TsctCrc16(crc_tmp16, 9));
					return 2;
				}
				else {
					SeccChrgStep = SECC_CHRG_STEP_HANDSH;
				}

			break;

			// Charging
			case SECC_CHRG_STEP_CHARGING :
				// charging Finish
				if((SeccRxData.stcode >= CSM_STAT_STOPCHRG) /*&& (SeccRxData.pwmvoltage >= 80 && SeccRxData.pwmvoltage <= 100)*/){
					// SeccChrgStep = SECC_CHRG_STEP_NONE;
					vasReadCd = false;
				}
				// Charging~~
				else {
					// period Count
					if((charge_cnt++ > 20))			// per 1s
					{
						charge_cnt = 0;
					}

					if(vasReadCd) {
						return ReadVasData();
					}

					if(charge_cnt%10 == 0) {		// per 500ms
						return ChkVasData();
					}
					
					else if(charge_cnt%10 == 5)		// per 500ms
					{
						return SendChrgPrm();
					}
					else
						return ChckSeccStat();
				}
			break;
		}
	}
	else {
		SeccChrgStep = SECC_CHRG_STEP_NONE;
	} 


	return false;
}

uint16_t reinitCnt = 0;

uint8_t SeccSenario(void)
{
	uint8_t ret8 = 0;

	// printf("[SeccSenario] Status Check : Init : %d, Chrg : %d status_fault : %d\r\n", SeccInitStep, SeccChrgStep, SeccTxData.status_fault);
	// proc with step
	if((SeccRxData.stcode == CSM_STAT_NONE) && (SeccInitStep == SECC_INIT_STEP_DONE)) {
		// if(++reinitCnt > 2 * 20) // 2sec
		// { 
			reinitCnt = 0;
			CtLogRed("Secc Re Initial by CSM Status Code : %d", SeccRxData.stcode);
			SeccInitStep = SECC_INIT_STEP_NONE;
			SeccTxData.status_fault &= ~(1<<SECC_STAT_CHARG);
			SeccTxData.status_fault |= 1<<SECC_STAT_STOP;
		// } 
	}

	else reinitCnt = 0;

	if(SeccInitStep < SECC_INIT_STEP_DONE) {

		return InitSecc();
	}

	if(SeccTxData.status_fault & (1<<SECC_STAT_CHARG))
	{
		ret8 = ChrgSecc();

		if(ret8)	return ret8;
	}
	else
	{
		SeccChrgStep = SECC_CHRG_STEP_NONE;
	}

	return ChckSeccStat();
}

static void* SeccRS232ReqTask(void* arg)
{
	char request[30];
	int size = 0;
	uint8_t ret_8 = 0;
	int ret = 0;

	uint8_t seccReqStep = 0;;

	SECC_MODBUS_READ_TX_S tmp_rd;

	SECC_MODBUS_WRITE_TX_S tmp_wr;

	// DumpBuffer("Reqeust to PLC Modem", request, 8);
	
	usleep(20*1000*1000);

	while (1)
	{
		// init
		size = 0;
		ret = 0;

		// validate proc
		if(PlcRespWaitCnt){
			if(++PlcRespWaitCnt > 10) {

				if(ret_8 == 1)
					CtLogYellow("Didn't Receive Resp [cmd : %lu, cnt : %d]", Led2Bed16(SeccModbusReadTx.ReqAddr), rebootCnt);
				if(ret_8 == 2)
					CtLogYellow("Didn't Receive Resp [cmd : %lu, cnt : %d]", Led2Bed16(SeccModbusReadTx.ReqAddr), rebootCnt);

				vasReadCd = false;

				bPlcConn = false;
				++rebootCnt;

				PlcRespWaitCnt = 0;
			}
			usleep(SECC_READ_ID_TASK_DELAY * 1000);
			continue;
		}

		if(SeccInitStep < SECC_INIT_STEP_DONE) {

			ret_8 = InitSecc();
		}

		else
		{
			seccReqStep++;

			if(seccReqStep > 2) seccReqStep = 0;

			switch(seccReqStep)
			{
				case 0:
					printf("\r\n\r\ncheck cp status\r\n\r\n");
					ret_8 = ChckSeccStat();
				break;

				case 1:
					ret_8 = SendChrgReady();
				break;

				case 2:
					seccReqStep = 0;
					ret_8 = SeccSenario();
				break;
				default:
					seccReqStep = 0;
					ret_8 = ChckSeccStat();
				break;
			}
		}


		if(ret_8) 
		{

			memset(request,0,sizeof(request));

			PlcRespWaitCnt = 1;

			// Read
			if(ret_8 == 1) {

				// while(Led2Bed16(SeccModbusReadTx.ReqAddrLen) > MAX_LEN) {

				// 	tmp_rd.ReqAddrLen = SeccModbusReadTx.ReqAddrLen;

				// 	SeccModbusReadTx.ReqAddrLen = Led2Bed16(MAX_LEN);

				// 	memcpy(request, &SeccModbusReadTx.Id01, 8);
				// 	write(SECC_DEV, request, 8);

				// 	if(Led2Bed16(SeccModbusReadTx.ReqAddr) >= 32001)
				// 		DumpBuffer("PLC Modem Reqeust", request, 8);					

				// 	SeccModbusReadTx.ReqAddr = Led2Bed16((Led2Bed16(SeccModbusReadTx.ReqAddr) + MAX_LEN));

				// 	SeccModbusReadTx.ReqAddrLen = Led2Bed16( Led2Bed16(tmp_rd.ReqAddrLen) - MAX_LEN );

				// 	usleep(SECC_READ_ID_TASK_DELAY * 1000);
				// }

				// if(Led2Bed16(SeccModbusReadTx.ReqAddrLen) > 0) {
				// 	memcpy(request, &SeccModbusReadTx.Id01, 8);
				// 	write(SECC_DEV, request, 8);

					
				// }

				memcpy(request, &SeccModbusReadTx.Id01, 8);
				write(SECC_DEV, request, 8);

				if(Led2Bed16(SeccModbusReadTx.ReqAddr) == 30061)
						DumpBuffer("PLC Modem Reqeust", request, 8);

				// if(Led2Bed16(SeccModbusReadTx.ReqAddr) >= 32001)
				// 	DumpBuffer("PLC Modem Reqeust", request, 8);

			}
			// Write
			else if(ret_8 == 2) {
				// memcpy(request, &SeccModbusWriteTx.Id01, 7 + SeccModbusWriteTx.Datalen);
				memcpy(request, &SeccModbusWriteTx.Id01, 7);
				memcpy(&request[7], SeccModbusWriteTx.Data16, SeccModbusWriteTx.Datalen);
				memcpy(&request[7 + SeccModbusWriteTx.Datalen], &SeccModbusWriteTx.Crc16, 2);
				write(SECC_DEV, request, 9 + SeccModbusWriteTx.Datalen);
				// DumpBuffer("PLC Modem Reqeust", request, 9 + SeccModbusWriteTx.Datalen);
			}
		}

		// printf("\r\n[%d]\r\n", ret);

		if(rebootCnt < 100)	
			usleep(SECC_READ_ID_TASK_DELAY * 1000);
		else
		{
			rebootCnt = 0;
			bPlcConn = false;
			SeccInitStep = SECC_INIT_STEP_NONE;
			CtLogRed("PLC Conn Error - Reboot PLC Modem. Wait for 35 Seconds...\r\n");

			sReadPlcTask = 0;

			//Do not disable ControlPilot for Upgrade PLC Modem
			ithGpioClear(GPIO_PLC_PWR_RELAY_CTL);
			
			sleep(3);
			CtLogYellow("[Secc] Recreate rs232 read thread..\n");
			pthread_create(&sReadPlcTask, NULL, SeccRS232ReadTask, NULL);
			pthread_detach(sReadPlcTask);

			ithGpioSet(GPIO_PLC_PWR_RELAY_CTL);
			sleep(30);
		}
	}
}

static uint16_t dataContinSize = 0;

bool ValidSecc(char* rx_Src, char* Src, uint16_t size)
{
	uint16_t tmp16;
	// DumpBuffer("Reqeust from PLC Modem", Src, size);

	if(!dataContinSize) {
		memset(Src, 0, 512);
		memcpy(Src, rx_Src, size);

		// size가 Data Lenth address 보다 작거나, Data Lenth Value 보다 작을 때 다음 값 까지 확인
		if(size < 2) {
			dataContinSize = size;
			// DumpBuffer("Reqeust from PLC Modem", rx_Src, size);
			// CtLogYellow("[read]PLC Msg Warnning : Fault Lenth, Check 2nd Packet\r\n");
			return false;
		}	
		
		if(Src[1] == FC_READ) {
			if((uint8_t)Src[2] > size-5) {
				dataContinSize = size;
				// DumpBuffer("Reqeust from PLC Modem", rx_Src, size);
				// CtLogYellow("[read]PLC Msg Warnning : Fault Lenth, Check 2nd Packet\r\n");
				return false;
			}
			else if((uint8_t)Src[2] < size-5) {
				dataContinSize = 0;
				// DumpBuffer("Reqeust from PLC Modem", Src, size);
				CtLogRed("PLC Msg Error : Fault Lenth %d / %d\r\n", Src[2], size-5);
				return false;
			}
		}
		else if(Src[1] == FC_WRITE) {
			if(8 > size) {
				dataContinSize = size;
				// DumpBuffer("Reqeust from PLC Modem", Src, size);
				// CtLogYellow("PLC Msg Warnning : Fault Lenth, Check 2nd Packet\r\n");
				return false;
			}
			else if(8 < size) {
				dataContinSize = 0;
				// DumpBuffer("Reqeust from PLC Modem", Src, size);
				CtLogRed("PLC Msg Error : Fault Lenth\r\n");
				return false;
			}			
		}
		else {
			dataContinSize = 0;
			CtLogRed("PLC Msg Error : Msg Type Error %d\r\n", Src[1]);
			return false;
		}
		// etx 로 끝나면 Validation 실행
	}
	// 이전 데이터 존재 시
	else {
		// 이전 데이터 사이즈 + 현재 데이터 사이즈가 516 이상이면 데이터 오류 처리
		if((dataContinSize + size) >= 512) {
			CtLogRed("PLC Msg Error  : Too Long size %lu\r\n", dataContinSize + size);
			dataContinSize = 0;
			return false;
		}

		memcpy(&(Src[dataContinSize]), rx_Src, size);
		size += dataContinSize;
		dataContinSize = 0;

		// size가 Data Lenth address 보다 작거나, Data Lenth Value 보다 작을 때 다음 값 까지 확인
		if(Src[1] == FC_READ) {
			if((uint8_t)Src[2] > size-5) {
				DumpBuffer("Reqeust from PLC Modem", Src, size);
				CtLogRed("PLC Msg Error : Fault Lenth\r\n");
				return false;
			}
			else if((uint8_t)Src[2] < size-5) {
				DumpBuffer("Reqeust from PLC Modem", Src, size);
				CtLogRed("PLC Msg Error : Fault Lenth\r\n");
				return false;
			}
		}
		else if(Src[1] == FC_WRITE) {
			if(8 > size) {
				DumpBuffer("Reqeust from PLC Modem", Src, size);
				CtLogRed("PLC Msg Error : Fault Lenth\r\n");
				return false;
			}
			else if(8 < size) {
				DumpBuffer("Reqeust from PLC Modem", Src, size);
				CtLogRed("PLC Msg Error : Fault Lenth\r\n");
				return false;
			}			
		}
		else {
			DumpBuffer("Reqeust from PLC Modem", Src, size);
			CtLogRed("PLC Msg Error : Msg Type Error %d\r\n", Src[1]);
			return false;
		}
	}

	// 여기서 부터는 한패킷 공통 
	if(size < 5){
		DumpBuffer("Reqeust from PLC Modem", Src, size);
		CtLogRed("PLC Msg Error : Msg Length under 5 [%d]\r\n", size);
		return false;
	}

	if(Src[0] != CHRG_ID){
		DumpBuffer("Reqeust from PLC Modem", Src, size);
		CtLogRed("PLC Msg Error : ID not 01 [%d]\r\n", Src[0]);
		return false;
	}
	

	// if((Src[1] == FC_READ) && (PlcRespWaitCnt == 0)){
	// 	printf("PLC Msg Error : NotYey Request but Receive Read Code \r\n");
	// 	return false;
	// }

	if(Src[1] == FC_READ){

		if(SeccModbusReadTx.FuncCode != Src[1]){
			CtLogRed("PLC Msg Error : Function Code Error [Send : %d Receive : %d]\r\n", SeccModbusReadTx.FuncCode, Src[1]);
			return false;
		}

		if((Led2Bed16(SeccModbusReadTx.ReqAddrLen)*2) != (uint8_t)Src[2]){
			CtLogRed("PLC Msg Error : Data Size Error [Send : %d Receive : %d]\r\n", Led2Bed16(SeccModbusReadTx.ReqAddrLen), Src[2]);
			return false;
		}

		if((uint8_t)Src[2] != size-5){
			CtLogRed("PLC Msg Error : Data Lenth Fault %d [Full size : %d]\r\n", Src[2], size);
			return false;
		}

		SeccModbusReadRx.DataLen = size-5;
	}	

	else if(Src[1] == FC_WRITE){

		if(SeccModbusWriteTx.FuncCode != Src[1]){
			DumpBuffer("Reqeust from PLC Modem", Src, size);
			CtLogRed("PLC Msg Error : Function Code Error [Send : %d Receive : %d]\r\n", SeccModbusWriteTx.FuncCode, Src[1]);
			return false;
		}

		tmp16 = ((Src[3] << 8) & 0xff00) + ((Src[2] << 0) & 0xff);

		if(SeccModbusWriteTx.ReqAddr != tmp16){
			DumpBuffer("Reqeust from PLC Modem", Src, size);
			CtLogRed("PLC Msg Error : Addr Error [Send : %lu / Receive : %lu]\r\n", Led2Bed16(SeccModbusWriteTx.ReqAddr), Led2Bed16(tmp16));
			return false;
		}

		tmp16 = ((Src[5] << 8) & 0xff00) + ((Src[4] << 0) & 0xff);

		if(SeccModbusWriteTx.ReqAddrLen != tmp16){
			DumpBuffer("Reqeust from PLC Modem", Src, size);
			CtLogRed("PLC Msg Error : Data Size Error [Send : %d Receive : %d]\r\n", SeccModbusWriteTx.ReqAddrLen, tmp16);
			return false;
		}

		if(8 != size){
			DumpBuffer("Reqeust from PLC Modem", Src, size);
			CtLogRed("PLC Msg Error : Data Lenth Fault %d [Full size : %d]\r\n", 8, size);
			return false;
		}
	}

	else {
		DumpBuffer("Reqeust from PLC Modem", Src, size);
		CtLogRed("PLC Msg Error : FC not 0x04 or 0x10 [%d]\r\n", Src[1]);
		return false;
	}

	tmp16 = (Src[size-2] << 8) & 0xff00;
	tmp16 += (uint16_t)Src[size-1] & 0xff;

	if(TsctCrc16(Src, size-2) != tmp16){		
		DumpBuffer("Reqeust from PLC Modem", Src, size);
		CtLogRed("PLC Msg Error : Crc Error Cal : %d [Full size : %d]\r\n", TsctCrc16(Src, size-2), tmp16);
		return false;
	}

	// printf("PLC Msg Check OK!!\r\n");

	return true;
}

bool vasParsFunc(SECC_VAS_READ_DATA* des, uint16_t *buf, uint16_t* src, uint8_t size) {

	int32_t tmp32 = SeccRxData.vasMapCnt - 10 - ( MAX_LEN * (vasReadCd-1) );

	uint8_t chkSumBuf = 0;

	uint8_t * buf8 = buf;

	uint8_t * src8 = src;

	// if((shmDataAppInfo.app_order!=APP_ORDER_CHARGE_READY) && (shmDataAppInfo.app_order!=APP_ORDER_CHARGING)) {
	// 	CtLogRed("When Other Charging Status Reqeust from PLC Modem");
	// 	vasReadCd = false;
	// 	return false;
	// }

	for(int i=0; i<size; i+=2) {
		memcpy(&buf8[ MAX_LEN * (vasReadCd-1) * 2 + i ], &src8[i+1], 1);
		memcpy(&buf8[ MAX_LEN * (vasReadCd-1) * 2 + i + 1 ], &src8[i], 1);
	} 

	if(tmp32 > MAX_LEN) {
		// DumpBuffer("Vas First Data", buf, size);
		// CtLogYellow("VAS Data Parsing~~~ [%d]", vasReadCd);
		vasReadCd++;
		return false;
	}
	else {
		// DumpBuffer("Vas Full Data", buf, size);
		CtLogYellow("VAS Last Data Parsing~~~ [%d]", vasReadCd);
		vasReadCd = false;
	}

	for(uint32_t i = 0; i < (SeccRxData.vasMapCnt - 10) * 2; ++i) {
		chkSumBuf ^= (uint8_t)buf8[i];
	}

	if(chkSumBuf != SeccRxData.vasChksum){		
		DumpBuffer("Reqeust from PLC Modem", buf, (SeccRxData.vasMapCnt - 10) * 2);
		CtLogRed("Vas Checksum Error : Crc Error Cal : %d / Rx CRC : %d\r\n", chkSumBuf, SeccRxData.vasChksum);
		return false;
	}
	else	CtLogGreen("Vas Checksum Ok [Size %lu]\r\n", size);


	// Data Parsing
	// DumpBuffer("Vas Full Data", buf, (SeccRxData.vasMapCnt - 10) * 2);

	des->timeStamp = (buf8[2] << 24) + (buf8[3] << 16) + (buf8[4] << 8) + buf8[5];	// ???

	printf("[VAS] TimeStap : %lu ////// \r\n", des->timeStamp);

	printf("[VAS] VIN :");

	memcpy(&des->vin[0], &buf8[8], 17);

	for(int i=0; i<17; i++){

		printf(" %c", des->vin[i]);
	}

	printf("\r\n");


	des->soc = buf8[27];

	// printf("[VAS] SOC : %lu ////// \r\n", des->soc);

	des->soh = buf8[30];

	// printf("[VAS] SOH : %lu ////// \r\n", des->soh);

	des->batPackCurr = (buf8[33] << 8) + (buf8[34]);		// ??

	// printf("[VAS] BatPackCurrent : %lu ////// \r\n", des->batPackCurr);

	des->batPackVolt = (buf8[37] << 8) + buf8[38];	// 37 38

	// printf("[VAS] BatPackVoltage : %lu ////// \r\n", des->batPackVolt);

	des->cellVoltLen = (buf8[40] << 8) + buf8[41];

	// printf("[VAS] CellVoltage Lenth : %lu ////// \r\n", des->cellVoltLen);

	// printf("[VAS] CellVoltage :");

	for(int i=0; i < des->cellVoltLen; i++) {

		des->batCellVolt[i] = buf8[42 + i];

		// printf(" %lu", des->batCellVolt[i]);
	}

	// printf("\r\n");

	des->modulTempLen = buf8[43 + des->cellVoltLen];	//	43 + des->cellVoltLen 

	// printf("[VAS] Module Temperature Lenth : %lu ////// \r\n", des->modulTempLen);

	// printf("[VAS] Module Temperature :");

	for(int i=0; i < des->modulTempLen; i++) {

		des->batModulTemp[i] = buf8[44 + des->cellVoltLen + i]; // 22 + des->cellVoltLen / 2

		// printf(" %lu", des->batModulTemp[i]);
	}

	// printf("\r\n");

	seccVasRawData[seccVasDataCnt].dataLenth = (SeccRxData.vasMapCnt - 10) * 2 - 6;

	memcpy(seccVasRawData[seccVasDataCnt].data, &buf8[6], seccVasRawData[seccVasDataCnt].dataLenth);

	CtLogGreen("[VAS] Parsing Success [%d]", seccVasDataCnt);

	return true;
}

void ParsRxData(void)
{
	bool ret_b = false;

	uint8_t tmp_mac;

	uint8_t tmp = SeccModbusReadRx.DataLen;
	// DumpBuffer("Check Rx Data", SeccModbusReadRx.Data16, SeccModbusReadRx.DataLen);

	switch (Led2Bed16(SeccModbusReadTx.ReqAddr)){
		case 30001:
			SeccRxData.ready = Led2Bed16(SeccModbusReadRx.Data16[(SeccModbusReadRx.DataLen - tmp)/2]);
			// printf("SeccRxData.ready : %d\r\n", SeccRxData.ready);
			tmp -= 2;
		if(tmp <= 0)	break;

		case 30002:
			SeccRxData.stcode = Led2Bed16(SeccModbusReadRx.Data16[(SeccModbusReadRx.DataLen - tmp)/2]);
			// printf("SeccRxData.stcode : %d\r\n SeccTxData.status_fault : %lu\r\n", SeccRxData.stcode, SeccTxData.status_fault);
			// printf("SeccTxData.status_fault : %lu\r\n", SeccTxData.status_fault);
			tmp -= 2;
		if(tmp <= 0)	break;

		case 30003:
			SeccRxData.secc_errcode = Led2Bed16(SeccModbusReadRx.Data16[(SeccModbusReadRx.DataLen - tmp)/2]);
			// printf("SeccRxData.secc_errcode : %d\r\n", SeccRxData.secc_errcode);
			tmp -= 2;
		if(tmp <= 0)	break;

		case 30004:
			SeccRxData.swVer = Led2Bed16(SeccModbusReadRx.Data16[(SeccModbusReadRx.DataLen - tmp)/2]);
			// printf("SeccRxData.swVer : %d\r\n", SeccRxData.swVer);
			tmp -= 2;
		if(tmp <= 0)	break;		

		case 30005:
			SeccRxData.chrgPrtcl = Led2Bed16(SeccModbusReadRx.Data16[(SeccModbusReadRx.DataLen - tmp)/2]);
			// printf("SeccRxData.chrgPrtcl : %d\r\n", SeccRxData.chrgPrtcl);
			tmp -= 2;
		if(tmp <= 0)	break;		

		case 30006:
			SeccRxData.pwmduty = Led2Bed16(SeccModbusReadRx.Data16[(SeccModbusReadRx.DataLen - tmp)/2]);
			// printf("SeccRxData.pwmduty : %d\r\n", SeccRxData.pwmduty);
			tmp -= 2;
		if(tmp <= 0)	break;

		case 30007:
			SeccRxData.pwmvoltage = Led2Bed16(SeccModbusReadRx.Data16[(SeccModbusReadRx.DataLen - tmp)/2]);
			// printf("SeccRxData.pwmvoltage : %d\r\n", SeccRxData.pwmvoltage);
			tmp -= 2;
		if(tmp <= 0)	break;

		case 32001:	// header + data size 10 ~ ffff+10
			SeccRxData.vasMapCnt = Led2Bed16(SeccModbusReadRx.Data16[(SeccModbusReadRx.DataLen - tmp)/2]);
			// printf("SeccRxData.vasMapCnt : %d\r\n", SeccRxData.vasMapCnt);
			tmp -= 2;
		if(tmp <= 0)	break;

		case 32002:	// Vas Enable
			SeccRxData.vasEnable = Led2Bed16(SeccModbusReadRx.Data16[(SeccModbusReadRx.DataLen - tmp)/2]);
			// printf("SeccRxData.vasEnable : %d\r\n", SeccRxData.vasEnable);
			tmp -= 2;
		if(tmp <= 0)	break;

		case 32003:	// Vas Server State
			SeccRxData.vasStat = Led2Bed16(SeccModbusReadRx.Data16[(SeccModbusReadRx.DataLen - tmp)/2]);
			// printf("SeccRxData.vasStat : %d\r\n", SeccRxData.vasStat);
			tmp -= 2;
		if(tmp <= 0)	break;

		case 32004:	// Vas Error code
			SeccRxData.vasErrcd = Led2Bed16(SeccModbusReadRx.Data16[(SeccModbusReadRx.DataLen - tmp)/2]);
			// printf("SeccRxData.vasErrcd : %d\r\n", SeccRxData.vasErrcd);
			tmp -= 2;
		if(tmp <= 0)	break;

		case 32005:	// Vas rx cnt
			if(SeccRxData.vasRxCnt != Led2Bed16(SeccModbusReadRx.Data16[(SeccModbusReadRx.DataLen - tmp)/2])){
				SeccRxData.vasRxCnt = Led2Bed16(SeccModbusReadRx.Data16[(SeccModbusReadRx.DataLen - tmp)/2]);
				if (SeccRxData.vasRxCnt != 0) {
					// CtLogGreen("Vas Data Read : %d\r\n", SeccRxData.vasRxCnt);
					vasReadCd = true;
				}
			}
			// printf("SeccRxData.vasRxCnt : %d\r\n", SeccRxData.vasRxCnt);
			
			tmp -= 2;
		if(tmp <= 0)	break;

		case 32006:
			// SeccRxData.vasRxCnt = Led2Bed16(SeccModbusReadRx.Data16[(SeccModbusReadRx.DataLen - tmp)/2]);
			// printf("SeccRxData.pwmvoltage : %d\r\n", SeccRxData.pwmvoltage);
			tmp -= 2;
		if(tmp <= 0)	break;

		case 32007:
			// SeccRxData.vasRxCnt = Led2Bed16(SeccModbusReadRx.Data16[(SeccModbusReadRx.DataLen - tmp)/2]);
			// printf("SeccRxData.pwmvoltage : %d\r\n", SeccRxData.pwmvoltage);
			tmp -= 2;
		if(tmp <= 0)	break;

		case 32008:
			// SeccRxData.vasRxCnt = Led2Bed16(SeccModbusReadRx.Data16[(SeccModbusReadRx.DataLen - tmp)/2]);
			// printf("SeccRxData.pwmvoltage : %d\r\n", SeccRxData.pwmvoltage);
			tmp -= 2;
		if(tmp <= 0)	break;

		case 32009:
			SeccRxData.vasChksum = Led2Bed16(SeccModbusReadRx.Data16[(SeccModbusReadRx.DataLen - tmp)/2]);
			// printf("SeccRxData.vasChksum : %d\r\n", SeccRxData.vasChksum);
			tmp -= 2;
		if(tmp <= 0)	break;	
		
		case 32010:
			SeccRxData.vasLen = Led2Bed16(SeccModbusReadRx.Data16[(SeccModbusReadRx.DataLen - tmp)/2]);
			// printf("[%d]SeccRxData.vasLen : %d\r\n", tmp, SeccRxData.vasLen);
			tmp -= 2;
		if(tmp <= 0)	break;	

		// case 32011:
		// 	vasParsFunc(&seccVasRxData[seccVasDataCnt], SeccModbusReadRx.Data16, SeccModbusReadRx.DataLen);
		// break;	
	}

	if(Led2Bed16(SeccModbusReadTx.ReqAddr) == 30061) {
		CtLogCyan("Check EVCC MAC Rx");

		for(int i=0; i<10; i++) {
			printf(" %u", SeccModbusReadRx.Data16[i]);
		}
		printf("\r\n\r\n");
	}

	// Vas Data Parsing
	if(Led2Bed16(SeccModbusReadTx.ReqAddr) > 32010) {

		ret_b = vasParsFunc(&seccVasRxData[seccVasDataCnt], &vasDataBuf, SeccModbusReadRx.Data16, SeccModbusReadRx.DataLen);
		
		if(ret_b) 
		{
			// IdaKeriData_idx = 0;		// Quit Duplication with IDA.

			if(seccVasDataCnt++ > 29) 
			{
				memcpy(&seccVasRxData[0], &(seccVasRxData[1]), sizeof(seccVasRxData[0])*29);
				seccVasDataCnt = 29;
			}
		}
	}
}

static void* SeccRS232ReadTask(void* arg)
{
	char pars_buf[512];

	usleep(10*1000*1000);

    while(sReadPlcTask)
    {
		char buffer[512];
		bool ret_b = false;

		memset(buffer, 0, 512);
		int bufsize = read(SECC_DEV, buffer, 512);

		if(bufsize) {

			// DumpBuffer("Check Rx Data", buffer, bufsize);

			ret_b = ValidSecc(buffer, pars_buf, bufsize);

			if(ret_b){
				// DumpBuffer("Rx Data", pars_buf, )
				bPlcConn = true;
				rebootCnt = 0;
				PlcRespWaitCnt = 0;
				if(pars_buf[1] == FC_READ){
					// if(Led2Bed16(SeccModbusReadTx.ReqAddr) > 32010) {
					// 	DumpBuffer("Check Vas Rx Data", pars_buf, SeccModbusReadRx.DataLen + 5);
					// }

					memset(SeccModbusReadRx.Data16, 0, sizeof(SeccModbusReadRx.Data16));
					memcpy(SeccModbusReadRx.Data16, &(pars_buf[3]), SeccModbusReadRx.DataLen);

					ParsRxData();
				}

				if(pars_buf[1] == FC_WRITE){
					// DumpBuffer("Check Rx Data", pars_buf, bufsize);

					if(SendChkFlg) {
						SendChkFlg = false;
						// DumpBuffer("Secc Write Response", pars_buf, bufsize);
					}	
				}

				
			}
		}

		else{
			// erro todo
		}


        usleep(SECC_READ_232_TASK_DELAY * 1000);
	}
		
	sReadPlcTask = 0;
	CtLogYellow("[RFID] exit read rs232 thread\n");
}

void SeccInit(void)
{

	ithGpioSetMode(GPIO_PLC_PWR_RELAY_CTL, ITH_GPIO_MODE0);
	ithGpioSetOut(GPIO_PLC_PWR_RELAY_CTL);
	ithGpioClear(GPIO_PLC_PWR_RELAY_CTL);	
	usleep(3*1000*1000);
	ithGpioSet(GPIO_PLC_PWR_RELAY_CTL);	

	CtLogYellow("[Secc] PLC Relay On\n");

	SeccTxData.status_fault = 1<<SECC_STAT_EVSE_READY;
	SeccTxData.NormVolt = 2200;
	// SeccTxData.MaxCurr = theConfig.chargingstatus * 45 ;	// 1000 / 220 * 10
	SeccTxData.MaxCurr = 315 ;	// 1000 / 220 * 10
	SeccTxData.NormFreq = 60;
	SeccTxData.MaxPower = 70;
	SeccTxData.payment = 0;

	SeccRxData.stcode = CSM_STAT_READY;
	SeccRxData.secc_errcode = 0xffff;
	

	if (sReadPlcTask==0)
	{
		CtLogYellow("[Secc] create rs232 read thread..\n");
		pthread_create(&sReadPlcTask, NULL, SeccRS232ReadTask, NULL);
		pthread_detach(sReadPlcTask);
	}		

	if (sReqPlcTask==0)
	{
		pthread_create(&sReqPlcTask, NULL, SeccRS232ReqTask, NULL);
		pthread_detach(sReqPlcTask);
	}
}
