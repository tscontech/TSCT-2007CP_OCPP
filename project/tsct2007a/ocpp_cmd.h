/**
 * @file ocpp_cmd.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-05-04
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __TSCTPACKET_H__
#define __TSCTPACKET_H__

typedef enum {
    TSCT_ERR_CODE_NONE = 0,
    TSCT_ERR_CODE_EMG,
    TSCT_ERR_CODE_BATDATA,
    TSCT_ERR_CODE_SECCBATDATA,
} TSCT_ERR_CODE;


// From CP Message
void MakeDataCmd_Auth(void);
void MakeDataCmd_Boot(void);
void MakeDataCmd_FwStat(UPDATE_STEP updateStep);
void MakeDataCmd_HB(void);
void MakeDataCmd_StartTs(void);
void MakeDataCmd_Stat(uint8_t cntrNo);
void MakeDataCmd_StopTs(void);

void DataProcCmd_Auth(void);
void DataProcCmd_Boot(void);
void DataProcCmd_DataTransCp(void);
void DataProcCmd_FwStat(void);
void DataProcCmd_HB(void);
void DataProcCmd_StartTs(void);
void DataProcCmd_Stat(void);
void DataProcCmd_StopTs(void);

// From CS Message
void DataProcCmd_CancelReserve(void);
void DataProcCmd_ChangeAvail(void);
void DataProcCmd_ChangeConfig(void);
void DataProcCmd_ClearCache(void);
void DataProcCmd_ClearChargeProf(void);
void DataProcCmd_DataTransCs(void);
void DataProcCmd_GetConfig(void);
void DataProcCmd_RmStopTr(void);
void DataProcCmd_UnlockConnect(void);

// From DataTrans 
void MakeDataCmd_DataTrans_j1(void);
void MakeDataCmd_DataTrans_cpSts(void);

#endif /*__TSCTPACKET_H__*/