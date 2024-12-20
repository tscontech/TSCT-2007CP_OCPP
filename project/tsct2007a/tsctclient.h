/**
*       @file
*               tsctclient.h
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2018.01.05 <br>
*               author: bmlee <br>
*               description: <br>
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


#define	MANUFACTURER_CODE_CP							'T'
#define	MANUFACTURER_CODE_M2M							'L'
#define	MANUFACTURER_CODE_RF							'V'

// �������� �ε���
#define	VERSION_INDEX_CP_APPLICATION					0
#define	VERSION_INDEX_SOUND								1
#define	VERSION_INDEX_TEXT								2
#define	VERSION_INDEX_UI								3
#define	VERSION_INDEX_CP_PARAMETER						4
#define	VERSION_INDEX_BASIC_COST						5
#define	VERSION_INDEX_ENCRYPT_KEY						6
#define	VERSION_INDEX_SELL_COST							7
#define	VERSION_INDEX_M2M_MODEM							8
#define	VERSION_INDEX_RF_PAD							9
#define	VERSION_INDEX_MEMBER							10

// �����ý��ۿ��� ������� Request�ϴ� ���

// Alarm Code ( BCD )
#define ALARM_NSCP_CP_POWER_ON                          0x1000    //������ Power ON
#define ALARM_NSCP_CP_BISI_START                        0x1001    //������ ����
#define ALARM_NSCP_CP_BISI_END                          0x1002    //
#define ALARM_NSCP_CP_NO_UPDATE                         0x1003    //
#define ALARM_NSCP_FRONT_DOOR_OPEN                      0x1004    //Front door ����
#define ALARM_NSCP_READ_DOOR_OPEN                       0x1005    //Rear door ����
#define ALARM_NSCP_SIDE_DOOR_OPEN                       0x1006    //Side door ����
#define ALARM_NSCP_INPUT_OVER_VOLT                      0x1007    //�Է� ������
#define ALARM_NSCP_INPUT_OVER_AMPERE                    0x1008    //�Է� ������
#define ALARM_NSCP_OUTPUT_OVER_VOLT                     0x1009    //��� ������
#define ALARM_NSCP_OUTPUT_OVER_AMPERE                   0x1010    //��� ������
#define ALARM_NSCP_OUTPUT_SHORT                         0x1011    //��� �ܶ�
#define ALARM_NSCP_OUTPUT_CONNECT                       0x1012    //��� ����
#define ALARM_NSCP_EMG_SW_ON                            0x1013    //����ư �۵�
#define ALARM_NSCP_FLOOR_SENSOR_ERR                     0x1014    //ħ�� ���� �̻�
#define ALARM_NSCP_DECLINE_ERR                          0x1015    //���� ���� �̻�
#define ALARM_NSCP_CRC_ERR                              0x1016    //CRC ����
#define ALARM_NSCP_2017                                 0x1017    //
#define ALARM_NSCP_PACKET_TIMEOUT_SERVER                0x1018    //Packet Time Out(��ý��۰� �����Ⱓ)
#define ALARM_NSCP_PACKET_TIMEOUT_M2MD                  0x1019    //Packet Time Out(������� M2MD��)
#define ALARM_NSCP_2020                                 0x1020    //
#define ALARM_NSCP_CONNECTOR_DOOR_OPEN                  0x1021    // Ŀ���� door open
#define ALARM_NSCP_CONNECTOR_DOOR_CLOSE                 0x1022    // Ŀ���� door close
#define ALARM_NSCP_M2M_COMM_ERR                         0x1023    // m2m ��Ŵܸ��� ��� ����
#define ALARM_NSCP_BASIC_COST_DOWN_FINISH               0x1024    // �⺻�ܰ����� ���� �Ϸ�
#define ALARM_NSCP_BASIC_COST_DOWN_FAIL                 0x1025    // �⺻�ܰ����� ���� ����
#define ALARM_NSCP_SELL_COST_DOWN_FINISH                0x1026    // �ǸŴܰ����� ���� �Ϸ�
#define ALARM_NSCP_SELL_COST_DOWN_FAIL                  0x1027    // �ǸŴܰ����� ���� ����
#define ALARM_NSCP_PROGRAM_DOWN_START       			0x1028    // ���α׷� �ٿ�ε� ���� ����
#define ALARM_NSCP_PROGRAM_DOWN_FINISH              	0x1029    // ���α׷� �ٿ�ε� ���� �Ϸ�
#define ALARM_NSCP_PROGRAM_DOWN_FAIL        		    0x1030    // ���α׷� �ٿ�ε� ���� ����
#define ALARM_NSCP_PROGRAM_UPDATE_SUCCESS               0x1031    // ���α׷� UPDATE ����
#define ALARM_NSCP_PROGRAM_UPDATE_FAIL                  0x1032    // ���α׷� UPDATE ����
#define ALARM_NSCP_SOUND_DOWN_START                     0x1033    // �������� �ٿ�ε� ���� ����
#define ALARM_NSCP_SOUND_DOWN_FINISH                    0x1034    // �������� �ٿ�ε� ���� �Ϸ�
#define ALARM_NSCP_SOUND_DOWN_FAIL                      0x1035    // �������� �ٿ�ε� ���� ����
#define ALARM_NSCP_SOUND_UPDATE_SUCCESS                 0x1036    // �������� UPDATE ����
#define ALARM_NSCP_SOUND_UPDATE_FAIL                    0x1037    // �������� UPDATE ����
#define ALARM_NSCP_TEXT_START                           0x1038    // �������� �ٿ�ε� ���� ����
#define ALARM_NSCP_TEXT_DOWN_FINISH                     0x1039    // �������� �ٿ�ε� ���� �Ϸ�
#define ALARM_NSCP_TEXT_DOWN_FAIL                       0x1040    // �������� �ٿ�ε� ���� ����
#define ALARM_NSCP_TEXT_UPDATE_SUCCESS                  0x1041    // �������� UPDATE ����
#define ALARM_NSCP_TEXT_UPDATE_FAIL                     0x1042    // �������� UPDATE ����
#define ALARM_NSCP_UI_START                             0x1043    // UI �ٿ�ε� ���� ����
#define ALARM_NSCP_UI_DOWN_FINISH                       0x1044    // UI �ٿ�ε� ���� �Ϸ�
#define ALARM_NSCP_UI_DOWN_FAIL                         0x1045    // UI �ٿ�ε� ���� ����
#define ALARM_NSCP_UI_UPDATE_SUCCESS                    0x1046    // UI UPDATE ����
#define ALARM_NSCP_UI_UPDATE_FAIL                       0x1047    // UI UPDATE ����
#define ALARM_NSCP_CHARGER_TYPE_ERROR                   0x1048    // ������ Type ����
#define ALARM_NSCP_CHARGER_ID_ERROR                     0x1049    // ������ ID ����
#define ALARM_NSCP_CHARGER_STATION_ID_ERROR             0x1050    // ������ ID ����
#define ALARM_NSCP_CUSTOMER_INFO_DOWN_START             0x1051    // ȸ������ �ٿ� ����
#define ALARM_NSCP_CUSTOMER_INFO_DOWN_SUCCESS           0x1052    // ȸ������ �ٿ� ����
#define ALARM_NSCP_CUSTOMER_INFO_DOWN_FAIL              0x1053    // ȸ������ �ٿ� ����
#define ALARM_NSCP_CUSTOMER_INFO_UPDATE_SUCCESS         0x1054    // ȸ������ ������Ʈ ����
#define ALARM_NSCP_CUSTOMER_INFO_UPDATE_FAIL            0x1055    // ȸ������ ������Ʈ ����
#define ALARM_NSCP_NON_TRANS_PACKET_SEND_FAIL           0x1056    // ������ ���� ���� ����
#define ALARM_NSCP_MESSAGE_RESEND_RESPONSE_FAIL         0x1057    // �ڷ� �� ���� ��û ����
#define ALARM_NSCP_VERSION_MATCH_OK                     0x1058    // ������ ��ġ��
#define ALARM_NSCP_M2M_PROGRAM_DOWN_START               0x1059    // ���α׷� �ٿ�ε� ���� ����(M2M)
#define ALARM_NSCP_M2M_PROGRAM_DOWN_SUCCESS             0x1060    // ���α׷� �ٿ�ε� ���� �Ϸ�(M2M)
#define ALARM_NSCP_M2M_PROGRAM_DOWN_FAIL                0x1061    // ���α׷� �ٿ�ε� ���� ����(M2M)
#define ALARM_NSCP_M2M_UPDATE_SUCCESS              		0x1062    // ���α׷� ������Ʈ ����(M2M)
#define ALARM_NSCP_M2M_UPDATE_FAIL                 		0x1063    // ���α׷� ������Ʈ ����(M2M)
#define ALARM_NSCP_RF_PROGRAM_DOWN_START                0x1064    // ���α׷� �ٿ�ε� ���� ����(RF)
#define ALARM_NSCP_RF_PROGRAM_DOWN_SUCCESS              0x1065    // ���α׷� �ٿ�ε� ���� �Ϸ�(RF)
#define ALARM_NSCP_RF_PROGRAM_DOWN_FAIL                 0x1066    // ���α׷� �ٿ�ε� ���� ����(RF)
#define ALARM_NSCP_RF_UPDATE_SUCCESS              		0x1067    // ���α׷� ������Ʈ ����(RF)
#define ALARM_NSCP_RF_UPDATE_FAIL                 		0x1068    // ���α׷� ������Ʈ ����(RF)
#define ALARM_NSCP_M2M_MODEM_POWER_ON                   0x1069    // M2M��Ŵܸ��� POWER ON
#define ALARM_NSCP_RF_READER_POWER_ON                   0x1070    // RFī��ܸ��� POWER ON
#define ALARM_NSCP_RF_READER_COMM_ERROR                 0x1071    // RFī��ܸ��� ��� ����

#define ORDER_CP_INSTALL_INFO                           0       //������ ��ġ ���� ����
#define ORDER_DEFAULT_COST_REQUEST                      1       //�⺻�ܰ� ���� ��û
#define ORDER_CP_PARAM_REQUEST                          2       //������ ��Ķ���� ��û
#define ORDER_ENCODE_KEY_REQUEST                        3       //��ȣȭ key ��û
#define ORDER_ALARM_POWER_ON                            4       //Power ON �˶� ����(c1)
#define ORDER_REQUEST_VER_INFO                          5       //�������� ��û(r1)
#define ORDER_ALARM_BUSI_START                          6       //�������� �˶� ����(c1)
#define	ORDER_ALARM_APP_DOWN_SUCCESS					7		// ������ ���ø����̼� �ٿ�ε� �Ϸ� �˶�
#define	ORDER_ALARM_APP_DOWN_FAIL						8		// ������ ���ø����̼� �ٿ�ε� ���� �˶�
#define	ORDER_ALARM_APP_UPDATE_SUCCESS					9		// ������ ���ø����̼� ������Ʈ �Ϸ� �˶�
#define	ORDER_ALARM_APP_UPDATE_FAIL						10		// ������ ���ø����̼� ������Ʈ ���� �˶�
#define	ORDER_ALARM_IMAGE_DOWN_SUCCESS					11		// IMAGE �ٿ�ε� �Ϸ� �˶�
#define	ORDER_ALARM_IMAGE_DOWN_FAIL						12		// IMAGE �ٿ�ε� ���� �˶�
#define	ORDER_ALARM_IMAGE_UPDATE_SUCCESS				13		// IMAGE ������Ʈ �Ϸ� �˶�
#define	ORDER_ALARM_IMAGE_UPDATE_FAIL					14		// IMAGE ������Ʈ ���� �˶�
#define	ORDER_ALARM_SOUND_DOWN_SUCCESS					15		// SOUND �ٿ�ε� �Ϸ� �˶�
#define	ORDER_ALARM_SOUND_DOWN_FAIL						16		// SOUND �ٿ�ε� ���� �˶�
#define	ORDER_ALARM_SOUND_UPDATE_SUCCESS				17		// SOUND ������Ʈ �Ϸ� �˶�
#define	ORDER_ALARM_SOUND_UPDATE_FAIL					18		// SOUND ������Ʈ ���� �˶�
#define	ORDER_ALARM_TEXT_DOWN_SUCCESS					19		// TEXT �ٿ�ε� �Ϸ� �˶�
#define	ORDER_ALARM_TEXT_DOWN_FAIL						20		// TEXT �ٿ�ε� ���� �˶�
#define	ORDER_ALARM_TEXT_UPDATE_SUCCESS					21		// TEXT ������Ʈ �Ϸ� �˶�
#define	ORDER_ALARM_TEXT_UPDATE_FAIL					22		// TEXT ������Ʈ ���� �˶�
#define	ORDER_ALARM_MEMBER_DOWN_SUCCESS					23		// MEMBER �ٿ�ε� �Ϸ� �˶�
#define	ORDER_ALARM_MEMBER_DOWN_FAIL					24		// MEMBER �ٿ�ε� ���� �˶�
#define	ORDER_ALARM_MEMBER_UPDATE_SUCCESS				25		// MEMBER ������Ʈ �Ϸ� �˶�
#define	ORDER_ALARM_MEMBER_UPDATE_FAIL					26		// MEMBER ������Ʈ ���� �˶�
#define	ORDER_ALARM_M2M_DOWN_START					    27		// M2M F/W �ٿ�ε� ���� �˶�
#define	ORDER_ALARM_M2M_DOWN_SUCCESS					28		// M2M F/W �ٿ�ε� �Ϸ� �˶�
#define	ORDER_ALARM_M2M_DOWN_FAIL						29		// M2M F/W �ٿ�ε� ���� �˶�
#define	ORDER_ALARM_M2M_UPDATE_SUCCESS					30		// M2M F/W ������Ʈ �Ϸ� �˶�
#define	ORDER_ALARM_M2M_UPDATE_FAIL						31		// M2M F/W ������Ʈ ���� �˶�
#define	ORDER_ALARM_RF_DOWN_START						32		// RF �ܸ��� F/W �ٿ�ε� ���� �˶�
#define	ORDER_ALARM_RF_DOWN_SUCCESS						33		// RF �ܸ��� F/W �ٿ�ε� �Ϸ� �˶�
#define	ORDER_ALARM_RF_DOWN_FAIL						34		// RF �ܸ��� F/W �ٿ�ε� ���� �˶�
#define	ORDER_ALARM_RF_UPDATE_SUCCESS					35		// RF �ܸ��� F/W ������Ʈ �Ϸ� �˶�
#define	ORDER_ALARM_RF_UPDATE_FAIL						36		// RF �ܸ��� F/W ������Ʈ ���� �˶�
#define	ORDER_MANAGER_SERVER_CONNECT					37		// M2M�� ����� IP & Port ���� �� ����(CMD_E5)
#define	ORDER_ALARM_NOT_DEFINE_ERROR					38		// �������� �԰ݿ� ���ǵ��� ���� �˶�����
#define	ORDER_ALARM_M2M_COMM_ERR						39		// M2M �� ��ſ���
#define	ORDER_ALARM_M2M_KT_M2MG_CONNECT_OK				40		// KT�� �� ����
#define	ORDER_ALARM_NSCP_CONNECTOR_DOOR_OPEN			41		// Ŀ���� door open
#define	ORDER_ALARM_NSCP_CONNECTOR_DOOR_CLOSE			42		// Ŀ���� door close

#define	VERSION_INIT_VALUE								"P000000a"			// ���� ���������� ������� �ʱⰪ
#define	VERSION_COUNT									11					// ���� �����ϴ� ���α׷� ����

#define INTERVAL_CMD									1000*3*1		// 3SEC
#define INTERVAL_CMD_b1                                 1000*60*3 	 //1��
#define INTERVAL_CMD_e1                                 1000*60*3  //1��
#define INTERVAL_CMD_b2                                 1000*60*3 	 //1��
#define INTERVAL_CMD_e2                                 1000*60*3  //1��

// Protocol Control Code
#define CTRL_CODE_SOH                                   0x01    // Start of Header
#define CTRL_CODE_STX                                   0x02    // Start of Text
#define CTRL_CODE_ETX                                   0x03    // End of Text
#define CTRL_CODE_EOT                                   0x04    // End of Transmission
#define CTRL_CODE_ACK                                   0x06    // ACK
#define CTRL_CODE_NAK                                   0x15    // NAK

typedef struct 
{
	unsigned char code;					// ������ ���� �ڵ�
	unsigned char yy[2];				// ��
	unsigned char mm[2];				// ��
	unsigned char dd[2];				// ��
	unsigned char change;				// ���Ϻ������
}VERSION_INFO;

typedef struct 
{
    unsigned char D1_Data_Index[2];
    unsigned char D1_Data_Flag;
    unsigned char P1_Data_Flag;

    unsigned char zz_event_code[2];                     //?�애 ?�벤?�코??

	unsigned char id_card_certify;                      // RFID Result Code

    unsigned char bcd_date[7];							// ?�자?�??
    unsigned char antenna_recv_degree;                  //?�테???�신감도
    unsigned char common_result_code;                   //범용 결과코드

	unsigned char DB_specific_info_code[2];				//db 고유?�보 코드 //2014_0828	

	char	program_mode;				// charger = 0x01, m2m = 0x02, rf = 0x03
	char	program_mode_sub;			// main program=0x01, UI=0x02, sound=0x03, text=0x04, member=0x05
	char	file_name[12];				// main & module program update filename
	char	file_size[9];				// main & module program update filesize
	short	file_block_index;			// old block index
	char	program_version[8];			// program version

	char   application_ver[8];
	char   basic_cost_ver[8];
	char   encryptkey_ver[8];
	char   m2m_ver[8];
	char   member_ver[8];
	char   parameter_ver[8];
	char   rf_ver[8];
	char   sell_cost_ver[8];
	char   sound_ver[8];
	char   text_ver[8];
	char   ui_ver[8];
    VERSION_INFO ver_info[VERSION_COUNT];		// 0=Main App, 1=?�성?�보, 2=문자?�보, 3=UI App, 4=?�영 ?�라미터, 5=기본 ?��? ?�금, 6=?�호???? 7=?�매 ?��? ?�금, 8=M2M, 9=RF

	unsigned char P1_start_time[7];                      //D1 packet ?�작?�자
    unsigned char P1_end_time[7];                         //D1 packet 종료?�자

    char C1_mode[2];
}VD_TEMP;

typedef enum {
	END_NONE = 0,
	END_NOMAL,
	END_BTN,
	END_EMG,
	END_UNPLUG,
	END_ERR,
	END_SERVER,
	END_CAR
} CHARGE_END_CODE;

typedef enum {
	QR_CANCLE_NONE = 0,
	QR_CANCLE_TIMEOUT,
	QR_CANCLE_BTN
}QR_CANCLE_CODE;

typedef enum {
	EVE_OVC = 0,
	EVE_OVV,
	EVE_UDV,
	EVE_RF,
	EVE_AMI,
	EVE_TOUCH
}SERVER_EVENT_CODE;


unsigned short EventCode_buf;

bool EventReportFlg;

bool bConnect;									// True : Socket Connect / False : Socket Close
uint16_t SEVER_DISCON_CNT;
bool bmakepacket_g1;							// True : Disconnect Layer -> Main Layer / False : Send g1 Packet
QR_CANCLE_CODE QR_Cancel_Code;
bool bQrwait;       // true : QR wait / false : idle
bool b1q_RcvFlg;	//

void ClientInit(void);
void WsClientInit(void);
void ClientExit(void);
bool CheckClientInit();

#endif
