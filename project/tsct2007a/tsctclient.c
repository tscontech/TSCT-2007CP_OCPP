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
//#include "tsctclient.h"
#include "tsctcommon.h"
#include "tsctcfg.h"
#include "ctrlboard.h"
#include "tsctpacket.h"

//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------

#define MAX_SERVERBUF_SIZE 			4096
#define MIN_SERVERBUF_SIZE 			0

#define MAX_DISCON_CNT_CHARGING 	29
#define MAX_DISCON_CNT_IDLE		 	4

//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------

struct timeval tvnet, tvres, tvb1, tve1;
unsigned long b1_timout_msec, e1_timout_msec, res_timout_msec, net_timout_min;
bool bE1TimeOut, bB1TimeOut, bresTimeOut, bnetTimeOut;

unsigned short SEQNO = 0;				// Packet Serial Number

uint8_t bFirst; 						// Power On Wait

uint8_t nAppOldOrder;					// Charger Old Status 

unsigned char send_buf[TSCT_BUFSIZE];
unsigned char recv_buf1[TSCT_BUFSIZE];

bool Socket_timeout=false;		// True : time out Count On / False : time out Count Off

int gSockfd = -1;				// TCPIP Socket 

pthread_t sClientTask = 0;

extern bool I1_reset_boot;

//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------
bool Parser(int nCh, const unsigned char* recv_buf, const int length);

/**
 * @brief Synchronize RTC Time with Sever, When Receive PACKET(1b) from Server
 * 
 */
static void CstCheckNetTimeSet(void)
{
	if(bNetTimeSync)
	{
		if(bGloAdminStatus)	return;  // when don't be a management menu.

		if(tNetTime.tm_year > 0)
		{
			long tNetRtcSec = 0;				
			tNetRtcSec = mktime((struct tm*)&tNetTime);
			itpRtcSetTime1(tNetRtcSec,0);					
			bNetTimeSync = false;				
			tNetTime.tm_year = 0;		
		}
	}	
}

static void ProtocolInit(void)
{	
	shmDataIfInfo.connect_status = 0;
	SEVER_DISCON_CNT = 0;
	charge_price_busi = Price_Empty;
	Resetb1(1000*60);
	//bFirst[ch] = true;
	c1_stat = c1_BUSIREADY;
	bBlocking = bBlcoking_Rec_OK;	

	NetworkReset();

	CtLogBlue("Protocol Init!!!!!");
	
	if(shmDataAppInfo.app_order == APP_ORDER_CHARGING)
		TSCT_ChargingStop();
	else if(shmDataAppInfo.app_order == APP_ORDER_CHARGING \
	 || shmDataAppInfo.app_order == APP_ORDER_CONNECTOR_SELECT \ 
	 || shmDataAppInfo.app_order == APP_ORDER_CHARGE_READY)
		GotoStartLayer();
}

static void CstCheckUserTimeSet(void)
{		
	if(timeSelectControl_ && !bGloAdminStatus)
	{			
		struct	tm tt;
		long rtcSec = 0;

		timeSelectControl_ = false; 			

		tt.tm_year = (CstConvertBCD(admintimeStamp[0])*100+ \
		CstConvertBCD(admintimeStamp[1]))-1900;

		tt.tm_mon = CstConvertBCD(admintimeStamp[2])-1;
		tt.tm_mday = CstConvertBCD(admintimeStamp[3]);
		tt.tm_hour = CstConvertBCD(admintimeStamp[4]);
		tt.tm_min = CstConvertBCD(admintimeStamp[5]);
		tt.tm_sec = CstConvertBCD(admintimeStamp[6]);

		rtcSec = mktime((struct tm*)&tt);
		itpRtcSetTime1(rtcSec,0);				
	}
}

static bool CstSameF1Packet(void)
{
	int beforeCnt = 0;

	if( gChargedDataFrame.nCount % 12 == 0 )    beforeCnt = 11;
	else 						beforeCnt = gChargedDataFrame.nCount % 12 -1;
	
	if(strcmp(gChargedDataFrame.nData[beforeCnt].startTime, (char *)shmDataAppInfo.charge_start_time) == 0 )		
		return true;

	return false;
}

short CopyDataPacket(char *pDest, char *pSrc, short len)
{
	if(len > 0)
	{
	    memcpy(pDest, pSrc, len); 
		return len;
	}
	else
	{
		printf("CopyDataPacket :: Data length Zero \n", len);
		return 0;
	}
}



static bool ResetNet(const int min)
{
	bnetTimeOut = false;
	net_timout_min = min;

	gettimeofday(&tvnet, NULL);
	printf("ResetNet :: %d \n", bnetTimeOut);
}

static bool TimeOutNet(void)
{
	struct timeval tvnet2;
	unsigned long t1 = 0;

    if( (net_timout_min < 1) || (bnetTimeOut) )        return false;
    
	t1 = tvnet.tv_sec;
	
    gettimeofday(&tvnet2, NULL);
    unsigned long t2 = tvnet2.tv_sec;

    if(t2 - t1 > (unsigned long)(net_timout_min * 60))
    {
        bnetTimeOut = true;
        return true;
    }/*
    else if(t2 - t1 < 0)
    {
        printf("\n Debug : ***** Dump : TimeOutRes ??? \n");
        bnetTimeOut = true;
        return true;
    }*/

    return false;
}

static bool ResetRespon(const int msec)
{
	bresTimeOut = false;
	res_timout_msec = msec;

	gettimeofday(&tvres, NULL);
	printf("ResetRes :: %d \n", bresTimeOut);
}

static bool TimeOutRespon(void)
{
	struct timeval tvres2;
	unsigned long t1 = 0;

    if( (res_timout_msec < 1) || (bresTimeOut) )        return false;
    
	t1 = tvres.tv_sec;
	
    gettimeofday(&tvres2, NULL);
    unsigned long t2 = tvres2.tv_sec;

    if(t2 - t1 > (unsigned long)(res_timout_msec/1000))
    {
        bresTimeOut = true;
        return true;
    }
    else if(t2 - t1 < 0)
    {
        printf("\n Debug : ***** Dump : TimeOutRes ??? \n");
        bresTimeOut = true;
        return true;
    }

    return false;
}

void Resetb1(const int msec)
{
	bB1TimeOut = false;
	b1_timout_msec = msec;

	gettimeofday(&tvb1, NULL);	
		
	printf("Resetb1 :: %d \n", bB1TimeOut);
}

static bool TimeOutb1(void)
{
	struct timeval tvb2;
	unsigned long t1 = 0;

    if( (b1_timout_msec < 1) || (bB1TimeOut) )        return false;
    
	t1 = tvb1.tv_sec;
	
    gettimeofday(&tvb2, NULL);
    unsigned long t2 = tvb2.tv_sec;

    if(t2 - t1 > (unsigned long)(b1_timout_msec/1000))
    {
        bB1TimeOut = true;
        return true;
    }
    else if(t2 - t1 < 0)
    {
        printf("\n Debug : ***** Dump : TimeOutb1 ??? \n");
        bB1TimeOut = true;
        return true;
    }

    return false;
}

void Resete1(const int msec)
{
	bE1TimeOut = false;
	e1_timout_msec = msec;

	gettimeofday(&tve1, NULL);
}

static bool TimeOute1(void)
{
	struct timeval tve2;  
	unsigned long t1 = 0;

    if( (e1_timout_msec < 1) || (bE1TimeOut) )        return false;    

	t1 = tve1.tv_sec;
	
    gettimeofday(&tve2, NULL);
    unsigned long t2 = tve2.tv_sec;

    if(t2 - t1 > (unsigned long)(e1_timout_msec/1000))
    {
        bE1TimeOut = true;
        return true;
    }
    else if(t2 - t1 < 0)
    {
        printf("\n Debug : TimeOute1 ??? \n");
        bE1TimeOut = true;
        return true;
    }
    return false;
}

static void I1_reset(void)
{
	I1_Reset_Set();
	printf("\n\n soon reset....\n\n");
	
	sleep(5);

	TSCT_NetworkExit();

	printf("end....\n");
	custom_reboot();
    
	while(1);
}

/**
 * @brief Make 2byte buf to value
 * 
 * @param value 
 * @param buf 
 */
void ByteOrdertwo(const unsigned short value, unsigned char* buf)
{
	bool big_endian= true;
    if(!buf)
    {
        return;
    }

    if(big_endian)
    {
        buf[0] = (unsigned char)(value >> 8);
        buf[1] = (unsigned char)(value);
    }
    else
    {
        buf[0] = (unsigned char)(value);
        buf[1] = (unsigned char)(value >> 8);
    }
}

/**
 * @brief Return 2byte buf
 * 
 * @param buf 
 * @return unsigned short 
 */
unsigned short ByteOrder(unsigned char* buf )
{
    unsigned short ret = 0;
	bool big_endian= true;
    if(big_endian)
    {
        ret = buf[0] << 8;
        ret |= buf[1];
    }
    else
    {
        ret = buf[0];
        ret |= buf[1] << 8;
    }

    return ret;
}


void MakeHeader(int ch)
{
    if(SEQNO > 9999)
    {
        SEQNO = 0;
    }
    else
    {
        SEQNO++;
    }

    CurrentDateTime(txPacket.header.send_date);
    ByteOrdertwo(SEQNO, txPacket.header.sno);
    txPacket.header.cp_type = shmDataAppInfo.charger_type;
	//txPacket.header.cp_temp = 0xff;
    memcpy(txPacket.header.cp_group_id, shmDataAppInfo.charger_station_id, sizeof(shmDataAppInfo.charger_station_id));
    memcpy(txPacket.header.cp_id, shmDataAppInfo.charger_id, sizeof(shmDataAppInfo.charger_id));

    shmDataIfInfo.sno = SEQNO;
}

static short CstCopyByte(char *pDst, char *pSrc, int length)
{
	int i;
	short result = 0;
	
	if((pDst==NULL)||(pSrc==NULL)||(length < 1))
		return result;
	
    for( i=0; i<length; i++)
    {
       pDst[i] = pSrc[i];	   
    }
	
	result = i;
	
	return result;
}

static void NetWrite(int ch, const bool response)
{
	int index = 0;
	unsigned int n;

    memset(send_buf, 0x00, sizeof(send_buf));

    send_buf[index++] = txPacket.stx;

	index += CstCopyByte(&send_buf[index], txPacket.header.send_date, sizeof(txPacket.header.send_date));
	index += CstCopyByte(&send_buf[index], txPacket.header.sno, sizeof(txPacket.header.sno));

    send_buf[index++] = txPacket.header.cp_type;
	//send_buf[index++] = txPacket.header.cp_temp;

	index += CstCopyByte(&send_buf[index], txPacket.header.cp_group_id, sizeof(txPacket.header.cp_group_id));
	index += CstCopyByte(&send_buf[index], txPacket.header.cp_id, sizeof(txPacket.header.cp_id));
	index += CstCopyByte(&send_buf[index], txPacket.header.ins, sizeof(txPacket.header.ins));
	index += CstCopyByte(&send_buf[index], txPacket.header.ml, sizeof(txPacket.header.ml));

    unsigned short ml = txPacket.header.ml[0];
    ml <<= 8;
    ml |= txPacket.header.ml[1];

	index += CstCopyByte(&send_buf[index], txPacket.vd, ml);

    unsigned short crc = CalcuCRC16((char*)&send_buf[1], index-1);
    ByteOrdertwo(crc, txPacket.crc);

	index += CstCopyByte(&send_buf[index], txPacket.crc, 2);

    send_buf[index++] = txPacket.etx;
	
	if( bConnect && TSCT_NetworkIsReady())
	{
		int ret = 0;
	
		ret = send(gSockfd, send_buf, index, 0);
		if(ret != index)
		{
			printf("Send Error !!!	tx size %d, ret %d \n", index, ret);

			if(gSockfd > 0)
				bConnect = false;
			printf(" [%d] Socket Error !!! ret %d ==> socket close... and try to re-connect !!!\n", ch, ret);
		}

		
		DumpMsg("Send Packet", ch, index, send_buf);
	}
}

static void MakeData(int ch, const unsigned short cmd)
{
    bool bCmdErr = false;
	unsigned char tmp_cmd;

    ByteOrdertwo(cmd, txPacket.header.ins);
    ByteOrdertwo(0, txPacket.header.ml);
    memset(txPacket.vd, 0x00, sizeof(txPacket.vd));

	// Edit1 S
	// Send Packet wait respon
	// exclude d1, f1, g1
	if((cmd >> 8) == 'd' || (cmd >> 8) == 'f' || (cmd >> 8) == 'g' || (cmd >> 8) == 'q'){
		bBlocking = bBlcoking_Rec_OK;
		SEVER_DISCON_CNT = 0;
	}
	else if((cmd >> 8) < 'a') {
		bBlocking = bBlcoking_Rec_OK;
		SEVER_DISCON_CNT = 0;
	}
	else if((cmd >> 8) == 'a'){
		bBlocking = bBlocking_a1;
	}else {
		bBlocking = (cmd >> 8) - 0x60;
		ResetRespon(60*1000);
	}
	if(ch == CH1)
	{
	    switch(cmd)
	    {
		    case CMD_a1: 	        MakeDataCmd_a1(CH1);        break;
		    case CMD_b1: 	        MakeDataCmd_b1(CH1);        break;
		    case CMD_c1: 	        MakeDataCmd_c1(CH1);        break;
		    case CMD_d1: 	        MakeDataCmd_d1(CH1);        break;
		    case CMD_e1: 	        MakeDataCmd_e1(CH1);        break;
		    case CMD_f1: 	        MakeDataCmd_f1(CH1);        break;
		    case CMD_g1: 	        MakeDataCmd_g1(CH1);        break;
		    case CMD_h1: 	        MakeDataCmd_h1(CH1);	    break;
		    case CMD_i1: 	        MakeDataCmd_i1(CH1);	    break;
		    case CMD_j1: 	        MakeDataCmd_j1(CH1);        break;
		    case CMD_k1: 	        MakeDataCmd_k1();	        break;
		    //case CMD_l1: 	        MakeDataCmd_l1();	        break;
			case CMD_n1: 	        MakeDataCmd_n1();	        break;
			case CMD_q1:			MakeDataCmd_q1(CH1);	    break;
			case CMD_q2:			MakeDataCmd_q2(CH1);	    break;
			case CMD_1C:			MakeDataCmd_1C(ch);			break;
			case CMD_1F:			MakeDataCmd_1F(ch);			break;
			case CMD_1I:			MakeDataCmd_1I(ch);			break;
			case CMD_1K:			MakeDataCmd_1K(ch);			break;
			case CMD_1N:			MakeDataCmd_1N(ch);			break;
		    default:
		        bCmdErr = true;
	    }
	}

    if(!bCmdErr)
    { 	
		txPacket.etx = CTRL_CODE_ETX;
		NetWrite(ch, true);
    }
}

void MakePacket(int ch, const unsigned short cmd)
{
    memset(&txPacket, 0x00, sizeof(PACKET));

    txPacket.stx = CTRL_CODE_STX;
    MakeHeader(ch);
    MakeData(ch, cmd);
}


int RsocketRead(void)
{
	int recvCount = 0;
	char *pRBuf = (char *)recv_buf1;

	memset(pRBuf, 0x00, TSCT_BUFSIZE);
	fd_set read_fds1; 
	FD_ZERO(&read_fds1); 
	FD_SET(gSockfd, &read_fds1);
	struct timeval tv1;
	tv1.tv_sec=1;
	tv1.tv_usec=0;
	select(gSockfd + 1, &read_fds1, NULL, NULL, &tv1); 
	if (FD_ISSET(gSockfd, &read_fds1)) 
	{ 
        recvCount=recv(gSockfd, pRBuf, TSCT_BUFSIZE, 0);
    }	
	return recvCount;
}

static bool NetReceiveData(void)
{
	int recvCount;
	unsigned char* pRxBuf = recv_buf1;
	
	recvCount= RsocketRead();

	if(recvCount > MIN_SERVERBUF_SIZE && recvCount < MAX_SERVERBUF_SIZE && (pRxBuf[0] == 0x02)){
		Parser(0, pRxBuf, recvCount);
		CstCheckNetTimeSet();  // For RTC I2C Error...
	}
	else if( recvCount < 0 ) // when the error of network is occurred...		
	{
		bConnect = false;
		//disconnect[ch]++;
		//ProtocolInit(ch);
		printf("\r\nNetReceiveData return False[%d]\r\n",recvCount);
		//return false;
	}
	return true;
}

static void Server_connect(void)
{
	//char ser_ver_ip[40];
	//char _p_[16];	
	char hostname[40];	
	int portno;	
	struct hostent *server;	
	struct sockaddr_in serveraddr;
	
	/*strncpy(_p_, theConfig.serverip, 16);
	sprintf(ser_ver_ip,"%d.%d.%d.%d", CharToInt(_p_, 3),CharToInt(&_p_[4], 3),CharToInt(&_p_[8], 3),CharToInt(&_p_[12], 3));*/
	
	//hostname = ser_ver_ip;	
	memcpy(hostname, theConfig.serverip, sizeof(theConfig.serverip));

	printf(" myip: %s, hostname: %s \n", theConfig.ipaddr, hostname);	
	//hostname = "com-me.tscontech.com";


    portno = theConfig.serverport;
	
	gSockfd = socket(AF_INET, SOCK_STREAM, 0);
	//fcntl( gSockfd, F_SETFL, O_NONBLOCK);

    if (gSockfd < 0) 
	{
		printf("ERROR opening socket\n");
		return;
	}
    /* gethostbyname: get the server's DNS entry */
    //server = gethostbyname(hostname);
	server = gethostbyname(theConfig.serverip);
    if (server == NULL) {
       // fprintf(stderr,"ERROR, no such host as %s\n", hostname);
		//printf(stderr);
		printf("ERROR, no such host as %s\n", hostname);
		return;
    }
	
	 /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,(char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

	printf("server address from DNS : %d\r\n", serveraddr.sin_addr.s_addr);

    /* connect: create a connection with the server */
    if (connect(gSockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) 
	{
		printf("Server Connect %s Error \n ", theConfig.serverip);
		bConnect = false;
	}
	else 		
	{
		printf("Server Connect Success \n ");
		bConnect = true;
	}
}



bool DataProc(int ch)
{
    unsigned short cmd = ByteOrder(rxPacket.header.ins);
	bool bCmdErr = false;

	if((cmd>>8 < 'A') && (((cmd & 0xff)-0x60) != bBlocking) && bBlocking != bBlcoking_Rec_OK)
		return true;
	else
		bBlocking = bBlcoking_Rec_OK;

    switch(cmd)
    {
		case CMD_1a:         DataProcCmd_1a(ch);     break;
		case CMD_1b:         DataProcCmd_1b(ch);     break;
		case CMD_1c:         DataProcCmd_1c(ch);     break;
		case CMD_1d:         DataProcCmd_1d(ch);     break;
		case CMD_1e:         DataProcCmd_1e(ch);     break;
		case CMD_1f:         DataProcCmd_1f(ch);     break;
		case CMD_1g:         DataProcCmd_1g();       break;
		case CMD_1h:         DataProcCmd_1h();       break;
		case CMD_1i:         DataProcCmd_1i();       break;
		case CMD_1j:         DataProcCmd_1j(ch);     break;
		case CMD_1k:      /*   DataProcCmd_1k();*/   break;
		case CMD_1l:       /*  DataProcCmd_1l();*/   break;
		case CMD_1q:         DataProcCmd_1q(ch);     break;
		case CMD_2q:         DataProcCmd_2q(ch);     break;
		case CMD_C1:		 DataProcCmd_C1(ch);	 break;
		case CMD_F1:		 DataProcCmd_F1(ch);	 break;
		case CMD_I1:		 DataProcCmd_I1(ch);	 break;
		case CMD_K1:		 DataProcCmd_K1(ch);	 break;
		case CMD_N1:		 DataProcCmd_N1(ch);	 break;
		default:
		    bCmdErr = true;
    }
    return !bCmdErr;
}

bool Parser(int nCh, const unsigned char* recv_buf, const int length)
{
	int ch = CH1;
    int index = 0;

	DumpMsg("Recv Packet", nCh, length, recv_buf);

    if(!recv_buf || !length){
        
		return false;
	}

    memset(&rxPacket, 0x00, sizeof(PACKET));
	
    rxPacket.stx = recv_buf[index++];
    index += CopyDataPacket((char *)&rxPacket.header.send_date[0], (char *)&recv_buf[index], sizeof(rxPacket.header.send_date));
    index += CopyDataPacket((char *)&rxPacket.header.sno[0], (char *)&recv_buf[index], sizeof(rxPacket.header.sno));

    rxPacket.header.cp_type = recv_buf[index++];
	//rxPacket.header.cp_temp = recv_buf[index++];

    index += CopyDataPacket((char *)&rxPacket.header.cp_group_id[0], (char *)&recv_buf[index], sizeof(rxPacket.header.cp_group_id));
    index += CopyDataPacket((char *)&rxPacket.header.cp_id[0], (char *)&recv_buf[index], sizeof(rxPacket.header.cp_id));
	
    index += CopyDataPacket(&rxPacket.header.ins[0], (char *)&recv_buf[index], sizeof(rxPacket.header.ins));
    index += CopyDataPacket(&rxPacket.header.ml[0], (char *)&recv_buf[index], sizeof(rxPacket.header.ml));

    unsigned short ml = ByteOrder(rxPacket.header.ml);

    index += CopyDataPacket((char *)&rxPacket.vd[0], (char *)&recv_buf[index], ml);
    index += CopyDataPacket((char *)&rxPacket.crc[0], (char *)&recv_buf[index], sizeof(rxPacket.crc));

    rxPacket.etx = recv_buf[index++];
    if(rxPacket.stx != CTRL_CODE_STX)
    {
        printf("Debug[%d] : stx error[0x02 != 0x%x]\n", ch, rxPacket.stx);
        return false;
    }

    if(rxPacket.etx != CTRL_CODE_ETX)
    {
        printf("Debug[%d] : etx error[0x03 != 0x%x]\n", ch, rxPacket.etx );
        //return false;
    }

    unsigned short crc1 = CalcuCRC16((char*)&recv_buf[1], index-1-3);
    unsigned short crc2 = ByteOrder(rxPacket.crc);

    if(crc1 != crc2)
    {
        printf ("Debug[%d] : crc error [0x%X, 0x%x]\n", ch, crc1, crc2);
        return false;
    }
	SEVER_DISCON_CNT = 0;
    return DataProc(nCh);
}

static void Execute(void)
{
	/** server mode */
	if(GetHomeLayer()) // Run only Waiting Timing 
	{
		if(reset_yn && (shmDataAppInfo.app_order == APP_ORDER_WAIT)) {
			printf("I1 Reset Start\r\n");
			I1_reset(); // 1I 발송 후 리부팅 진행
		}
	}

	// First Time No Action
	if(bFirst <= 2) 
    {
		printf("Debug : ***** countFirst[%d] \n ", bFirst);
        bFirst++;
        return;
    } else	
    {
        if(c1_stat == c1_EMPTY)
            c1_stat = c1_POWERON;
    }

	// wait Response (Send : bBlocking True / Receive : bBlocking False)
	if(bBlocking)	
	{
		unsigned char max_wait_cnt;
		
		if(shmDataAppInfo.app_order == APP_ORDER_CHARGING) 
			max_wait_cnt = MAX_DISCON_CNT_CHARGING;	// 30time
		else 
			max_wait_cnt = MAX_DISCON_CNT_IDLE;	// 5time

		// when Waiting Packet a1, If Go to Main Layer, Socket Close.
		if(bBlocking == bBlocking_a1) {
			if(shmDataAppInfo.app_order == APP_ORDER_WAIT){
				bConnect = false;
				printf("[a1]REQ Close Socket\r\n");
				bBlocking = bBlcoking_Rec_OK;
				Resetb1(3000);	// After 3sec, send b1
			}
			return;
		}

		// If No Response for 1min, Socket Reopen & ReSend Packet. 
		// IF Continue Server Disconnecting, go to Reboot.
		if(TimeOutRespon()){
			SEVER_DISCON_CNT++;
			CtLogBlue("Server No Response[%d]", SEVER_DISCON_CNT);

			if(gSockfd > 0)	
				close(gSockfd);	
			usleep(200*1000);
			Server_connect();
			printf("[%c1]Socket Reopen[%d] discon cnt : %d\r\n\n\n", 'a'- 1 + bBlocking, bConnect, SEVER_DISCON_CNT);			

			if(SEVER_DISCON_CNT > max_wait_cnt){
				ProtocolInit();
				sleep(5);
				custom_reboot();
				while(1);
			}	

			MakePacket(0,CMD_01+(bBlocking<<8));
		}												
		return;
	}
	/** .... ing */
	
	int tmp_ch = CstGetUserActiveChannel();		// for Charger Active Chanel

	/** server protocol ... */
	if(c1_stat < c1_BUSISTART)  {
        switch(c1_stat)
        {
        	case c1_POWERON:				
				printf("bPowerOn \n");
				MakePacket(0, CMD_c1);
			break;
			
			case c1_BUSIREADY:
				MakePacket(0, CMD_c1);
			break;
        }
    } else {
		if(charge_price_busi != VIP_Price_OK)	// 
		{		
			//dest_kind[ch] = 0x01;

			if(I1_reset_boot) 
				MakePacket(0, CMD_n1);	 

			MakePacket(0, CMD_j1);
			return;					
		}
		
		/*if(shmDataAppInfo.app_order == APP_ORDER_CHARGING \
		|| shmDataAppInfo.app_order == APP_ORDER_CONNECTOR_SELECT)*/
		if( (shmDataAppInfo.app_order >= APP_ORDER_CHARGE_READY && shmDataAppInfo.app_order <= APP_ORDER_CHARGE_END) && !bSendF1)
		{
			shmDataAppInfo.eqp_mode[1] = 0x09;
			if(TimeOute1())
            {
                if(shmDataIfInfo.card_auth == CARD_AUTH_OK_AFTER) // 2012_0424
                {
					MakePacket(0, CMD_e1);
					Resete1(INTERVAL_CMD_e1);
                }
            }
		}
		else
		{
			if(bQrwait == false){
				shmDataAppInfo.eqp_mode[1] = 0x05;
				if(TimeOutb1())
				{
					MakePacket(0, CMD_b1);
					Resetb1(INTERVAL_CMD_b1);
				}
			}
		}
	}	

	/** change UI ...*/
	if(nAppOldOrder != shmDataAppInfo.app_order)
    {
		//printf(" shmDataAppInfo[%d].app_order  [[%d]] \n", ch, shmDataAppInfo.app_order);
        nAppOldOrder = shmDataAppInfo.app_order;
		
		switch(shmDataAppInfo.app_order)
        {
			case APP_ORDER_WAIT:
				shmDataIfInfo.card_auth = CARD_AUTH_WAIT;
				if(bmakepacket_g1)
					MakePacket(0, CMD_g1);
				if(QR_Cancel_Code){
					MakePacket(0, CMD_q2);
					QR_Cancel_Code = QR_CANCLE_NONE;
				}					
				bQrwait = false;
				break;
			
			case APP_ORDER_CARD_READER:				
				if(shmDataIfInfo.card_auth) 
					shmDataIfInfo.card_auth = CARD_AUTH_WAIT;
				if(shmDataAppInfo.auth_type[0] == SERVER_AUTH){
					bQrwait = true;
					MakePacket(0, CMD_q1); 	// Request QR Auth  
				}
				break;
							
            case APP_ORDER_CUSTOMER_AUTH:                
                MakePacket(0, CMD_a1);  
                break;
				
			case APP_ORDER_CONNECTOR_SELECT:				
				// After Auth No Act b1
				//MakePacket(ch, CMD_b1);
				break;
				
			case APP_ORDER_CHARGE_INPUT:
				break;
				
			case APP_ORDER_CHARGE_READY:				
				break;
				
            case APP_ORDER_CHARGING:
                if(shmDataIfInfo.card_auth == CARD_AUTH_OK) // 2012_0424
                {             
					shmDataIfInfo.card_auth = CARD_AUTH_OK_AFTER;
                    MakePacket(0, CMD_d1);
			}
                break;
				

			case APP_ORDER_CHARGING_STOP:	// Charging Stop
			case APP_ORDER_CHARGE_END:		// EMG Stop

				MakePacket(0, CMD_f1);

				break;
				
			case APP_ORDER_FINISH:					
				break;

			default :
				printf("Receive Other AppOrder %d\r\n", shmDataAppInfo.app_order);
				break;
        }
    }
	if(EventCode_buf)
	{
		MakePacket(0, CMD_c1);
	}
		
}

void NetRun(void) 
{
	if(!bConnect){
		if(gSockfd > 0)	{
			close(gSockfd);	
		}
		usleep(200*1000);
		Server_connect();		
		usleep(200*1000);
	}
	
	if(bConnect){
		if(NetReceiveData())
			Execute();	
	}	
}

static void* NetClientThread(void* arg)
{
	bool iteEthGetLink_;			// Ethernet Link Status

	while(1)  // waiting for Netwotk Enabled when booting.
	{
		if (theConfig.ConfirmSelect != USER_AUTH_NET) return;

		if(bGloAdminStatus) {sleep(1); continue;}
		
		CstCheckUserTimeSet();	// For RTC I2C Error...
	
		CstNetFileFunc();
		
		if(TSCT_NetworkIsReady() && iteEthGetLink())		break;
		
		usleep(100*1000);
	}
	
	while(sClientTask > 0)
	{
		if (theConfig.ConfirmSelect != USER_AUTH_NET) return;

		if(bGloAdminStatus)	{sleep(1); continue;}

		CstCheckUserTimeSet();	// For RTC I2C Error...
		
		if(iteEthGetLink()==0){			// check ethernet link
			iteEthGetLink_ = false;
			bConnect = false;
		}
		else{
			if(iteEthGetLink_ == false){  // Network cable is re-connected....
				NetworkReset();
				iteEthGetLink_ = true;
			}
			if(TSCT_NetworkIsReady())	
				NetRun();			
			else bConnect = false;
		}
		
		// if not network logic
		if(Socket_timeout == false && bConnect == false){ // 
			ResetNet(30);
			Socket_timeout = true;
		}
		if(bConnect){
			Socket_timeout = false;
		}
		if(TimeOutNet() && Socket_timeout){
			ProtocolInit();
			sleep(5);
			custom_reboot();
			while(1);
			Socket_timeout = false;
		}
					
		
		CstNetFileFunc();
		
		sleep(1);

		//usleep(500*1000);
	}
}

void ClientInit(void)
{	
	shmDataAppInfo.member_type = 0;
	
	reset_yn = false;

	memset(send_buf, 0x00, sizeof(send_buf));
    memset(recv_buf1, 0x00, sizeof(recv_buf1));

	SEVER_DISCON_CNT = 0;
	memset(&txPacket, 0x00, sizeof(PACKET));	
		
	ProtocolInit();
	c1_stat = c1_POWERON; 
	
	if (sClientTask == 0)
	{
		CtLogYellow(" create Client thread..\n");
		pthread_create(&sClientTask, NULL, NetClientThread, NULL);
		pthread_detach(sClientTask);
	}
}

void ClientExit(void)
{	
	sClientTask = 0;
}

bool CheckClientInit()
{	
	return (sClientTask != 0);
}

