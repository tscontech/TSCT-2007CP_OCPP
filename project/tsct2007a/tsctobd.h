/**
*       @file
*               tsctobd.h
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2017.12.05 <br>
*               author: dyhwang <br>
*               description: <br>
*/
#ifndef __TSCTOBDREADER_H__
#define __TSCTOBDREADER_H__

typedef void (*CardReaderListener)(char*, int size);

typedef struct {
    uint8_t Stx8;
    uint16_t Lenth16;
    uint8_t Cmd8;
    uint8_t Data8[512];
    uint16_t Crc16;
    uint8_t Etx8;
} IDA_DATA_FORM;

IDA_DATA_FORM IdaTxDataForm;
IDA_DATA_FORM IdaRxDataForm[5];

typedef struct {
    uint8_t Soc;                // 0.5 %
    uint16_t PackVolt;           // 0.1 V
    uint16_t PackCurr;           // 0.1 A
    uint8_t CelVoltMin;          // 0.02 V
    uint8_t CelVoltMax;          // 0.02 V
    uint8_t CellTempMin;         // 1 degree
    uint8_t CellTempMax;        // 1 deg C

    uint8_t CurrRatio;          // 1 %
    uint8_t CurrRatio_old;          // 1 %
} IDA_DATA;

typedef struct {
    uint32_t TimeStamp;         // UTC
    char Vin[17];              // ASCII
    uint16_t Soc;              // 0.5 %
    uint8_t Soh;              // 1 %
    uint16_t PackCurr;         // 0.1 A
    uint16_t PackVolt;         // 0.1 V
    uint8_t CellVolt[192];        // 0.02 V
    uint8_t cellVoltLen;
    uint8_t ModulTemp[20];        // 1 deg C
    uint8_t modulTempLen;
} IDA_KERI_DATA;

IDA_DATA IdaData;

IDA_KERI_DATA IdaKeriData[30];

typedef struct {
    uint16_t dataLenth;
    uint8_t data[1024];
} IDA_VAS_RAW_DATA;

IDA_VAS_RAW_DATA idaVasRawData[30];

uint8_t seccVasDataCnt;

uint8_t IdaKeriData_idx;

#endif

