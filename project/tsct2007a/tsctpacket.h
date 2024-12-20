/**
 * @file tsctpacket.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-03-09
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __TSCTPACKET_H__
#define __TSCTPACKET_H__

#include "tsctclient.h"
#include <time.h>

typedef enum {
    c1_EMPTY = 0,
    c1_POWERON = 1,
    c1_BUSIREADY,
    c1_BUSISTART
}c1_STATUS;

typedef enum {
	Price_Empty = 0x00,
	Member_Price_OK,
	NoMember_Price_OK,
	VIP_Price_OK
}CHG_PRICE;

struct HEADER 
{
    unsigned char send_date[7];         // BCD [20][11][12][15][16][28][59]
    unsigned char sno[2];               // HEX
    unsigned char cp_type;              // HEX
    //unsigned char cp_temp;              // HEX
    unsigned char cp_group_id[8];       // ASC
    unsigned char cp_id[2];             // ASC
    unsigned char ins[2];               // HEX
    unsigned char ml[2];                // HEX
};

typedef struct 
{
    unsigned char stx;                  // HEX
    struct HEADER header;
    unsigned char vd[TSCT_BUFSIZE];             // HEX
    unsigned char crc[2];               // HEX
    unsigned char etx;                  // HEX
}PACKET;

/*Request to Sever*/
void MakeDataCmd_a1(int ch);
void MakeDataCmd_b1(int ch);
void MakeDataCmd_c1(int ch);
void MakeDataCmd_d1(int ch);
void MakeDataCmd_e1(int ch);
void MakeDataCmd_f1(int ch);
void MakeDataCmd_g1(int ch);
void MakeDataCmd_h1(int ch); // Add S-traffic packet field 190308 _dsAn
void MakeDataCmd_i1(int ch);
void MakeDataCmd_j1(int ch);
void MakeDataCmd_k1();
void MakeDataCmd_n1();
void MakeDataCmd_q1(int ch);
void MakeDataCmd_q2(int ch);
void MakeDataCmd_r1();

/*Response to Sever*/
void MakeDataCmd_1C(int ch);
void MakeDataCmd_1I(int ch);
void MakeDataCmd_1K(int ch);
void MakeDataCmd_1F(int ch);
void MakeDataCmd_1N(int ch);

/*Receive Response from Server*/
void DataProcCmd_1a(int ch);
void DataProcCmd_1b(int ch);
void DataProcCmd_1c(int ch);
void DataProcCmd_1d(int ch);
void DataProcCmd_1e(int ch);
void DataProcCmd_1f(int ch);
void DataProcCmd_1g();
void DataProcCmd_1h();
void DataProcCmd_1i();
void DataProcCmd_1j(int ch);
void DataProcCmd_1q(int ch);
void DataProcCmd_2q(int ch);

/*Receive Request from Server*/
void DataProcCmd_C1(int ch);
void DataProcCmd_F1(int ch);
void DataProcCmd_K1(int ch);
void DataProcCmd_I1(int ch);
void DataProcCmd_N1(int ch);

void Resetb1(const int msec);
void Resete1(const int msec);
void ByteOrdertwo(const unsigned short value, unsigned char* buf);
unsigned short ByteOrder(unsigned char* buf );
short CopyDataPacket(char *pDest, char *pSrc, short len);

PACKET txPacket;
PACKET rxPacket;

CHG_PRICE charge_price_busi;

struct tm tNetTime;

c1_STATUS c1_stat;

bool bNetTimeSync;  // When Receive b1, Set Time Sync Flag

bool bSendF1;		//	F1 Send Check / True : f1 Send / False : d1 send

bool reset_yn;

#endif