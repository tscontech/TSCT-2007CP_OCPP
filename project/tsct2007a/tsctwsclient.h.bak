/**
 * @file tsctwsclient.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-05-03
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __TSCTCLIENT_H__
#define __TSCTCLIENT_H__

//
#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <unistd.h>

#include "ctrlboard.h"

//#define CS_URL_ADDR "ws://192.168.1.52:6300/ocppj/"
#define CS_URL_ADDR "ws://192.168.1.32:6100/ocpp/"

#define ACTION_ETC 20

// From CP Message
#define CP_REQ_AUTHORIZE          	"Authorize"
#define CP_REQ_BOOTNOTIFICATION   	"BootNotification" 

// From CS Message
#define CS_REQ_CHANGECONFIG			"ChangeConfigurationRequest"

// Configuration Key
#define CONFIG_KEY_METERVALINTVL	"MeterValueSampleInterval"

// Message Response Status
#define OCPP_STATUS_ACCEPT 		"Accepted"
#define OCPP_STATUS_PENDING		"Pending"
#define OCPP_STATUS_REJECTED 	"Rejected"
#define OCPP_STATUS_REBOOT 		"RebootRequired"
#define OCPP_STATUS_NOTSUPPT 	"NotSupported"

#define MSG_TYPE_CALL			2
#define MSG_TYPE_CALLRES		3
#define MSG_TYPE_ERROR			4

#define EvntQMaxLen				4

#define MAX_CONECTOR_ID			1

#define MAX_CFG 24

#define METER_VAL_SAMP_TYPE		1
#define METER_VAL_CLOCK_TYPE	2
#define METER_VAL_TRIG_TYPE		3

typedef enum {
	CP_STATUS_CODE_NONE = 0,
	CP_STATUS_CODE_PEND,
	CP_STATUS_CODE_AVAIL,
	CP_STATUS_CODE_PREPARE,
	CP_STATUS_CODE_CHARGING,
	CP_STATUS_CODE_SUSPENDEDEV,
	CP_STATUS_CODE_SUSPENDEDEVSE,
	CP_STATUS_CODE_FNISH,
	CP_STATUS_CODE_RESERVED,
	CP_STATUS_CODE_UNAVAIL,
	CP_STATUS_CODE_FAULT,
} CP_STATUS_CODE;

typedef enum {
	CP_REQ_ACTION_CODE_NONE = 0,
	CP_REQ_ACTION_CODE_AUTH,
	CP_REQ_ACTION_CODE_BOOT,
	CP_REQ_ACTION_CODE_DATATRANS,
	CP_REQ_ACTION_CODE_DIAGSTAT,
	CP_REQ_ACTION_CODE_FWSTAT,
	CP_REQ_ACTION_CODE_HB,
	CP_REQ_ACTION_CODE_METERVAL,
	CP_REQ_ACTION_CODE_STARTTS,
	CP_REQ_ACTION_CODE_STAT,
	CP_REQ_ACTION_CODE_STOPTS
} CP_REQ_ACTION_CODE;

typedef enum {
	CS_REQ_ACTION_CODE_NONE = 0,
	CS_REQ_ACTION_CODE_CANCLERESEVE,
	CS_REQ_ACTION_CODE_CHANGEAVAIL,
	CS_REQ_ACTION_CODE_CHANGECONFIG,
	CS_REQ_ACTION_CODE_CLEARCACHE,
	CS_REQ_ACTION_CODE_CLEARCAHRGPROF,
	CS_REQ_ACTION_CODE_DATATRANSFER,
	CS_REQ_ACTION_CODE_GETCOMPOSITSCHED,
	CS_REQ_ACTION_CODE_GETCONFIG,
	CS_REQ_ACTION_CODE_GETDIAG,
	CS_REQ_ACTION_CODE_GETLOCALVER,
	CS_REQ_ACTION_CODE_REMOTESTART,
	CS_REQ_ACTION_CODE_REMOTESTOP,
	CS_REQ_ACTION_CODE_RESERVE,
	CS_REQ_ACTION_CODE_RESET,
	CS_REQ_ACTION_CODE_SNDLOCALLIST,
	CS_REQ_ACTION_CODE_SETCHGPROFILE,
	CS_REQ_ACTION_CODE_TRIGGERMSG,
	CS_REQ_ACTION_CODE_UNLOCKCONNECT,
	CS_REQ_ACTION_CODE_UPDATEFW
} CS_REQ_ACTION_CODE;

typedef struct{
	uint64_t TrId;
	char Time_Stamp[50];
	uint32_t MeterStop_Val;
	char IdTag[21];
	STOP_REASON Stop_Reason;
    uint8_t Connector_No;
}STOPTSVAL_TYPE;

STOPTSVAL_TYPE StopTsConfig;


typedef struct{
	bool reqStartTsFlg;
	// uint64_t TrId;
	char Time_Stamp[50];
	uint32_t MeterStart_Val;
	char IdTag[21];
    uint8_t Connector_No;
	bool faultChargFlg;
} STARTTS_TYPE;

STARTTS_TYPE startTsQ;

typedef enum {
	TRIG_MSG_STAT_NONE = 0,
	TRIG_MSG_STAT_BOOT,
	TRIG_MSG_STAT_DIAG,
	TRIG_MSG_STAT_FW,
	TRIG_MSG_STAT_HB,
	TRIG_MSG_STAT_METER,
	TRIG_MSG_STAT_STAT,
} TRIG_MSG_STAT;

typedef enum {
	DIAG_STAT_NONE = 0,
	DIAG_STAT_UPLOAD,
	DIAG_STAT_UPLOADING,
	DIAG_STAT_UPLOADED,
	DIAG_STAT_UPLOADFAIL,
} DIAG_STAT;

typedef struct {                          
	bool StopTransactionOnEVSideDisconnect;
    // Status
	CP_STATUS_CODE bPreCpStat[MAX_CONECTOR_ID+1];
	/*	Available : Main Layer
		Prepare : CP 9V, CardAuth Ok
		SuspendedEv : 

	*/
    CP_STATUS_CODE bCpStat[MAX_CONECTOR_ID+1];	
	uint64_t bTrId[MAX_CONECTOR_ID+1];
	uint8_t bReqAuthNo;							// 0 : Notyet Authorization , 1 : Request Auth for Charging Start , 2 : Request Auth for Charging Stop
	uint8_t bReqStartTsNo;						// CH1 : 1 , CH2 : 2 , CH1 OK : 3, CH2 OK : 4 , CH1 Fail : 5 , CH2 Fail : 6
	bool bReqStopTsFlg;							// True : Req to Send Stop TR , False : Send Stop TR OK
	uint8_t bReqRmtStartTsNo;					// 1 : CH1 Remote Start	, 2 : CH2 Remote Start ,False : IDLE
	bool bReqRmtStopTSFlg;						// True : Req to Stop TS to System , False : IDLE
	uint8_t bReqResetNo;						// 0 : IDlE , 1 : SOFT , 2 : HARD
	char scnd_card_no[21];
	char parentId[21];
	bool bReqEmgBtnFlg;
	uint8_t batInfo_cfgCnt;
	// bool bBootPendFlg;

	bool AllowOfflineTxForUnknownId;

	TRIG_MSG_STAT triggerMsgStat;
	uint8_t trigMsgCntrId;

	char diagLogUrl[100];
	DIAG_STAT diagLogReqStep;

} CS_CONFIG_VAL;

CS_CONFIG_VAL CsConfigVal;


typedef enum {
	AUTH_CACHE_STAT_NONE = 0,
	AUTH_CACHE_STAT_SUCC,
	AUTH_CACHE_STAT_FAIL
} AUTH_CACHE_STAT_CODE;

typedef struct {
	AUTH_CACHE_STAT_CODE AuthCachStat;
	char AuthCachId[21];
} AUTH_CACHE_VAL;

AUTH_CACHE_VAL AuthCacheVal[5];


int LocalListVer;

typedef struct {
	char Status;
	char Id[21];
	char ExDate[50];
	char Pid[21];
} LOCAL_LIS_VAL;

LOCAL_LIS_VAL LocalListVal[5];

typedef enum{
	TYPE_CODE_STR = 0,
	TYPE_CODE_INT,
	TYPE_CODE_ARR,
	TYPE_CODE_OBJ,
	TYPE_CODE_BOOL,
	TYPE_CODE_LITE,
	TYPE_CODE_OBJ_STR,			// Custom Type for Battery Data Object 2 String
} TYPE_CODE;

typedef struct sPayloadLite{
    char property_name[10];
    char property_contants[15];
} PAYLOAD_LITE;

typedef struct sPayload{
	TYPE_CODE data_type;	
    char property_name[60];
    char property_contants[350];
	uint8_t subPayload_len;
	uint8_t subPayloadArrLen;
	struct sPayload* sub_Payload;
} PAYLOAD;

typedef struct {
	//--------------Header------------------------
	uint8_t Msg_type;		// CALL : 2, CALLRES : 3, ERROR : 4
    uint64_t UniqueID;		
	char UniqueID_Char[70];
    uint8_t Action_Code;
	//--------------Payload------------------------
	uint8_t Payload_len;	// Rx : cnt-1, Tx : cnt
    PAYLOAD Payload[10];
} MSG_STRUCT;

typedef enum {
	CLIENT_NONE_STEP = 0,
	CLIENT_BOOT_STEP,
	CLIENT_PENDING_STEP,
	CLIENT_REJECT_STEP,
	CLIENT_INIT_STEP,
	CLIENT_IDLE_STEP,
	CLIENT_RESWAIT_STEP,
	CLIENT_RES_STEP
} CLIENT_STEP_CODE;

CLIENT_STEP_CODE bClientStep;

// old
typedef enum {
	END_NONE = 0,
	END_NOMAL,
	END_BTN,
	END_EMG,
	END_UNPLUG,
	END_ERR,
	END_SERVER,
	END_CAR,
	END_CARD
} CHARGE_END_CODE;

MSG_STRUCT Call_Tx_Msg;

MSG_STRUCT Tx_Msg;

MSG_STRUCT Rx_Msg;

typedef struct {
    char CfgKeyName[60];
    bool CfgKeyRw;
    TYPE_CODE CfgKeyType;
    char CfgKeyDataCha[60];
	uint32_t CfgKeyDataInt
}TSCT_CFG_KEY;

// 	1	"AuthorizeRemoteTxRequests",
// 	2	"ClockAlignedDataInterval",
// 	3	"ConnectionTimeOut",
// 	4	"ConnectorPhaseRotation",
// 	5	"GetConfigurationMaxKeys",
// 	6	"HeartbeatInterval",
// 	7	"LocalAuthorizeOffline",
// 	8	"LocalPreAuthorize",
// 	9	"MeterValuesAlignedData",
// 	10	"MeterValuesSampledData",
// 	11	"MeterValueSampleInterval",
// 	12	"NumberOfConnectors",
// 	13	"ResetRetries",
// 	14	"StopTransactionOnInvalidId",
// 	15	"StopTxnAlignedData",
// 	16	"StopTxnSampledData",
// 	17	"SupportedFeatureProfiles",
// 	18	"TransactionMessageAttempts",
// 	19	"TransactionMessageRetryInterval",
// 	20	"UnlockConnectorOnEVSideDisconnect",

// Notyet Use
// 	21	"LocalAuthListEnabled",
// 	22	"LocalAuthListMaxLength",
// 	23	"SendLocalListMaxLength",
// 	24	"ChargeProfileMaxStackLevel",
// 	25	"ChargingScheduleAllowedChargingRateUnit",
// 	26	"ChargingScheduleMaxPeriods",
// 	27	"MaxChargingProfilesInstalled"
TSCT_CFG_KEY CfgKeyVal[MAX_CFG];

typedef struct {
	uint8_t Meterval_Flg;
	uint64_t Uid_Mv;
	uint8_t Connect_Id;
	char Time_Stamp[50];
	uint32_t Sampled_Val;
} METERVAL_TYPE;

#define MAX_QNO_METERVAL	5

METERVAL_TYPE MeterValQ[MAX_QNO_METERVAL];

typedef enum {
	UPDATE_STEP_NONE = 0,
	UPDATE_STEP_CHKDATE,
	UPDATE_STEP_DOWNLOAD,
	UPDATE_STEP_DOWNLOADING,
	UPDATE_STEP_DOWNLOADED,
	UPDATE_STEP_DOWNLOADFAIL,
	UPDATE_STEP_INSTAL,
	UPDATE_STEP_INSTALED,
	UPDATE_STEP_INSTALFAIL,
} UPDATE_STEP;

typedef struct {
	UPDATE_STEP updateStep;
	char fwUpdateUrl[70];
	char Time_Stamp[50];
} FW_UPDATE;

FW_UPDATE fwUpdateVals;

struct tm tNetTime;

struct sPayloadLite subPayloadBat[4500];
struct sPayload tx_sub_Payload[180];
struct sPayload sub_Payload1[30];
struct sPayload sub_Payload2[30];
struct sPayload sub_Payload3[30];

bool bWaitResFlg;	// Waiting Response : True , Idle : False

bool bConnect;

bool bResEvntFlg;

bool bReqEvntFlg;

char bCpStatChgFlg;

bool bConfigSaveFlg;

uint8_t vasLastDataCnt;

bool cpStatChk;		// true : connect	/ false : disconn

void TsctCheckNetTimeSet(void);

uint8_t Parse_Data(char* curl_ws_recv_buf, size_t rlen);

void SetCpStatus(CP_STATUS_CODE code_no ,uint8_t ch_no);
CP_STATUS_CODE GetCpStatus(uint8_t ch_no);

bool CheckUpdateFwDate(void);

#endif  /*__TSCTCLIENT_H__*/