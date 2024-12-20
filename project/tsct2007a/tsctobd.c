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
#include "tsctobd.h"
#include "tsctcfg.h"
#include "scene.h"

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


//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------
#define OBD_READ_232_TASK_DELAY		200 // ms
#define OBD_READ_ID_TASK_DELAY		1000 // ms

#define HEADER_SIZE 	0x07

#define STX_CODE		0x02
#define ETX_CODE		0x03

#define ACK_CODE		0x06
#define NACK_CODE		0x15

#define REQ_MSG_CODE	0x01
#define RES_MSG_CODE	0x02

// Charger -> IDA Command
#define CMD_STARTCHRG	0x10
#define CMD_REPORTPRM	0x11
#define CMD_CHRGINFO	0x12
#define CMD_STOPCHRG	0x1F

// IDA -> Charger Command
#define CMD_GETMACID	0x01
#define CMD_GETWIFI		0x02
#define CMD_REPRTWIFI	0x03
#define CMD_SETCURR		0x21
#define CMD_SENDBWD		0x22
#define CMD_SENDKERID	0x23
#define CMD_EMGSTOP		0x2A

//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------
static pthread_t sReqPlcTask;

static pthread_t sReadPlcTask;

struct timeval IdaTimeval;

uint16_t RespWaitCnt = false;
uint8_t ResWaitCmd = 0x00;

static uint16_t dataContinSize = 0;



//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------




unsigned short calcCRC(unsigned char* crcdata, int len) 
{
	const unsigned short crc16tab[256] = 
	{
		0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 
		0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF, 
		0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6, 
		0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE, 
		0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485, 
		0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D, 
		0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4, 
		0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC, 
		0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823, 
		0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B, 
		0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12, 
		0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A, 
		0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41, 
		0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49, 
		0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70, 
		0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78, 
		0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F, 
		0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067, 
		0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E, 
		0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256, 
		0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D, 
		0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 
		0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C, 
		0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634, 
		0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB, 
		0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3, 
		0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A, 
		0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92, 
		0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9, 
		0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1, 
		0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8, 
		0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
	};

    unsigned short crc = 0;

    for(int i = 0; i < len; i++) 
    {
        crc = (crc << 8) ^ crc16tab[((crc >> 8) ^ *crcdata++) & 0x00FF];
    }

    return crc;
}

/**
 * @brief Little Endian Data => Big Endian Data (16bit) 
 */
// static uint16_t Led2Bed16(uint16_t Led16)
// {
// 	uint16_t tmp_u16;

// 	tmp_u16 = (Led16 >> 8) & 0xff;
// 	tmp_u16 += (Led16 << 8) & 0xff00;

// 	return tmp_u16;
// }

static uint8_t MakeTx(uint8_t Addr8, uint16_t AddrLen16)
{
	// uint8_t CrcTmp[20];

	IdaTxDataForm.Stx8 = STX_CODE;
	IdaTxDataForm.Lenth16 = AddrLen16;
	IdaTxDataForm.Cmd8 = Addr8;
	// IdaTxDataForm.Data8 = 
	IdaTxDataForm.Crc16 = calcCRC(&(IdaTxDataForm.Lenth16), 2 + AddrLen16);
	IdaTxDataForm.Etx8 = ETX_CODE;

	return 1;
}

static uint8_t SendCmd_StartChrg(void)
{
	uint16_t tmp_buf16;

	memset(IdaTxDataForm.Data8, 0, 14);

	IdaTxDataForm.Data8[0] = 0x01;

	tmp_buf16 = TSCTGetAMICurrent();

	// tmp_buf16 = 32 * 10;

	memcpy(&(IdaTxDataForm.Data8[5]), &(tmp_buf16), 2);

	tmp_buf16 = TSCTGetAMIVolt() * 10;

	// tmp_buf16 = 220 * 10;

	memcpy(&(IdaTxDataForm.Data8[9]), &(tmp_buf16), 2);

	gettimeofday(&IdaTimeval, NULL);

	memcpy(&(IdaTxDataForm.Data8[1]), &(IdaTimeval.tv_sec), 4);

	MakeTx(CMD_CHRGINFO, 14);

	return 14;
}



static uint8_t SendCmd_ReportParam(void)
{
	uint16_t tmp_buf16;

	memset(IdaTxDataForm.Data8, 0, 14);

	IdaTxDataForm.Data8[0] = 0x00;

	tmp_buf16 = TSCTGetAMICurrent();

	memcpy(&(IdaTxDataForm.Data8[5]), &(tmp_buf16), 2);

	tmp_buf16 = TSCTGetAMIVolt() * 10;

	memcpy(&(IdaTxDataForm.Data8[9]), &(tmp_buf16), 2);

	gettimeofday(&IdaTimeval, NULL);

	memcpy(&(IdaTxDataForm.Data8[1]), &(IdaTimeval.tv_sec), 4);

	MakeTx(CMD_CHRGINFO, 14);

	return 14;
}




static void SendBuff(char* Des, uint8_t BuffLen)
{
	memcpy(Des, &IdaTxDataForm.Stx8, 1);
	Des += 1;
	memcpy(Des, &IdaTxDataForm.Lenth16, 2);
	Des += 2;
	memcpy(Des, &IdaTxDataForm.Cmd8, 1);
	Des += 1;
	memcpy(Des, &IdaTxDataForm.Data8, BuffLen);
	Des += BuffLen;
	memcpy(Des, &IdaTxDataForm.Crc16, 2);
	Des += 2;
	memcpy(Des, &IdaTxDataForm.Etx8, 1);
	Des += 1;
}

uint8_t ObdSenario(void)
{
	// printf("evcc Flag : %d / apporder : %d\r\n", evccIdFlg, APP_ORDER_CHARGING);
	if((shmDataAppInfo.app_order == APP_ORDER_CHARGING) && (evccIdFlg)) {
		return SendCmd_StartChrg();
	}
	
	return SendCmd_ReportParam();
}

static void* ObdRS232ReqTask(void* arg)
{
	char request[30];
	uint8_t ret_8 = 0;

	uint8_t RespMissCnt = 0;

	// DumpBuffer("Reqeust to PLC Modem", request, 8);
	
	usleep(2000*1000);

	while (1)
	{
		// validate proc
		if(RespWaitCnt){
			if(++RespWaitCnt > 10) {
				RespMissCnt++;
				CtLogRed("[ObdRS232ReqTask] miss Response [cmd %d /cnt %d]", ResWaitCmd, RespMissCnt);
				ResWaitCmd = 0x00;
				RespWaitCnt = 0;				
				usleep(OBD_READ_ID_TASK_DELAY * 1000);
				continue;
			}
		}
		
		ret_8 = ObdSenario();

		if(ret_8) {

			memset(request,0,sizeof(request));

			RespWaitCnt = 1;

			ResWaitCmd = IdaTxDataForm.Cmd8;

			if(ret_8) {
				SendBuff(request, ret_8-1);
				write(OBD_READER_DEV, request, ret_8+6);
				// DumpBuffer("PLC Modem Reqeust", request, ret_8+6);
			}
		}

		// printf("\r\n[%d]\r\n", ret);
	
		usleep(OBD_READ_ID_TASK_DELAY * 1000);
	}
}

uint8_t ValidIda(char* rx_Src, char* Src, uint16_t size)
{

	uint16_t crc_tmp16;

	int16_t stx_idx = -1;

	uint16_t data_length = 0;
	uint8_t command = 0;
	uint16_t end_idx = 0;

	uint16_t packet_len = 0;

	uint8_t Packet_No = 0;



	// DumpBuffer("Reqeust from PLC Modem", Src, size);

	// data parsing logic

	// 이전 파싱 데이터 없을 시
	if(!dataContinSize) {
		memset(Src, 0, 512);
		memcpy(Src, rx_Src, size);
		// etx 로 끝나지 않으면 이후 파싱 데이터 까지 확인
		if(rx_Src[size-1] != ETX_CODE) {
			dataContinSize = size;
			CtLogYellow("OBD Msg Warnning : not find ETX, Check 2nd Packet\r\n");
			return false;
		}
		// etx 로 끝나면 Validation 실행
	}
	// 이전 데이터 존재 시
	else {
		// 이전 데이터 사이즈 + 현재 데이터 사이즈가 516 이상이면 데이터 오류 처리
		if((dataContinSize + size) > 516) {
			CtLogRed("[Data Size Check Error]  : Too Long size %lu\r\n", dataContinSize + size);
			dataContinSize = 0;
			return false;
		}

		// etx 로 끝나지 않으면 데이터 오류 처리
		if(rx_Src[size-1] != ETX_CODE) {
			dataContinSize = 0;
			CtLogRed("PLC Msg Error : Not Find ETX in 2nd Packet\r\n");
			return false;
		}


		memcpy(&(Src[dataContinSize]), rx_Src, size);
		size += dataContinSize;
		dataContinSize = 0;
	}


	// 5 data Packet by One Reading
	for(int i=0; (i < 5); i++) {

		stx_idx = -1;
		
		// Find stx
		for(int j=0; j<(size-end_idx); j++) {
			if(Src[end_idx + j] == STX_CODE) {
				stx_idx = end_idx + j;
				break;
			}
		}

		// if i==0 check
		if (stx_idx == -1) {
			if(i==0) {
				CtLogRed("PLC Msg Error : not find STX %d\r\n", Src[i]);
				// DumpBuffer("Reqeust from PLC Modem", Src, size);
			}
			return Packet_No;
		}


		// Check Header Size
		if(size < stx_idx + HEADER_SIZE) {
			CtLogRed("PLC Msg Error : Msg Length under 7 [%d]\r\n", size);
			// DumpBuffer("Reqeust from PLC Modem", Src, size);
			return Packet_No;
		}


		data_length = Src[stx_idx+1] + Src[stx_idx+2] * 0x100;
		command = Src[stx_idx+3];
		
		end_idx = stx_idx + (HEADER_SIZE-2) + data_length;

		packet_len = HEADER_SIZE - 1 + data_length;

		// printf("[Size Check] size : %d, data len : %d\r\n", size, data_length);

		// check buffer size 
		if (size < (end_idx+1)) {
			CtLogRed("PLC Msg Error : size too short [ size : %d < data len : %d ]\r\n", size, data_length);
			// DumpBuffer("Reqeust from PLC Modem", Src, size);
			return Packet_No;
		}

		if(Src[end_idx] != ETX_CODE) {
			CtLogRed("PLC Msg Error : not ETX [%d]\r\n", Src[size-1]);
			// DumpBuffer("Reqeust from PLC Modem", Src, size);
			return Packet_No;
		}


		// check crc
		crc_tmp16 = (Src[end_idx-1] << 8) & 0xff00;
		crc_tmp16 += (uint16_t)Src[end_idx-2] & 0xff;

		if(calcCRC((Src+stx_idx+1), packet_len-4) != crc_tmp16){		
			CtLogRed("PLC Msg Error : Crc Error [Cal : %d / rx : %d]\r\n", calcCRC((Src+stx_idx+1), packet_len-4), crc_tmp16);
			// DumpBuffer("Reqeust from PLC Modem", Src, size);
			return Packet_No;
		}

		// Check cmd N*U		

		memcpy(&(IdaRxDataForm[i].Lenth16), &Src[stx_idx+1], packet_len-1);

		// printf("Validation Success %d \r\n",i);

		Packet_No = i+1;
	}
}




// No 정리
static bool ParsRxData(uint8_t No)
{
	bool validDataFlg = false;

	uint16_t id_tmp = 0;

	uint8_t id_tmp1 = 0;

	IDA_KERI_DATA* pIdaKeriData;

	// DumpBuffer("Check Rx Data", IdaRxDataForm.Data8, IdaRxDataForm.Lenth16 - 1);
	// printf("Check rx data Length : %d / cmd : %d / data ", IdaRxDataForm[No].Lenth16, IdaRxDataForm[No].Cmd8);
	// for(int i=0; i<(IdaRxDataForm[No].Lenth16-1); i++){
	// 	printf("%d ", IdaRxDataForm[No].Data8[i]);
	// }
	// printf("\r\n");

	switch (IdaRxDataForm[No].Cmd8){
		case CMD_GETMACID :
			
			// IdaTxDataForm.Data8[0] = ACK_CODE;

			// if(theConfig.OperationMode == OP_CHECK_MODE) {
				for(int i=0; i<12; i+=2) 
				{
					IdaTxDataForm.Data8[i/2] = 0;

					if((theConfig.chkModeMac[i] >= '0') && (theConfig.chkModeMac[i] <= '9')) {
						IdaTxDataForm.Data8[i/2] = ((theConfig.chkModeMac[i] - '0') << 4);
					}
					else if((theConfig.chkModeMac[i] >= 'A') && (theConfig.chkModeMac[i] <= 'F')) {
						IdaTxDataForm.Data8[i/2] = ((theConfig.chkModeMac[i] - 'A' + 10) << 4);
					}

					if((theConfig.chkModeMac[i+1] >= '0') && (theConfig.chkModeMac[i+1] <= '9')) {
						IdaTxDataForm.Data8[i/2] += (theConfig.chkModeMac[i+1] - '0');
					}
					else if((theConfig.chkModeMac[i+1] >= 'A') && (theConfig.chkModeMac[i+1] <= 'F')) {
						IdaTxDataForm.Data8[i/2] += (theConfig.chkModeMac[i+1] - 'A' + 10);
					}

					IdaTxDataForm.Data8[6+i] = theConfig.chkModeMac[i];
					IdaTxDataForm.Data8[6+i+1] = theConfig.chkModeMac[i+1];
				}
			// }

			// for(int i=1; i<=12; i++) {
			// 	IdaTxDataForm.Data8[i+5] = 0x00;
			// }

			// memcpy(IdaTxDataForm.Data8, "9012A100CB12")

			IdaTxDataForm.Lenth16 = 19;

			CtLogGreen("[ParsRxData] Get EVCC MAC Addr ");
			for(int i=0; i<18; i++){
				printf("%u ", IdaTxDataForm.Data8[i]);
			}
			printf("\r\n");
			
			return true;
		break;

		case CMD_GETWIFI :
			return false;
		break;

		case CMD_REPRTWIFI :
			if((shmDataAppInfo.app_order != APP_ORDER_CHARGING) && (shmDataAppInfo.app_order != APP_ORDER_CHARGE_READY))
			{
				return false;
			}
			// else
			// {
			// 	if()
			// 	return true;
			// }
		break;

		case CMD_SETCURR :
			if((IdaRxDataForm[No].Data8[0] >= 0x00) && (IdaRxDataForm[No].Data8[0] <= 0x64)) {

				IdaData.CurrRatio = IdaRxDataForm[No].Data8[0];
				IdaTxDataForm.Data8[0] = ACK_CODE;
				IdaTxDataForm.Lenth16 = 2;

				CtLogGreen("[ParsRxData] Set CurrRatio to %d [%d]\r\n", IdaData.CurrRatio, IdaRxDataForm[No].Data8[0]);

				return true;
			}
			else 
				return false;
		break;

		case CMD_SENDBWD :

			if(IdaRxDataForm[No].Lenth16 == 0x0a){
				IdaData.Soc = IdaRxDataForm[No].Data8[0];
				memcpy(&IdaData.PackVolt, &IdaRxDataForm[No].Data8[1], 2);
				memcpy(&IdaData.PackCurr, &IdaRxDataForm[No].Data8[3], 2);
				IdaData.CelVoltMin = IdaRxDataForm[No].Data8[5];
				IdaData.CelVoltMax = IdaRxDataForm[No].Data8[6];
				IdaData.CellTempMin = IdaRxDataForm[No].Data8[7];
				IdaData.CellTempMax = IdaRxDataForm[No].Data8[8];

				IdaTxDataForm.Data8[0] = ACK_CODE;
				IdaTxDataForm.Lenth16 = 2;

				CtLogGreen("Receive CMD_SENDBWD Soc %d / Pack Volt %d / Current %d / CellvoltMin %d / %d / %d / %d", \
				IdaData.Soc, IdaData.PackVolt, IdaData.PackCurr, IdaData.CelVoltMin, IdaData.CelVoltMax, IdaData.CellTempMin, IdaData.CellTempMax);

				return true;
			}
			else {
				return false;
			}

		break;

		case CMD_SENDKERID :
			// if((shmDataAppInfo.app_order!=APP_ORDER_CHARGE_READY) && (shmDataAppInfo.app_order!=APP_ORDER_CHARGING)) {
			// 	CtLogRed("When Other Charging Status Reqeust from PLC Modem");
			// 	return false;
			// }

			// 1. Valid Timestamp Data
			if(IdaRxDataForm[No].Lenth16 < 7) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}
			if(IdaRxDataForm[No].Data8[0] != 0xA1) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}
			if(IdaRxDataForm[No].Data8[1] != 0x04) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}

		

			// 2. Valid VIN Data

			if(IdaRxDataForm[No].Lenth16 < (7 + 19)) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}
			if(IdaRxDataForm[No].Data8[6] != 0xA2) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}
			if(IdaRxDataForm[No].Data8[7] != 0x11) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}


			// 3. Valid SOC data

			if(IdaRxDataForm[No].Lenth16 < (7 + 19 + 3)) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}
			if(IdaRxDataForm[No].Data8[25] != 0xA3) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}
			if(IdaRxDataForm[No].Data8[26] != 0x01) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}		

			// 4. Valid SOH data

			if(IdaRxDataForm[No].Lenth16 < (7 + 19 + 3 + 3)) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}
			if(IdaRxDataForm[No].Data8[28] != 0xA4) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}
			if(IdaRxDataForm[No].Data8[29] != 0x01) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}		

			// 5. Valid Pack Current data

			if(IdaRxDataForm[No].Lenth16 < (7 + 19 + 3 + 3 + 4)) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}
			if(IdaRxDataForm[No].Data8[31] != 0xA5) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}
			if(IdaRxDataForm[No].Data8[32] != 0x02) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}		

			// 6. Valid Pack Voltage data

			if(IdaRxDataForm[No].Lenth16 < (7 + 19 + 3 + 3 + 4 + 4)) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}
			if(IdaRxDataForm[No].Data8[35] != 0xA6) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}
			if(IdaRxDataForm[No].Data8[36] != 0x02) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}		

			// 7. Valid Cell Voltage data

			if(IdaRxDataForm[No].Lenth16 < (7 + 19 + 3 + 3 + 4 + 4 + 3)) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}
			if(IdaRxDataForm[No].Data8[39] != 0xA7) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}
			// if(IdaRxDataForm[No].Data8[40] != 0x02) {
			// 	CtLogRed("Receive Fail CMD_SENDKERID ");
			// 	return false;
			// }		

			id_tmp = IdaRxDataForm[No].Data8[40] * 0x100 + IdaRxDataForm[No].Data8[41];

			if(id_tmp > 192) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}

			if(IdaRxDataForm[No].Lenth16 < (7 + 19 + 3 + 3 + 4 + 4 + 3 + id_tmp)) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}

			// 8. Valid Module Temp data

			if(IdaRxDataForm[No].Lenth16 < (7 + 19 + 3 + 3 + 4 + 4 + 3 + id_tmp + 2)) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}
			if(IdaRxDataForm[No].Data8[42 + id_tmp] != 0xA8) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}
			// if(IdaRxDataForm[No].Data8[40] != 0x02) {
			// 	CtLogRed("Receive Fail CMD_SENDKERID ");
			// 	return false;
			// }		

			id_tmp1 = IdaRxDataForm[No].Data8[43 + id_tmp];

			if(id_tmp1 > 20) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}

			if(IdaRxDataForm[No].Lenth16 < (7 + 19 + 3 + 3 + 4 + 4 + 3 + id_tmp + 2 + id_tmp1)) {
				CtLogRed("Receive Fail CMD_SENDKERID ");
				return false;
			}

			// Data Setting


			// IdaKeriData_idx = 0;
			if(IdaKeriData_idx > 29) {
				memcpy(IdaKeriData, &(IdaKeriData[1]), sizeof(IdaKeriData[0])*29);
				IdaKeriData_idx = 29;
			}

			pIdaKeriData = &IdaKeriData[IdaKeriData_idx];

			// 1. timestmamp
			// need time sync
			pIdaKeriData->TimeStamp = IdaRxDataForm[No].Data8[2] * 0x1000000 + IdaRxDataForm[No].Data8[3] * 0x10000 \
			 + IdaRxDataForm[No].Data8[4] * 0x100 + IdaRxDataForm[No].Data8[5];
			// printf("Start Kerid Data Valid!!! \r\n 1. Time Stamp Data : %lu\r\n", pIdaKeriData->TimeStamp);

			// 2. Vin
			memcpy(pIdaKeriData->Vin, &(IdaRxDataForm[No].Data8[8]), 17);
			// printf("2. VIN Data : ");
			// for(int i = 0; i<17; i++) {
			// 	printf("%c ",pIdaKeriData->Vin[i]);
			// }
			// printf("\r\n");

			// 3. SOC
			pIdaKeriData->Soc = IdaRxDataForm[No].Data8[27];
			// printf("3. SOC Data : %d\r\n", pIdaKeriData->Soc);

			// 4. SOH
			pIdaKeriData->Soh = IdaRxDataForm[No].Data8[30];
			// printf("4. SOH Data : %d\r\n", pIdaKeriData->Soh);

			// 5. Current
			pIdaKeriData->PackCurr = IdaRxDataForm[No].Data8[33] * 0x100 + IdaRxDataForm[No].Data8[34];
			// printf("5. Pack Current Data : %lu\r\n", pIdaKeriData->PackCurr);

			// 6. Voltage
			pIdaKeriData->PackVolt = IdaRxDataForm[No].Data8[37] * 0x100 + IdaRxDataForm[No].Data8[38];
			// printf("6. Pack Voltage Data : %lu\r\n", pIdaKeriData->PackVolt);

			// 7. Cell Voltage
			// printf("7. Cell Voltage Data : ");
			for(int i=0; i<id_tmp; i++) {
				pIdaKeriData->cellVoltLen = id_tmp;
				pIdaKeriData->CellVolt[i] = IdaRxDataForm[No].Data8[42 + i];
				// printf("%u ", pIdaKeriData->CellVolt[i]);
			}
			// printf("\r\n");

			// 8. Modules Temperature
			// printf("8. Temperature Data : ");
			for(int i=0; i<id_tmp1; i++) {
				pIdaKeriData->modulTempLen = id_tmp1;
				pIdaKeriData->ModulTemp[i] = IdaRxDataForm[No].Data8[44 + id_tmp + i];
				// printf("%u ", pIdaKeriData->ModulTemp[i]);
			}
			// printf("\r\n");

			idaVasRawData[IdaKeriData_idx].dataLenth = IdaRxDataForm[No].Lenth16 - 6;

			memcpy(idaVasRawData[IdaKeriData_idx].data, &IdaRxDataForm[No].Data8[6], idaVasRawData[IdaKeriData_idx].dataLenth);

			IdaTxDataForm.Data8[0] = ACK_CODE;
			IdaTxDataForm.Lenth16 = 2;

			CtLogGreen("Receive CMD_SENDKERID %d / %d", IdaKeriData_idx, idaVasRawData[IdaKeriData_idx].dataLenth);

			IdaKeriData_idx++;

	
			return true;
			
		break;

		case CMD_EMGSTOP :
			// Get Bat Keri Data();
			if((shmDataAppInfo.app_order == APP_ORDER_CHARGING) || (shmDataAppInfo.app_order == APP_ORDER_CHARGE_READY)) {
				
				CtLogRed("OBD EMG Stop!!!!!!");

				shmDataAppInfo.charge_comp_status = END_ERR;
				StopCharge();
				ituLayerGoto(ituSceneFindWidget(&theScene, "ch2FinishLayer"));

				IdaTxDataForm.Data8[0] = ACK_CODE;
				IdaTxDataForm.Lenth16 = 2;
				
				return true;
			}
			else	return false;
		break;

		default :
			return false;
		break;
	}
}

static void* ObdRS232ReadTask(void* arg)
{
	char pars_buf[512];

    while(1)
    {
		char request[30];
		char rx_buf[512];
		uint8_t ret_8 = false;
		// bool ret_b = false;

		memset(rx_buf, 0, 512);
		int bufsize = read(OBD_READER_DEV, rx_buf, 512);

		if(bufsize) {
			// DumpBuffer("[Raw Rx Data]", rx_buf, bufsize);
			ret_8 = ValidIda(rx_buf, pars_buf, bufsize);
		}

		if(ret_8) {

			for(int i=0; i<ret_8; i++) {

				if((IdaRxDataForm[i].Cmd8 >= CMD_STARTCHRG) && (IdaRxDataForm[i].Cmd8 <= 0x1F)) {

					if(ResWaitCmd != IdaRxDataForm[i].Cmd8) {
						CtLogRed("PLC Msg Error : Different Response Cmd Code Tx with Rx Msg [wiat cmd : %d / rx cmd : %d]\r\n", ResWaitCmd, IdaRxDataForm[0].Cmd8);
						// DumpBuffer("Reqeust from PLC Modem", buffer, bufsize);
					}
					else {
						// rx ack nack 

						if(IdaRxDataForm[i].Data8[0] == ACK_CODE) {
							RespWaitCnt = 0;

							ResWaitCmd = 0x00;
							CtLogGreen("[Rx] Ack CMD : %d\r\n",IdaRxDataForm[i].Cmd8);
						}
						else {
							CtLogRed("[Rx] Nack (%d) CMD : %d\r\n",IdaRxDataForm[i].Data8[0], IdaRxDataForm[i].Cmd8);
						}

					}
				}
				else /*if(ret_8 == REQ_MSG_CODE)*/ {

					if(ParsRxData(i)) {
						MakeTx(IdaRxDataForm[i].Cmd8, IdaTxDataForm.Lenth16);
						
						SendBuff(request, IdaTxDataForm.Lenth16 - 1);
						write(OBD_READER_DEV, request, IdaTxDataForm.Lenth16 + 6);

						DumpBuffer("Check Response Data", request, IdaTxDataForm.Lenth16 + 6);
					}
					else {
						IdaTxDataForm.Data8[0] = NACK_CODE;
						
						MakeTx(IdaRxDataForm[i].Cmd8, 2);

						SendBuff(request, 1);
						write(OBD_READER_DEV, request, 8);

						DumpBuffer("Check Response Data", request, 8);
					}
				}

			}

			// }
		}

		else{
			// erro todo
		}


        usleep(OBD_READ_232_TASK_DELAY * 1000);
	}
		
	sReadPlcTask = 0;
	CtLogYellow("[Obd] exit read rs232 thread\n");
}

void ObdInit(void)
{
	IdaData.CurrRatio = 100;
	IdaData.CurrRatio_old = 100;

	IdaKeriData_idx = 0;

	if (sReadPlcTask==0)
	{
		CtLogYellow("[Obd] create rs232 read thread..\n");
		pthread_create(&sReadPlcTask, NULL, ObdRS232ReadTask, NULL);
		pthread_detach(sReadPlcTask);
	}		

	if (sReqPlcTask==0)
	{
		pthread_create(&sReqPlcTask, NULL, ObdRS232ReqTask, NULL);
		pthread_detach(sReqPlcTask);
	}
}
