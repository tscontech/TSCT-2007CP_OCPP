/**
*       @file
*               cstsamwonfa.h
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2019.05.13 <br>
*               author: ktlee <br>
*               description: <br>
*/
#ifndef __CSTSAMWONFA_H__
#define __CSTSAMWONFA_H__

#ifdef SAMWONFA_CD_TERMINAL
// SamWon FA Credite Terminal/////////////
typedef struct 
{
    char stx;           // Start of Transaction (0x02)
    char SendDate[7];   // BCD YYYYMMDD hhmmss
	char SeqNum[2];		// 2byte hex (0x0000 ~ 0xFFFF), when a day be changed or a charger is reset, reset to 0x0000	
    char type;			// Hex 1 byte 0xFF all, [car 0x01 DC demo, 0x02 AC normal, 0x03 DC demo+AC3, 0x04 AC3, 0x05 DC combo, 0x06 DC demo+AC3+DC combo]
                        //                      [bus 0x11 DC quick, 0x12 AC Normal, 0x13 Non Touch, 0x14 AC Quick]
                        //                      [a two wheeled verhicle  0x21 quick, 0x22 normal]
    char StationId[8];	// ASC 8 byte, 2, 2, 3, 2
	char CPId[2];		// ASC 2 byte 
	char Ins[2];		// Hex 1 byte, Instruction Identifier, command...
	char ML[2];			// Hex 2 byte Message Length, minimum of buffer size is 1024 byte.
}CT_HEADER;  // Credite Card Terminal data Header..

typedef struct 
{
    CT_HEADER header;          	// Credite Terminal Message Header
    char VD[1024];           	// Variable Data
	unsigned short CRC;			// 2byte hex, CRC Checksum from SendDate to VD	
    unsigned char etx;			// Hex 1 byte End of transaction 0x03
}CT_MASSAGE;  // Credite Card Terminal Message..


typedef struct 
{
    char phoneNum[12];      // ASC 12 byte Modem MDN
    char M2MVer[8];         // ASC 8 byte manufacturer 1, year 2, month 2, day 2, others 1
	char M2MMan;			// Hex 1byte manufacturer code, LS('L') M2MLink('M'), SamWonFA('S'), SeaHanRF('R') 	
    char RFVer[8];          // ASC 8 byte manufacturer 1, year 2, month 2, day 2, others 1    
	char RFMan;				// Hex 1byte manufacturer code, VisionFactory('V') 	
}CT_TER_INFO;  // Credite Card Terminal Information..

typedef struct 
{
	unsigned char ResCode;			// Hex 1 byte 0x00 Ok
    unsigned char date[7];      		// BCD 7 byte time info of Payment 
    unsigned char ApprovalCode[11];  // ASC 11 byte approval code of payment
    unsigned char oripay_date[7]; 		// BCD 7 byte time info of won-Payment
	unsigned char PayNumber[12];		// ASC 12 byte payment number
	unsigned char ResMsg[32];        // ASC 32 byte normal approval or reason of no approval( display a reason on LCD )    
	unsigned char CardRes[2];		// ASC 2 byte response code of credite card company( normal approval '00') 	
	unsigned char CdCompayMsg[100];	// ANSI Hangul, the end of message is ':'. 		
}CT_PAY_RES;  // Credite Card Terminal Information..

typedef struct 
{
    char date[7];      	// BCD 7 byte. time of Event occur 
    char ErrCode[2];  	// BCD 2 byte. code of error event
	char status;		// HEX 1 byte. 0x31 occure, 0x32 recover
}CT_ErrData;  // Credite Card Terminal Error Event..


//// define Instruction...command..
#define CT_REQ_RESET	'A4'  // request reset.
#define CT_REQ_ST_CON	'B4'  // request a status of connection.
#define CT_REQ_IF_TER	'C4'  // request a info of terminal.
#define CT_REQ_POLL		'D4'  // request a polling. status of RFID connection.
#define CT_REQ_PAY		'G4'  // request a payment.
#define CT_REQ_CON		'J4'  // request a connect to server.
#define CT_REQ_SET_ANT	'K4'  // request a setting of RF Antenna.
#define CT_REQ_CANCEL	'L4'  // request a cancel of before payment.
#define CT_REQ_TX_ACK	'Z4'  // ACK 0x06 OR NAK 0x15.

#define CT_REQ_IF_ALARM	'a4'  // request a info of alarm.
#define CT_REQ_RX_ACK	'z4'  // ACK 0x06 OR NAK 0x15.
#define CT_REQ_RX_CARD	'b4'  // Read Card Number.

#define CT_RES_RESET	'4A'  // request reset.
#define CT_RES_ST_CON	'4B'  // request a status of connection.
#define CT_RES_IF_TER	'4C'  // request a info of terminal.
#define CT_RES_POLL		'4D'  // request a polling. status of RFID connection.
#define CT_RES_PAY		'4G'  // request a payment.
#define CT_RES_CON		'4J'  // request a connect to server.
#define CT_RES_SET_ANT	'4K'  // request a setting of RF Antenna.
#define CT_RES_CANCEL	'4L'  // request a cancel of before payment.
#define CT_RES_TX_ACK	'4Z'  // ACK 0x06 OR NAK 0x15.

#define CT_RES_IF_ALARM	'4a'  // request a info of alarm.
#define CT_RES_RX_ACK	'4z'  // ACK 0x06 OR NAK 0x15.
#define CT_RES_RX_CARD	'4b'  // Read Card Number.

//// define Response Error..
#define CT_RES_NOERR		(0x00)
#define CT_RES_NOAUTHTER	(0x01)  // don't progress to charge.
#define CT_RES_NOLOGIN		(0x02)
#define CT_RES_CARD_ERR		(0x03)	// don't progress to charge.
#define CT_RES_TIMEOUT		(0x80)
#define CT_RES_SYSFAULT		(0x81)
#define CT_RES_NONETCON		(0x82)  // progress charge, after conection is OK, the non-tx-messages should be sent. 
#define CT_RES_ETCERR		(0xFF)

//// define NAK Reason code.
#define CT_NAK_CRC_ERR		(0x01)  // CRC Error.
#define CT_NAK_AUTH_FAIL	(0x02)
#define CT_NAK_TYPE_ERR		(0x03)
#define CT_NAK_ID_ERR		(0x04)	// don't progress to charge.
#define CT_NAK_STID_ERR		(0x05)
#define CT_NAK_IDX_ERR		(0x06)
#define CT_NAK_LENG_ERR		(0x07)  // progress charge, after conection is OK, the non-tx-messages should be sent. 

// SamWon FA Credite Terminal  End /////////////

unsigned char nPaidType;  // 0x01  pre charge, 0x02 physical charge.

typedef void (*SamwonFaListener)(char*, int size);

void SamwonfaStopMonitoring(void);
void SamwonfaStartMonitoring(SamwonFaListener listener);
bool SamwonCheckTest(void);
void SamwonfaInit(void);
bool SamwonfaCheck(void);
#endif
#endif

