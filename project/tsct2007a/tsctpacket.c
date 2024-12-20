/**
 * @file tsctpacket.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-03-09
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
#include "tsctpacket.h"

unsigned short AlarmCodeBCD[43]={
	0,	0,	0,	0,				// 0, 1, 2, 3
	ALARM_NSCP_CP_POWER_ON,    	// ORDER_ALARM_POWER_ON, 4
	0, 							// 5
	ALARM_NSCP_CP_BISI_START,  	// ORDER_ALARM_BUSI_START 6
	ALARM_NSCP_PROGRAM_DOWN_FINISH, 	// ORDER_ALARM_APP_DOWN_SUCCESS 7
	ALARM_NSCP_PROGRAM_DOWN_FAIL, 		// ORDER_ALARM_APP_DOWN_FAIL 8
	ALARM_NSCP_PROGRAM_UPDATE_SUCCESS, 	// ORDER_ALARM_APP_UPDATE_SUCCESS 9
	ALARM_NSCP_PROGRAM_UPDATE_FAIL, 	// ORDER_ALARM_APP_UPDATE_FAIL 10
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,		//  11, 12, 13, 14, 15, 16, 17, 18, 19, 20
	0, 0,   							//  21, 22 
	ALARM_NSCP_CUSTOMER_INFO_DOWN_SUCCESS, 		// ORDER_ALARM_MEMBER_DOWN_SUCCESS 23
	0,											// 24
	ALARM_NSCP_CUSTOMER_INFO_UPDATE_SUCCESS, 	// ORDER_ALARM_MEMBER_UPDATE_SUCCESS 25
	0,											//  26
	ALARM_NSCP_M2M_PROGRAM_DOWN_START, 			// ORDER_ALARM_M2M_DOWN_START 27
	ALARM_NSCP_M2M_PROGRAM_DOWN_SUCCESS, 		// ORDER_ALARM_M2M_DOWN_SUCCESS 28
	ALARM_NSCP_M2M_PROGRAM_DOWN_FAIL, 			//ORDER_ALARM_M2M_DOWN_FAIL 29
	ALARM_NSCP_M2M_UPDATE_SUCCESS,   			// ORDER_ALARM_M2M_UPDATE_SUCCESS 30
	ALARM_NSCP_M2M_UPDATE_FAIL, 				// ORDER_ALARM_M2M_UPDATE_FAIL 31
	ALARM_NSCP_RF_PROGRAM_DOWN_START, 			// ORDER_ALARM_RF_DOWN_START 32
	ALARM_NSCP_RF_PROGRAM_DOWN_SUCCESS, 		// ORDER_ALARM_RF_DOWN_SUCCESS 33
	ALARM_NSCP_RF_PROGRAM_DOWN_FAIL,  			// ORDER_ALARM_RF_DOWN_FAIL 34
	ALARM_NSCP_RF_UPDATE_SUCCESS,   			// ORDER_ALARM_RF_UPDATE_SUCCESS 35
	ALARM_NSCP_RF_UPDATE_FAIL,   				// ORDER_ALARM_RF_UPDATE_FAIL 36
	0, 0,										// 37, 38
	ALARM_NSCP_M2M_COMM_ERR,   					// ORDER_ALARM_M2M_COMM_ERR 39
	0,											// 40
	ALARM_NSCP_CONNECTOR_DOOR_OPEN, 			// ORDER_ALARM_NSCP_CONNECTOR_DOOR_OPEN 41
	ALARM_NSCP_CONNECTOR_DOOR_CLOSE,  			// ORDER_ALARM_NSCP_CONNECTOR_DOOR_CLOSE 42 	   
};

unsigned short Sever_Event_Code_Arr[] = {
	0xE010,
	0xE020,
	0xE021,
	0xE030,
	0xE040,
	0xE050
};

VD_TEMP vd_temp;

bool bN1_nack;				// True : Send Nack / False : Send ack
bool bF1_nack;				// True : Send Nack / False : Send ack

extern char Security_NO_buf[3];

static short CopyEpqData(int ch)
{
	short idx = 0;

	if(CstGetEpqStatus(CST_SELF_TEST)) // 0x05 waiting mode ; 0x09 charging mode ; 0x11 test mode ; 0x21 error mode ; 
											//0x41 charging complete ; 0x81 emergency mode 
	{
		shmDataAppInfo.eqp_mode[1] = 0x11;
	}
	else if(CstGetEpqStatus(CST_EMG_SW)) 
	{
		shmDataAppInfo.eqp_mode[1] = 0x81;
	}
	else if(CstGetEpqStatus(CST_SELF_TEST)) 
	{
		shmDataAppInfo.eqp_mode[1] = 0x21;	   
	}

	if(theConfig.OperationMode == OP_FREE_MODE)
		shmDataAppInfo.eqp_mode[1] |= 0x80;
	

	idx += CopyDataPacket(&txPacket.vd[idx], shmDataAppInfo.eqp_mode, 2);
	idx += CopyDataPacket(&txPacket.vd[idx], shmDataAppInfo.eqp_status, 8);
	idx += CopyDataPacket(&txPacket.vd[idx], shmDataAppInfo.eqp_watt, 4);

	return idx;
}

void MakeDataCmd_a1(int ch)
{
    short index = CopyEpqData(ch);		

	index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.card_no, 16); 

	int tmp_ch = CstGetUserActiveChannel();

	txPacket.vd[index] = tmp_ch + 1;
	index ++;

	if(tmp_ch == CH1)
		txPacket.vd[index] = 5;
	else
		txPacket.vd[index] = 4;
	index ++;

	ByteOrdertwo(index, txPacket.header.ml);	
}

void MakeDataCmd_b1(int ch)
{
    short index = CopyEpqData(ch);

	//int tmp_ch = CstGetUserActiveChannel();
	
	//tmp_ch + 1;
	txPacket.vd[index] = 0;		index ++;

	/*if(tmp_ch == CH1)
		txPacket.vd[index] = 5;
	else
		txPacket.vd[index] = 4;*/
	txPacket.vd[index] = 0;		index ++;

	ByteOrdertwo(index, txPacket.header.ml);
}

void MakeDataCmd_c1(int ch)
{
    shmDataAppInfo.eqp_mode[1] = 0x00;	// Not Busi Start
	short index = CopyEpqData(ch);	
	char swversion[3], ctrlversion[3];
	
    CurrentDateTime(&txPacket.vd[index]); 	index += 7;

    if(c1_stat == c1_POWERON)
        ByteOrdertwo(AlarmCodeBCD[ORDER_ALARM_POWER_ON], &txPacket.vd[index]);
    else if(c1_stat == c1_BUSIREADY)
        ByteOrdertwo(AlarmCodeBCD[ORDER_ALARM_BUSI_START], &txPacket.vd[index]);

	if(EventCode_buf){
		for(int i=0;i<sizeof(Sever_Event_Code_Arr); i++){
			if(EventCode_buf & (1<<i)){
				ByteOrdertwo(Sever_Event_Code_Arr[i], &txPacket.vd[index]);
				//EventCode_buf &= ~(1<<i); 
				break;
			}
		}
	}

    index += 2;

    txPacket.vd[index] = 0x31; index += 1;
	txPacket.vd[index] = MANUFACTURER_CODE_CP; index += 1;

	swversion[0] = theConfig.applver[1]; swversion[1] = theConfig.applver[3]; swversion[2] = theConfig.applver[5];
	ctrlversion[0] = '0'; ctrlversion[1] = '0'; ctrlversion[2] = '0';

	index += CopyDataPacket(&txPacket.vd[index], swversion, sizeof(swversion));
	index += CopyDataPacket(&txPacket.vd[index], ctrlversion, sizeof(ctrlversion));

	ByteOrdertwo(index, txPacket.header.ml);
}

void MakeDataCmd_d1(int ch)
{		
	shmDataAppInfo.eqp_mode[1] = 0x09;	// 2019.05.27 ktlee
 
   short index = CopyEpqData(ch);

    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.card_no, sizeof(shmDataAppInfo.card_no));

	//0x01 full 0x02 ???????0x03 ???
	txPacket.vd[index] = shmDataAppInfo.charge_request_type;  index ++;
    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.charge_request_watt, sizeof(shmDataAppInfo.charge_request_watt));
    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.charge_request_money, sizeof(shmDataAppInfo.charge_request_money));
	txPacket.vd[index] = shmDataAppInfo.payment_type;    index ++;
	txPacket.vd[index] = shmDataAppInfo.battery_status;		index += 1;
    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.battery_soc, sizeof(shmDataAppInfo.battery_soc));
    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.battery_volume, sizeof(shmDataAppInfo.battery_volume));
    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.battery_watt, sizeof(shmDataAppInfo.battery_watt));
    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.battery_voltage, sizeof(shmDataAppInfo.battery_voltage));

    txPacket.vd[index] = 0x30;index += 1;
    txPacket.vd[index] = 0x30;index += 1;
    txPacket.vd[index] = 0x30;index += 1;
    txPacket.vd[index] = 0x30;index += 1;

    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.charge_comp_expect, sizeof(shmDataAppInfo.charge_comp_expect));
    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.supply_current, sizeof(shmDataAppInfo.supply_current));

	int tmp_ch = CstGetUserActiveChannel();

	txPacket.vd[index] = tmp_ch + 1;
	index ++;

	if(tmp_ch == CH1)
		txPacket.vd[index] = 5;
	else
		txPacket.vd[index] = 4;
	index ++;

	ByteOrdertwo(index, txPacket.header.ml);
	memcpy(shmDataAppInfo.charge_start_time, txPacket.header.send_date, 7 );

	// wait f1
	bSendF1 = false;

	Resete1(INTERVAL_CMD); // after 3sec, send e1
	
	shmDataAppInfo.charge_comp_status = END_NOMAL;	// Set ChargeFinish Status to Default
}

void MakeDataCmd_e1(int ch)
{		
	shmDataAppInfo.eqp_mode[1] = 0x09;	// 2019.05.27 ktlee

	short index = CopyEpqData(ch);		 
	
    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.card_no, 16);

	txPacket.vd[index] = shmDataAppInfo.charge_request_type;  index ++;
	
    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.charge_request_watt, sizeof(shmDataAppInfo.charge_request_watt));
    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.charge_request_money, sizeof(shmDataAppInfo.charge_request_money));

	txPacket.vd[index] = shmDataAppInfo.payment_type;    index ++;

    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.battery_soc, sizeof(shmDataAppInfo.battery_soc));
    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.charge_watt, sizeof(shmDataAppInfo.charge_watt));
    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.charge_money, sizeof(shmDataAppInfo.charge_money));
    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.charge_price, sizeof(shmDataAppInfo.charge_price));

	txPacket.vd[index] = shmDataAppInfo.battery_status;		index += 1;

	for(int i=0;i<4;i++){
		shmDataAppInfo.battery_voltage[i] = (TSCTGetAMIVolt()>>((3-i)*8)) & 0xff;
		shmDataAppInfo.battery_current[i] = (TSCTGetAMICurrent()>>((3-i)*8)) & 0xff;
	}

    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.battery_volume, sizeof(shmDataAppInfo.battery_volume));
    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.battery_watt, sizeof(shmDataAppInfo.battery_watt));
    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.battery_voltage, sizeof(shmDataAppInfo.battery_voltage));
    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.battery_current, sizeof(shmDataAppInfo.battery_current));
    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.battery_temperature, sizeof(shmDataAppInfo.battery_temperature));

    txPacket.vd[index] = 0x30;	index ++;
    txPacket.vd[index] = 0x30;	index ++;
    txPacket.vd[index] = 0x30;	index ++;
    txPacket.vd[index] = 0x30;	index ++;

    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.charge_remain_time, sizeof(shmDataAppInfo.charge_remain_time));
	
	int tmp_ch = CstGetUserActiveChannel();

	txPacket.vd[index] = tmp_ch + 1;
	index ++;

	if(tmp_ch == CH1)
		txPacket.vd[index] = 5;
	else
		txPacket.vd[index] = 4;
	index ++;

	ByteOrdertwo(index, txPacket.header.ml);
}

void MakeDataCmd_f1(int ch)
{
	shmDataAppInfo.eqp_mode[1] = 0x41; // charge success state
	short index = CopyEpqData(ch); 

	bSendF1 = true;

	index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.card_no, sizeof(shmDataAppInfo.card_no));
	index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.charge_watt, sizeof(shmDataAppInfo.charge_watt));
	index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.charge_provide_time, sizeof(shmDataAppInfo.charge_provide_time));
	index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.charge_money, sizeof(shmDataAppInfo.charge_money));
	txPacket.vd[index] = shmDataAppInfo.payment_type;    index ++;
	index += CopyDataPacket(&txPacket.vd[index], &shmDataAppInfo.charge_comp_status, sizeof(shmDataAppInfo.charge_comp_status));
	index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.charge_start_time, 7 );
	//index += CopyDataPacket(&txPacket.vd[index], vd_temp.DB_specific_info_code, sizeof(vd_temp.DB_specific_info_code)); //2014_0828

	int tmp_ch = CstGetUserActiveChannel();

	txPacket.vd[index] = tmp_ch + 1;
	index ++;

	if(tmp_ch == CH1)
		txPacket.vd[index] = 5;
	else
		txPacket.vd[index] = 4;
	index ++;

	ByteOrdertwo(index, txPacket.header.ml);	

	//usleep(300*1000);

	Resetb1(INTERVAL_CMD);	//after 3sec, Send b1
	//MakePacket(0,CMD_b1);
}

void MakeDataCmd_g1(int ch)
{
	bmakepacket_g1 = false;

	shmDataAppInfo.eqp_mode[1] = 0x05;

	short index = CopyEpqData(ch);

	index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.charge_start_time, 7 );
	//index += CopyDataPacket(&txPacket.vd[index], vd_temp.DB_specific_info_code, sizeof(vd_temp.DB_specific_info_code)); //2014_0828

	int tmp_ch = CstGetUserActiveChannel();

	txPacket.vd[index] = tmp_ch + 1;
	index ++;

	if(tmp_ch == CH1)
		txPacket.vd[index] = 5;
	else
		txPacket.vd[index] = 4;
	index ++;

	ByteOrdertwo(index, txPacket.header.ml);
}

void MakeDataCmd_h1(int ch) // Add S-traffic packet field 190308 _dsAn
{

}

void MakeDataCmd_i1(int ch)
{
	short index = CopyEpqData(ch);
	char buf[2] = {0, };
	
#if defined(sTraffic_Server)
	sprintf(buf, "%c%c", 0x00, 0x01 );

	printf("sTrafficNoTxData.size = [%d] \n",sTrafficNoTxData.size);
	memcpy(&txPacket.vd[index], buf , 2);  index +=2;
	memcpy(&txPacket.vd[index],(char *)sTrafficNoTxData.data, sTrafficNoTxData.size);
	index += sTrafficNoTxData.size;
#endif	

	ByteOrdertwo(index, txPacket.header.ml);
}

void MakeDataCmd_j1(int ch)
{
	shmDataAppInfo.eqp_mode[1] = 0x00;	// Not Busi Start
	short index = CopyEpqData(ch);
	if(charge_price_busi == Price_Empty)
		txPacket.vd[index] = Member_Price_OK;
	else if(charge_price_busi == Member_Price_OK)
		txPacket.vd[index] = NoMember_Price_OK; 
	else if(charge_price_busi == NoMember_Price_OK)
		txPacket.vd[index] = VIP_Price_OK; 
	
	index += 1;	

	ByteOrdertwo(index, txPacket.header.ml);
}

void MakeDataCmd_k1()
{
	short index = CopyEpqData(CH1);
	ByteOrdertwo(index, txPacket.header.ml);
}

void MakeDataCmd_l1()
{
	short index = CopyEpqData(CH1);
	ByteOrdertwo(index, txPacket.header.ml);
}

void MakeDataCmd_m1()
{
  //
}

void MakeDataCmd_n1()
{
    int index = 0;
    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.eqp_mode, sizeof(shmDataAppInfo.eqp_mode));
    index += CopyDataPacket(&txPacket.vd[index], shmDataAppInfo.eqp_status, sizeof(shmDataAppInfo.eqp_status));
}

void MakeDataCmd_o1()
{

}

void MakeDataCmd_p1()
{

}

void MakeDataCmd_q1(int ch)
{
	b1q_RcvFlg = false;

	shmDataAppInfo.eqp_mode[1] = 0x00;	// Not Busi Start
	short index = CopyEpqData(ch);

	txPacket.vd[index] = 0x00;
	index ++;	

	txPacket.vd[index] = 0x90; 
	index ++;	

	int tmp_ch = CstGetUserActiveChannel();

	txPacket.vd[index] = tmp_ch + 1;
	index ++;

	if(tmp_ch == CH1)
		txPacket.vd[index] = 5;
	else
		txPacket.vd[index] = 4;
	index ++;

	memcpy(&txPacket.vd[index],Security_NO_buf,2);
	index += 2;

	ByteOrdertwo(index, txPacket.header.ml);
}

void MakeDataCmd_q2(int ch)
{
	shmDataAppInfo.eqp_mode[1] = 0x00;	// Not Busi Start
	short index = CopyEpqData(ch);

	txPacket.vd[index] = QR_Cancel_Code; 
	index ++;

	QR_Cancel_Code = QR_CANCLE_NONE;

	int tmp_ch = CstGetUserActiveChannel();

	txPacket.vd[index] = tmp_ch + 1;
	index ++;

	if(tmp_ch == CH1)
		txPacket.vd[index] = 5;
	else
		txPacket.vd[index] = 4;
	index ++;

	ByteOrdertwo(index, txPacket.header.ml);
}

void MakeDataCmd_r1()
{
	short index = CopyEpqData(CH1);
	ByteOrdertwo(index, txPacket.header.ml);
}

void MakeDataCmd_1C(int ch)
{
   short index = 0;
   unsigned char result_code;
   unsigned char fail_code;

   index = CopyEpqData(ch);

   if(GetHomeLayer()){
		result_code = 0x06;
		fail_code = 0x00;
	}
	else{
		result_code = 0x15;
		fail_code = 0x4f;
	}

	memcpy(&txPacket.vd[index], &result_code, 1);      index++;

	memcpy(&txPacket.vd[index], &fail_code, 1);      index++;

	ByteOrdertwo(index, txPacket.header.ml);
}

void MakeDataCmd_1I(int ch)
{
   short index = 0;
   unsigned char result_code[1];

   txPacket.header.ml[0] = 0x00;
   txPacket.header.ml[1] = 0x10;

	index = CopyEpqData(ch);	

	if(GetHomeLayer()){
		result_code[0] = 0x06;
		reset_yn = true;
	}
	else{
		result_code[0] = 0x15;
		reset_yn = false;
	}
    memcpy(&txPacket.vd[index], result_code, 1); index += 1; // ????코??1????

	result_code[0] = 0x00;
    memcpy(&txPacket.vd[index], result_code, 1); index += 1;	 // ????코??1?????????00	

	ByteOrdertwo(index, txPacket.header.ml);
}

void MakeDataCmd_1K(int ch)
{

   short index = 0;
   unsigned char result_code[1];

   txPacket.header.ml[0] = 0x00;
   txPacket.header.ml[1] = 0x10;

	// to be update
	index = CopyEpqData(ch);	

	if(!checkUpdate())
		result_code[0] = 0x06;
	else
		result_code[0] = 0x15;
		
    memcpy(&txPacket.vd[index], result_code, 1); index += 1;	 // ????코??1?????????00	

	result_code[0] = 0x00;
    memcpy(&txPacket.vd[index], result_code, 1); index += 1; // ????코??1????

	ByteOrdertwo(index, txPacket.header.ml);
}

void MakeDataCmd_1F(int ch)
{
   short index = 0;
   unsigned char result_code;
   unsigned char fail_code;

	// to be update
	index = CopyEpqData(ch);	

	if(!bF1_nack && GetMeterCon() && GetServerCon() && !EmgControl)
	{
		//shmDataIfInfo.card_auth = CARD_AUTH_OK;
		//shmDataAppInfo[ch_type].auth_type[0] = SERVER_AUTH;
		result_code = CTRL_CODE_ACK;   //ACK
		fail_code = 0x00;
		//CstSetUserActiveChannel(ch_type);
		//bDevChannel = ch_type;
	}
	else
	{
		//shmDataIfInfo.card_auth = CARD_AUTH_FAILD;
     	result_code = CTRL_CODE_NAK;   //NAK		
		fail_code = 0x4f;
	}
	bF1_nack = false;

	memcpy(&txPacket.vd[index], &result_code, 1);      				index++;
	
	memcpy(&txPacket.vd[index], &fail_code, 1);      				index++;
		
	for(int i=0; i<16; i++)
		txPacket.vd[index+i] = shmDataAppInfo.card_no[i];
	index += 16;

	txPacket.vd[index] = CstGetUserActiveChannel()+1;          		index++;

	if(CstGetUserActiveChannel())
		txPacket.vd[index] = 4;
	else
		txPacket.vd[index] = 5;   		
	index++;

	ByteOrdertwo(index, txPacket.header.ml);
	
	if(result_code == CTRL_CODE_ACK)      ituLayerGoto(ituSceneFindWidget(&theScene, "CardWaitLayer"));
	result_code = 0x00;
}

void MakeDataCmd_1N(int ch)
{
   short index = 0;
   unsigned char result_code;
   unsigned char fail_code;

	// to be update
	index = CopyEpqData(ch);	

	if(!bN1_nack)
	{
		result_code = 0x06;   //ACK
		fail_code = 0x00;
	}
	// keep charge
	else
	{
     	result_code = 0x15;   //NAK		
		fail_code = 0x4f;
	}	

	memcpy(&txPacket.vd[index], &result_code, 1);      index++;

	result_code = 0x00;
	memcpy(&txPacket.vd[index], &fail_code, 1);      index++;

	ByteOrdertwo(index, txPacket.header.ml);

	// go to stop Charging
	if(!bN1_nack)
		TSCT_ChargingStop();

	bN1_nack = false;
}

void DataProcCmd_1a(int ch)
{
	int index = 0;

	int tmp_ch = CstGetUserActiveChannel();		// for Charger Active Chanel
	
	index += 16;
    
	vd_temp.id_card_certify = rxPacket.vd[index]; 	
	
	index += 15;

	switch(rxPacket.vd[index]){
		case MEM_MEBER : 
			shmDataAppInfo.member_type = MEM_MEBER-1;
			break;
		case MEM_NON :
            shmDataAppInfo.member_type = MEM_NON-1;
			break;
		case MEM_VIP :
            shmDataAppInfo.member_type = MEM_NON-1;
			break;
		default :
            shmDataAppInfo.member_type = MEM_MEBER-1;
			break;
	}

    if(vd_temp.id_card_certify == 0x01) 
    {
		printf("========Debug : [[[true]]] vd_temp[%d].id_card_certify[%d]========== \n", tmp_ch, vd_temp.id_card_certify);
        shmDataIfInfo.card_auth = CARD_AUTH_OK;
		Resetb1(INTERVAL_CMD_b1);
    }
    else
    {
		shmDataAppInfo.app_order = APP_ORDER_AUTH_METHOD;
		printf("========Debug : [[[false]]] vd_temp[%d].id_card_certify[%d]========== \n", tmp_ch, vd_temp.id_card_certify);
		shmDataIfInfo.card_auth = CARD_AUTH_FAILD;
		Resetb1(INTERVAL_CMD);
		//MakePacket(0,CMD_b1);
    }
}

void DataProcCmd_1b(int ch)
{
	int index = 0;
	time_t time = CstGetTime();
	struct tm *tm = localtime(&time);

    vd_temp.common_result_code = rxPacket.vd[index]; 	index ++;
	if(vd_temp.common_result_code == 0x06) //ACK
	{
		tNetTime.tm_hour = CstConvertBCD(rxPacket.header.send_date[4]);
		tNetTime.tm_min = CstConvertBCD(rxPacket.header.send_date[5]);
				 
		if((tNetTime.tm_hour != tm->tm_hour) || (tNetTime.tm_min != tm->tm_min ))
		{	
			tNetTime.tm_year = (CstConvertBCD(rxPacket.header.send_date[0])*100+ \
						  CstConvertBCD(rxPacket.header.send_date[1]))-1900;

			tNetTime.tm_mon = CstConvertBCD(rxPacket.header.send_date[2])-1;
			tNetTime.tm_mday = CstConvertBCD(rxPacket.header.send_date[3]);
			tNetTime.tm_hour = CstConvertBCD(rxPacket.header.send_date[4]);
			tNetTime.tm_min = CstConvertBCD(rxPacket.header.send_date[5]);
			tNetTime.tm_sec = CstConvertBCD(rxPacket.header.send_date[6]);
		
			bNetTimeSync = true;
			CstCheckDayDataDate((unsigned int)((tNetTime.tm_year<<16)|(tNetTime.tm_mon<<8)|(tNetTime.tm_mday)));		
			printf("Client#%d tNetTime.tm_year[%d] tNetTime.tm_mon[%d] tNetTime.tm_mday[%d] tNetTime.tm_hour[%d] tNetTime.tm_min[%d] \n", \
					ch, tNetTime.tm_year,tNetTime.tm_mon,tNetTime.tm_mday,tNetTime.tm_hour,tNetTime.tm_min );			
		}
	}
}	

void DataProcCmd_1c(int ch)
{
    if(c1_stat == c1_POWERON)
        c1_stat = c1_BUSIREADY;
    else if(c1_stat == c1_BUSIREADY)
        c1_stat = c1_BUSISTART;

	struct	tm tt;
	time_t time = CstGetTime();
	struct tm *tm = localtime(&time);
		
	if((tt.tm_hour != tm->tm_hour) || (tt.tm_min != tm->tm_min ))
	{
		tNetTime.tm_year = (CstConvertBCD(rxPacket.header.send_date[0])*100+ \
						CstConvertBCD(rxPacket.header.send_date[1]))-1900;

		tNetTime.tm_mon = CstConvertBCD(rxPacket.header.send_date[2])-1;
		tNetTime.tm_mday = CstConvertBCD(rxPacket.header.send_date[3]);
		tNetTime.tm_hour = CstConvertBCD(rxPacket.header.send_date[4]);
		tNetTime.tm_min = CstConvertBCD(rxPacket.header.send_date[5]);
		tNetTime.tm_sec = CstConvertBCD(rxPacket.header.send_date[6]);
		
		bNetTimeSync = true;
		printf("Client[%d] tNetTime.tm_year[%d] tNetTime.tm_mon[%d] tNetTime.tm_mday[%d] tNetTime.tm_hour[%d] tNetTime.tm_min[%d] \n",\
			ch, tNetTime.tm_year,tNetTime.tm_mon+1,tNetTime.tm_mday,tNetTime.tm_hour,tNetTime.tm_min ); 
	}
	
	if(theConfig.timeinit[0] != 0x32)
	{
		printf("void DataProcCmd_1c() ,void DataProcCmd_1c() reboot \n");
		theConfig.timeinit[0] = 0x32;
		ConfigSave();
	}

	if(EventCode_buf){
		for(int i=0;i<sizeof(Sever_Event_Code_Arr); i++){
			if(EventCode_buf & (1<<i)){
				//ByteOrdertwo(Sever_Event_Code_Arr[i], &txPacket.vd[index]);
				EventCode_buf &= ~(1<<i); 
				break;
			}
		}
	}
}

void DataProcCmd_1d(int ch)
{
    int index = 0;

    vd_temp.common_result_code = rxPacket.vd[index]; 	index ++;
	//index += CopyDataPacket(&vd_temp.DB_specific_info_code[0], &rxPacket.vd[index], sizeof(vd_temp.DB_specific_info_code));
}

void DataProcCmd_1e(int ch)
{
    vd_temp.common_result_code = rxPacket.vd[0];
}

void DataProcCmd_1f(int ch)
{
    vd_temp.common_result_code = rxPacket.vd[0];   
}

void DataProcCmd_1g()
{
    vd_temp.common_result_code = rxPacket.vd[0];
}

void DataProcCmd_1h()
{

}

void DataProcCmd_1i()
{
}

void DataProcCmd_1j(int ch)
{
    int		index = 0, nCnt;
	
    printf("Debug : ch[%d] DataProcCmd_1j [%d]", ch, ByteOrder(rxPacket.header.ml));

	charge_price_busi = rxPacket.vd[index]; index += 1;

	memcpy(shmDataIfInfo.ver_info, &rxPacket.vd[index], sizeof(shmDataIfInfo.ver_info));
    index += sizeof(shmDataIfInfo.ver_info);

    index += CopyDataPacket(&shmDataIfInfo.cp_type, &rxPacket.vd[index], sizeof(shmDataIfInfo.cp_type));
    index += CopyDataPacket(&shmDataIfInfo.apply_date[0][0], &rxPacket.vd[index], sizeof(shmDataIfInfo.apply_date[0]));

	char temp = charge_price_busi - 1;
	for(nCnt = 0; nCnt<24; nCnt++)
	{
    	index += CopyDataPacket(shmDataIfInfo.unit_cost[temp][nCnt], &rxPacket.vd[index], sizeof(shmDataIfInfo.unit_cost[temp][nCnt]));
	}

	
    printf("Debug : ***** ver_info [%02X][%02X][%02X][%02X][%02X][%02X][%02X][%02X]", shmDataIfInfo.ver_info[0], shmDataIfInfo.ver_info[1], shmDataIfInfo.ver_info[2], shmDataIfInfo.ver_info[3], shmDataIfInfo.ver_info[4], shmDataIfInfo.ver_info[5], shmDataIfInfo.ver_info[6], shmDataIfInfo.ver_info[7]);
    printf("Debug : ***** cp_type [%02X]", shmDataIfInfo.cp_type);
    printf("Debug : ***** apply_date [%02X][%02X][%02X][%02X][%02X][%02X][%02X]", shmDataIfInfo.apply_date[0], shmDataIfInfo.apply_date[1], shmDataIfInfo.apply_date[2], shmDataIfInfo.apply_date[3], shmDataIfInfo.apply_date[4], shmDataIfInfo.apply_date[5], shmDataIfInfo.apply_date[6]);

	for(nCnt=0; nCnt<24; nCnt++)
		printf("unit price by hour[%d] : [%04d] \n",nCnt, FourByteOrder(&shmDataIfInfo.unit_cost[temp][nCnt][0]));
   
    if(charge_price_busi != VIP_Price_OK){
        sleep(1);
        MakePacket(ch,CMD_j1);
    } else {
        shmDataIfInfo.connect_status = 1;

        sleep(1);

		Resetb1(INTERVAL_CMD_b1);
		MakePacket(ch,CMD_b1);
    }    
}

void DataProcCmd_1q(int ch)
{
    printf("Debug : ch[%d] DataProcCmd_1q [%d]", ch, ByteOrder(rxPacket.header.ml));

	b1q_RcvFlg = true;

	if(rxPacket.vd[0] != 0x06){
		b1q_RcvFlg = false;
		GotoStartLayer();
	}
}

void DataProcCmd_2q(int ch)
{
    printf("Debug : ch[%d] DataProcCmd_2q [%d]", ch, ByteOrder(rxPacket.header.ml));
}

/*** Server Request*/

void DataProcCmd_C1(int ch)
{		
	short index = 0;

	if(GetHomeLayer()){
		index++;
		if(theConfig.OperationMode == rxPacket.vd[index] & 0x80)
			theConfig.OperationMode = OP_FREE_MODE;
		else
			theConfig.OperationMode = OP_NORMAL_MODE;
		index++;
		theConfig.FreeChargingTime = rxPacket.vd[index];
	}
	
	MakePacket(ch, CMD_1C);

	if(GetHomeLayer())	{

		ConfigSave();

		printf("\n\n soon reset....\n\n");

		usleep(2000*1000);

		custom_reboot();
		
		while(1);
	}
	else
		clearUpdate();	// Set the Upadate Status
}

void DataProcCmd_F1(int ch)
{		
	short index = 0;
	uint8_t ch_type;	
	//bBusiStart[ch] = true;
	
	bQrwait = false;

	for(int i=0; i<16; i++){
		shmDataAppInfo.card_no[i] = rxPacket.vd[i];	
	}
	index += 16;

	//CstSetUserActiveChannel(ch_type);
	//bDevChannel = ch_type;

	if(rxPacket.vd[index] !=(CstGetUserActiveChannel()+1))
		bF1_nack = true;

	index++;		

   	if(rxPacket.vd[index] == 4)
		ch_type = CH2;
	else if(rxPacket.vd[index] == 5)
		ch_type = CH1;
	else{
		printf("\r\nNack cntr type[%d]\r\n", rxPacket.vd[index]);
		ch_type = 0x3;	// None
		bF1_nack = true;
	}
	if(ch_type != CstGetUserActiveChannel())
		bF1_nack = true;

	if(shmDataAppInfo.app_order != APP_ORDER_CARD_READER || shmDataAppInfo.auth_type[0] != SERVER_AUTH){
		printf("\r\nNack Raeson : %d/%d\r\n", shmDataAppInfo.app_order, shmDataAppInfo.auth_type[0]);
		bF1_nack = true;
	}
	index++;

	if(memcmp(&rxPacket.vd[index],Security_NO_buf,2))
		bF1_nack = true;
		
	//printf("ch[%d]  DataProcCmd_F1.. \n", ch);		

	MakePacket(ch, CMD_1F);
}

void DataProcCmd_K1(int ch)
{		
	short index = 0;
	short ftp_upmode = 0x00;
	
	printf("ch[%d]  DataProcCmd_K1.. \n", ch);

	// Charger Update Code

	index++;

	// Charger Update
	ftp_upmode = rxPacket.vd[index];
	index++;

	// FTP Server URL
	short tmp_index = 0;
	//if(rxPacket.vd[index] > 0x40) 		// domain
	//{
		for(;(rxPacket.vd[index + tmp_index] != NULL) && (tmp_index < 40); tmp_index++) 
		{
			theConfig.ftpDns[tmp_index] = rxPacket.vd[index + tmp_index];
		}
		theConfig.ftpDns[tmp_index] = NULL;
	//}
	/*else if (rxPacket.vd[index] < 0x3a)	// ipNumber
	{
		for(;(rxPacket.vd[index + tmp_index] != NULL) && (tmp_index < 40); tmp_index++) 
		{
			theConfig.ftpDns[tmp_index] = rxPacket.vd[index + tmp_index];
		}
		theConfig.ftpDns[tmp_index] = NULL;
	}*/
	printf("Receive FTP Server URL : %s\r\n", theConfig.ftpDns);
	index+=40;

	// FTP Server Port
	tmp_index = 0;
	theConfig.ftpPort[tmp_index] = rxPacket.vd[index];
	tmp_index++; index++;
	theConfig.ftpPort[tmp_index] = rxPacket.vd[index];
	tmp_index++; index++;
	printf("Receive FTP Server Port : %s\r\n", theConfig.ftpPort);

	// FTP User ID
	tmp_index = 0;
	for(;tmp_index < 10; tmp_index++)
		theConfig.ftpId[tmp_index] = rxPacket.vd[index+tmp_index];
	printf("Receive FTP Server ID : %s\r\n", theConfig.ftpId);
	index+=10;

	// FTP User PWD
	tmp_index = 0;
	for(;tmp_index < 16; tmp_index++)
		theConfig.ftpPw[tmp_index] = rxPacket.vd[index+tmp_index];
	printf("Receive FTP Server PW : %s\r\n", theConfig.ftpPw);
	index+=16;
	
	// FTP File Path
	for(tmp_index = 0;tmp_index < 100; tmp_index++)
		theConfig.ftpPath[tmp_index] = rxPacket.vd[index+tmp_index];
	printf("Receive FTP Server path : %s\r\n", theConfig.ftpPath);
	index+=100;

	if(!GetHomeLayer())
		ftp_upmode = 0x00;

	switch(ftp_upmode)
	{
		case 0x01 : // Update Charger FW
			FtpFwUpdate_func();
			break;
		case 0x11 : // Update QR Join Memmber
		case 0x12 : // Update QR Non Member
		default : 
			break;
	}

	MakePacket(ch, CMD_1K);

	if(!checkUpdate())	{
		printf("\n\n soon reset....\n\n");

		usleep(2000*1000);

		custom_reboot();
		
		while(1);
	}
	else
		clearUpdate();	// Set the Upadate Status
}

void DataProcCmd_I1(int ch)
{		
	short index = 0;
//	bBusiStart[ch] = true;
	
	printf("ch[%d]  DataProcCmd_I1.. \n", ch);
	
	txPacket.vd[index] = rxPacket.vd;		

	MakePacket(ch, CMD_1I);
}

void DataProcCmd_N1(int ch)
{		
	unsigned char memcard_no[16];

	ChannelType activeCh = CstGetUserActiveChannel();

	memcpy(memcard_no, rxPacket.vd,16);

	// Check Charging
	// Check Member Card Number
	if(shmDataAppInfo.app_order == APP_ORDER_CHARGING \
	 && !memcmp(memcard_no, shmDataAppInfo.card_no, 16))
	{
		// response Ack and stop charging
		bN1_nack = false;
	}
	else
	{
		// response Nack and keeep charge
		bN1_nack = true;
	}
	// 
	MakePacket(ch, CMD_1N);
}