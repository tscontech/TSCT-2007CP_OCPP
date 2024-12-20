/**
 * @file ocpp_cmd.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-05-04
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <sys/times.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "SDL/SDL.h"
#include <time.h>
#include <strings.h>    /* for bzero, strcasecmp, and strncasecmp */
#include "scene.h"
// #include "tsctclient.h"
#include "tsctcommon.h"
#include "tsctcfg.h"
#include "ctrlboard.h"
#include "ocpp_cmd.h"

#define A2_IDDR


// const char* AllConfigArr[] = {
// 	"AuthorizeRemoteTxRequests",
// 	"ClockAlignedDataInterval",
// 	"ConnectionTimeOut",

// }

// const char* Config_Arr[] = {
// 	"NONE",
// 	"MeterValueSampleInterval",
// };

typedef enum{
	RES_ACCEPT = 0,
	RES_PENDING,
	RES_REJECTED,
	RES_REBOOT,
	RES_NOTSUPPT,
	RES_SCHEDULED,
} RES_TYPE;

char batDataName [8][10] = {
	"timeStamp", "vin", "soc", "soh", "bpa", "bpv", "bsv", "bmt"
};

char batDataNameEnc [2][12] = {
	"timeStamp", "batteryData", "soc", "soh", "bpa", "bpv", "bsv", "bmt"
};


static uint64_t String2Integer(char* str)
{
	uint64_t len, ret=0;
	
	len = strlen(str);
	for(int i=0;i<len;i++){
		if(str[i] >= '0' && str[i] <= '9'){
			ret += (str[i] - '0') * (uint64_t)pow(10,len-1-i);
		}
	}
	return ret;
}

void MakeCallRes(RES_TYPE ret)
{
	Tx_Msg.Payload_len = 1;
	Tx_Msg.Msg_type = MSG_TYPE_CALLRES;
	memcpy(Tx_Msg.UniqueID_Char, Rx_Msg.UniqueID_Char,sizeof(Tx_Msg.UniqueID_Char));
	// Tx_Msg.UniqueID = Rx_Msg.UniqueID;
	// Tx_Msg.Action_Code = CS_REQ_ACTION_CODE_CHANGECONFIG;

    memset(Tx_Msg.Payload[0].property_name, 0x00, sizeof(Tx_Msg.Payload[0].property_name));
	memcpy(Tx_Msg.Payload[0].property_name, "status",sizeof("status"));

	memset(Tx_Msg.Payload[0].property_contants, 0x00, sizeof(Tx_Msg.Payload[0].property_contants));
	if(ret == RES_NOTSUPPT)
		memcpy(Tx_Msg.Payload[0].property_contants, OCPP_STATUS_NOTSUPPT, sizeof(OCPP_STATUS_NOTSUPPT));	
	else if(ret == RES_REJECTED)
		memcpy(Tx_Msg.Payload[0].property_contants, OCPP_STATUS_REJECTED, sizeof(OCPP_STATUS_REJECTED));	
	else if(ret == RES_SCHEDULED)
		memcpy(Tx_Msg.Payload[0].property_contants, "Scheduled", sizeof("Scheduled"));	
	else
		memcpy(Tx_Msg.Payload[0].property_contants, OCPP_STATUS_ACCEPT, sizeof(OCPP_STATUS_ACCEPT));
	Tx_Msg.Payload[0].data_type = TYPE_CODE_STR;
}

void GetDateTime(unsigned char* buf)
{
	char temp[50]= "";
	int index = 0, n;
	time_t time = CstGetTime();
	struct tm *tm = localtime(&time);
	sprintf(buf, "%d-%02d-%02dT%02d:%02d:%02d.000Z", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec );
}

long CnvrtStr2Utc(unsigned char* buf)
{
	long tmp_ret;
	struct tm tmp_time;
	// Time Syncro

	tmp_time.tm_year = (((buf[0] - '0') % 10) * 1000)	\
	+ (((buf[1] - '0') % 10) * 100)						\
	+ (((buf[2] - '0') % 10) * 10)						\
	+ (((buf[3] - '0') % 10) * 1) \
	- 1900;

	tmp_time.tm_mon = (((buf[5] - '0') % 10) * 10)	\
	+ (((buf[6] - '0') % 10) * 1);		

	tmp_time.tm_mday = (((buf[8] - '0') % 10) * 10)	\
	+ (((buf[9] - '0') % 10) * 1);

	tmp_time.tm_hour = (((buf[11] - '0') % 10) * 10)	\
	+ (((buf[12] - '0') % 10) * 1);						

	tmp_time.tm_min = (((buf[14] - '0') % 10) * 10)	\
	+ (((buf[15] - '0') % 10) * 1);	

	tmp_time.tm_sec = (((buf[17] - '0') % 10) * 10)	\
	+ (((buf[18] - '0') % 10) * 1);	

	tmp_ret = mktime((struct tm*)&tmp_time);

	return tmp_ret;
}

/* From CP Message */
void MakeDataCmd_Auth(void)
{
	char temp_buf[18];

	Tx_Msg.Msg_type = MSG_TYPE_CALL;
	Tx_Msg.UniqueID = CstGetTime_Msec_test();
    Tx_Msg.Payload_len = 1;
	Tx_Msg.Action_Code = CP_REQ_ACTION_CODE_AUTH;

	memset(Tx_Msg.Payload[0].property_name, 0x00, sizeof(Tx_Msg.Payload[0].property_name));
	memcpy(Tx_Msg.Payload[0].property_name, "idTag",sizeof("idTag"));

	if(CsConfigVal.bReqAuthNo == 2){
		memset(Tx_Msg.Payload[0].property_contants, 0x00, sizeof(Tx_Msg.Payload[0].property_contants));
		memcpy(Tx_Msg.Payload[0].property_contants, CsConfigVal.scnd_card_no, sizeof(CsConfigVal.scnd_card_no));
	}
	else{
		memset(Tx_Msg.Payload[0].property_contants, 0x00, sizeof(Tx_Msg.Payload[0].property_contants));
		memcpy(Tx_Msg.Payload[0].property_contants, shmDataAppInfo.card_no, sizeof(shmDataAppInfo.card_no));
		memset(&(Tx_Msg.Payload[0].property_contants[16]), '\0', 1);
	}
	Tx_Msg.Payload[0].data_type = TYPE_CODE_STR;
}

void DataProcCmd_Auth(void)
{
	if(!strcmp(Rx_Msg.Payload[0].property_name, "idTagInfo")){
		for(int i=0;i<=Rx_Msg.Payload[0].subPayload_len;i++){
			if(!strcmp(Rx_Msg.Payload[0].sub_Payload[i].property_name, "status")){	// Allow
				if(!strcmp(Rx_Msg.Payload[0].sub_Payload[i].property_contants, "Accepted"))
				{
					shmDataIfInfo.card_auth = CARD_AUTH_OK;
				//	printf("[DataProcCmd_Auth] Card Auth Pass\r\n");
				}
				else {
					shmDataAppInfo.app_order = APP_ORDER_AUTH_METHOD;
					shmDataIfInfo.card_auth = CARD_AUTH_FAILD;
				//	printf("[DataProcCmd_Auth] Card Auth Fail %s\r\n", Rx_Msg.Payload[0].sub_Payload[i].property_contants);
				}				
			}
			else if(!strcmp(Rx_Msg.Payload[0].sub_Payload[i].property_name, "parentIdTag")){
				memcpy(CsConfigVal.parentId, Rx_Msg.Payload[0].sub_Payload[i].property_contants, strlen(Rx_Msg.Payload[0].sub_Payload[i].property_contants)+1);
				printf("[DataProcCmd_Auth] PID : %s\r\n", CsConfigVal.parentId);
			}
			else{
				printf("[DataProcCmd_Auth] Finding status...\r\n");
			}
		}
	}
	else
	{
		printf("[DataProcCmd_Auth] Non Check OK idTagInfo\r\n");
	}	
	
	CsConfigVal.bReqAuthNo = false;	
}

void MakeDataCmd_Boot(void)
{
	Tx_Msg.Msg_type = MSG_TYPE_CALL;
	Tx_Msg.UniqueID = CstGetTime_Msec_test();
    Tx_Msg.Payload_len = 3;
	Tx_Msg.Action_Code = CP_REQ_ACTION_CODE_BOOT;

    memset(Tx_Msg.Payload[0].property_name, 0x00, sizeof(Tx_Msg.Payload[0].property_name));
	memcpy(Tx_Msg.Payload[0].property_name, "chargePointVendor",sizeof("chargePointVendor"));

	memset(Tx_Msg.Payload[0].property_contants, 0x00, sizeof(Tx_Msg.Payload[0].property_contants));
	memcpy(Tx_Msg.Payload[0].property_contants, "tscontech",sizeof("tscontech"));
	Tx_Msg.Payload[0].data_type = TYPE_CODE_STR;

    memset(Tx_Msg.Payload[1].property_name, 0x00, sizeof(Tx_Msg.Payload[1].property_name));
	memcpy(Tx_Msg.Payload[1].property_name, "chargePointModel",sizeof("chargePointModel"));

	memset(Tx_Msg.Payload[1].property_contants, 0x00, sizeof(Tx_Msg.Payload[1].property_contants));
	memcpy(Tx_Msg.Payload[1].property_contants, SW_MODEL,sizeof(SW_MODEL));
	Tx_Msg.Payload[1].data_type = TYPE_CODE_STR;

	memset(Tx_Msg.Payload[2].property_name, 0x00, sizeof(Tx_Msg.Payload[2].property_name));
	memcpy(Tx_Msg.Payload[2].property_name, "firmwareVersion",sizeof("firmwareVersion"));

	memset(Tx_Msg.Payload[2].property_contants, 0x00, sizeof(Tx_Msg.Payload[2].property_contants));
	memcpy(Tx_Msg.Payload[2].property_contants, SW_VERSION,sizeof(SW_VERSION));
	Tx_Msg.Payload[2].data_type = TYPE_CODE_STR;
}

void DataProcCmd_Boot(void)
{
	time_t cur_time = CstGetTime();
	struct tm *cur_tm = localtime(&cur_time);
	struct tm tmp_time;

	for(int i=0;i<=Rx_Msg.Payload_len ;i++)
	{
		if(!strcmp(Rx_Msg.Payload[i].property_name, "interval"))
		{
			CfgKeyVal[5].CfgKeyDataInt = String2Integer(Rx_Msg.Payload[i].property_contants);
			if(CfgKeyVal[5].CfgKeyDataInt==0)
				CfgKeyVal[5].CfgKeyDataInt = 10;	// Default 10sec
		}
		else if(!strcmp(Rx_Msg.Payload[i].property_name, "status"))
		{
			if(!strcmp(Rx_Msg.Payload[i].property_contants, OCPP_STATUS_ACCEPT)){
				// Need Time Setting
				if(GetCpStatus(0) <= CP_STATUS_CODE_PEND)
				{
					shmDataIfInfo.connect_status = true;
					for(int i=0; i<=MAX_CONECTOR_ID; i++)
					{
						if(theConfig.chargingstatus & (1<<i))
						{
							SetCpStatus(CP_STATUS_CODE_AVAIL, i);
							if((i>0) && cpStatChk)
								SetCpStatus(CP_STATUS_CODE_PREPARE, i);
						}
						else
							SetCpStatus(CP_STATUS_CODE_UNAVAIL, i);		

						if(i == MAX_CONECTOR_ID)
						{
							bChargerCostCheck = 1;
						}					
					}
					/*
					if(theConfig.chargingstatus & (1<<(MAX_CONECTOR_ID+1))){
						if(StopTsConfig.Connector_No == 0)
							SetCpStatus(CP_STATUS_CODE_FNISH, 1);
						else
							SetCpStatus(CP_STATUS_CODE_FNISH, 2);
					}
					*/
				}
			}
			// for Boot Notification Rejected, No Respond Any Message
			else if(!strcmp(Rx_Msg.Payload[i].property_contants, OCPP_STATUS_REJECTED)){
				for(int i=0;i<=MAX_CONECTOR_ID;i++){
					SetCpStatus(CP_STATUS_CODE_NONE, i);
					CsConfigVal.bPreCpStat[i] = CP_STATUS_CODE_NONE;
				}
			}
			else{
				for(int i=0;i<=MAX_CONECTOR_ID;i++){	// Pending
					SetCpStatus(CP_STATUS_CODE_PEND, i);
					CsConfigVal.bPreCpStat[i] = CP_STATUS_CODE_PEND;
				}
			}
		}
		else if(!strcmp(Rx_Msg.Payload[i].property_name, "currentTime"))
		{
			// Time Syncro
			tmp_time.tm_year = (((Rx_Msg.Payload[i].property_contants[0] - '0') % 10) * 1000)	\
			+ (((Rx_Msg.Payload[i].property_contants[1] - '0') % 10) * 100)						\
			+ (((Rx_Msg.Payload[i].property_contants[2] - '0') % 10) * 10)						\
			+ (((Rx_Msg.Payload[i].property_contants[3] - '0') % 10) * 1) \
			- 1900;

			tmp_time.tm_mon = (((Rx_Msg.Payload[i].property_contants[5] - '0') % 10) * 10)	\
			+ (((Rx_Msg.Payload[i].property_contants[6] - '0') % 10) * 1);	
			--(tmp_time.tm_mon);

			tmp_time.tm_mday = (((Rx_Msg.Payload[i].property_contants[8] - '0') % 10) * 10)	\
			+ (((Rx_Msg.Payload[i].property_contants[9] - '0') % 10) * 1);

			tmp_time.tm_hour = (((Rx_Msg.Payload[i].property_contants[11] - '0') % 10) * 10)	\
			+ (((Rx_Msg.Payload[i].property_contants[12] - '0') % 10) * 1);		

			tmp_time.tm_min = (((Rx_Msg.Payload[i].property_contants[14] - '0') % 10) * 10)	\
			+ (((Rx_Msg.Payload[i].property_contants[15] - '0') % 10) * 1);	

			tmp_time.tm_sec = (((Rx_Msg.Payload[i].property_contants[17] - '0') % 10) * 10)	\
			+ (((Rx_Msg.Payload[i].property_contants[18] - '0') % 10) * 1);	
			memcpy(&tNetTime, &tmp_time, sizeof(tNetTime));
			TsctCheckNetTimeSet();
			/*
			if(tmp_time.tm_min != cur_tm->tm_min){
				memcpy(&tNetTime, &tmp_time, sizeof(tNetTime));
				TsctCheckNetTimeSet();
			}
			*/
		}
	}
}

void TransUtc2Date(uint32_t utcVal, char* rstBuf) {
	uint8_t secBuf, minBuf, hourBuf, dayBuf, monthBuf, yearBuf;
	uint16_t remainDay = 0;

	uint32_t DAY2SEC = 24 * 60 * 60;
	uint16_t YM2DAY = 365;

	// utcVal += 9 * 60 * 60;

	secBuf = utcVal % 60;
	minBuf = (utcVal / 60) % 60;
	hourBuf = ((utcVal / 60) / 60) % 24;

	remainDay = utcVal / DAY2SEC;

	for(yearBuf = 0;;) {
		if((yearBuf % 4) == 2) 	YM2DAY = 366;
		else 					YM2DAY = 365;
		
		if(remainDay > YM2DAY){
			remainDay -= YM2DAY;
			yearBuf++;
		}
		else break;
	}

	for(monthBuf = 1;;) {
		if(monthBuf == 2) 	YM2DAY = 28;
		else if((monthBuf == 1) || (monthBuf == 3) || (monthBuf == 5) || (monthBuf == 7) || (monthBuf == 8) \
		|| (monthBuf == 10) || (monthBuf == 12))	YM2DAY = 31;
		else 										YM2DAY = 30;
		
		if(remainDay > YM2DAY){
			remainDay -= YM2DAY;
			monthBuf++;
		}
		else break;
	}
	dayBuf = remainDay;

	sprintf(rstBuf, "%d-%02d-%02dT%02d:%02d:%02d.000Z", yearBuf+1970, monthBuf, dayBuf, hourBuf, minBuf, secBuf);
}

void DataProcCmd_DataTransCp(void)
{
	char messageId[2];
	memset(messageId, 0x00, sizeof(messageId));
	char unitPriceData_tmp[8];
	memset(unitPriceData_tmp, 0x00, sizeof(unitPriceData_tmp));
	char unitPriceData[8];
	memset(unitPriceData, 0x00, sizeof(unitPriceData));
	
	if(strstr(Rx_Msg.Payload[0].DATATRANS_data, "messageId"))
	{
		int n = strstr(Rx_Msg.Payload[0].DATATRANS_data, "messageId") - Rx_Msg.Payload[0].DATATRANS_data;
		memcpy(messageId, &Rx_Msg.Payload[0].DATATRANS_data[n+14], 2);

		if(strcmp(messageId, "j1") == 0)
		{
			n = strstr(Rx_Msg.Payload[0].DATATRANS_data, "unitPrice") - Rx_Msg.Payload[0].DATATRANS_data;
			strncpy(unitPriceData_tmp, &Rx_Msg.Payload[0].DATATRANS_data[n+14], 8);
			n = strstr(unitPriceData_tmp, "\"") - unitPriceData_tmp;
			strncpy(unitPriceData, &unitPriceData_tmp[0], n -1);
			CtLogYellow("unitPriceData: %s", unitPriceData);
			shmDataIfInfo.OCPP_iUnitprice = atof(unitPriceData);
			printf("충전 단가 금액: %f\n", shmDataIfInfo.OCPP_iUnitprice);
		}
	}
	memset(Rx_Msg.Payload[0].DATATRANS_data, 0x00, sizeof(Rx_Msg.Payload[0].DATATRANS_data));
}


void MakeDataCmd_DiagStat(DIAG_STAT diagStat)
{
	Tx_Msg.Msg_type = MSG_TYPE_CALL;

	// printf("\r\nCheck Micro Second %lld / %lld\r\n", CstGetTime_Msec_test(), CstGetTime());

	Tx_Msg.UniqueID = CstGetTime_Msec_test();

    Tx_Msg.Payload_len = 1;

	Tx_Msg.Action_Code = CP_REQ_ACTION_CODE_DIAGSTAT;

    memset(Tx_Msg.Payload[0].property_name, 0x00, sizeof(Tx_Msg.Payload[0].property_name));
	memset(Tx_Msg.Payload[0].property_contants, 0x00, sizeof(Tx_Msg.Payload[0].property_contants));
	memcpy(Tx_Msg.Payload[0].property_name, "status",sizeof("status"));
	Tx_Msg.Payload[0].data_type = TYPE_CODE_STR;

	if(diagStat == DIAG_STAT_NONE)
		memcpy(Tx_Msg.Payload[0].property_contants, "Idle",sizeof("Idle"));

	else if(diagStat == DIAG_STAT_UPLOADING)
		memcpy(Tx_Msg.Payload[0].property_contants, "Uploading",sizeof("Uploading"));

	else if(diagStat == DIAG_STAT_UPLOADED)
		memcpy(Tx_Msg.Payload[0].property_contants, "Uploaded",sizeof("Uploaded"));		

	else if(diagStat == DIAG_STAT_UPLOADFAIL)
		memcpy(Tx_Msg.Payload[0].property_contants, "UploadFailed",sizeof("UploadFailed"));
}

// not Def
void DataProcCmd_DiagStat(void)
{

}

void MakeDataCmd_FwStat(UPDATE_STEP updateStep)
{
	Tx_Msg.Msg_type = MSG_TYPE_CALL;

	// printf("\r\nCheck Micro Second %lld / %lld\r\n", CstGetTime_Msec_test(), CstGetTime());

	Tx_Msg.UniqueID = CstGetTime_Msec_test();

    Tx_Msg.Payload_len = 1;

	Tx_Msg.Action_Code = CP_REQ_ACTION_CODE_FWSTAT;

    memset(Tx_Msg.Payload[0].property_name, 0x00, sizeof(Tx_Msg.Payload[0].property_name));
	memcpy(Tx_Msg.Payload[0].property_name, "status",sizeof("status"));

	memset(Tx_Msg.Payload[0].property_contants, 0x00, sizeof(Tx_Msg.Payload[0].property_contants));
	Tx_Msg.Payload[0].data_type = TYPE_CODE_STR;

	if(updateStep == UPDATE_STEP_NONE)
		memcpy(Tx_Msg.Payload[0].property_contants, "Idle",sizeof("Idle"));

	else if(updateStep == UPDATE_STEP_DOWNLOAD)
		memcpy(Tx_Msg.Payload[0].property_contants, "Downloading",sizeof("Downloading"));

	else if(updateStep == UPDATE_STEP_DOWNLOADED)
		memcpy(Tx_Msg.Payload[0].property_contants, "Downloaded",sizeof("Downloaded"));
		
	else if(updateStep == UPDATE_STEP_DOWNLOADFAIL)
		memcpy(Tx_Msg.Payload[0].property_contants, "DownloadFailed",sizeof("DownloadFailed"));

	else if(updateStep == UPDATE_STEP_INSTAL)
		memcpy(Tx_Msg.Payload[0].property_contants, "Installing",sizeof("Installing"));

	else if(updateStep == UPDATE_STEP_INSTALED)
		memcpy(Tx_Msg.Payload[0].property_contants, "Installed",sizeof("Installed"));

	else if(updateStep == UPDATE_STEP_INSTALFAIL)
		memcpy(Tx_Msg.Payload[0].property_contants, "InstallationFailed",sizeof("InstallationFailed"));
}

// not Def
void DataProcCmd_FwStat(void){}


void MakeDataCmd_HB(void)
{
	Tx_Msg.Msg_type = MSG_TYPE_CALL;

	Tx_Msg.UniqueID = CstGetTime_Msec_test();
    Tx_Msg.Payload_len = 0;
	Tx_Msg.Action_Code = CP_REQ_ACTION_CODE_HB;
	Tx_Msg.Payload[0].data_type = TYPE_CODE_STR;
}

void DataProcCmd_HB(void)
{
	time_t cur_time = CstGetTime();
	struct tm *cur_tm = localtime(&cur_time);
	struct tm tmp_time;
	

	if(!strcmp(Rx_Msg.Payload[0].property_name, "currentTime")){
		// Time Syncro
		tmp_time.tm_year = (((Rx_Msg.Payload[0].property_contants[0] - '0') % 10) * 1000)	\
		+ (((Rx_Msg.Payload[0].property_contants[1] - '0') % 10) * 100)						\
		+ (((Rx_Msg.Payload[0].property_contants[2] - '0') % 10) * 10)						\
		+ (((Rx_Msg.Payload[0].property_contants[3] - '0') % 10) * 1) \
		- 1900;

		tmp_time.tm_mon = (((Rx_Msg.Payload[0].property_contants[5] - '0') % 10) * 10)	\
		+ (((Rx_Msg.Payload[0].property_contants[6] - '0') % 10) * 1);		

		tmp_time.tm_mday = (((Rx_Msg.Payload[0].property_contants[8] - '0') % 10) * 10)	\
		+ (((Rx_Msg.Payload[0].property_contants[9] - '0') % 10) * 1);
		tmp_time.tm_mday = tmp_time.tm_mday -1;

		tmp_time.tm_hour = (((Rx_Msg.Payload[0].property_contants[11] - '0') % 10) * 10)	\
		+ (((Rx_Msg.Payload[0].property_contants[12] - '0') % 10) * 1);						

		tmp_time.tm_min = (((Rx_Msg.Payload[0].property_contants[14] - '0') % 10) * 10)	\
		+ (((Rx_Msg.Payload[0].property_contants[15] - '0') % 10) * 1);	

		tmp_time.tm_sec = (((Rx_Msg.Payload[0].property_contants[17] - '0') % 10) * 10)	\
		+ (((Rx_Msg.Payload[0].property_contants[18] - '0') % 10) * 1);	

		if(tmp_time.tm_min != cur_tm->tm_min){
			memcpy(&tNetTime, &tmp_time, sizeof(tNetTime));
			TsctCheckNetTimeSet();
		}
	}
}

void MakeDataCmd_MeterVal(void)
{
	char temp_buf[50];
	// uint32_t temp_buf32 = 0;

	Tx_Msg.Msg_type = MSG_TYPE_CALL;

	// Tx_Msg.UniqueID = CstGetTime_Msec_test();
	Tx_Msg.UniqueID = MeterValQ[0].Uid_Mv;

    Tx_Msg.Payload_len = 3;

	Tx_Msg.Action_Code = CP_REQ_ACTION_CODE_METERVAL;

    memset(Tx_Msg.Payload[0].property_name, 0x00, sizeof(Tx_Msg.Payload[0].property_name));
	memcpy(Tx_Msg.Payload[0].property_name, "connectorId",sizeof("connectorId"));

	memset(Tx_Msg.Payload[0].property_contants, 0x00, sizeof(Tx_Msg.Payload[0].property_contants));
	sprintf(Tx_Msg.Payload[0].property_contants,"%d",MeterValQ[0].Connect_Id);
	Tx_Msg.Payload[0].data_type = TYPE_CODE_INT;

	memset(Tx_Msg.Payload[1].property_name, 0x00, sizeof(Tx_Msg.Payload[1].property_name));
	memcpy(Tx_Msg.Payload[1].property_name, "transactionId",sizeof("transactionId"));

	memset(Tx_Msg.Payload[1].property_contants, 0x00, sizeof(Tx_Msg.Payload[1].property_contants));
	sprintf(temp_buf,"%llu",CsConfigVal.bTrId[bDevChannel+1]);

	memcpy(Tx_Msg.Payload[1].property_contants, temp_buf,sizeof(temp_buf));
	Tx_Msg.Payload[1].data_type = TYPE_CODE_INT;

    memset(Tx_Msg.Payload[2].property_name, 0x00, sizeof(Tx_Msg.Payload[2].property_name));
	memcpy(Tx_Msg.Payload[2].property_name, "meterValue",sizeof("meterValue"));

	Tx_Msg.Payload[2].data_type = TYPE_CODE_ARR;

	Tx_Msg.Payload[2].sub_Payload = tx_sub_Payload;
	Tx_Msg.Payload[2].subPayload_len = 1;

	//sprintf((Tx_Msg.Payload[1].sub_Payload)->property_contants, "{\"timestamp\":\"%s\",\"sampledValue\":[{\"value\":\"%lu\"}]}", MeterValQ[0].Time_Stamp, MeterValQ[0].Sampled_Val);
	//printf("[MakeDataCmd_MeterVal] %s \r\n", (Tx_Msg.Payload[1].sub_Payload)->property_contants);
	
	if(MeterValQ[0].Meterval_Flg == METER_VAL_CLOCK_TYPE)
		sprintf((Tx_Msg.Payload[2].sub_Payload)->property_contants, "{\"timestamp\":\"%s\",\"sampledValue\":[{\"value\":\"%lu\",\"measurand\":\"Energy.Active.Import.Register\",\"context\":\"Sample.Clock\",\"unit\":\"Wh\"},{\"value\":\"%d\",\"measurand\":\"Current.Export\",\"context\":\"Sample.Clock\",\"unit\":\"A\"},{\"value\":\"%d\",\"measurand\":\"Voltage\",\"context\":\"Sample.Clock\",\"unit\":\"V\"}]}", MeterValQ[0].Time_Stamp, (MeterValQ[0].Sampled_Val * 10), MeterValQ[0].Current/100, MeterValQ[0].Volt/10);
	else if(MeterValQ[0].Meterval_Flg == METER_VAL_SAMP_TYPE)
	{
		sprintf((Tx_Msg.Payload[2].sub_Payload)->property_contants, "{\"timestamp\":\"%s\",\"sampledValue\":[{\"value\":\"%lu\",\"measurand\":\"Energy.Active.Import.Register\",\"context\":\"Sample.Periodic\",\"unit\":\"Wh\"},{\"value\":\"%d\",\"measurand\":\"Current.Export\",\"context\":\"Sample.Periodic\",\"unit\":\"A\"},{\"value\":\"%d\",\"measurand\":\"Voltage\",\"context\":\"Sample.Periodic\",\"unit\":\"V\"}]}", MeterValQ[0].Time_Stamp, (MeterValQ[0].Sampled_Val * 10), MeterValQ[0].Current/100, MeterValQ[0].Volt/10);
	}
	else if(MeterValQ[0].Meterval_Flg == METER_VAL_TRIG_TYPE)
		sprintf((Tx_Msg.Payload[2].sub_Payload)->property_contants, "{\"timestamp\":\"%s\",\"sampledValue\":[{\"value\":\"%lu\",\"measurand\":\"Energy.Active.Import.Register\",\"context\":\"Trigger\",\"unit\":\"Wh\"},{\"value\":\"%d\",\"measurand\":\"Current.Export\",\"context\":\"Trigger\",\"unit\":\"A\"},{\"value\":\"%d\",\"measurand\":\"Voltage\",\"context\":\"Trigger\",\"unit\":\"V\"}]}", MeterValQ[0].Time_Stamp, (MeterValQ[0].Sampled_Val * 10), MeterValQ[0].Current/100, MeterValQ[0].Volt/10);
}

void DataProcCmd_MeterVal(void)
{
	if(Rx_Msg.UniqueID == MeterValQ[0].Uid_Mv)
		Reset_MeterVal_Q();
}

void MakeDataCmd_StartTs(void)
{
	char temp_buf[50];
	uint32_t temp_buf32 = 0;

	Tx_Msg.Msg_type = MSG_TYPE_CALL;
	Tx_Msg.UniqueID = CstGetTime_Msec_test();
    Tx_Msg.Payload_len = 4;
	Tx_Msg.Action_Code = CP_REQ_ACTION_CODE_STARTTS;

	memset(Tx_Msg.Payload[0].property_name, 0x00, sizeof(Tx_Msg.Payload[0].property_name));
	memcpy(Tx_Msg.Payload[0].property_name, "connectorId",sizeof("connectorId"));

	memset(Tx_Msg.Payload[0].property_contants, 0x00, sizeof(Tx_Msg.Payload[0].property_contants));
	sprintf(temp_buf, "%d", startTsQ.Connector_No);
	memcpy(Tx_Msg.Payload[0].property_contants, temp_buf,sizeof(temp_buf));
	Tx_Msg.Payload[0].data_type = TYPE_CODE_INT;

    memset(Tx_Msg.Payload[1].property_name, 0x00, sizeof(Tx_Msg.Payload[1].property_name));
	memcpy(Tx_Msg.Payload[1].property_name, "idTag",sizeof("idTag"));

	memset(Tx_Msg.Payload[1].property_contants, 0x00, sizeof(Tx_Msg.Payload[1].property_contants));
	memcpy(Tx_Msg.Payload[1].property_contants, startTsQ.IdTag, sizeof(startTsQ.IdTag));
	memset(&(Tx_Msg.Payload[1].property_contants[16]), '\0', 1);
	Tx_Msg.Payload[1].data_type = TYPE_CODE_STR;	

	// for(int i=0;i<4;i++){
	// 	temp_buf32 += (uint32_t)(shmDataAppInfo.eqp_watt[i] << ((3-i)*8));
	// }
	sprintf(temp_buf,"%lu", startTsQ.MeterStart_Val* 10);

	memset(Tx_Msg.Payload[2].property_name, 0x00, sizeof(Tx_Msg.Payload[2].property_name));
	memcpy(Tx_Msg.Payload[2].property_name, "meterStart",sizeof("meterStart"));

	memset(Tx_Msg.Payload[2].property_contants, 0x00, sizeof(Tx_Msg.Payload[2].property_contants));
	memcpy(Tx_Msg.Payload[2].property_contants, temp_buf,sizeof(temp_buf));
	Tx_Msg.Payload[2].data_type = TYPE_CODE_INT;

    memset(Tx_Msg.Payload[3].property_name, 0x00, sizeof(Tx_Msg.Payload[3].property_name));
	memcpy(Tx_Msg.Payload[3].property_name, "timestamp",sizeof("timestamp"));

	//GetDateTime(startTsQ.Time_Stamp);

	memset(Tx_Msg.Payload[3].property_contants, 0x00, sizeof(Tx_Msg.Payload[3].property_contants));
	memcpy(Tx_Msg.Payload[3].property_contants, startTsQ.Time_Stamp,sizeof(startTsQ.Time_Stamp));
	Tx_Msg.Payload[3].data_type = TYPE_CODE_STR;		
}

void DataProcCmd_StartTs(void)
{
	startTsQ.reqStartTsFlg = false;
	for(int i=0;i<=Rx_Msg.Payload_len ;i++){
		if(!strcmp(Rx_Msg.Payload[i].property_name, "idTagInfo")){
			for(int j=0;j<=Rx_Msg.Payload[i].subPayload_len;j++){
				if(!strcmp(Rx_Msg.Payload[i].sub_Payload[j].property_name, "status")){
					if(!strcmp(Rx_Msg.Payload[i].sub_Payload[j].property_contants, "Accepted")){
						// SetCpStatus(CP_STATUS_CODE_CHARGING, bDevChannel+1);					
						// CsConfigVal.bReqStartTsNo += MAX_CONECTOR_ID;
						startTsQ.faultChargFlg = false;
					}
					else
					{
						// CsConfigVal.bCpStat[bDevChannel+1] = CP_STATUS_CODE_AVAIL;
						// CsConfigVal.bReqStartTsNo += (MAX_CONECTOR_ID*2);
						startTsQ.faultChargFlg = true;
					}
				}
				else if(!strcmp(Rx_Msg.Payload[i].sub_Payload[j].property_name, "parentIdTag")){
					// memcpy(CsConfigVal.parentId, Rx_Msg.Payload[i].sub_Payload[j].property_contants, sizeof(CsConfigVal.parentId));
				}
			}
		}
		else if(!strcmp(Rx_Msg.Payload[i].property_name, "transactionId")){
			int len = strlen(Rx_Msg.Payload[i].property_contants);
			CsConfigVal.bTrId[bDevChannel+1] = strtoull(Rx_Msg.Payload[i].property_contants, NULL, len);
		}
	}
}

void MakeDataCmd_Stat(uint8_t cntrNo)
{
	char temp_buf[50];
	Tx_Msg.Msg_type = MSG_TYPE_CALL;
	Tx_Msg.UniqueID = CstGetTime_Msec_test();	
	Tx_Msg.Payload_len = 5;
	//Tx_Msg.Payload_len = 3;
	Tx_Msg.Action_Code = CP_REQ_ACTION_CODE_STAT;

    memset(Tx_Msg.Payload[0].property_name, 0x00, sizeof(Tx_Msg.Payload[0].property_name));
	memcpy(Tx_Msg.Payload[0].property_name, "connectorId",sizeof("connectorId"));

	memset(Tx_Msg.Payload[1].property_name, 0x00, sizeof(Tx_Msg.Payload[1].property_name));
	memcpy(Tx_Msg.Payload[1].property_name, "timestamp",sizeof("timestamp"));

	memset(Tx_Msg.Payload[2].property_name, 0x00, sizeof(Tx_Msg.Payload[2].property_name));
	memcpy(Tx_Msg.Payload[2].property_name, "status",sizeof("status"));

	memset(Tx_Msg.Payload[3].property_name, 0x00, sizeof(Tx_Msg.Payload[3].property_name));
	memcpy(Tx_Msg.Payload[3].property_name, "errorCode",sizeof("errorCode"));	

	memset(Tx_Msg.Payload[4].property_name, 0x00, sizeof(Tx_Msg.Payload[4].property_name));
	memcpy(Tx_Msg.Payload[4].property_name, "vendorErrorCode",sizeof("vendorErrorCode"));	

	
	memset(Tx_Msg.Payload[0].property_contants, 0x00, sizeof(Tx_Msg.Payload[0].property_contants));
	sprintf(Tx_Msg.Payload[0].property_contants, "%d", cntrNo);
	Tx_Msg.Payload[0].data_type = TYPE_CODE_INT;
	
	GetDateTime(temp_buf);
	memset(Tx_Msg.Payload[1].property_contants, 0x00, sizeof(Tx_Msg.Payload[1].property_contants));
	memcpy(Tx_Msg.Payload[1].property_contants, temp_buf,sizeof(temp_buf));
	Tx_Msg.Payload[1].data_type = TYPE_CODE_STR;	


	memset(Tx_Msg.Payload[3].property_contants, 0x00, sizeof(Tx_Msg.Payload[3].property_contants));
	memcpy(Tx_Msg.Payload[3].property_contants, "NoError",sizeof("NoError"));
	Tx_Msg.Payload[3].data_type = TYPE_CODE_STR;

	memset(Tx_Msg.Payload[4].property_contants, 0x00, sizeof(Tx_Msg.Payload[4].property_contants));
	memcpy(Tx_Msg.Payload[4].property_contants, "",sizeof(""));
	Tx_Msg.Payload[4].data_type = TYPE_CODE_STR;


	switch (GetCpStatus(cntrNo))
	{
	case CP_STATUS_CODE_AVAIL:
		memset(Tx_Msg.Payload[2].property_contants, 0x00, sizeof(Tx_Msg.Payload[2].property_contants));
		memcpy(Tx_Msg.Payload[2].property_contants, "Available",sizeof("Available"));
		Tx_Msg.Payload[2].data_type = TYPE_CODE_STR;
		break;

	case CP_STATUS_CODE_PREPARE:
		memset(Tx_Msg.Payload[2].property_contants, 0x00, sizeof(Tx_Msg.Payload[2].property_contants));
		memcpy(Tx_Msg.Payload[2].property_contants, "Preparing",sizeof("Preparing"));
		Tx_Msg.Payload[2].data_type = TYPE_CODE_STR;
		break;

	case CP_STATUS_CODE_CHARGING:
		memset(Tx_Msg.Payload[2].property_contants, 0x00, sizeof(Tx_Msg.Payload[2].property_contants));
		memcpy(Tx_Msg.Payload[2].property_contants, "Charging",sizeof("Charging"));
		Tx_Msg.Payload[2].data_type = TYPE_CODE_STR;
		break;

	case CP_STATUS_CODE_SUSPENDEDEV:
		memset(Tx_Msg.Payload[2].property_contants, 0x00, sizeof(Tx_Msg.Payload[2].property_contants));
		memcpy(Tx_Msg.Payload[2].property_contants, "SuspendedEV",sizeof("SuspendedEV"));
		Tx_Msg.Payload[2].data_type = TYPE_CODE_STR;
		Tx_Msg.Payload_len++;

		memset(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name, 0x00, sizeof(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name));
		memcpy(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name, "info",sizeof("info"));

		memset(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants, 0x00, sizeof(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants));
		memcpy(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants, "EV side disconnected",sizeof("EV side disconnected"));
		Tx_Msg.Payload[Tx_Msg.Payload_len].data_type = TYPE_CODE_STR;

		break;

	case CP_STATUS_CODE_SUSPENDEDEVSE:
		memset(Tx_Msg.Payload[2].property_contants, 0x00, sizeof(Tx_Msg.Payload[2].property_contants));
		memcpy(Tx_Msg.Payload[2].property_contants, "SuspendedEVSE",sizeof("SuspendedEVSE"));
		Tx_Msg.Payload[2].data_type = TYPE_CODE_STR;
		break;		

	case CP_STATUS_CODE_FNISH:
		memset(Tx_Msg.Payload[2].property_contants, 0x00, sizeof(Tx_Msg.Payload[2].property_contants));
		memcpy(Tx_Msg.Payload[2].property_contants, "Finishing",sizeof("Finishing"));
		Tx_Msg.Payload[2].data_type = TYPE_CODE_STR;
		break;		

	case CP_STATUS_CODE_RESERVED:
		memset(Tx_Msg.Payload[2].property_contants, 0x00, sizeof(Tx_Msg.Payload[2].property_contants));
		memcpy(Tx_Msg.Payload[2].property_contants, "Reserved",sizeof("Reserved"));
		Tx_Msg.Payload[2].data_type = TYPE_CODE_STR;
		break;		

	case CP_STATUS_CODE_UNAVAIL:
		memset(Tx_Msg.Payload[2].property_contants, 0x00, sizeof(Tx_Msg.Payload[2].property_contants));
		memcpy(Tx_Msg.Payload[2].property_contants, "Unavailable",sizeof("Unavailable"));
		Tx_Msg.Payload[2].data_type = TYPE_CODE_STR;
		break;		

	case CP_STATUS_CODE_FAULT:
		memset(Tx_Msg.Payload[2].property_contants, 0x00, sizeof(Tx_Msg.Payload[2].property_contants));
		memcpy(Tx_Msg.Payload[2].property_contants, "Faulted",sizeof("Faulted"));
		Tx_Msg.Payload[2].data_type = TYPE_CODE_STR;

		memset(Tx_Msg.Payload[3].property_contants, 0x00, sizeof(Tx_Msg.Payload[3].property_contants));
		memcpy(Tx_Msg.Payload[3].property_contants, "Error",sizeof("Error"));
		Tx_Msg.Payload[3].data_type = TYPE_CODE_STR;

		memset(Tx_Msg.Payload[4].property_contants, 0x00, sizeof(Tx_Msg.Payload[4].property_contants));
		memcpy(Tx_Msg.Payload[4].property_contants, "",sizeof(""));
		Tx_Msg.Payload[4].data_type = TYPE_CODE_STR;
		
		break;						

	default:
		printf("Undefined CP Status Send %d\r\n",GetCpStatus(cntrNo));
		break;
	}
}

void DataProcCmd_Stat(void)
{
	uint8_t tmp_int;

	tmp_int = String2Integer(Call_Tx_Msg.Payload[0].property_contants);
	
	CsConfigVal.bPreCpStat[tmp_int] = GetCpStatus(tmp_int);
	if(bChargerCostCheck == 1)
		bChargerCostCheck =2;
}

void MakeDataCmd_StopTs(void)
{
	char temp_buf[50];
	uint32_t temp_buf32 = 0;
	Tx_Msg.Msg_type = MSG_TYPE_CALL;
	Tx_Msg.UniqueID = CstGetTime_Msec_test();	
    Tx_Msg.Payload_len = 3;
	Tx_Msg.Action_Code = CP_REQ_ACTION_CODE_STOPTS;

    memset(Tx_Msg.Payload[0].property_name, 0x00, sizeof(Tx_Msg.Payload[0].property_name));
	memcpy(Tx_Msg.Payload[0].property_name, "transactionId",sizeof("transactionId"));

	memset(Tx_Msg.Payload[0].property_contants, 0x00, sizeof(Tx_Msg.Payload[0].property_contants));
	sprintf(temp_buf,"%llu",CsConfigVal.bTrId[bDevChannel+1]);

	memcpy(Tx_Msg.Payload[0].property_contants, temp_buf,sizeof(temp_buf));
	Tx_Msg.Payload[0].data_type = TYPE_CODE_INT;

	memset(Tx_Msg.Payload[1].property_name, 0x00, sizeof(Tx_Msg.Payload[1].property_name));
	memcpy(Tx_Msg.Payload[1].property_name, "timestamp",sizeof("timestamp"));

	memset(Tx_Msg.Payload[1].property_contants, 0x00, sizeof(Tx_Msg.Payload[1].property_contants));
	memcpy(Tx_Msg.Payload[1].property_contants, StopTsConfig.Time_Stamp,sizeof(StopTsConfig.Time_Stamp));
	Tx_Msg.Payload[1].data_type = TYPE_CODE_STR;

	memset(Tx_Msg.Payload[2].property_name, 0x00, sizeof(Tx_Msg.Payload[2].property_name));
	memcpy(Tx_Msg.Payload[2].property_name, "meterStop",sizeof("meterStop"));

	sprintf(temp_buf,"%lu",StopTsConfig.MeterStop_Val * 10);

	memset(Tx_Msg.Payload[2].property_contants, 0x00, sizeof(Tx_Msg.Payload[2].property_contants));
	memcpy(Tx_Msg.Payload[2].property_contants, temp_buf,sizeof(temp_buf));
	Tx_Msg.Payload[2].data_type = TYPE_CODE_INT;

	if(CsConfigVal.bReqRmtStopTSFlg){
		memset(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name, 0x00, sizeof(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name));
		memcpy(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name, "reason",sizeof("reason"));

		memset(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants, 0x00, sizeof(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants));
		memcpy(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants, "Remote",sizeof("Remote"));

		Tx_Msg.Payload[Tx_Msg.Payload_len].data_type = TYPE_CODE_STR;
		Tx_Msg.Payload_len++;
	}
	else if(shmDataAppInfo.charge_comp_status == END_UNPLUG){
		memset(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name, 0x00, sizeof(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name));
		memcpy(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name, "reason",sizeof("reason"));

		memset(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants, 0x00, sizeof(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants));
		memcpy(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants, "EVDisconnected",sizeof("EVDisconnected"));

		Tx_Msg.Payload[Tx_Msg.Payload_len].data_type = TYPE_CODE_STR;
		Tx_Msg.Payload_len++;
	}
	else if(shmDataAppInfo.charge_comp_status == END_SERVER){
		memset(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name, 0x00, sizeof(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name));
		memcpy(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name, "reason",sizeof("reason"));

		memset(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants, 0x00, sizeof(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants));
		if(CsConfigVal.bReqResetNo == 1)
			memcpy(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants, "SoftReset",sizeof("SoftReset"));
		else
			memcpy(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants, "HardReset",sizeof("HardReset"));

		Tx_Msg.Payload[Tx_Msg.Payload_len].data_type = TYPE_CODE_STR;
		Tx_Msg.Payload_len++;
	}
	else if(shmDataAppInfo.charge_comp_status == END_BTN || shmDataAppInfo.charge_comp_status == END_CARD){
		memset(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name, 0x00, sizeof(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name));
		memcpy(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name, "reason",sizeof("reason"));

		memset(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants, 0x00, sizeof(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants));
		memcpy(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants, "Local",sizeof("Local"));

		Tx_Msg.Payload[Tx_Msg.Payload_len].data_type = TYPE_CODE_STR;
		Tx_Msg.Payload_len++;
	}
	else if(StopTsConfig.Stop_Reason == STOP_REASON_POWER){
		memset(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name, 0x00, sizeof(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name));
		memcpy(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name, "reason",sizeof("reason"));

		memset(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants, 0x00, sizeof(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants));
		memcpy(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants, "PowerLoss",sizeof("PowerLoss"));

		Tx_Msg.Payload[Tx_Msg.Payload_len].data_type = TYPE_CODE_STR;
		Tx_Msg.Payload_len++;
	}

	memset(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name, 0x00, sizeof(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name));
	memcpy(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name, "idTag",sizeof("idTag"));

	memset(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants, 0x00, sizeof(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants));
	memcpy(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants, StopTsConfig.IdTag, sizeof(StopTsConfig.IdTag));
	// memset(&(Tx_Msg.Payload[Tx_Msg.Payload_len].property_contants[16]), '\0', 1);
	
	Tx_Msg.Payload[Tx_Msg.Payload_len].data_type = TYPE_CODE_STR;
	Tx_Msg.Payload_len++;

	//////
	/*
	memset(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name, 0x00, sizeof(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name));
	memcpy(Tx_Msg.Payload[Tx_Msg.Payload_len].property_name, "transactionData",sizeof("transactionData"));

	Tx_Msg.Payload[Tx_Msg.Payload_len].data_type = TYPE_CODE_ARR;

	Tx_Msg.Payload[Tx_Msg.Payload_len].sub_Payload = tx_sub_Payload;
	Tx_Msg.Payload[Tx_Msg.Payload_len].subPayload_len = 1;
	sprintf((Tx_Msg.Payload[Tx_Msg.Payload_len].sub_Payload)->property_contants, 
	"{\"timestamp\":\"%s\",\"sampledValue\":[{\"value\":\"%lu\",\"measurand\":\"Energy.Active.Import.Register\",\"context\":\"Transaction.End\",\"unit\":\"Wh\"},{\"value\":\"%d\",\"measurand\":\"Current.Export\",\"context\":\"Transaction.End\",\"unit\":\"A\"},{\"value\":\"%d\",\"measurand\":\"Voltage\",\"context\":\"Transaction.End\",\"unit\":\"V\"}]}", StopTsConfig.Time_Stamp, (MeterValQ[0].Sampled_Val * 10), MeterValQ[0].Current/100, MeterValQ[0].Volt/10);
	Tx_Msg.Payload_len++;
*/
	CsConfigVal.bReqRmtStopTSFlg = false;
}

void DataProcCmd_StopTs(void)
{
	if((theConfig.chargingstatus & (1<<(MAX_CONECTOR_ID+1))) \
	&& (StopTsConfig.Stop_Reason == STOP_REASON_POWER))
		SetCpStatus(CP_STATUS_CODE_AVAIL,bDevChannel+1);

	else if(GetCpStatus(bDevChannel+1) == CP_STATUS_CODE_AVAIL);
	
	else
		SetCpStatus(CP_STATUS_CODE_FNISH,bDevChannel+1);
	CsConfigVal.bReqStopTsFlg = false;
	theConfig.chargingstatus &= ~(1<<(MAX_CONECTOR_ID+1));
	
	shmDataAppInfo.charge_comp_status = END_NONE;

	bConfigSaveFlg = true;
	// ConfigSave();
}



/** From CS Message **/
// ND
void DataProcCmd_CancelReserve(void){

}

void DataProcCmd_ChangeAvail(void)
{
	RES_TYPE ret, ret1, ret2;
	uint8_t tmp_cntrId;
	bool bSetVal;

	for(int i=0;i<=Rx_Msg.Payload_len ;i++){
		if(!strcmp(Rx_Msg.Payload[i].property_name, "connectorId")){
			tmp_cntrId = String2Integer(Rx_Msg.Payload[i].property_contants);
			if(tmp_cntrId > MAX_CONECTOR_ID)	ret1 = RES_REJECTED;
			else 								ret1 = RES_ACCEPT;
		}
		else if(!strcmp(Rx_Msg.Payload[i].property_name, "type")){
			if(!strcmp(Rx_Msg.Payload[i].property_contants, "Inoperative")){
				ret2 = RES_ACCEPT;
				bSetVal = false;
			}
			else if(!strcmp(Rx_Msg.Payload[i].property_contants, "Operative")){
				ret2 = RES_ACCEPT;
				bSetVal = true;
			}
			else{
				printf("[Change Available] Undefined Type Name %s\r\n", Rx_Msg.Payload[i].property_contants);	
				ret2 = RES_REJECTED;
			}
		}
		else	ret2 = RES_REJECTED;
	}

	if(ret1 == RES_REJECTED || ret2 == RES_REJECTED){
		ret = RES_REJECTED;
	}
	else	ret = RES_ACCEPT;

	if(shmDataAppInfo.app_order != APP_ORDER_WAIT){
		// ret = RES_SCHEDULED;
		ret = RES_REJECTED;
	}


	if(ret != RES_REJECTED){
		if(tmp_cntrId == 0){
			if(bSetVal){
				for(int i=0;i<=MAX_CONECTOR_ID;i++){
					if(GetCpStatus(i) == CP_STATUS_CODE_AVAIL)
						bCpStatChgFlg |= (1 << i);
					SetCpStatus(CP_STATUS_CODE_AVAIL,i);
					theConfig.chargingstatus |= (1<<i);
				}
			}	
			else{
				for(int i=0;i<=MAX_CONECTOR_ID;i++){
					if(GetCpStatus(i) == CP_STATUS_CODE_UNAVAIL)
						bCpStatChgFlg |= (1 << i);
					SetCpStatus(CP_STATUS_CODE_UNAVAIL,i);
					theConfig.chargingstatus = 0;
				}
			}
		}
		else{
			if(bSetVal){
				if(GetCpStatus(tmp_cntrId) == CP_STATUS_CODE_AVAIL)
						bCpStatChgFlg |= (1 << tmp_cntrId);
				SetCpStatus(CP_STATUS_CODE_AVAIL,tmp_cntrId);
				theConfig.chargingstatus |= (1<<tmp_cntrId);
			}	
			else{
				if(GetCpStatus(tmp_cntrId) == CP_STATUS_CODE_UNAVAIL)
						bCpStatChgFlg |= (1 << tmp_cntrId);
				SetCpStatus(CP_STATUS_CODE_UNAVAIL,tmp_cntrId);
				theConfig.chargingstatus &= ~(1<<tmp_cntrId);
			}
		}		
	}

	// ConfigSave();
	bConfigSaveFlg = true;

	MakeCallRes(ret);
}

void DataProcCmd_ChangeConfig(void)
{
	RES_TYPE ret = RES_REJECTED;

	if(!strcmp(Rx_Msg.Payload[0].property_name, "key") && !strcmp(Rx_Msg.Payload[1].property_name, "value")){
		if(!strcmp(Rx_Msg.Payload[0].property_contants, "MeterValueSampleInterval")){
			if(Rx_Msg.Payload[1].property_contants[0] == '-'){
				ret = RES_REJECTED;
			}
			else{
				CfgKeyVal[10].CfgKeyDataInt = String2Integer(Rx_Msg.Payload[1].property_contants);
				ret = RES_ACCEPT;
			}
		}
		else if(!strcmp(Rx_Msg.Payload[0].property_contants, "ClockAlignedDataInterval")){
			if(Rx_Msg.Payload[1].property_contants[0] == '-'){
				ret = RES_REJECTED;
			}
			else{
				CfgKeyVal[1].CfgKeyDataInt = String2Integer(Rx_Msg.Payload[1].property_contants);
				ret = RES_ACCEPT;
			}
		}
		else if(!strcmp(Rx_Msg.Payload[0].property_contants, "LocalPreAuthorize")){
			if(!strcmp(Rx_Msg.Payload[1].property_contants, "true")){
				CfgKeyVal[7].CfgKeyDataInt = true;
				ret = RES_ACCEPT;
			}
			else if(!strcmp(Rx_Msg.Payload[1].property_contants, "false")){
				CfgKeyVal[7].CfgKeyDataInt = false;
				ret = RES_ACCEPT;
			}
			else{
				printf("Undefined Key Value %s\r\n", Rx_Msg.Payload[1].property_contants);
				ret = RES_REJECTED;
			}
		}
		else if(!strcmp(Rx_Msg.Payload[0].property_contants, "AuthorizationCacheEnabled")){
			if(!strcmp(Rx_Msg.Payload[1].property_contants, "true"))
				ret = RES_NOTSUPPT;
		}
		else if(!strcmp(Rx_Msg.Payload[0].property_contants, "ConnectionTimeOut")){
			if(Rx_Msg.Payload[1].property_contants[0] == '-'){
				ret = RES_REJECTED;
			}
			else{
				CfgKeyVal[2].CfgKeyDataInt = String2Integer(Rx_Msg.Payload[1].property_contants);
				ret = RES_ACCEPT;
			}			
		}
		else if(!strcmp(Rx_Msg.Payload[0].property_contants, "AuthorizeRemoteTxRequests")){
			if(!strcmp(Rx_Msg.Payload[1].property_contants, "true")){
				CfgKeyVal[0].CfgKeyDataInt = true;
				ret = RES_ACCEPT;
			}
			else{
				CfgKeyVal[0].CfgKeyDataInt = false;
				ret = RES_ACCEPT;
			}
		}
		else if(!strcmp(Rx_Msg.Payload[0].property_contants, "UnlockConnectorOnEVSideDisconnect")){
			if(!strcmp(Rx_Msg.Payload[1].property_contants, "true")){
				// CsConfigVal.StopTransactionOnEVSideDisconnect = true;
				CfgKeyVal[20].CfgKeyDataInt = true;
				ret = RES_ACCEPT;
			}
			else{
				// CsConfigVal.StopTransactionOnEVSideDisconnect = false;
				CfgKeyVal[20].CfgKeyDataInt = false;
				ret = RES_ACCEPT;
			}
		}
		else if(!strcmp(Rx_Msg.Payload[0].property_contants, "StopTransactionOnEVSideDisconnect")){
			if(!strcmp(Rx_Msg.Payload[1].property_contants, "true")){
				// CsConfigVal.StopTransactionOnEVSideDisconnect = true;
				CfgKeyVal[13].CfgKeyDataInt = true;
				ret = RES_ACCEPT;
			}
			else{
				// CsConfigVal.StopTransactionOnEVSideDisconnect = false;
				CfgKeyVal[13].CfgKeyDataInt = false;
				ret = RES_ACCEPT;
			}
		}
		else if(!strcmp(Rx_Msg.Payload[0].property_contants, "MinimumStatusDuration")){
			ret = RES_NOTSUPPT;
		}
		else if(!strcmp(Rx_Msg.Payload[0].property_contants, "LocalAuthorizeOffline")){
			if(!strcmp(Rx_Msg.Payload[1].property_contants, "true")){
				CfgKeyVal[6].CfgKeyDataInt = true;
				ret = RES_ACCEPT;
			}
			else if(!strcmp(Rx_Msg.Payload[1].property_contants, "false")){
				CfgKeyVal[6].CfgKeyDataInt = false;
				ret = RES_ACCEPT;
			}
			else{
				printf("Undefined Key Value %s\r\n", Rx_Msg.Payload[1].property_contants);
				ret = RES_REJECTED;
			}
		}
		else if(!strcmp(Rx_Msg.Payload[0].property_contants, "LocalAuthListEnabled")){
			if(!strcmp(Rx_Msg.Payload[1].property_contants, "true")){
				// CfgKeyVal[21].CfgKeyDataInt = true;
				ret = RES_ACCEPT;
			}
			else if(!strcmp(Rx_Msg.Payload[1].property_contants, "false")){
				// CfgKeyVal[21].CfgKeyDataInt = false;
				ret = RES_ACCEPT;
			}
			else{
				printf("Undefined Key Value %s\r\n", Rx_Msg.Payload[1].property_contants);
				ret = RES_REJECTED;
			}
		}
		else if(!strcmp(Rx_Msg.Payload[0].property_contants, "AllowOfflineTxForUnknownId")){
			if(!strcmp(Rx_Msg.Payload[1].property_contants, "true")){
				CfgKeyVal[23].CfgKeyDataInt = true;
				// CsConfigVal.AllowOfflineTxForUnknownId = true;
				ret = RES_ACCEPT;
			}
			else if(!strcmp(Rx_Msg.Payload[1].property_contants, "false")){
				CfgKeyVal[23].CfgKeyDataInt = false;
				// CsConfigVal.AllowOfflineTxForUnknownId = false;
				ret = RES_ACCEPT;
			}
			else{
				printf("Undefined Key Value %s\r\n", Rx_Msg.Payload[1].property_contants);
				ret = RES_REJECTED;
			}
		}
		else if(!strcmp(Rx_Msg.Payload[0].property_contants, "StopTransactionOnInvalidId")){
			if(!strcmp(Rx_Msg.Payload[1].property_contants, "true")){
				CfgKeyVal[14].CfgKeyDataInt = true;
				ret = RES_ACCEPT;
			}
			else if(!strcmp(Rx_Msg.Payload[1].property_contants, "false")){
				CfgKeyVal[14].CfgKeyDataInt = false;
				ret = RES_ACCEPT;
			}
			else{
				printf("Undefined Key Value %s\r\n", Rx_Msg.Payload[1].property_contants);
				ret = RES_REJECTED;
			}
		}
		else{
			printf("Undefined Key Name %s\r\n", Rx_Msg.Payload[0].property_contants);
			ret = RES_NOTSUPPT;	// return Rejected
		}
	}
	else{
		printf("[Change Config] Undefined Name %s, %s\r\n", Rx_Msg.Payload[0].property_name, Rx_Msg.Payload[1].property_name);
		ret = RES_REJECTED;
	} 	

	MakeCallRes(ret);
}

void DataProcCmd_ClearCache(void)
{
	RES_TYPE ret;

	ret = RES_REJECTED;

	MakeCallRes(ret);
}

void DataProcCmd_ClearChargeProf(void)
{
	RES_TYPE ret;

	ret = RES_ACCEPT;

	MakeCallRes(ret);	
}

void DataProcCmd_DataTransCs(void){
	RES_TYPE ret = RES_REJECTED;

	char tmp_str256[256];
	char tmp_str2[2];
	uint8_t cmdType = 0;
	uint8_t parStep = 0;



	for(int i=0;i<=Rx_Msg.Payload_len ;i++){
		if(!strcmp(Rx_Msg.Payload[i].property_name, "vendorId")){
			// if(strcmp(Rx_Msg.Payload[i].property_contants, "kr.or.keco"))
			// 	return false;
		}
		else if(!strcmp(Rx_Msg.Payload[i].property_name, "messageId")){
			if(!strcmp(Rx_Msg.Payload[i].property_contants, "BatteryInfoConfiguration")) {
				cmdType = 1;
			}
		}
		else if(!strcmp(Rx_Msg.Payload[i].property_name, "data")){
			memcpy(tmp_str256, Rx_Msg.Payload[i].property_contants, strlen(Rx_Msg.Payload[i].property_contants));
			CtLogYellow("Check BatteryInfoConfiguration Data : %s", tmp_str256);
		}
	}

	// BatteryInfoConfiguration 
	if(cmdType == 1) {
		for(int i=0; i < strlen(tmp_str256); i++) {	// Str_Len - "configCnt" Lenth - "1" Length
			if(parStep == 0){
				if(!strncmp(&tmp_str256[i], "configCnt", 9)) {
					parStep = 1;
				}
			}
			else if(parStep == 1) {
				if(tmp_str256[i] == ':') {
					parStep = 2;
				}
			}
			else if(parStep == 2) {
				if((tmp_str256[i] > '0') && (tmp_str256[i] <= '9')) {

					parStep = 3;
					tmp_str2[0] = tmp_str256[i];

					if((tmp_str256[i+1] >= '0') && (tmp_str256[i+1] <= '9')) {
						parStep = 4;
						tmp_str2[1] = tmp_str256[i+1];
						CtLogYellow("Check num2 %d", tmp_str2[1]);
					}
					
					break;
				}
			}
		}

		CtLogYellow("Check num1 %d -> parse step %d", tmp_str2[0], parStep);

		if(parStep == 3) {
			CsConfigVal.batInfo_cfgCnt = tmp_str2[0] - '0';
			CtLogGreen("Set batInfo_cfgCnt : %d", CsConfigVal.batInfo_cfgCnt);
			ret = RES_ACCEPT;
		}
		else if (parStep == 4) {
			CsConfigVal.batInfo_cfgCnt = (tmp_str2[0] - '0') * 10 + tmp_str2[1] - '0';
			CtLogGreen("Set batInfo_cfgCnt : %d", CsConfigVal.batInfo_cfgCnt);
			ret = RES_ACCEPT;
		}

		if(CsConfigVal.batInfo_cfgCnt > 20) {
			CtLogRed("BatteryInfo Count Setting Error %d => 1",CsConfigVal.batInfo_cfgCnt);
			CsConfigVal.batInfo_cfgCnt = 1;
			ret = RES_REJECTED;
		}
	}

	else {
		CtLogYellow("Didn't Find BatteryInfoConfiguration!!");
	}

	MakeCallRes(ret);
}


static void CheckCfgKey(char* rw_buf, uint8_t no)
{
	if(CfgKeyVal[no].CfgKeyRw) 	sprintf(rw_buf,"true");
	else						sprintf(rw_buf,"false");
}


static void WriteCfgKey(PAYLOAD* dstPayload, uint8_t no)
{
	char rw_buf[6], val_buf[60];

	char* dst_buff = (dstPayload->sub_Payload + dstPayload->subPayload_len)->property_contants;

	CheckCfgKey(rw_buf, no);

	sprintf(dst_buff, "{\"key\":\"%s\",\"readonly\":%s,\"value\":\"", \
	CfgKeyVal[no].CfgKeyName, \
	CfgKeyVal[no].CfgKeyRw ? &("true") : &("false"));

	switch (CfgKeyVal[no].CfgKeyType)
	{
	case TYPE_CODE_BOOL:
		sprintf(val_buf, "%s\"}", 
		CfgKeyVal[no].CfgKeyDataInt ? &("true") : &("false"));
		break;

	case TYPE_CODE_INT:
		sprintf(val_buf, "%u\"}", 
		CfgKeyVal[no].CfgKeyDataInt);
		break;

	case TYPE_CODE_STR:
		sprintf(val_buf, "%s\"}", 
		CfgKeyVal[no].CfgKeyDataCha);		
		break;		
	
	default:
		break;
	}
	strcat(dst_buff, val_buf);
	printf("[WriteCfgKey] %hd\r\n", no);
	// dstPayload->subPayload_len++;
}

/// @brief 
/// @param  Input : Get Request Configuration Key from Server // Process : Check Configuration Key // Output : Configuration Key -> Sever
void DataProcCmd_GetConfig(void)
{
	RES_TYPE ret;

	bool CheckFlg = false;
	char temp_buf_char[30];
	uint8_t tmep_buf_uint8;
	uint8_t unknown_size=0;

	Tx_Msg.Payload_len = 1;
	Tx_Msg.Msg_type = MSG_TYPE_CALLRES;
	memcpy(Tx_Msg.UniqueID_Char, Rx_Msg.UniqueID_Char,sizeof(Tx_Msg.UniqueID_Char));
	// Tx_Msg.UniqueID = Rx_Msg.UniqueID;
	Tx_Msg.Payload[0].data_type = TYPE_CODE_ARR;

	// for Empty (Request All Configuration Key)	
	if(Rx_Msg.Payload[0].property_name[0] == '\0'){
		memset(Tx_Msg.Payload[0].property_name, 0x00, sizeof(Tx_Msg.Payload[0].property_name));
		memcpy(Tx_Msg.Payload[0].property_name, "configurationKey",sizeof("configurationKey"));

		Tx_Msg.Payload[0].sub_Payload = tx_sub_Payload;
		Tx_Msg.Payload[0].subPayload_len = 0;

		for(int i=0;i<MAX_CFG;i++){
			WriteCfgKey(Tx_Msg.Payload, i);
			Tx_Msg.Payload[0].subPayload_len++;
		}
/*
		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \															//0
		"{\"key\":\"AuthorizeRemoteTxRequests\",\"readonly\":false,\"value\":\"%s\"}", \
		CsConfigVal.AuthorizeRemoteTxRequests ? &("true") : &("false"));

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \															//1	
		"{\"key\":\"ClockAlignedDataInterval\",\"readonly\":false,\"value\":\"%d\"}", \
		CsConfigVal.ClockAlignedDataInterval);

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"ConnectionTimeOut\",\"readonly\":false,\"value\":\"%d\"}", \			
		CsConfigVal.ConnectionTimeOut);

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"ConnectorPhaseRotation\",\"readonly\":false,\"value\":\"%s\"}", \  		
		CsConfigVal.ConnectorPhaseRotation);

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"GetConfigurationMaxKeys\",\"readonly\":true,\"value\":\"%d\"}", \		
		CsConfigVal.GetConfigurationMaxKeys);

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"HeartbeatInterval\",\"readonly\":false,\"value\":\"%d\"}", \				// 5
		CsConfigVal.HeartbeatInterval);

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"LocalAuthorizeOffline\",\"readonly\":false,\"value\":\"%s\"}", \			// 6
		CsConfigVal.LocalAuthorizeOffline ? &("true") : &("false"));

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"LocalPreAuthorize\",\"readonly\":false,\"value\":\"%s\"}", \				// 7
		CsConfigVal.LocalPreAuthorize ? &("true") : &("false"));

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"MeterValuesAlignedData\",\"readonly\":false,\"value\":\"%s\"}", \		// 8
		CsConfigVal.MeterValuesAlignedData);

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"MeterValuesSampledData\",\"readonly\":false,\"value\":\"%s\"}", \		// 9
		CsConfigVal.MeterValuesSampledData);

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"MeterValueSampleInterval\",\"readonly\":false,\"value\":\"%d\"}", \		// 10
		CsConfigVal.MeterValueSampleInterval);

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"NumberOfConnectors\",\"readonly\":true,\"value\":\"%d\"}", \				// 11
		CsConfigVal.NumberOfConnectors);

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"ResetRetries\",\"readonly\":false,\"value\":\"%d\"}", \					// 12
																			CsConfigVal.ResetRetries);

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"StopTransactionOnInvalidId\",\"readonly\":false,\"value\":\"%s\"}", \	// 13
																			CsConfigVal.StopTransactionOnInvalidId ? &("true") : &("false"));

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"StopTxnAlignedData\",\"readonly\":false,\"value\":\"%s\"}", \			// 14
																			CsConfigVal.StopTxnAlignedData);

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"StopTxnSampledData\",\"readonly\":false,\"value\":\"%s\"}", \			// 15
																			CsConfigVal.StopTxnSampledData);

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"SupportedFeatureProfiles\",\"readonly\":true,\"value\":\"%s\"}", \		// 16
																			CsConfigVal.SupportedFeatureProfiles);

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"TransactionMessageAttempts\",\"readonly\":false,\"value\":\"%d\"}", \	// 17
																			CsConfigVal.TransactionMessageAttempts);

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"TransactionMessageRetryInterval\",\"readonly\":false,\"value\":\"%d\"}", \	// 18
																			CsConfigVal.TransactionMessageRetryInterval);

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"UnlockConnectorOnEVSideDisconnect\",\"readonly\":false,\"value\":\"%s\"}", \	// 19
																			CsConfigVal.UnlockConnectorOnEVSideDisconnect ? &("true") : &("false"));

		//Local Auth List Management
		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"LocalAuthListEnabled\",\"readonly\":false,\"value\":\"%s\"}", \				// 20
		CsConfigVal.LocalAuthListEnabled ? &("true") : &("false"));

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"LocalAuthListMaxLength\",\"readonly\":true,\"value\":\"%d\"}", \				// 21
																			CsConfigVal.LocalAuthListMaxLength);

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"SendLocalListMaxLength\",\"readonly\":true,\"value\":\"%d\"}", \				// 22
																			CsConfigVal.SendLocalListMaxLength);

		//Smart Charging Profile
		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"ChargeProfileMaxStackLevel\",\"readonly\":true,\"value\":\"%d\"}", \			// 23
																			CsConfigVal.ChargeProfileMaxStackLevel);

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"ChargingScheduleAllowedChargingRateUnit\",\"readonly\":true,\"value\":\"%s\"}", \	// 24
																			CsConfigVal.ChargingScheduleAllowedChargingRateUnit);

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"ChargingScheduleMaxPeriods\",\"readonly\":true,\"value\":\"%d\"}", \
																			CsConfigVal.ChargingScheduleMaxPeriods);

		sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, \
		"{\"key\":\"MaxChargingProfilesInstalled\",\"readonly\":true,\"value\":\"%d\"}", \
																			CsConfigVal.MaxChargingProfilesInstalled);
*/																			
	}
	else if(!strcmp(Rx_Msg.Payload[0].property_name, "key")){

		Tx_Msg.Payload[0].sub_Payload = tx_sub_Payload;
		Tx_Msg.Payload[0].subPayload_len = 0;

		memset(Tx_Msg.Payload[0].property_name, 0x00, sizeof(Tx_Msg.Payload[0].property_name));
		memcpy(Tx_Msg.Payload[0].property_name, "configurationKey",sizeof("configurationKey"));

		for(Tx_Msg.Payload[0].subPayload_len=0; Tx_Msg.Payload[0].subPayload_len<=Rx_Msg.Payload[0].subPayload_len; Tx_Msg.Payload[0].subPayload_len++)
		{
			for(int i=0;((i<MAX_CFG) && !CheckFlg);i++)
			{
				if(!strcmp((Rx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants , CfgKeyVal[i].CfgKeyName)){
					WriteCfgKey(Tx_Msg.Payload, i);
					CheckFlg = true;
				}
			}
			// if(!strcmp((Rx_Msg.Payload[0].sub_Payload)->property_contants , "AllowOfflineTxForUnknownId"))
			// {
			// 	sprintf(Tx_Msg.Payload->sub_Payload->property_contants, \
			// 	"{\"key\":\"AllowOfflineTxForUnknownId\",\"readonly\":false,\"value\":\"%s\"}", \
			// 	CsConfigVal.AllowOfflineTxForUnknownId ? &("true") : &("false"));
			// }
			// else 
			if(!CheckFlg)
			{
				memset(Tx_Msg.Payload[0].property_name, 0x00, sizeof(Tx_Msg.Payload[0].property_name));
				memcpy(Tx_Msg.Payload[0].property_name, "unknownKey",sizeof("unknownKey"));

				memset(Tx_Msg.Payload[0].property_contants, 0x00, sizeof(Tx_Msg.Payload[0].property_contants));
				memcpy(Tx_Msg.Payload[0].property_contants, "\"Unknow Key\"",sizeof("\"Unknow Key\""));				
				Tx_Msg.Payload[0].subPayload_len = 0;
				break;
			}
			CheckFlg = false;
			/*
			if(!strcmp((Rx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants , "AuthorizeRemoteTxRequests")){
				sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len - unknown_size)->property_contants, "{\"key\":\"AuthorizeRemoteTxRequests\",\"readonly\":false,\"value\":\"%s\"}", \
																			CsConfigVal.AuthorizeRemoteTxRequests ? &("true") : &("false"));
			}
			else if(!strcmp((Rx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, "ClockAlignedDataInterval")){
				sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len - unknown_size)->property_contants, "{\"key\":\"ClockAlignedDataInterval\",\"readonly\":false,\"value\":\"%d\"}", \
																			CsConfigVal.ClockAlignedDataInterval);
			}
			else if(!strcmp((Rx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, "ConnectionTimeOut")){
				sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len - unknown_size)->property_contants, "{\"key\":\"ConnectionTimeOut\",\"readonly\":false,\"value\":\"%d\"}", \
																			CsConfigVal.ConnectionTimeOut);
			}
			else if(!strcmp((Rx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, "ConnectorPhaseRotation")){
				sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len - unknown_size)->property_contants, "{\"key\":\"ConnectorPhaseRotation\",\"readonly\":false,\"value\":\"%s\"}", \
																			CsConfigVal.ConnectorPhaseRotation);
			}
			else if(!strcmp((Rx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, "GetConfigurationMaxKeys")){
				sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len - unknown_size)->property_contants, "{\"key\":\"GetConfigurationMaxKeys\",\"readonly\":true,\"value\":\"%d\"}", \
																			CsConfigVal.GetConfigurationMaxKeys);
			}
			else if(!strcmp((Rx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, "HeartbeatInterval")){
				sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len - unknown_size)->property_contants, "{\"key\":\"HeartbeatInterval\",\"readonly\":false,\"value\":\"%d\"}", \
																			CsConfigVal.HeartbeatInterval);
			}
			else if(!strcmp((Rx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, "LocalAuthorizeOffline")){
				sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len - unknown_size)->property_contants, "{\"key\":\"LocalAuthorizeOffline\",\"readonly\":false,\"value\":\"%s\"}", \
																			CsConfigVal.LocalAuthorizeOffline ? &("true") : &("false"));
			}
			else if(!strcmp((Rx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, "LocalPreAuthorize")){
				sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len - unknown_size)->property_contants, "{\"key\":\"LocalPreAuthorize\",\"readonly\":false,\"value\":\"%s\"}", \
																			CsConfigVal.LocalPreAuthorize ? &("true") : &("false"));
			}
			else if(!strcmp((Rx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, "MeterValuesAlignedData")){
				sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len - unknown_size)->property_contants, "{\"key\":\"MeterValuesAlignedData\",\"readonly\":false,\"value\":\"%s\"}", \
																			CsConfigVal.MeterValuesAlignedData);
			}
			else if(!strcmp((Rx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, "MeterValuesSampledData")){
				sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len - unknown_size)->property_contants, "{\"key\":\"MeterValuesSampledData\",\"readonly\":false,\"value\":\"%s\"}", \
																			CsConfigVal.MeterValuesSampledData);
			}
			else if(!strcmp((Rx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, "MeterValueSampleInterval")){
				sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len - unknown_size)->property_contants, "{\"key\":\"MeterValueSampleInterval\",\"readonly\":false,\"value\":\"%d\"}", \
																			CsConfigVal.MeterValueSampleInterval);
			}
			else if(!strcmp((Rx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, "NumberOfConnectors")){
				sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len - unknown_size)->property_contants, "{\"key\":\"NumberOfConnectors\",\"readonly\":true,\"value\":\"%d\"}", \
																			CsConfigVal.NumberOfConnectors);
			}
			else if(!strcmp((Rx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, "ResetRetries")){
				sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len - unknown_size)->property_contants, "{\"key\":\"ResetRetries\",\"readonly\":false,\"value\":\"%d\"}", \
																			CsConfigVal.ResetRetries);
			}
			else if(!strcmp((Rx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, "StopTransactionOnInvalidId")){
				sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len - unknown_size)->property_contants, "{\"key\":\"StopTransactionOnInvalidId\",\"readonly\":false,\"value\":\"%s\"}", \
																			CsConfigVal.StopTransactionOnInvalidId ? &("true") : &("false"));
			}
			else if(!strcmp((Rx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, "StopTxnAlignedData")){
				sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len - unknown_size)->property_contants, "{\"key\":\"StopTxnAlignedData\",\"readonly\":false,\"value\":\"%s\"}", \
																			CsConfigVal.StopTxnAlignedData);
			}
			else if(!strcmp((Rx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, "StopTxnSampledData")){
				sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len - unknown_size)->property_contants, "{\"key\":\"StopTxnSampledData\",\"readonly\":false,\"value\":\"%s\"}", \
																			CsConfigVal.StopTxnSampledData);
			}
			else if(!strcmp((Rx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, "SupportedFeatureProfiles")){
				sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len - unknown_size)->property_contants, "{\"key\":\"SupportedFeatureProfiles\",\"readonly\":true,\"value\":\"%s\"}", \
																			CsConfigVal.SupportedFeatureProfiles);
			}
			else if(!strcmp((Rx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, "TransactionMessageAttempts")){
				sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len - unknown_size)->property_contants, "{\"key\":\"TransactionMessageAttempts\",\"readonly\":false,\"value\":\"%d\"}", \
																			CsConfigVal.TransactionMessageAttempts);
			}
			else if(!strcmp((Rx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, "TransactionMessageRetryInterval")){
				sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len - unknown_size)->property_contants, "{\"key\":\"TransactionMessageRetryInterval\",\"readonly\":false,\"value\":\"%d\"}", \
																			CsConfigVal.TransactionMessageRetryInterval);
			}
			else if(!strcmp((Rx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, "UnlockConnectorOnEVSideDisconnect")){
				sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len - unknown_size)->property_contants, "{\"key\":\"UnlockConnectorOnEVSideDisconnect\",\"readonly\":false,\"value\":\"%s\"}", \
																			CsConfigVal.UnlockConnectorOnEVSideDisconnect ? &("true") : &("false"));
			}
			else{
				unknown_size++;
				sprintf((Tx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants, "\"Unknow Key\"");
				printf("[DataProcCmd_GetConfig] Receive Unknown Key : %s\r\n", (Rx_Msg.Payload[0].sub_Payload + Tx_Msg.Payload[0].subPayload_len)->property_contants);
			}
			*/
		}
/*
		if(unknown_size == Tx_Msg.Payload[0].subPayload_len){
			memset(Tx_Msg.Payload[0].property_name, 0x00, sizeof(Tx_Msg.Payload[0].property_name));
			memcpy(Tx_Msg.Payload[0].property_name, "unknownKey",sizeof("unknownKey"));

			memset(Tx_Msg.Payload[0].property_contants, 0x00, sizeof(Tx_Msg.Payload[0].property_contants));
			memcpy(Tx_Msg.Payload[0].property_contants, "\"Unknow Key\"",sizeof("\"Unknow Key\""));				
		}
		else{
			memset(Tx_Msg.Payload[0].property_name, 0x00, sizeof(Tx_Msg.Payload[0].property_name));
			memcpy(Tx_Msg.Payload[0].property_name, "configurationKey",sizeof("configurationKey"));
			if(unknown_size > 0){
				memset(Tx_Msg.Payload[1].property_name, 0x00, sizeof(Tx_Msg.Payload[1].property_name));
				memcpy(Tx_Msg.Payload[1].property_name, "unknownKey",sizeof("unknownKey"));

				memset(Tx_Msg.Payload[0].property_contants, 0x00, sizeof(Tx_Msg.Payload[0].property_contants));
				memcpy(Tx_Msg.Payload[0].property_contants, "\"Unknow Key\"",sizeof("\"Unknow Key\""));						
			}
		}
		// Tx_Msg.Payload[0].subPayload_len ++;
		// snprintf(temp_buf_char,"[DataProcCmd_GetConfig] contants : %s\r\n", Rx_Msg.Payload[0].property_contants);
*/
	}
	else {
		memset(Tx_Msg.Payload[0].property_name, 0x00, sizeof(Tx_Msg.Payload[0].property_name));
		memcpy(Tx_Msg.Payload[0].property_name, "unknownKey",sizeof("unknownKey"));

		memset(Tx_Msg.Payload[0].property_contants, 0x00, sizeof(Tx_Msg.Payload[0].property_contants));
		memcpy(Tx_Msg.Payload[0].property_contants, "\"Unknow Key\"",sizeof("\"Unknow Key\""));	
	}	

	// MakeCallRes(ret);
}

void DataProcCmd_GetDiag(void)
{
	for(int i=0;i<=Rx_Msg.Payload_len ;i++){
		if(!strcmp(Rx_Msg.Payload[i].property_name, "location")){
			memset(CsConfigVal.diagLogUrl, 0x00, sizeof(CsConfigVal.diagLogUrl));
			memcpy(CsConfigVal.diagLogUrl, Rx_Msg.Payload[i].property_contants, sizeof(Rx_Msg.Payload[i].property_contants));	

			CsConfigVal.diagLogReqStep = DIAG_STAT_UPLOAD;
		}
	}

	Tx_Msg.Payload_len = 1;
	Tx_Msg.Msg_type = MSG_TYPE_CALLRES;
	memcpy(Tx_Msg.UniqueID_Char, Rx_Msg.UniqueID_Char,sizeof(Tx_Msg.UniqueID_Char));

    memset(Tx_Msg.Payload[0].property_name, 0x00, sizeof(Tx_Msg.Payload[0].property_name));
	memcpy(Tx_Msg.Payload[0].property_name, "fileName",sizeof("fileName"));

	memset(Tx_Msg.Payload[0].property_contants, 0x00, sizeof(Tx_Msg.Payload[0].property_contants));
	memcpy(Tx_Msg.Payload[0].property_contants, "Cs0000000002_1.txt", sizeof("Cs0000000002_1.txt"));
	Tx_Msg.Payload[0].data_type = TYPE_CODE_STR;

	// if(CfgKeyVal[20].CfgKeyDataInt) {
	// 	memset(Tx_Msg.Payload[0].property_contants, 0x00, sizeof(Tx_Msg.Payload[0].property_contants));

	// 	sprintf(Tx_Msg.Payload[0].property_contants, "%d", LocalListVer);
	// }	
}

void DataProcCmd_UnlockConnect(void)
{
	// bool ret = false;
	for(int i=0;i<=Rx_Msg.Payload_len ;i++){
		if(!strcmp(Rx_Msg.Payload[i].property_name, "connectorId")){
			memset(CsConfigVal.diagLogUrl, 0x00, sizeof(CsConfigVal.diagLogUrl));
			memcpy(CsConfigVal.diagLogUrl, Rx_Msg.Payload[i].property_contants, sizeof(Rx_Msg.Payload[i].property_contants));	

			if(String2Integer(Rx_Msg.Payload[i].property_contants)>MAX_CONECTOR_ID)
			{
				// ret = true;
			}
		}
	}

	// if(ret)
	// {
		RES_TYPE ret = RES_NOTSUPPT;;

		MakeCallRes(ret);
	// }
	// else
	// {
	// 	Tx_Msg.Payload_len = 1;
	// 	Tx_Msg.Msg_type = MSG_TYPE_CALLRES;
	// 	memcpy(Tx_Msg.UniqueID_Char, Rx_Msg.UniqueID_Char,sizeof(Tx_Msg.UniqueID_Char));

	// 	memset(Tx_Msg.Payload[0].property_name, 0x00, sizeof(Tx_Msg.Payload[0].property_name));
	// 	memcpy(Tx_Msg.Payload[0].property_name, "status",sizeof("status"));

	// 	memset(Tx_Msg.Payload[0].property_contants, 0x00, sizeof(Tx_Msg.Payload[0].property_contants));
	// 	memcpy(Tx_Msg.Payload[0].property_contants, "Unlocked", sizeof("Unlocked"));
	// }


}

void DataProcCmd_GetLocalListVer(void)
{
	Tx_Msg.Payload_len = 1;
	Tx_Msg.Msg_type = MSG_TYPE_CALLRES;
	memcpy(Tx_Msg.UniqueID_Char, Rx_Msg.UniqueID_Char,sizeof(Tx_Msg.UniqueID_Char));

    memset(Tx_Msg.Payload[0].property_name, 0x00, sizeof(Tx_Msg.Payload[0].property_name));
	memcpy(Tx_Msg.Payload[0].property_name, "listVersion",sizeof("listVersion"));

	memset(Tx_Msg.Payload[0].property_contants, 0x00, sizeof(Tx_Msg.Payload[0].property_contants));

	memcpy(Tx_Msg.Payload[0].property_contants, "-1", sizeof("-1"));
	Tx_Msg.Payload[0].data_type = TYPE_CODE_INT;

	// if(CfgKeyVal[20].CfgKeyDataInt) {
	// 	memset(Tx_Msg.Payload[0].property_contants, 0x00, sizeof(Tx_Msg.Payload[0].property_contants));

	// 	sprintf(Tx_Msg.Payload[0].property_contants, "%d", LocalListVer);
	// }
}

void DataProcCmd_RmStart(void)
{
	RES_TYPE ret;
	bool bIdTagFlg = false;
	uint8_t bCntrNo = 1;

	ret = RES_REJECTED;

	ScreenOnScenario();

	for(int i=0;i<=Rx_Msg.Payload_len ;i++){
		if(!strcmp(Rx_Msg.Payload[i].property_name, "idTag")){
			memset(shmDataAppInfo.card_no, 0x00, sizeof(shmDataAppInfo.card_no));
			memcpy(shmDataAppInfo.card_no, Rx_Msg.Payload[i].property_contants, sizeof(shmDataAppInfo.card_no));	
			bIdTagFlg = true;
		}
		if(!strcmp(Rx_Msg.Payload[i].property_name, "connectorId")){
			bCntrNo = String2Integer(Rx_Msg.Payload[i].property_contants);
		}
	}
	if(bIdTagFlg && GetCpStatus(bCntrNo) != CP_STATUS_CODE_CHARGING && bCntrNo > 0){
		CsConfigVal.bReqRmtStartTsNo = bCntrNo;
		ret = RES_ACCEPT;
	}
	MakeCallRes(ret);
}

void DataProcCmd_RmStopTr(void)
{
	RES_TYPE ret = RES_REJECTED;

	if(!strcmp(Rx_Msg.Payload[0].property_name, "transactionId"))
	{
		if(String2Integer(Rx_Msg.Payload[0].property_contants) == CsConfigVal.bTrId[bDevChannel+1])
		{
			// Charging Transaction Stop
			if( (GetCpStatus(bDevChannel+1) == CP_STATUS_CODE_CHARGING) || (GetCpStatus(bDevChannel+1) == CP_STATUS_CODE_SUSPENDEDEV) )
			{
				ret = RES_ACCEPT;
				CsConfigVal.bReqRmtStopTSFlg = true;
			}
			else
				ret = RES_REJECTED;
		}
		else if(CsConfigVal.bTrId[bDevChannel+1] == 0)
		{	// Non Charging Transaction
			ret = RES_REJECTED;
		}
		else{
			ret = RES_REJECTED;
		}
	}
	else {
		ret = RES_REJECTED;
		printf("UnDefine Payload %s\r\n", Rx_Msg.Payload[0].property_name);
	}		

	MakeCallRes(ret);
}

void DataProcCmd_Reset(void)
{
	RES_TYPE ret = RES_REJECTED;

	if(!strcmp(Rx_Msg.Payload[0].property_name, "type")){
		if(!strcmp(Rx_Msg.Payload[0].property_contants, "Hard")){
			ret = RES_ACCEPT;
			CsConfigVal.bReqResetNo = 2;
		}
		else if(!strcmp(Rx_Msg.Payload[0].property_contants, "Soft")){	// Non Charging Transaction
			ret = RES_ACCEPT;
			CsConfigVal.bReqResetNo = 1;
		}
		else{
			ret = RES_REJECTED;
			printf("UnDefine Payload contants %s\r\n", Rx_Msg.Payload[0].property_contants);
		}
	}
	else {
		ret = RES_REJECTED;
		printf("UnDefine Payload %s\r\n", Rx_Msg.Payload[0].property_name);
	}		

	MakeCallRes(ret);
}

void DataProcCmd_SenLocalList(void)
{
	RES_TYPE ret = RES_NOTSUPPT;

	// RES_TYPE ret = RES_ACCEPT;

	MakeCallRes(ret);

	// uint8_t ret8_1, ret8_2 = false;
	// uint32_t tmp_u32;

	// if(!(CfgKeyVal[20].CfgKeyDataInt));
	// else {

	// 	for(int i=0;i<=Rx_Msg.Payload_len ;i++) {

	// 		if(!strcmp(Rx_Msg.Payload[i].property_name, "listVersion")){
				
	// 			tmp_u32 = String2Integer(Rx_Msg.Payload[0].property_contants);
	// 		}
	// 		else if(!strcmp(Rx_Msg.Payload[i].property_name, "updateType")){
	// 			if(!strcmp(Rx_Msg.Payload[0].property_contants, "Differential")) {
					
	// 			}
	// 			else if(!strcmp(Rx_Msg.Payload[0].property_contants, "Full")) {

	// 			}
	// 			else {
	// 				ret8_1 = false;
	// 			}
	// 		}

	// 		else if(!strcmp(Rx_Msg.Payload[i].property_name, "localAuthorizationList")){

	// 		}

	// 	}

	// }

	// if(bIdTagFlg && (GetCpStatus(bCntrNo) != CP_STATUS_CODE_CHARGING) && (bCntrNo > 0)){
	// 	CsConfigVal.bReqRmtStartTsNo = bCntrNo;
	// 	ret = RES_ACCEPT;
	// }


	// if(ret8 == false) {

		// Tx_Msg.Payload_len = 1;
		// Tx_Msg.Msg_type = MSG_TYPE_CALLRES;
		// memcpy(Tx_Msg.UniqueID_Char, Rx_Msg.UniqueID_Char,sizeof(Tx_Msg.UniqueID_Char));

		// memset(Tx_Msg.Payload[0].property_name, 0x00, sizeof(Tx_Msg.Payload[0].property_name));
		// memcpy(Tx_Msg.Payload[0].property_name, "status",sizeof("status"));

		// memset(Tx_Msg.Payload[0].property_contants, 0x00, sizeof(Tx_Msg.Payload[0].property_contants));

		// memcpy(Tx_Msg.Payload[0].property_contants, "Accepted", sizeof("Accepted"));
		// Tx_Msg.Payload[0].data_type = TYPE_CODE_STR;

	// }

}

void DataProcCmd_TriggerMsg(void)
{
	TRIG_MSG_STAT tmp_trigStat = TRIG_MSG_STAT_NONE;

	uint8_t tmp_trigMsgCntrId = 0;

	RES_TYPE ret = RES_REJECTED;

	for(int i=0; i<=Rx_Msg.Payload_len; i++)
	{
		if(!strcmp(Rx_Msg.Payload[i].property_name, "requestedMessage"))
		{
			if(!strcmp(Rx_Msg.Payload[i].property_contants, "BootNotification"))
			{
				tmp_trigStat = TRIG_MSG_STAT_BOOT;
			}
			else if(!strcmp(Rx_Msg.Payload[i].property_contants, "DiagnosticsStatusNotification"))
			{
				tmp_trigStat = TRIG_MSG_STAT_DIAG;
			}
			else if(!strcmp(Rx_Msg.Payload[i].property_contants, "FirmwareStatusNotification"))
			{
				tmp_trigStat = TRIG_MSG_STAT_FW;
			}
			else if(!strcmp(Rx_Msg.Payload[i].property_contants, "Heartbeat"))
			{
				tmp_trigStat = TRIG_MSG_STAT_HB;
			}
			else if(!strcmp(Rx_Msg.Payload[i].property_contants, "MeterValues"))
			{
				tmp_trigStat = TRIG_MSG_STAT_METER;
			}
			else if(!strcmp(Rx_Msg.Payload[i].property_contants, "StatusNotification"))
			{
				tmp_trigStat = TRIG_MSG_STAT_STAT;
			}
		}
		else if(!strcmp(Rx_Msg.Payload[i].property_name, "connectorId"))
		{
			tmp_trigMsgCntrId = String2Integer(Rx_Msg.Payload[i].property_contants);		
		}
	}

	//valid
	if(tmp_trigStat == TRIG_MSG_STAT_NONE)
	{
		ret = RES_REJECTED;
	}
	else if((tmp_trigStat == TRIG_MSG_STAT_STAT) || (tmp_trigStat == TRIG_MSG_STAT_METER))
	{
		if(tmp_trigMsgCntrId > MAX_CONECTOR_ID)
		{
			ret = RES_REJECTED;
		}
		else
		{
			CsConfigVal.triggerMsgStat = tmp_trigStat;
			ret = RES_ACCEPT;
		}
	}
	else
	{
		CsConfigVal.triggerMsgStat = tmp_trigStat;
		ret = RES_ACCEPT;
	}

	MakeCallRes(ret);
	
}

void DataProcCmd_UpdateFw(void)
{
	for(int i=0; i<=Rx_Msg.Payload_len; i++){
		if(!strcmp(Rx_Msg.Payload[i].property_name, "location"))
		{
			memset(fwUpdateVals.fwUpdateUrl, 0x00, sizeof(fwUpdateVals.fwUpdateUrl));
			memcpy(fwUpdateVals.fwUpdateUrl, Rx_Msg.Payload[i].property_contants, sizeof(Rx_Msg.Payload[i].property_contants));	
		}
		if(!strcmp(Rx_Msg.Payload[i].property_name, "retrieveDate"))
		{
			memset(fwUpdateVals.Time_Stamp, 0x00, sizeof(fwUpdateVals.Time_Stamp));
			memcpy(fwUpdateVals.Time_Stamp, Rx_Msg.Payload[i].property_contants, sizeof(Rx_Msg.Payload[i].property_contants));	
		}

		fwUpdateVals.updateStep = UPDATE_STEP_CHKDATE;
	}

	Tx_Msg.Payload_len = 0;
	Tx_Msg.Msg_type = MSG_TYPE_CALLRES;
	memcpy(Tx_Msg.UniqueID_Char, Rx_Msg.UniqueID_Char,sizeof(Tx_Msg.UniqueID_Char));
}

// trigger Msg Function


void MakeDataCmd_DataTrans_j1(void){
	Tx_Msg.Msg_type = MSG_TYPE_CALL;
	Tx_Msg.UniqueID = CstGetTime_Msec_test();	
    Tx_Msg.Payload_len = 3;
	Tx_Msg.Action_Code = CP_REQ_ACTION_CODE_DATATRANS;

    memset(Tx_Msg.Payload[0].property_name, 0x00, sizeof(Tx_Msg.Payload[0].property_name));
	memcpy(Tx_Msg.Payload[0].property_name, "vendorId",sizeof("vendorId"));

	memset(Tx_Msg.Payload[0].property_contants, 0x00, sizeof(Tx_Msg.Payload[0].property_contants));
	memcpy(Tx_Msg.Payload[0].property_contants, "tscontech",sizeof("tscontech"));
	Tx_Msg.Payload[0].data_type = TYPE_CODE_STR;

	memset(Tx_Msg.Payload[1].property_name, 0x00, sizeof(Tx_Msg.Payload[1].property_name));
	memcpy(Tx_Msg.Payload[1].property_name, "messageId",sizeof("messageId"));

	memset(Tx_Msg.Payload[1].property_contants, 0x00, sizeof(Tx_Msg.Payload[1].property_contants));
	memcpy(Tx_Msg.Payload[1].property_contants, "j1",sizeof("j1"));
	Tx_Msg.Payload[1].data_type = TYPE_CODE_STR;

	memset(Tx_Msg.Payload[2].property_name, 0x00, sizeof(Tx_Msg.Payload[2].property_name));
	memcpy(Tx_Msg.Payload[2].property_name, "data",sizeof("data"));

	memset(Tx_Msg.Payload[2].property_contants, 0x00, sizeof(Tx_Msg.Payload[2].property_contants));
	memcpy(Tx_Msg.Payload[2].property_contants, "",sizeof(""));
	Tx_Msg.Payload[2].data_type = TYPE_CODE_STR;
}
void MakeDataCmd_DataTrans_cpSts(void){
	Tx_Msg.Msg_type = MSG_TYPE_CALL;
	Tx_Msg.UniqueID = CstGetTime_Msec_test();	
    Tx_Msg.Payload_len = 3;
	Tx_Msg.Action_Code = CP_REQ_ACTION_CODE_DATATRANS;

	 memset(Tx_Msg.Payload[0].property_name, 0x00, sizeof(Tx_Msg.Payload[0].property_name));
	memcpy(Tx_Msg.Payload[0].property_name, "vendorId",sizeof("vendorId"));

	memset(Tx_Msg.Payload[0].property_contants, 0x00, sizeof(Tx_Msg.Payload[0].property_contants));
	memcpy(Tx_Msg.Payload[0].property_contants, "tscontech",sizeof("tscontech"));
	Tx_Msg.Payload[0].data_type = TYPE_CODE_STR;

	memset(Tx_Msg.Payload[1].property_name, 0x00, sizeof(Tx_Msg.Payload[1].property_name));
	memcpy(Tx_Msg.Payload[1].property_name, "messageId",sizeof("messageId"));

	memset(Tx_Msg.Payload[1].property_contants, 0x00, sizeof(Tx_Msg.Payload[1].property_contants));
	memcpy(Tx_Msg.Payload[1].property_contants, "cpSts",sizeof("cpSts"));
	Tx_Msg.Payload[1].data_type = TYPE_CODE_STR;

	memset(Tx_Msg.Payload[2].property_name, 0x00, sizeof(Tx_Msg.Payload[2].property_name));
	memcpy(Tx_Msg.Payload[2].property_name, "data",sizeof("data"));

	Tx_Msg.Payload[2].data_type = TYPE_CODE_ARR;

	Tx_Msg.Payload[2].sub_Payload = tx_sub_Payload;
	Tx_Msg.Payload[2].subPayload_len = 1;
	char source = '\\';
	sprintf((Tx_Msg.Payload[2].sub_Payload)->property_contants, 
	//"{\"position\":},{\"temperature\":\"%d\"},{\"humidity\":}",ChargerTemperate);
	//"{\"temperature\":\"%d\"}",ChargerTemperate);
	//"{%c\"position%c\":%c\"%s%c\"},{%c\"temperature%c\":%c\"%d%c\"},{%c\"humidity%c\":%c\"%s%c\"}",source,source,source,"1",source,source,source,source,ChargerTemperate, source,source,source,source,"1",source);
	"{%c\"position%c\":%c\"%s%c\"},{%c\"temperature%c\":%c\"%d%c\"},{%c\"humidity%c\":%c\"%s%c\"}",source,source,source,"1",source,source,source,source,"1", source,source,source,source,"1",source);
}