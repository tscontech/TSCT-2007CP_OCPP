/**
*       @file
*               cstclient.c
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2018.01.11 <br>
*               author: bmlee <br>
*               description: <br>
*/

#include <sys/times.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "SDL/SDL.h"
//#include <time.h>
#include <strings.h>    /* for bzero, strcasecmp, and strncasecmp */
#include "scene.h"
#include "tsctwsclient.h"
#include "tsctcommon.h"
#include "tsctcfg.h"
#include "ctrlboard.h"

#include "curl/curl.h"

#include "ocpp_cmd.h"

//#include "tsctjson.h"

CURL *curl = NULL;
pthread_mutex_t curlMutex;
bool isBrokenSocket;

struct curl_blob blob;

const char* cp_req_msg[] = {
	"NONE",
	"Authorize",
	"BootNotification",
	"DataTransfer",
	"DiagnosticsStatusNotification",
	"FirmwareStatusNotification",
	"Heartbeat",
	"MeterValues",
	"StartTransaction",
	"StatusNotification",
	"StopTransaction"
};

const char* cs_req_msg[] = {
	"NONE",
	"CancelReservation",
	"ChangeAvailability",
	"ChangeConfiguration",
	"ClearCache",
	"ClearChargingProfile",
	"DataTransfer",
	"GetCompositeSchedule",
	"GetConfiguration",
	"GetDiagnostics",
	"GetLocalListVersion",
	"RemoteStartTransaction",
	"RemoteStopTransaction",
	"ReserveNow",
	"Reset",
	"SendLocalList",
	"SetChargingProfile",
	"TriggerMessage",
	"UnlockConnector",
	"UpdateFirmware"
};

pthread_t sWSClientTask = 0;

pthread_t sWSRxTask = 0;

struct timeval tv_1, tv_2, tv_res, tv_curr;

uint8_t bRecvConnCnt;

char curl_ws_send_buf[64*1024];

char curl_ws_recv_buf[1*1024];		// GetConfig Max 700 ~ 800 byte

// char* curl_ws_send_buf;

// char* curl_ws_recv_buf;		// GetConfig Max 700 ~ 800 byte

char ocppConfigKeyName[MAX_CFG][40] = 
{
	"AuthorizeRemoteTxRequests",
	"ClockAlignedDataInterval",
	"ConnectionTimeOut",
	"ConnectorPhaseRotation",
	"GetConfigurationMaxKeys",
	"HeartbeatInterval",
	"LocalAuthorizeOffline",
	"LocalPreAuthorize",
	"MeterValuesAlignedData",
	"MeterValuesSampledData",
	"MeterValueSampleInterval",
	"NumberOfConnectors",
	"ResetRetries",
	"StopTransactionOnEVSideDisconnect",
	"StopTransactionOnInvalidId",
	"StopTxnAlignedData",
	"StopTxnSampledData",
	"SupportedFeatureProfiles",
	"TransactionMessageAttempts",
	"TransactionMessageRetryInterval",
	"UnlockConnectorOnEVSideDisconnect",
	// "Local",
	// "LocalAuthListEnabled",
	"LocalAuthListMaxLength",
	"SendLocalListMaxLength",
	// "Smart",
	// "ChargeProfileMaxStackLevel",
	// "ChargingScheduleAllowedChargingRateUnit",
	// "ChargingScheduleMaxPeriods",
	// "MaxChargingProfilesInstalled"

	"AllowOfflineTxForUnknownId",
};

long lastClockTvSec = 0;
static void* WSClientThread(void* arg);
void TsctCheckNetTimeSet(void)
{
	if(bGloAdminStatus)	return;  // when don't be a management menu.
	printf("\r\nTime Syncronize\r\n");
	printf("\r\n%d-%d-%d-%d-%d-%d\r\n",tNetTime.tm_year, tNetTime.tm_mon, tNetTime.tm_mday, tNetTime.tm_hour, tNetTime.tm_min, tNetTime.tm_sec);
	if(tNetTime.tm_year > 0)
	{
		long tNetRtcSec = 0;				
		tNetRtcSec = mktime((struct tm*)&tNetTime);
		itpRtcSetTime1(tNetRtcSec,0);					
		tNetTime.tm_year = 0;		
	}
}

void SetCpStatus(CP_STATUS_CODE code_no ,uint8_t ch_no)
{
	CtLogBlue("Set CP Status %d", code_no);
	CsConfigVal.bCpStat[ch_no] = code_no;
}

CP_STATUS_CODE GetCpStatus(uint8_t ch_no)
{
	return CsConfigVal.bCpStat[ch_no];
}

void Add_MeterVal_Q(uint8_t meterValType)
{
	uint8_t Qno = 0;
	uint32_t t = 0;

	while(Qno < MAX_QNO_METERVAL){
		if(MeterValQ[Qno].Meterval_Flg)		Qno++;	
		else								break;
	}

	if(Qno == MAX_QNO_METERVAL){
		for(int i = 0; i<(MAX_QNO_METERVAL-1); i++)
			memcpy(&MeterValQ[i],&MeterValQ[i+1],sizeof(MeterValQ[i]));

		MeterValQ[MAX_QNO_METERVAL-1].Meterval_Flg = meterValType;
		MeterValQ[MAX_QNO_METERVAL-1].Uid_Mv = CstGetTime_Msec_test();
		MeterValQ[MAX_QNO_METERVAL-1].Connect_Id = bDevChannel + 1;
		GetDateTime(MeterValQ[MAX_QNO_METERVAL-1].Time_Stamp);
		MeterValQ[MAX_QNO_METERVAL-1].Sampled_Val = 0;
		
		for(int i=0;i<4;i++)
			t += (uint32_t)(shmDataAppInfo.eqp_watt[i] << ((3-i)*8));

		MeterValQ[Qno].Sampled_Val = t- startTsQ.MeterStart_Val;
		if(MeterValQ[Qno].Sampled_Val < 0)
		{
			MeterValQ[Qno].Sampled_Val = 0;
		}
		else{
			startTsQ.MeterStart_Val = t;
		}
		MeterValQ[Qno].Current = TSCTGetAMICurrent();
		MeterValQ[Qno].Volt = TSCTGetAMIVolt();
		CtLogRed("[Add_MeterVal_Q %d] Meterval_Flg %d / Sampled_Val %d / %d.%02dA / %d V", Qno, MeterValQ[Qno].Meterval_Flg, MeterValQ[Qno].Sampled_Val, MeterValQ[Qno].Current/100,(MeterValQ[Qno].Current%100), MeterValQ[Qno].Volt/10);
	}
	else{
		MeterValQ[Qno].Meterval_Flg = meterValType;
		MeterValQ[Qno].Uid_Mv = CstGetTime_Msec_test();
		MeterValQ[Qno].Connect_Id = bDevChannel + 1;
		GetDateTime(MeterValQ[Qno].Time_Stamp);
		MeterValQ[Qno].Sampled_Val = 0;
		
		for(int i=0;i<4;i++)
			t += (uint32_t)(shmDataAppInfo.eqp_watt[i] << ((3-i)*8));

		MeterValQ[Qno].Sampled_Val = t- startTsQ.MeterStart_Val;
		if(MeterValQ[Qno].Sampled_Val < 0)
		{
			MeterValQ[Qno].Sampled_Val = 0;
		}
		else{
			startTsQ.MeterStart_Val = t;
		}
		
		MeterValQ[Qno].Current = TSCTGetAMICurrent();
		MeterValQ[Qno].Volt = TSCTGetAMIVolt();
		CtLogRed("[Add_MeterVal_Q %d] Meterval_Flg %d / Sampled_Val %d / %d.%02dA / %d V", Qno, MeterValQ[Qno].Meterval_Flg, MeterValQ[Qno].Sampled_Val, MeterValQ[Qno].Current/100,(MeterValQ[Qno].Current%100), MeterValQ[Qno].Volt/10);
	}
}

void Reset_MeterVal_Q(void)
{
	uint8_t Qno = 0;

	while(Qno < MAX_QNO_METERVAL){
		if(MeterValQ[Qno].Meterval_Flg)		Qno++;	
		else								break;
	}
	for(int i = 0; i < Qno; i++)
	{
		MeterValQ[i].Meterval_Flg = false;
		MeterValQ[i].Uid_Mv = 0;
		MeterValQ[i].Connect_Id = 0;
		MeterValQ[i].Sampled_Val = 0;
		MeterValQ[i].Current = 0;
		MeterValQ[i].Volt = 0;
	}
}

int Check_Sampled_Time() {
	if((GetCpStatus(bDevChannel+1) != CP_STATUS_CODE_CHARGING) || !sChCharging)
	{
		Check_Sampled_flg = false;
		return false;
	}
		
	time_t cur_time = CstGetTime();
	struct tm *currentTime = localtime(&cur_time);
	int secondsPastHour = currentTime->tm_min * 60 + currentTime->tm_sec; // 정시 기준 경과된 초
    int secondsUntilNextCycle = 60 - (secondsPastHour % 60);
	//printf("다음 주기까지 남은 시간: %d초\n", secondsUntilNextCycle);
	if(secondsUntilNextCycle >=60 && Check_Sampled_flg == false)
	{
		Check_Sampled_flg = true;
		return METER_VAL_CLOCK_TYPE;
	}
	if(secondsUntilNextCycle == 1)
		Check_Sampled_flg = false;

	return false;
}

bool Check_Meter_Time(){
	if((GetCpStatus(bDevChannel+1) != CP_STATUS_CODE_CHARGING) || !sChCharging)
	{
		Check_Meter_flg = false;
		return false;
	}
		
	time_t cur_time = CstGetTime();
	struct tm *currentTime = localtime(&cur_time);
	int secondsPastHour = currentTime->tm_min * 60 + currentTime->tm_sec; // 정시 기준 경과된 초
    int secondsUntilNextCycle = 180 - (secondsPastHour % 180); 
	//int secondsUntilNextCycle = 60 - (secondsPastHour % 60); 

//	printf("현재 시간: %02d:%02d:%02d\n", currentTime->tm_hour, currentTime->tm_min, currentTime->tm_sec);
//    printf("다음 주기까지 남은 시간: %d초\n", secondsUntilNextCycle);

	if(secondsUntilNextCycle >=180 && Check_Meter_flg == false)
	{
		Check_Meter_flg = true;
		return true;
	}
	if(secondsUntilNextCycle == 1)
		Check_Meter_flg = false;

	return false;
}

bool Check_HB_Time(struct timeval* tval){
	//if(GetCpStatus(bDevChannel+1) == CP_STATUS_CODE_CHARGING)
	//	return false;

	gettimeofday(&tv_curr, NULL);

	if(tv_curr.tv_sec < tval->tv_sec){
		gettimeofday(tval, NULL);
		return false;
	}
	else if((tv_curr.tv_sec - tval->tv_sec) >= CfgKeyVal[5].CfgKeyDataInt)
		return true;		
	else	
		return false;
}

bool Check_Resp_Time(struct timeval* tval){
	gettimeofday(&tv_curr, NULL);

	if(tv_curr.tv_sec < tval->tv_sec){
		gettimeofday(tval, NULL);
		return false;
	}
	else if((tv_curr.tv_sec - tval->tv_sec) >= CfgKeyVal[18].CfgKeyDataInt)
		return true;		
	else	
		return false;
}

void Reset_Time(struct timeval* tval){
	gettimeofday(tval, NULL);
}

bool CheckUpdateFwDate(void)
{
	time_t curr_time = CstGetTime();

	if(fwUpdateVals.updateStep)
	{
		if(curr_time > CnvrtStr2Utc(fwUpdateVals.Time_Stamp))
		{
			// printf("\r\n\r\n[CheckUpdateFwDate] time check OK - app_order : %u\r\n\r\n", shmDataAppInfo.app_order);
			// if(shmDataAppInfo.app_order == APP_ORDER_WAIT)
				return true;
		}
		else
		{
			printf("\r\n\r\n[CheckUpdateFwDate] time check - curr : %lld / set : %lld \r\n\r\n", curr_time, CnvrtStr2Utc(fwUpdateVals.Time_Stamp));
		}
	}

	return false;
}

static CURLcode WS_Server_connect(void)
{
	CURLcode ret = CURLE_OK;
	char ConnURLAddr_buf[120];
	char chBuf[15];
/*
	//pthread_mutex_lock(&curlMutex);
	if(curl != NULL)
	{
		isBrokenSocket = true;
		curl_easy_cleanup(curl);
		curl = NULL;
	}
	*/
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	
	if(curl)	{
		memset(ConnURLAddr_buf, 0x00, sizeof(ConnURLAddr_buf));
		memcpy(ConnURLAddr_buf,theConfig.serverip,sizeof(theConfig.serverip));
		sprintf(chBuf, ":%d", theConfig.serverport);
		strcat(ConnURLAddr_buf, chBuf);
		strcat(ConnURLAddr_buf, "/ocpp/");
		strcat(ConnURLAddr_buf, theConfig.siteid);
		strcat(ConnURLAddr_buf, theConfig.devid1);
		curl_easy_setopt(curl, CURLOPT_URL, ConnURLAddr_buf);
		curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);

		printf("$$$$$$$$$$$$$$$$$$$$$$$$curl_easy_perform Start\r\n");
		
		// Make a Handshake
		ret = curl_easy_perform(curl);
		if(ret){
			printf("$$$$$$$$$$$$$$$$$$$$$$$$CURL WebSocket Connect Error %d", ret);
			printf(" / url : %s\r\n",ConnURLAddr_buf);
			curl_easy_cleanup(curl);
			curl = NULL;
            bConnect = false;
		}
		else{
			printf("$$$$$$$$$$$$$$$$$$$$$$$$curl_easy_perform Finish\r\n");	
			printf(" / url : %s\r\n",ConnURLAddr_buf);	
			bConnect = true;
			bWaitResFlg = false;	
			isBrokenSocket = false;
		}
	}
	//pthread_mutex_unlock(&curlMutex);
  
    return ret;
}

/** Receive MSG Logic **/

static uint8_t Rx_CallRes_DataProc(uint8_t cmd)
{
	switch(Rx_Msg.Action_Code){
		case CP_REQ_ACTION_CODE_AUTH:
			DataProcCmd_Auth();
		break;
		case CP_REQ_ACTION_CODE_BOOT:
			DataProcCmd_Boot();
		break;
		case CP_REQ_ACTION_CODE_DATATRANS:
			DataProcCmd_DataTransCp();
		break;
		case CP_REQ_ACTION_CODE_DIAGSTAT:
			DataProcCmd_DiagStat();
		break;
		case CP_REQ_ACTION_CODE_FWSTAT:
		break;
		case CP_REQ_ACTION_CODE_HB:
			DataProcCmd_HB();
		break;
		case CP_REQ_ACTION_CODE_METERVAL:
			DataProcCmd_MeterVal();
		break;
		case CP_REQ_ACTION_CODE_STARTTS:
			DataProcCmd_StartTs();
		break;
		case CP_REQ_ACTION_CODE_STAT:
			DataProcCmd_Stat();
		break;
		case CP_REQ_ACTION_CODE_STOPTS:
			DataProcCmd_StopTs();
		break;
		default:
			printf("Non Defined Action Response Command %d\r\n", Rx_Msg.Action_Code);
		break;
	}
}

static uint8_t Rx_Call_DataProc(uint8_t cmd)
{
	Tx_Msg.Msg_type = MSG_TYPE_CALLRES;
	switch(cmd){
		case CS_REQ_ACTION_CODE_NONE:
			printf("None Command Code\n");
			break;

		case CS_REQ_ACTION_CODE_CANCLERESEVE:
			DataProcCmd_CancelReserve();
			break;

		case CS_REQ_ACTION_CODE_CHANGEAVAIL:
			DataProcCmd_ChangeAvail();
			break;
		case CS_REQ_ACTION_CODE_CHANGECONFIG:
			DataProcCmd_ChangeConfig();
		break;
		case CS_REQ_ACTION_CODE_CLEARCACHE:
			DataProcCmd_ClearCache();
			break;
		case CS_REQ_ACTION_CODE_CLEARCAHRGPROF:
			DataProcCmd_ClearChargeProf();
			break;
		case CS_REQ_ACTION_CODE_DATATRANSFER:
			DataProcCmd_DataTransCs();
			break;
		case CS_REQ_ACTION_CODE_GETCOMPOSITSCHED:

			break;
		case CS_REQ_ACTION_CODE_GETCONFIG:
			DataProcCmd_GetConfig();
			break;
		case CS_REQ_ACTION_CODE_GETDIAG:
			DataProcCmd_GetDiag();
			break;		
		case CS_REQ_ACTION_CODE_GETLOCALVER:
			DataProcCmd_GetLocalListVer();
			break;	
		case CS_REQ_ACTION_CODE_REMOTESTART:
			DataProcCmd_RmStart();
			break;								
		case CS_REQ_ACTION_CODE_REMOTESTOP:
			DataProcCmd_RmStopTr();
			break;	
		case CS_REQ_ACTION_CODE_RESERVE:
		
			break;	
		case CS_REQ_ACTION_CODE_RESET:
			DataProcCmd_Reset();
			break;
		case CS_REQ_ACTION_CODE_SNDLOCALLIST:
			DataProcCmd_SenLocalList();
			break;
		case CS_REQ_ACTION_CODE_SETCHGPROFILE:
		
			break;
		case CS_REQ_ACTION_CODE_TRIGGERMSG:
			DataProcCmd_TriggerMsg();
			break;		
		case CS_REQ_ACTION_CODE_UNLOCKCONNECT:
			DataProcCmd_UnlockConnect();
			break;	
		case CS_REQ_ACTION_CODE_UPDATEFW:
			DataProcCmd_UpdateFw();
			break;										
		default:
			printf("Non Defined Action Command %d\r\n", Rx_Msg.Action_Code);
			bClientStep = CLIENT_IDLE_STEP;
			return 1;
	}	
	return 0;
}

void Clear_Rx_Msg_Buff(void)
{
	memset(&Rx_Msg, '\0', sizeof(MSG_STRUCT));
	memset(sub_Payload1, '\0', sizeof(sub_Payload1));
	memset(sub_Payload2, '\0', sizeof(sub_Payload2));
	memset(sub_Payload3, '\0', sizeof(sub_Payload3));
	// memset(tx_sub_Payload, '\0', sizeof(tx_sub_Payload));
	// tx_sub_Payload
}

void Dump_Recv_Buf(char *str)
{
	size_t slen=0, tlen=0;
	bool str_flg = false;

	slen = strlen(str);
	for(int i=0; i<slen; i++){
		if(str[i] == '[')
			tlen++;
		else if(str[i] == ']'){
			tlen--;
			if(tlen == 0){
				memset(&(str[i+1]),NULL,1);
				break;
			}
		}	
	}
	// printf("\r\nRecv success : %s\r\n", str);
}

bool Tsct_Curl_Ws_Recv(void)
{
	CURLcode ret = 1;
	size_t rlen;
	struct curl_ws_frame* meta;
/*
	//pthread_mutex_lock(&curlMutex);
	if(curl == NULL || isBrokenSocket) 
	{
		//pthread_mutex_unlock(&curlMutex);
		sleep(1);
		return false;
	}
	*/
	ret = curl_ws_recv(curl, curl_ws_recv_buf, sizeof(curl_ws_recv_buf), &rlen, &meta);
	//pthread_mutex_unlock(&curlMutex);
	if(ret == CURLE_AGAIN) {
		return false;
	}
	// Check curl receive Error
	else if(ret > 0){
		Clear_Rx_Msg_Buff();
		CtLogRed("\r\nRecv fail %d\r\n", ret); //Error Code 56 - Connection Lost
		/*
		//pthread_mutex_lock(&curlMutex);
		if(bConnect)
		{
			isBrokenSocket = true;
			curl_easy_cleanup(curl);
			curl = NULL;
		}
		//pthread_mutex_unlock(&curlMutex);
		*/
		bConnect = false;
		sleep(1);
		return false;
	}
	else{
		bRecvConnCnt = 0;
		OcppRxMsgLog("%s", curl_ws_recv_buf);
	}	

	// Parse Received Data
	// ********** Parsing error action
	Clear_Rx_Msg_Buff();
	curl_ws_recv_buf[rlen] ='\0';
	ret = Parse_Data(curl_ws_recv_buf, rlen);
	if(ret != 0){
		CtLogRed("Message Format Error %d\r\n", ret);
		return false;
	}

	if(Rx_Msg.Msg_type == 4)
	{
		bConnect = false;
		ServerCallError = true;
	}
	else 
	{
		ServerCallError = false;
		// for Before Receive BootNotification Response
		if(GetCpStatus(0) == CP_STATUS_CODE_NONE){
			// Call Response Data Proc
			if((Rx_Msg.Msg_type == 3) && (Rx_Msg.Action_Code == CP_REQ_ACTION_CODE_BOOT))
			{
				if(bWaitResFlg && (Call_Tx_Msg.UniqueID == Rx_Msg.UniqueID)){
					bWaitResFlg = false;
					Rx_CallRes_DataProc(Rx_Msg.Action_Code);
				}
			}
			else if((Rx_Msg.Msg_type == 2) && (Rx_Msg.Action_Code == CS_REQ_ACTION_CODE_DATATRANSFER)) {
				Rx_Call_DataProc(CS_REQ_ACTION_CODE_DATATRANSFER);
			}
		}
		else{
			// Call Response Data Proc
			if(Rx_Msg.Msg_type == 3){
				if(bWaitResFlg && (Call_Tx_Msg.UniqueID == Rx_Msg.UniqueID))
				{
					bWaitResFlg = false;
					Rx_CallRes_DataProc(Rx_Msg.Action_Code);
				}
			}
			// Call Data Proc
			else if(Rx_Msg.Msg_type == 2){
				ret = Rx_Call_DataProc(Rx_Msg.Action_Code);
				// ************* Rsponse Error to server
				if(ret)	printf("Non Defined Action Command %d\r\n", Rx_Msg.Action_Code);
				else	Tsct_Curl_Ws_Trans();
			} 
		}
	}
}

static void* WSRxThread(void* arg)
{
	while(1)
	{
		if(bConnect)
		{
			Tsct_Curl_Ws_Recv();
		}
		usleep(1000*1000);
	}
}

///---------------------------------------------------- Under Function Ok ----------------------

void convStr2Sstr(char* desBuf, char* strBuf) {
	strcat(desBuf, "\\\"");
	strcat(desBuf, strBuf);
	strcat(desBuf, "\\\"");
}

void convStr2Int(char* desBuf, char* strBuf) {
	strcat(desBuf, strBuf);
}

void convStr2Str(char* desBuf, char* strBuf) {
	strcat(desBuf, "\"");
	strcat(desBuf, strBuf);
	strcat(desBuf, "\"");
}

void convInt2Int(char* desBuf, int val) {
	char temp_buf[15];

	sprintf(temp_buf,"%ld", val);
	strcat(desBuf,temp_buf);
}

void convLLong2Str(char* desBuf, uint64_t val) {
	char temp_buf[25];

	sprintf(temp_buf,"\"%llu\"", val);
	strcat(desBuf,temp_buf);
}



/** 
 * Transmit MSG Logic 
 * **/

static void Make_Ocpp_Msg(char* send_buf)
{
	struct sPayload* tmpPayload;
	uint16_t batIdx = 0;

	char temp_buf[130]; // ex) for convert int to string ... 

	int keyLen;

	PAYLOAD* mainObj;
	PAYLOAD* subObj;
	PAYLOAD* subSubObj;

	// start header
	sprintf(send_buf,"[");
	
	convInt2Int(send_buf, Tx_Msg.Msg_type);

	convStr2Int(send_buf, ",");

	if(Tx_Msg.Msg_type == MSG_TYPE_CALL){
		convLLong2Str(send_buf, Tx_Msg.UniqueID);

		convStr2Int(send_buf, ",");

		convStr2Str(send_buf, cp_req_msg[Tx_Msg.Action_Code]);
	}
	else if(Tx_Msg.Msg_type == MSG_TYPE_CALLRES){
		convStr2Str(send_buf, Rx_Msg.UniqueID_Char);
	}
	// end header

	if(Tx_Msg.Action_Code == CP_REQ_ACTION_CODE_DATATRANS)
	{
		DATATRANS_UniqueID = Tx_Msg.UniqueID;
		//printf("\r\nDATATRANS_UniqueID : %d\r\n", DATATRANS_UniqueID);
	}

	// start payload
	convStr2Int(send_buf, ",{");
	
	keyLen = Tx_Msg.Payload_len;

	for(uint8_t keyIdx = 0; keyIdx < keyLen; keyIdx++){
		
		mainObj = &Tx_Msg.Payload[keyIdx];

		if(keyIdx != 0) convStr2Int(send_buf, ",");

		if(mainObj->data_type == TYPE_CODE_INT){
			convStr2Str(send_buf, mainObj->property_name);
			convStr2Int(send_buf, ":");
			convStr2Int(send_buf, mainObj->property_contants);			
		}
		else if(mainObj->data_type == TYPE_CODE_ARR){
			
			convStr2Str(send_buf, mainObj->property_name);

			convStr2Int(send_buf, ":");

			convStr2Int(send_buf, "[");

			for(int i = 0; i<mainObj->subPayload_len; i++){
				
				subObj = &mainObj->sub_Payload[i];

				if(i != 0) convStr2Int(send_buf, ",");

				convStr2Int(send_buf, subObj->property_contants);
			}
			convStr2Int(send_buf, "]");

			mainObj->subPayload_len = 0;
		}
		// string or object
		else{
			// check Object
			if(mainObj->subPayload_len > 0) {
					
				convStr2Str(send_buf, mainObj->property_name);

				convStr2Int(send_buf, ":");
				
				if(mainObj->data_type == TYPE_CODE_OBJ_STR)
					convStr2Int(send_buf, "\"{");
				else
					convStr2Int(send_buf, "{");
			
				for(int sub1Idx=0; sub1Idx < mainObj->subPayload_len; sub1Idx++) {

					subObj = &mainObj->sub_Payload[sub1Idx];

					if(sub1Idx != 0) convStr2Int(send_buf, ",");

					if(mainObj->data_type == TYPE_CODE_OBJ_STR)
						convStr2Sstr(send_buf, subObj->property_name);
					else
						convStr2Str(send_buf, subObj->property_name);

					convStr2Int(send_buf, ":");

					if(subObj->data_type == TYPE_CODE_ARR) {

						convStr2Int(send_buf, "[");

						for(int sub2Idx=0; sub2Idx < subObj->subPayloadArrLen; sub2Idx++) {

							if(sub2Idx != 0) convStr2Int(send_buf, ",");

							convStr2Int(send_buf, "{");

							for(int sub3Idx=0; sub3Idx < subObj->subPayload_len; sub3Idx++) {

								subSubObj = &subObj->sub_Payload[sub3Idx + (subObj->subPayload_len * sub2Idx)];
								
								if(sub3Idx != 0) convStr2Int(send_buf, ",");

								if(mainObj->data_type == TYPE_CODE_OBJ_STR)
									convStr2Sstr(send_buf, subSubObj->property_name);
								else
									convStr2Str(send_buf, subSubObj->property_name);

								convStr2Int(send_buf, ":");

								if(subSubObj->data_type == TYPE_CODE_LITE) {

									convStr2Int(send_buf, "[");

									for(int sub4Idx=0; sub4Idx < subSubObj->subPayloadArrLen; sub4Idx++){
										
										if(sub4Idx != 0) convStr2Int(send_buf, ",");
										
										convStr2Int(send_buf, "{");

										for(int sub5Idx=0; sub5Idx < subSubObj->subPayload_len; sub5Idx++) {

											if(mainObj->data_type == TYPE_CODE_OBJ_STR) {
												convStr2Sstr(send_buf, subPayloadBat[batIdx].property_name);

												convStr2Int(send_buf, ":");

												convStr2Sstr(send_buf, subPayloadBat[batIdx].property_contants);
											}
											else {
												convStr2Str(send_buf, subPayloadBat[batIdx].property_name);

												convStr2Int(send_buf, ":");

												convStr2Str(send_buf, subPayloadBat[batIdx].property_contants);
											}

											batIdx++;
										}

										convStr2Int(send_buf, "}");
									}
									convStr2Int(send_buf, "]");
								}
								else {

									if(mainObj->data_type == TYPE_CODE_OBJ_STR)
										convStr2Sstr(send_buf, subSubObj->property_contants);
									else
										convStr2Str(send_buf, subSubObj->property_contants);								
								}
							}
							convStr2Int(send_buf, "}");
						}
						convStr2Int(send_buf, "]");
					}
					else {	

						if(mainObj->data_type == TYPE_CODE_OBJ_STR)
							convStr2Sstr(send_buf, subObj->property_contants);
						else
							convStr2Str(send_buf, subObj->property_contants);
					}

				}

				if(mainObj->data_type == TYPE_CODE_OBJ_STR)
					convStr2Int(send_buf, "}\"");
				else
					convStr2Int(send_buf, "}");
			}
			// check String
			else {
				
				convStr2Str(send_buf, mainObj->property_name);

				convStr2Int(send_buf, ":");

				convStr2Str(send_buf, mainObj->property_contants);
			}
		}
	} // end for :: payload's keys

	convStr2Int(send_buf, "}");

	// end Payload

	convStr2Int(send_buf, "]");

	switch(Tx_Msg.Action_Code)
	{
		case CP_REQ_ACTION_CODE_BOOT : //Fall Through
		case CP_REQ_ACTION_CODE_STARTTS : 
			sleep(3);
			break;
		default : 
			break;
	}

}

void Tsct_Curl_Ws_Trans(void)	// CALL
{
	CP_REQ_ACTION_CODE cmd;
	CURLcode ret = 1;
	size_t bytes_encode;
	size_t bytes_sent;
	// char curl_ws_send_buf[51200];

	Make_Ocpp_Msg(curl_ws_send_buf);

	// printf("Start Msg %s\r\n", curl_ws_send_buf);

	// sleep(1);
	/*
	//pthread_mutex_lock(&curlMutex);
	if(curl == NULL || isBrokenSocket) 
	{
		//pthread_mutex_unlock(&curlMutex);
		sleep(1);
		return;
	}
	*/
	ret = curl_ws_send(curl, curl_ws_send_buf, strlen(curl_ws_send_buf), &bytes_sent, 0, CURLWS_TEXT);
	if(ret){
		CtLogRed("WS send fail %d\r\n", ret);
		Clear_Rx_Msg_Buff();
		// if(bConnect)
		// 	// curl_easy_cleanup(curl);
		bConnect = false;
		sleep(1);
		// WS_Server_connect();		
		return;
		isBrokenSocket = false;
	}
	//pthread_mutex_unlock(&curlMutex);
	// else{
		OcppTxMsgLog("%s", curl_ws_send_buf);
		// printf("\r\nSend Success :\r\n %s\r\n", curl_ws_send_buf);
		// printf("\r\nSend Success :\r\n\r\n");
		if(Tx_Msg.Msg_type == MSG_TYPE_CALL){
			bWaitResFlg = true;
			// if(Tx_Msg.Action_Code == CP_REQ_ACTION_CODE_DATATRANS)
			// 	bWaitResFlg = false;
			Reset_Time(&tv_res);
			memcpy(&Call_Tx_Msg,&Tx_Msg,sizeof(Call_Tx_Msg));
		}
	// }
}

uint8_t remainVasCnt = 0;

bool SendVasData(uint8_t* vasDataCnt, uint8_t cmd)
{
	CURLcode ret = CURLE_OK;

	if((shmDataAppInfo.app_order != APP_ORDER_CHARGE_READY) && (shmDataAppInfo.app_order != APP_ORDER_CHARGING)) {
		if(remainVasCnt <= 100)
			remainVasCnt++;
	}
	else {
		remainVasCnt = 0;
	}


	if(remainVasCnt > (2*20)) 
	{
		if(*vasDataCnt >= 1) 
		{
			// CtLogMagenta("Call the Remain Data Trans %d", *vasDataCnt);
			// vasLastDataCnt = *vasDataCnt;

			// MakeDataCmd_DataTransCp(cmd);
			// memcpy(seccVasRxData, &(seccVasRxData[0]), sizeof(SECC_VAS_READ_DATA)*(30));

			*vasDataCnt = 0;
			vasLastDataCnt = 0;

			// return true;
		}
	}

	else if(*vasDataCnt >= CsConfigVal.batInfo_cfgCnt) 
	{
		vasLastDataCnt = 0;

		//MakeDataCmd_DataTransCp(cmd);

		memcpy(seccVasRxData, &(seccVasRxData[CsConfigVal.batInfo_cfgCnt]), sizeof(SECC_VAS_READ_DATA)*(30-CsConfigVal.batInfo_cfgCnt));
		*vasDataCnt -= CsConfigVal.batInfo_cfgCnt;

		return true;
	}

	return false;
}

static bool OCPP_CALL_Senario(void)
{
	if(bWaitResFlg) {
		if(Check_Resp_Time(&tv_res)){
			bConnect = false;
			bWaitResFlg = false;
		}
		return false;
	}

	if(GetCpStatus(0) <= CP_STATUS_CODE_PEND ){
		if (Check_HB_Time(&tv_1)) {
			MakeDataCmd_Boot();
			Reset_Time(&tv_1);
			return true; // data send
		}
		else return false;// nothing
	}
	else if(Check_Meter_Time()){
			MakeDataCmd_MeterVal();
			return true;
	}
	else 
	{
		for(int i=0;i<=MAX_CONECTOR_ID;i++)
		{
			if( (GetCpStatus(i) != CsConfigVal.bPreCpStat[i]) \
			|| ( bCpStatChgFlg & (1<<i) ) )
			{
				printf("Change Status[%d] %d => %d\r\n",i,CsConfigVal.bPreCpStat[i], GetCpStatus(i));
				MakeDataCmd_Stat(i);	
				bCpStatChgFlg &= ~(1<<i);
				return true;
			}
		}

		if(bChargerCostCheck == 2)
		{
			bChargerCostCheck =0;
			MakeDataCmd_DataTrans_j1();
			return true;
		}

		// CP Status : Available
		if(CsConfigVal.bReqAuthNo)
		{
			MakeDataCmd_Auth();
			return true;
		}

		if(CsConfigVal.bReqMemberFlg)
		{
			CsConfigVal.bReqMemberFlg = false;
			sleep(1);
			MakeDataCmd_DataTrans_mbrUntpc();
			return true;
		}

		// CP Status : Prepare
		if(startTsQ.reqStartTsFlg){
			MakeDataCmd_StartTs();
			return true;
		}

		if(CsConfigVal.bReqStopTsFlg){
			CsConfigVal.bReqStopTsFlg = false;
			Add_MeterVal_Q(METER_VAL_TRANS_TYPE);
			GetDateTime(StopTsConfig.Time_Stamp);
			StopTsConfig.MeterStop_Val = 0;
			for(int i = 0; i<4; i++)
				StopTsConfig.MeterStop_Val += (uint32_t)(shmDataAppInfo.eqp_watt[i] << ((3-i)*8));
			memcpy(StopTsConfig.IdTag, shmDataAppInfo.card_no, sizeof(shmDataAppInfo.card_no));
			memset(&(StopTsConfig.IdTag[16]), '\0', 1);
		 	MakeDataCmd_StopTs();
			return true;
		}
		if(CsConfigVal.bReqChargeAmtFlg)
		{
			CsConfigVal.bReqChargeAmtFlg = false;
			MakeDataCmd_DataTrans_chargeAmt();
			return true;
		}

		if(CsConfigVal.bQREventFlg == 1){
			CsConfigVal.bQREventFlg = 0;
		 	MakeDataCmd_DataTrans_q1();
		 	return true;
		}

		if(CsConfigVal.bQREventFlg == 2){
		 	CsConfigVal.bQREventFlg = 0;
			MakeDataCmd_DataTrans_q2();
		 	return true;
		}
/*
		if((theConfig.chargingstatus & (1<<(MAX_CONECTOR_ID+1))) \
		 && (shmDataAppInfo.app_order == APP_ORDER_WAIT) ){
			MakeDataCmd_StopTs();

			return true;
		}

		if(CsConfigVal.bReqEmgBtnFlg){
			MakeDataCmd_DataTransCp(TSCT_ERR_CODE_EMG);
			return true;
		}
*/
		switch (CsConfigVal.triggerMsgStat)
		{
			case TRIG_MSG_STAT_BOOT:
				CsConfigVal.triggerMsgStat = TRIG_MSG_STAT_NONE;
			break;

			case TRIG_MSG_STAT_DIAG:
				CsConfigVal.triggerMsgStat = TRIG_MSG_STAT_NONE;
				MakeDataCmd_DiagStat(DIAG_STAT_NONE);
				return true;
			break;

			case TRIG_MSG_STAT_FW:
				CsConfigVal.triggerMsgStat = TRIG_MSG_STAT_NONE;
				MakeDataCmd_FwStat(UPDATE_STEP_NONE);
				return true;
			break;

			case TRIG_MSG_STAT_HB:
				CsConfigVal.triggerMsgStat = TRIG_MSG_STAT_NONE;
				MakeDataCmd_HB();
				return true;
			break;

			case TRIG_MSG_STAT_METER:
				CsConfigVal.triggerMsgStat = TRIG_MSG_STAT_NONE;
				Add_MeterVal_Q(METER_VAL_TRIG_TYPE);
				MakeDataCmd_MeterVal();
				return true;
			break;

			case TRIG_MSG_STAT_STAT:
				CsConfigVal.triggerMsgStat = TRIG_MSG_STAT_NONE;
				MakeDataCmd_Stat(1);
				return true;
			break;

			case TRIG_MSG_STAT_NONE:
			default:
				break;
		}
/*		
		if(SendVasData(&seccVasDataCnt, TSCT_ERR_CODE_SECCBATDATA))
			return true;
*/
		// Diag Status
		if(CsConfigVal.diagLogReqStep >= DIAG_STAT_UPLOADING)
		{
			MakeDataCmd_DiagStat(CsConfigVal.diagLogReqStep);
			CsConfigVal.diagLogReqStep = UPDATE_STEP_NONE;
			return true;
		}	

		// FW Update
		if(fwUpdateVals.updateStep == UPDATE_STEP_INSTAL)
		{
			MakeDataCmd_FwStat(UPDATE_STEP_INSTAL);
			fwUpdateVals.updateStep = UPDATE_STEP_NONE;
			return true;
		}

		if(fwUpdateVals.updateStep == UPDATE_STEP_INSTALED)
		{
			theConfig.chargingstatus = 3;
			// SetCpStatus(CP_STATUS_CODE_AVAIL, 0);
			// SetCpStatus(CP_STATUS_CODE_AVAIL, 1);
			MakeDataCmd_FwStat(UPDATE_STEP_INSTALED);
			fwUpdateVals.updateStep = UPDATE_STEP_NONE;
			return true;
		}

		if(fwUpdateVals.updateStep == UPDATE_STEP_INSTALFAIL)
		{
			theConfig.chargingstatus = 3;
			// SetCpStatus(CP_STATUS_CODE_AVAIL, 0);
			// SetCpStatus(CP_STATUS_CODE_AVAIL, 1);
			MakeDataCmd_FwStat(UPDATE_STEP_INSTALFAIL);
			fwUpdateVals.updateStep = UPDATE_STEP_NONE;
			return true;
		}
		
		if(fwUpdateVals.updateStep == UPDATE_STEP_DOWNLOAD)
		{
			MakeDataCmd_FwStat(UPDATE_STEP_DOWNLOAD);
			fwUpdateVals.updateStep = UPDATE_STEP_DOWNLOADING;
			return true;
		}

		if(fwUpdateVals.updateStep == UPDATE_STEP_DOWNLOADED)
		{
			MakeDataCmd_FwStat(UPDATE_STEP_DOWNLOADED);
			fwUpdateVals.updateStep = UPDATE_STEP_INSTAL;
			return true;
		}

		if(fwUpdateVals.updateStep == UPDATE_STEP_DOWNLOADFAIL)
		{
			theConfig.chargingstatus = 3;
			// SetCpStatus(CP_STATUS_CODE_AVAIL, 0);
			// SetCpStatus(CP_STATUS_CODE_AVAIL, 1);
			MakeDataCmd_FwStat(UPDATE_STEP_DOWNLOADFAIL);
			fwUpdateVals.updateStep = UPDATE_STEP_NONE;
			return true;
		}

		if(Check_HB_Time(&tv_1)){
			MakeDataCmd_HB();
			Reset_Time(&tv_1);
			return true;
		}
		if(OCPP_cpSts_flg == true)
		{
			OCPP_cpSts_flg = false;
			MakeDataCmd_DataTrans_cpSts();
			return true;
		}
		
		return false;
	}  
}

static void NetRun(void)
{
	bool ret;

	if(bConfigSaveFlg){
		ConfigSave();
		bConfigSaveFlg = false;
	}

    if(!bConnect){
//		pthread_mutex_init(&curlMutex, NULL);
        //usleep(200*1000);
		//printf("bConnect false \r\n");
		usleep(200*1000);
		WS_Server_connect();		
		//printf("re connect \r\n");
		//usleep(200*1000);
		usleep(1000*1000);
    }
    else{
		ret = OCPP_CALL_Senario();
		if(ret) {
			Tsct_Curl_Ws_Trans();
		}
    }
}

static void* WSClientThread(void* arg)
{
	bool iteEthGetLink_WS = false;			// Ethernet Link Status
	bool FirstConnectFlg = false;
	uint8_t meterValType;

    while(sWSClientTask > 0)
    {
		// printf("[WsClientThread] TID : %d\n", pthread_self());
        if (theConfig.ConfirmSelect != USER_AUTH_NET) {sleep(1); continue;}

		if(bGloAdminStatus) {sleep(1); continue;}

		if(!FirstConnectFlg){			// for First Connection
			if(TSCT_NetworkIsReady() && iteEthGetLink())	FirstConnectFlg = true;
			else {sleep(1); continue;}
		}	

		if(CsConfigVal.bReqStartTsNo)
		{
			CsConfigVal.bReqStartTsNo = 0;
			startTsQ.reqStartTsFlg = true;
			startTsQ.Connector_No = 1;
			startTsQ.faultChargFlg = false;
			GetDateTime(startTsQ.Time_Stamp);
			startTsQ.MeterStart_Val = 0;

			for(int i =0;i<4;i++)
				startTsQ.MeterStart_Val += (uint32_t)(shmDataAppInfo.eqp_watt[i] << ((3-i)*8));

			memcpy(startTsQ.IdTag, shmDataAppInfo.card_no, sizeof(shmDataAppInfo.card_no));
			memset(&(startTsQ.IdTag[16]), '\0', 1);

			//Reset_Time(&tv_2);
		}

		meterValType = Check_Sampled_Time();

		if(meterValType)
		{
			printf("[WSClientThread] meterValType = %d\n", meterValType);
			Add_MeterVal_Q(meterValType);
			//if(meterValType != 0) Reset_Time(&tv_2);
			StopTsConfig.Connector_No = 1;
			//bConfigSaveFlg = true;
		}

        if(iteEthGetLink()==0){	
			Clear_Rx_Msg_Buff();
			iteEthGetLink_WS = false;
			bConnect = false;
			/*
			//pthread_mutex_lock(&curlMutex);
			if(curl != NULL)
			{
				isBrokenSocket = true;
				printf("[WsClientThread]1 curl_easy_cleanup()");
				curl_easy_cleanup(curl);
				curl = NULL;
			}
			//pthread_mutex_unlock(&curlMutex);
			*/
			usleep(200*1000); 
			continue;
		}

		if(iteEthGetLink_WS == false){
			NetworkReset();
			iteEthGetLink_WS = true;
			/*
			//pthread_mutex_lock(&curlMutex);
			if(curl != NULL)
			{
				isBrokenSocket = true;
				printf("[WsClientThread]2 curl_easy_cleanup()");
				curl_easy_cleanup(curl);
				curl = NULL;
			}
			//pthread_mutex_unlock(&curlMutex);
			*/
			usleep(200*1000); 
			continue;
		}
		if(TSCT_NetworkIsReady())	
			NetRun();
		else
			bConnect = false;
		
		usleep(50*1000);
    }        
	sWSClientTask = 0;
}

// config file
void Init_CfgKey(void)
{
	for(int i=0; i<MAX_CFG; i++)
	{
		memcpy(CfgKeyVal[i].CfgKeyName, ocppConfigKeyName[i], sizeof(ocppConfigKeyName[i]));
	}
	// memcpy(CfgKeyVal[0].CfgKeyName, "AuthorizeRemoteTxRequests", sizeof("AuthorizeRemoteTxRequests"));
	CfgKeyVal[0].CfgKeyRw = false;
	CfgKeyVal[0].CfgKeyType = TYPE_CODE_BOOL;
	CfgKeyVal[0].CfgKeyDataInt = true;

	// memcpy(CfgKeyVal[1].CfgKeyName, "ClockAlignedDataInterval", sizeof("ClockAlignedDataInterval"));
	CfgKeyVal[1].CfgKeyRw = false;
	CfgKeyVal[1].CfgKeyType = TYPE_CODE_INT;
	CfgKeyVal[1].CfgKeyDataInt = 180;

	// memcpy(CfgKeyVal[2].CfgKeyName, "ConnectionTimeOut", sizeof("ConnectionTimeOut"));
	CfgKeyVal[2].CfgKeyRw = false;
	CfgKeyVal[2].CfgKeyType = TYPE_CODE_INT;
	CfgKeyVal[2].CfgKeyDataInt = 90;
	
	// memcpy(CfgKeyVal[3].CfgKeyName, "ConnectorPhaseRotation", sizeof("ConnectorPhaseRotation"));
	CfgKeyVal[3].CfgKeyRw = false;
	CfgKeyVal[3].CfgKeyType = TYPE_CODE_STR;
	sprintf(CfgKeyVal[3].CfgKeyDataCha, "NotApplicable");

	// memcpy(CfgKeyVal[4].CfgKeyName, "GetConfigurationMaxKeys", sizeof("GetConfigurationMaxKeys"));
	CfgKeyVal[4].CfgKeyRw = true;
	CfgKeyVal[4].CfgKeyType = TYPE_CODE_INT;
	CfgKeyVal[4].CfgKeyDataInt = MAX_CFG;

	// memcpy(CfgKeyVal[5].CfgKeyName, "HeartbeatInterval", sizeof("HeartbeatInterval"));
	CfgKeyVal[5].CfgKeyRw = false;
	CfgKeyVal[5].CfgKeyType = TYPE_CODE_INT;
	CfgKeyVal[5].CfgKeyDataInt = 10;

	// memcpy(CfgKeyVal[6].CfgKeyName, "LocalAuthorizeOffline", sizeof("LocalAuthorizeOffline"));
	CfgKeyVal[6].CfgKeyRw = false;
	CfgKeyVal[6].CfgKeyType = TYPE_CODE_BOOL;
	CfgKeyVal[6].CfgKeyDataInt = true;	

	// memcpy(CfgKeyVal[7].CfgKeyName, "LocalPreAuthorize", sizeof("LocalPreAuthorize"));
	CfgKeyVal[7].CfgKeyRw = false;
	CfgKeyVal[7].CfgKeyType = TYPE_CODE_BOOL;
	CfgKeyVal[7].CfgKeyDataInt = true;	

	// memcpy(CfgKeyVal[8].CfgKeyName, "MeterValuesAlignedData", sizeof("MeterValuesAlignedData"));
	CfgKeyVal[8].CfgKeyRw = false;
	CfgKeyVal[8].CfgKeyType = TYPE_CODE_STR;
	sprintf(CfgKeyVal[8].CfgKeyDataCha, "Energy.Active.Import.Register");

	// memcpy(CfgKeyVal[9].CfgKeyName, "MeterValuesSampledData", sizeof("MeterValuesSampledData"));
	CfgKeyVal[9].CfgKeyRw = false;
	CfgKeyVal[9].CfgKeyType = TYPE_CODE_STR;
	sprintf(CfgKeyVal[9].CfgKeyDataCha, "Energy.Active.Import.Register");

	// memcpy(CfgKeyVal[10].CfgKeyName, "MeterValueSampleInterval", sizeof("MeterValueSampleInterval"));
	CfgKeyVal[10].CfgKeyRw = false;
	CfgKeyVal[10].CfgKeyType = TYPE_CODE_INT;
	CfgKeyVal[10].CfgKeyDataInt = 60*3;

	// memcpy(CfgKeyVal[11].CfgKeyName, "NumberOfConnectors", sizeof("NumberOfConnectors"));
	CfgKeyVal[11].CfgKeyRw = true;
	CfgKeyVal[11].CfgKeyType = TYPE_CODE_INT;
	CfgKeyVal[11].CfgKeyDataInt = 1;	

	// memcpy(CfgKeyVal[12].CfgKeyName, "ResetRetries", sizeof("ResetRetries"));
	CfgKeyVal[12].CfgKeyRw = false;
	CfgKeyVal[12].CfgKeyType = TYPE_CODE_INT;
	CfgKeyVal[12].CfgKeyDataInt = 0;

	// memcpy(CfgKeyVal[13].CfgKeyName, "StopTransactionOnEVSideDisconnect", sizeof("StopTransactionOnEVSideDisconnect"));
	CfgKeyVal[13].CfgKeyRw = false;
	CfgKeyVal[13].CfgKeyType = TYPE_CODE_BOOL;
	CfgKeyVal[13].CfgKeyDataInt = true;

	// memcpy(CfgKeyVal[13].CfgKeyName, "StopTransactionOnInvalidId ", sizeof("StopTransactionOnInvalidId "));
	CfgKeyVal[14].CfgKeyRw = false;
	CfgKeyVal[14].CfgKeyType = TYPE_CODE_BOOL;
	CfgKeyVal[14].CfgKeyDataInt = false;

	// memcpy(CfgKeyVal[14].CfgKeyName, "StopTxnAlignedData", sizeof("StopTxnAlignedData"));
	CfgKeyVal[15].CfgKeyRw = false;
	CfgKeyVal[15].CfgKeyType = TYPE_CODE_STR;
	sprintf(CfgKeyVal[15].CfgKeyDataCha, "Energy.Active.Import.Register");

	// memcpy(CfgKeyVal[15].CfgKeyName, "StopTxnSampledData", sizeof("StopTxnSampledData"));
	CfgKeyVal[16].CfgKeyRw = false;
	CfgKeyVal[16].CfgKeyType = TYPE_CODE_STR;
	sprintf(CfgKeyVal[16].CfgKeyDataCha, "Energy.Active.Import.Register");

	// memcpy(CfgKeyVal[16].CfgKeyName, "SupportedFeatureProfiles", sizeof("SupportedFeatureProfiles"));
	CfgKeyVal[17].CfgKeyRw = true;
	CfgKeyVal[17].CfgKeyType = TYPE_CODE_STR;
	sprintf(CfgKeyVal[17].CfgKeyDataCha, "Core, FirmwareManagement, RemoteTrigger");

	// memcpy(CfgKeyVal[17].CfgKeyName, "TransactionMessageAttempts", sizeof("TransactionMessageAttempts"));
	CfgKeyVal[18].CfgKeyRw = false;
	CfgKeyVal[18].CfgKeyType = TYPE_CODE_INT;
	CfgKeyVal[18].CfgKeyDataInt = 5;

	// memcpy(CfgKeyVal[18].CfgKeyName, "TransactionMessageRetryInterval", sizeof("TransactionMessageRetryInterval"));
	CfgKeyVal[19].CfgKeyRw = false;
	CfgKeyVal[19].CfgKeyType = TYPE_CODE_INT;
	CfgKeyVal[19].CfgKeyDataInt = 10;		

	// memcpy(CfgKeyVal[19].CfgKeyName, "UnlockConnectorOnEVSideDisconnect", sizeof("UnlockConnectorOnEVSideDisconnect"));
	CfgKeyVal[20].CfgKeyRw = false;
	CfgKeyVal[20].CfgKeyType = TYPE_CODE_BOOL;
	CfgKeyVal[20].CfgKeyDataInt = true;		

	//Local Auth List Management

	// memcpy(CfgKeyVal[20].CfgKeyName, "LocalAuthListEnabled", sizeof("LocalAuthListEnabled"));
	// CfgKeyVal[21].CfgKeyRw = false;
	// CfgKeyVal[21].CfgKeyType = TYPE_CODE_BOOL;
	// CfgKeyVal[21].CfgKeyDataInt = false;		

	// memcpy(CfgKeyVal[21].CfgKeyName, "LocalAuthListMaxLength", sizeof("LocalAuthListMaxLength"));
	CfgKeyVal[21].CfgKeyRw = true;
	CfgKeyVal[21].CfgKeyType = TYPE_CODE_INT;
	CfgKeyVal[21].CfgKeyDataInt = 5;

	// memcpy(CfgKeyVal[22].CfgKeyName, "SendLocalListMaxLength", sizeof("SendLocalListMaxLength"));
	CfgKeyVal[22].CfgKeyRw = true;
	CfgKeyVal[22].CfgKeyType = TYPE_CODE_INT;
	CfgKeyVal[22].CfgKeyDataInt = 5;	

	// AllowOfflineTxForUnknownId	
	CfgKeyVal[23].CfgKeyRw = false;
	CfgKeyVal[23].CfgKeyType = TYPE_CODE_BOOL;
	CfgKeyVal[23].CfgKeyDataInt = true;

	// //Smart Charging Profile

	// memcpy(CfgKeyVal[23].CfgKeyName, "ChargeProfileMaxStackLevel", sizeof("ChargeProfileMaxStackLevel"));
	// CfgKeyVal[23].CfgKeyRw = true;
	// CfgKeyVal[23].CfgKeyType = TYPE_CODE_INT;
	// CfgKeyVal[23].CfgKeyDataInt = 0;	

	// memcpy(CfgKeyVal[24].CfgKeyName, "ChargingScheduleAllowedChargingRateUnit", sizeof("ChargingScheduleAllowedChargingRateUnit"));
	// CfgKeyVal[24].CfgKeyRw = true;
	// CfgKeyVal[24].CfgKeyType = TYPE_CODE_STR;
	// memset(CfgKeyVal[24].CfgKeyDataCha, '\0', 1);

	// memcpy(CfgKeyVal[25].CfgKeyName, "ChargingScheduleMaxPeriods", sizeof("ChargingScheduleMaxPeriods"));
	// CfgKeyVal[25].CfgKeyRw = true;
	// CfgKeyVal[25].CfgKeyType = TYPE_CODE_INT;
	// CfgKeyVal[25].CfgKeyDataInt = 0;		

	// memcpy(CfgKeyVal[26].CfgKeyName, "MaxChargingProfilesInstalled", sizeof("MaxChargingProfilesInstalled"));
	// CfgKeyVal[26].CfgKeyRw = true;
	// CfgKeyVal[26].CfgKeyType = TYPE_CODE_INT;
	// CfgKeyVal[26].CfgKeyDataInt = 0;		
}

void Init_Config_Val(void)
{
	for(int i=0;i<=MAX_CONECTOR_ID;i++){
		SetCpStatus(CP_STATUS_CODE_NONE,i);
		CsConfigVal.bPreCpStat[i] = CP_STATUS_CODE_NONE;
	}

	// CsConfigVal.StopTransactionOnEVSideDisconnect = true;
	// CsConfigVal.MinimumStatusDuration = 10;
	CsConfigVal.bReqAuthNo = false;
	CsConfigVal.bReqResetNo = 0;

	CsConfigVal.batInfo_cfgCnt = 20;

	cpStatChk = false;		// true : connect	/ false : disconn

	Reset_Time(&tv_1);
	Reset_Time(&tv_2);
	Reset_Time(&tv_curr);
	// CsConfigVal.Meter
}


void CreateTestVasData(void)
{
	seccVasDataCnt = 20;

	for(int i=0; i<20; i++) {

		seccVasRxData[i].timeStamp = 1722390648;

		seccVasRawData[i].data[0] = 0xA2;
		seccVasRawData[i].data[1] = 0x11;

		for(int j=0; j<17; j++)
			seccVasRawData[i].data[2+j] = 'A';
		
		seccVasRawData[i].data[19] = 0xA3;
		seccVasRawData[i].data[20] = 0x01;

		seccVasRawData[i].data[21] = 20;

		seccVasRawData[i].data[22] = 0xA4;
		seccVasRawData[i].data[23] = 0x01;

		seccVasRawData[i].data[24] = 20;

		seccVasRawData[i].data[25] = 0xA5;
		seccVasRawData[i].data[26] = 0x02;

		seccVasRawData[i].data[27] = 0x0f;
		seccVasRawData[i].data[28] = 0xA0;

		seccVasRawData[i].data[29] = 0xA6;
		seccVasRawData[i].data[30] = 0x02;

		seccVasRawData[i].data[31] = 0x1f;
		seccVasRawData[i].data[32] = 0x40;

		seccVasRawData[i].data[33] = 0xA7;
		seccVasRawData[i].data[34] = 0x00;
		seccVasRawData[i].data[35] = 0xC0;

		for(int j=0; j<192; j++)
			seccVasRawData[i].data[36+j] = 0xc8;

		seccVasRawData[i].data[228] = 0xA8;
		seccVasRawData[i].data[229] = 0x14;

		for(int j=0; j<20; j++)
			seccVasRawData[i].data[230+j] = 0xc8;

		seccVasRawData[i].dataLenth = 250;

		seccVasRawData[i].data[250] = NULL;
	}
}

void WsClientInit(void)
{	
	shmDataAppInfo.member_type = 0;	

	// CreateTestVasData();
	Init_Config_Val();
	Init_CfgKey();
	// curl_resp_timeout_set(CfgKeyVal[18].CfgKeyDataInt);
	// curl_ws_send_buf = malloc(64*1024 * sizeof(char));	
	// curl_ws_recv_buf = malloc(1024 * sizeof(char));		// GetConfig Max 700 ~ 800 byte

	//curl_global_init(CURL_GLOBAL_DEFAULT);

	//pthread_mutex_init(&curlMutex, NULL);
	ServerCallError = true;

	if (sWSRxTask == 0)
	{
		CtLogYellow(" create Client thread..\n");
		pthread_create(&sWSRxTask, NULL, WSRxThread, NULL);
		pthread_detach(sWSRxTask);
	}

	usleep(100);

	if (sWSClientTask == 0)
	{
		CtLogYellow(" create Client thread..\n");
		pthread_create(&sWSClientTask, NULL, WSClientThread, NULL);
		pthread_detach(sWSClientTask);
	}

	bClientStep = CLIENT_BOOT_STEP;
}

bool CheckClientInit()
{	
	return (sWSClientTask != 0);
}