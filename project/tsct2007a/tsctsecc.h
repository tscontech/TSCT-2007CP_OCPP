/**
 * @file tsctsecc.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2024-03-21
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef __TSCTSECC_H__
#define __TSCTSECC_H__

typedef struct {
    unsigned char Id01;         // 0
    unsigned char FuncCode;     // 1
    unsigned short ReqAddr;     // 2 3
    unsigned short  ReqAddrLen; // 4 5
    unsigned short Crc16;       // 6 7
} SECC_MODBUS_READ_TX_S;

typedef struct {
    unsigned char Id01;         // 0
    unsigned char FuncCode;     // 1
    unsigned char DataLen;      // 2
    unsigned short Data16[512];  // 3 72
    unsigned short Crc16;       // 73 74    
} SECC_MODBUS_READ_RX_S;

typedef struct {
    unsigned char Id01;         // 0
    unsigned char FuncCode;     // 1
    unsigned short ReqAddr;     // 2 3
    unsigned short ReqAddrLen; // 4 5
    unsigned char Datalen;      // 6
    unsigned short Data16[127];  // 7 205
    unsigned short Crc16;       // 6 7
} SECC_MODBUS_WRITE_TX_S;

typedef struct {
    unsigned char Id01;         // 0
    unsigned char FuncCode;     // 1
    unsigned short ReqAddr;     // 2 3
    unsigned short  ReqAddrLen; // 4 5
    unsigned short Crc16;       // 6 7
} SECC_MODBUS_WRITE_RX_S;

SECC_MODBUS_READ_TX_S SeccModbusReadTx;
SECC_MODBUS_READ_RX_S SeccModbusReadRx;

SECC_MODBUS_WRITE_TX_S SeccModbusWriteTx;
SECC_MODBUS_WRITE_RX_S SeccModbusWriteRx;

// typedef {
//     unsigned char FuncCode;
    
// } SECC_TX_QUEUE;

typedef enum {
    CSM_STAT_NONE,
    CSM_STAT_READY,
    CSM_STAT_WAITHANDSHAKE,
    CSM_STAT_SESSIONREADY,
    CSM_STAT_AUTHCHECK,
    CSM_STAT_CHARGPRMCHK,
    CSM_STAT_CABLECHK,
    CSM_STAT_PRECHRG,
    CSM_STAT_CHRG,
    CSM_STAT_STOPCHRG,
    CSM_STAT_FAILURESTOP,
    CSM_STAT_NARMALSTOP,
} CSM_STAT;

typedef struct  {
    unsigned short    ready;
    unsigned short    stcode;
    unsigned short    secc_errcode;
    unsigned short    swVer;
    unsigned short    chrgPrtcl;
    unsigned short    pwmduty;
    unsigned short    pwmvoltage; 
    unsigned short    rev_30008;
    unsigned short    rev_30009;
    unsigned short    rev_30010;
    unsigned short     evid[4];

    unsigned short    payment;
    unsigned short    req_type;
    unsigned short    schedule;
    unsigned short    starttime1;  
    unsigned short    starttime2;  

    unsigned short    optionparams;
    unsigned short    chgstatus;
    unsigned short    dc_errcode;
    unsigned short    soc;
    unsigned short    rt_full;
    unsigned short    rt_bulk;
    unsigned short    tcurrent;
    unsigned short    tvoltage;
    unsigned short    max_current;
    unsigned short    max_volatage;   

    unsigned short    evcapacity;
    unsigned short    req_energy;
    unsigned short    ev_max_power;
    unsigned short    fullsoc;
    unsigned short    bulksoc;
    unsigned short    dmy[10];

    // Vas Meta Data [32001 ~ 32010]
    unsigned short vasMapCnt;
    unsigned short vasEnable;
    unsigned short vasStat;
    unsigned short vasErrcd;
    unsigned short vasRxCnt;
    unsigned short vasReserv1;
    unsigned short vasReserv2;
    unsigned short vasReserv3;
    unsigned short vasChksum;
    unsigned short vasLen;

} SECC_READ_DATA;

SECC_READ_DATA SeccRxData;

typedef struct {
    // Vas Data [32010 ~ 32522]
    uint32_t timeStamp;
    char vin[17];
    uint8_t soc;
    uint8_t soh;
    uint16_t batPackCurr;
    uint16_t batPackVolt;
    uint8_t batCellVolt[65535];
    uint8_t cellVoltLen;
    uint8_t batModulTemp[255];
    uint8_t modulTempLen;
} SECC_VAS_READ_DATA;

SECC_VAS_READ_DATA seccVasRxData[30];

typedef struct {
    uint16_t dataLenth;
    uint8_t data[1024];
} SECC_VAS_RAW_DATA;

SECC_VAS_RAW_DATA seccVasRawData[30];

uint8_t seccVasDataCnt;

typedef enum{
    SECC_STAT_EVSE_READY,
    SECC_STAT_CHARG,
    SECC_STAT_STOP,
    SECC_STAT_MALFUNC,
    SECC_STAT_EMG,
    SECC_STAT_AUTHFIN,
    SECC_STAT_CHRGPRMFIN,
    SECC_STAT_ISOL,
} SECC_STATUS_FAULT;

typedef struct  {
    uint16_t    status_fault;
    uint16_t    limit_status;
    uint16_t    reservx1;
    uint16_t    reservx2;
    uint16_t    optional;
    uint16_t    chg_type;
    uint16_t    payment;
    uint16_t    reserved3[3];  
    uint16_t    voltage;
    uint16_t    current;
    uint32_t    watthour;
    uint16_t    reserved4[6];  
    uint16_t    MaxCurr;
    uint16_t    NormVolt;
    uint16_t    NormFreq;
    uint16_t    MaxPower;
} SECC_WRITE_DATA; 

SECC_WRITE_DATA SeccTxData;

bool evccIdFlg;

typedef enum{
	SECC_INIT_STEP_NONE = 0,
	SECC_INIT_STEP_CSMR,
	SECC_INIT_STEP_EVSER,
	SECC_INIT_STEP_CHRGPRM,
	SECC_INIT_STEP_CHRGTYPE,
	SECC_INIT_STEP_PAYOPT,
	SECC_INIT_STEP_INIT,
	SECC_INIT_STEP_CSMSTAT,
	SECC_INIT_STEP_CSMERR,
	SECC_INIT_STEP_PWMDUTY,
	SECC_INIT_STEP_PWMVOLT,
	SECC_INIT_STEP_VASREQ,
	SECC_INIT_STEP_VASREADREQ,
	SECC_INIT_STEP_VASCHK,
	SECC_INIT_STEP_DONE,
}SECC_INIT_STEP;

SECC_INIT_STEP SeccInitStep;
// SECC_INIT_STEP SeccInitStep = SECC_INIT_STEP_NONE;

typedef enum{
	SECC_CHRG_STEP_NONE = 0,
    SECC_CHRG_STEP_REQID,
    SECC_CHRG_STEP_CHKID,
	SECC_CHRG_STEP_START,
	SECC_CHRG_STEP_HANDSH,
	SECC_CHRG_STEP_AUTHCHK,
	SECC_CHRG_STEP_PRMCHK,
	SECC_CHRG_STEP_CHARGING,
} SECC_CHRG_STEP;

SECC_CHRG_STEP SeccChrgStep;
// SECC_CHRG_STEP SeccChrgStep = SECC_CHRG_STEP_NONE;
bool Secc_IsReady(void);
#endif