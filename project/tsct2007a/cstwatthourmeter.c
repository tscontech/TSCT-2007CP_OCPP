/**
*       @file
*               cstwatthourmeter.c
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2018.12.18 <br>
*               author: dyhwang <br>
*				modify: dsAhn <br>
*               description: <br>
*/
#include <string.h>
#include <unistd.h>
#include "ite/audio.h"
#include "audio_mgr.h"
#include "ctrlboard.h"
#include "tsctcfg.h"
#include "crc16table.h"

//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------
#define READ_485_TASK_DELAY		120 // ms


//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------
typedef struct{

	int sRS485errors;
	int AmiErrCnt;
	int itp_RS485_dev;
	int RS485_Enable;
	ITHRS485Port RS485_Port;

	pthread_t sWhmReadTask;
	pthread_t sWhmMonTask;

	ReadQueue sWhmReadQueue;
	WHMListener sWhmListener;
	char sWhmId[4];
}AMIDATA;

static AMIDATA amidata;

bool bAmiStartFlg = false;
bool bAmiErrChk = true;
uint32_t ipositiveActiveEnergy=0;
uint16_t gcurrent =0;
uint16_t gvolt=0;
uint32_t gwattx=0;
extern bool booting_set;
static uint8_t EM2P_REQ[8];
static uint8_t responsex[256]; 
static char buffer[256];
static int bufsize;
//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------
static int RS485Write(int dev, char *data, size_t size){
	//DumpBuffer("485 Reqeust", data, size);
	return write(dev, data, size);
}


static int RS485Read(int dev, char *data, size_t size){
	int ret = read(dev, data, size);
//	if (ret > 0)
//		DumpBuffer("485 Response", data, ret);
	
	return ret;
}

static void* RS485ReadTask(void* arg){

	ReadQueueInit(&amidata.sWhmReadQueue);
		
    while(1) {
		int i;
		
		bufsize = RS485Read(amidata.itp_RS485_dev, buffer, 256);
		
		for (i = 0; i < bufsize; i++)
			ReadQueueEnqueue(&amidata.sWhmReadQueue, buffer[i]);
		
        usleep(READ_485_TASK_DELAY * 1000);
    }

//	sWhmData[ch].sWhmReadTask = 0;
//	CtLogYellow("[WHM#%d] exit rs485 read thread\n", ch);
}


static int Bcd2Decimal(char hex){	
    return ((hex & 0xF0) >> 4) * 10 + (hex & 0x0F);	
}


static float Compose4BcdValue(char hex0, char hex1, char hex2, char hex3){
	return (Bcd2Decimal(hex0)*10000 + Bcd2Decimal(hex1)*100 +
				  Bcd2Decimal(hex2) + Bcd2Decimal(hex3)*0.01);
}


static float Compose2BcdValue(char hex0, char hex1){
	return (Bcd2Decimal(hex0)*10 + Bcd2Decimal(hex1)*0.1);
}

static char Checksum(char *data, int size){
	int i;
	char checksum = 0;
	
	for (i = 0; i < size-2; i++)
		checksum = checksum ^ *(data+i);

	return checksum;
}

static void RequestIdNew(int ch) {
	amidata.AmiErrCnt = 0; 
	CstSetEpqStatus(CST_COM_WHM, true);
}


static uint16_t crc16(uint8_t *buffer, uint16_t buffer_length){
    uint8_t crc_hi = 0xFF;  // high CRC byte initialized
    uint8_t crc_lo = 0xFF;  // low CRC byte initialized
    unsigned int i;         // will index into CRC lookup
    /* pass through message buffer */
    while (buffer_length--) {
        i = crc_hi ^ *buffer++; /* calculate the CRC  */
        crc_hi = crc_lo ^ table_crc_hi[i];
        crc_lo = table_crc_lo[i];
    }
    return (crc_hi << 8 | crc_lo);
}

void debugprinthex(uint8_t *bp, uint16_t sz){
	uint16_t i;
	for ( i=0; i<sz; i++)
			printf(" %02X", *(bp+i));
        printf("\r\n");
}

void convert_big_to_lit(uint16_t *le,uint16_t *be,  uint16_t wcnt){
  char ii;
  for ( ii=0; ii<wcnt; ii++){
      le[ii] = SWAP_WORD(be[ii]);
  }
}
#define MBRQ_SZ	21
#define MBRF_SZ	MBRQ_SZ*2+5
#define MB_CURRENT_ADDR	29
#define MB_VOLTAGE_ADDR	27
#define MB_ACCPWR_ADDR	3


static bool RequestValueNew(int ch) {
	float current;
	uint16_t crc, rxcrc;
	uint16_t size = 0 , findstart=0;
	uint16_t volt;
	unsigned int booting_ser_int = 0;

    EM2P_REQ[0] = 0;
    EM2P_REQ[1] = 3;
    EM2P_REQ[2] = 0;
    EM2P_REQ[3] = 0;
    EM2P_REQ[4] = 0;
    EM2P_REQ[5] = MBRQ_SZ;
    crc= crc16(EM2P_REQ,6);
    EM2P_REQ[6] =  (uint8_t )((crc & 0xff00) / 0x100 );
    EM2P_REQ[7] =  (uint8_t )(crc & 0xff) ;

	ithGpioSetMode(GPIO_RS485_EN1, ITH_GPIO_MODE0); ///txEnable on
	ithGpioSetOut(GPIO_RS485_EN1);
	ithGpioSet(GPIO_RS485_EN1);

	RS485Write(amidata.itp_RS485_dev, EM2P_REQ, 8);
    usleep(20*1000);

	ithGpioClear(GPIO_RS485_EN1); ///txEnable off

	memset(responsex, 0, 256);
	while (!ReadQueueIsEmpty(&amidata.sWhmReadQueue) && (size<200 )) {
		responsex[size++] = ReadQueueDequeue(&amidata.sWhmReadQueue);
	}

// Response Size Check JCKIM	
	// if (size <255){
	// 	debugprinthex(responsex,size);
	// } 
	if (size == MBRF_SZ) { 
		// printf("[AMI] response size check ok %d \r\n", size);
		amidata.sRS485errors=0;
	} else {
		// LOG OFF
		// printf("[AMI] response size check NOK sz:%d errcnt:%d\r\n"
							// , size,amidata.sRS485errors++);
		// DumpBuffer("[AMI ERR PACKET]\r\n", responsex, size);
		return false;
	}
// Response CRC Check JCKIM	
	crc= crc16(responsex,MBRF_SZ-2);
	rxcrc = responsex[MBRF_SZ-2]*0x100+  responsex[MBRF_SZ-1];
	
	if (crc ==  rxcrc) {
		// printf( "[AMI] OK CRC !!!! ==> RX CRC: 0x%04x CALC CRC: 0x%04x\r\n", rxcrc, responsex );	
		// DumpBuffer("[AMI ERR PACKET]", responsex, size);
		amidata.sRS485errors=0;
	} else {
		printf( "[AMI] ERROR CRC !!!! ===> RX CRC: 0x%04x CALC CRC: 0x%04x errcnt:%d\r\n"
		, rxcrc, responsex , amidata.sRS485errors++ );	
		//  DumpBuffer("[AMI ERR PACKET]\r\n", responsex, size);

		return false;
	}
	ipositiveActiveEnergy = responsex[MB_ACCPWR_ADDR]*0x1000000+responsex[MB_ACCPWR_ADDR+1]*0x10000
							+responsex[MB_ACCPWR_ADDR+2]*0x100+responsex[MB_ACCPWR_ADDR+3];
							
	current = ((float )( responsex[MB_CURRENT_ADDR]*0x100 + responsex[MB_CURRENT_ADDR+1]))/ 100.;
	gcurrent =  (uint16_t) (current *100.0);
	volt =  responsex[MB_VOLTAGE_ADDR]*0x100 + responsex[MB_VOLTAGE_ADDR+1];
	gvolt= volt *10;
	gwattx = gvolt * gcurrent/1000;
	if (current >0.2)
		printf("\n\n======[AMI] %d0Wh, %.2fA, %03dV  watt:%ld ERRCNT: %d =======\r\n\n", 
		 ipositiveActiveEnergy, current, volt/10, gwattx, amidata.sRS485errors);
	if (amidata.sWhmListener != NULL)
				(*amidata.sWhmListener)(ch, current, volt, ipositiveActiveEnergy);

	if(!booting_set) { // Run only once at boot
		booting_ser_int = ipositiveActiveEnergy;
		ValueOrderFourByte(shmDataAppInfo.eqp_watt, booting_ser_int);
		printf("\n\n[RequestValueNew] shmDataAppInfo[%d].eqp_watt = [%02x][%02x][%02x][%02x] \n\n"
				, ch
				, shmDataAppInfo.eqp_watt[0], shmDataAppInfo.eqp_watt[1]
				, shmDataAppInfo.eqp_watt[2], shmDataAppInfo.eqp_watt[3]);
		booting_set = true;
	}
	return true;
}

int GetAMIValue(void){ ///mod
	return ipositiveActiveEnergy;
}

static void* ValueMonitoringTask(void* arg){
	int ch = (int)arg;
	int jckim;
	amidata.sRS485errors = 10;

	sleep(5);
				
	while (1)  {
		RequestValueNew(ch);
		if (amidata.sRS485errors < 10 || bGloAdminStatus) 
				bAmiErrChk = false;			
		else	{
			bAmiErrChk = true;		
		}
					
		usleep( 2100*1000);
	}
}

void start_read_ami(void){
		if (amidata.sWhmMonTask == NULL) { 
			CtLogYellow("[AMI] create monitoring thread..\n");		
			pthread_create(&amidata.sWhmMonTask, NULL, ValueMonitoringTask, (void *)CH1);
			pthread_detach(amidata.sWhmMonTask);
    }
}



void  WattHourMeterStartMonitoring(ChannelType ch, WHMListener listener)	// Listener
{
	CtLogYellow("ch[%d] \n\r", ch);
	amidata.sWhmListener = listener;
}

/**
 * @brief Stop AMI Listening
 * 
 * @param ch 
 */
void WattHourMeterStopMonitoring(ChannelType ch) // 
{	
	amidata.sWhmListener = NULL;
	CtLogYellow("[WHM#%d] =============>\n", ch);
}


void WattHourMeterInit(void){

	ITHUartParity RS485_UartParity = ITH_UART_NONE;
	
	amidata.RS485_Enable = GPIO_RS485_EN1;
	amidata.itp_RS485_dev = RS485_IPT1;
	amidata.RS485_Port = RS485_DEV1;
	amidata.sRS485errors = 0;
	amidata.sWhmListener = NULL;

	ithGpioSetMode(amidata.RS485_Enable, ITH_GPIO_MODE0);
	ithGpioSetOut(amidata.RS485_Enable);
	ithGpioClear(amidata.RS485_Enable);
	// ioctl(amidata.itp_RS485_dev, ITP_IOCTL_RESET, (void*)&RS485_UartParity);
	// ioctl(amidata.itp_RS485_dev, ITP_IOCTL_ON, (void*)&amidata.RS485_Port);

	usleep(100*1000);
	amidata.AmiErrCnt = 0; 

	if (amidata.sWhmReadTask == 0){
		CtLogYellow("[AMI] create rs485 read thread..\n", CH1);
		pthread_create(&amidata.sWhmReadTask, NULL, RS485ReadTask, (void *)CH1);
		pthread_detach(amidata.sWhmReadTask);
	}
	usleep(100*1000);
	start_read_ami();
}

uint16_t TSCTGetAMIVolt(void)
{
	return gvolt;
}

uint16_t TSCTGetAMICurrent(void)
{
	return gcurrent;
}