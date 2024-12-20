/**
*       @file
*               cstcommon.c
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2017.12.05 <br>
*               author: dyhwang <br>
*               description: <br>
*/
#include <string.h>
#include <unistd.h>
#include "ite/audio.h"
#include "audio_mgr.h"
#include "ctrlboard.h"
#include "tsctcfg.h"
#include "tsctcommon.h"
#include "crc16table.h"
#include "curl/curl.h"

//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------
#define CHECK_TASK_DELAY				10 * 1000 // ms
#define GND_FAULT_MON_TASK_DELAY		1000

#define CP_PWM_FREQ			1000
#define CP_PWM_DUTY_CYCLE	47 // => 53.3%

//-----------------------------------------------------------------------
// Global Variable
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------


static ChannelType sUserActiveChannel = CH1;

static ChargeStep sChannelStep[2] = {STEP_START, STEP_START};
static float gChUsedEnergy = 0.0;
static int gChChargeTime = 0;

static int bUpdateFail = 0x01; // Zero : Update sucess / Not Zero : Udate Fail / Set after send 1K

static char CarNumEUC_KR[44][2] = 
{  
 {0xB0,0xA1},{0xB3,0xAA},{0xB4,0xD9},{0xB6,0xF3},  /*�� �� �� ��*/  	
 {0xB8,0xB6},{0xB0,0xC5},{0xB3,0xCA},{0xB4,0xF5},  /*�� �� �� ��*/ 
 {0xB7,0xAF},{0xB8,0xD3},{0xB9,0xF6},{0xBC,0xAD},  /*�� �� �� ��*/ 
 {0xBE,0xEE},{0xC0,0xFA},{0xB0,0xED},{0xB3,0xEB},  /*�� �� �� ��*/ 
 {0xB5,0xB5},{0xB7,0xCE},{0xB8,0xF0},{0xBA,0xB8},  /*�� �� �� ��*/ 
 {0xBC,0xD2},{0xBF,0xC0},{0xC1,0xB6},{0xB1,0xB8},  /*�� �� �� ��*/ 
 {0xB4,0xA9},{0xB5,0xCE},{0xB7,0xE7},{0xB9,0xAB},  /*�� �� �� ��*/ 
 {0xBA,0xCE},{0xBC,0xF6},{0xBF,0xEC},{0xC1,0xD6},  /*�� �� �� ��*/ 
 {0xBE,0xC6},{0xB9,0xD9},{0xBB,0xE7},{0xC0,0xDA},  /*�� �� �� ��*/ 
 {0xB9,0xE8},{0xC7,0xCF},{0xC7,0xE3},{0xC8,0xA3},  /*�� �� �� ȣ*/ 
 {0xC5,0xD7},{0xBD,0xBA},{0xC6,0xAE},{0xC4,0xAB}   /*�� �� Ʈ ī*/	
};

static char CarNumUTF8[44][3] = 
{  
 {0xEA,0xB0,0x80},{0xEB,0x82,0x98},{0xEB,0x8B,0xA4},{0xEB,0x9D,0xBC},  /*�� �� �� ��*/  	
 {0xEB,0xA7,0x88},{0xEA,0xB1,0xB0},{0xEB,0x84,0x88},{0xEB,0x8D,0x94},  /*�� �� �� ��*/ 
 {0xEB,0x9F,0xAC},{0xEB,0xA8,0xB8},{0xEB,0xB2,0x84},{0xEC,0x84,0x9C},  /*�� �� �� ��*/ 
 {0xEC,0x96,0xB4},{0xEC,0xA0,0x80},{0xEA,0xB3,0xA0},{0xEB,0x85,0xB8},  /*�� �� �� ��*/ 
 {0xEB,0x8F,0x84},{0xEB,0xA1,0x9C},{0xEB,0xAA,0xA8},{0xEB,0xB3,0xB4},  /*�� �� �� ��*/ 
 {0xEC,0x86,0x8C},{0xEC,0x98,0xA4},{0xEC,0xA1,0xB0},{0xEA,0xB5,0xAC},  /*�� �� �� ��*/ 
 {0xEB,0x88,0x84},{0xEB,0x91,0x90},{0xEB,0xA3,0xA8},{0xEB,0xAC,0xB4},  /*�� �� �� ��*/ 
 {0xEB,0xB6,0x80},{0xEC,0x88,0x98},{0xEC,0x9A,0xB0},{0xEC,0xA3,0xBC},  /*�� �� �� ��*/ 
 {0xEC,0x95,0x84},{0xEB,0xB0,0x94},{0xEC,0x82,0xAC},{0xEC,0x9E,0x90},  /*�� �� �� ��*/ 
 {0xEB,0xB0,0xB0},{0xED,0x95,0x98},{0xED,0x97,0x88},{0xED,0x98,0xB8},  /*�� �� �� ȣ*/ 
 {0xED,0x85,0x8C},{0xEC,0x8A,0xA4},{0xED,0x8A,0xB8},{0xEC,0xB9,0xB4}   /*�� �� Ʈ ī*/	
};

//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------
void CstEUCKR2UTF8(char *UTF8, char *EUKkr)
{
	int i;
	char temp[2];
	temp[0] = *EUKkr;   EUKkr++;
	temp[1] = *EUKkr;
	
	for(i=0; i<44; i++)
	{
		if(( CarNumEUC_KR[i][0] == temp[0]) && ( CarNumEUC_KR[i][1] == temp[1]) )
		{
			memcpy(UTF8, CarNumUTF8[i], 3);
			return;
		}
	}
}

void CstEUCKR2UTF8Test(void)
{
	int i;
	for(i=0; i<44; i++)
	{
		char EUC_Kr[2], UTF8[3];

		memcpy(EUC_Kr, CarNumEUC_KR[i], 2);
		memcpy(UTF8, CarNumUTF8[i], 3);
		printf("[%d] EUK_KR : %s, UTF-8: %s \n", i,EUC_Kr, UTF8);
	}
}


void DumpBuffer(char* msg, char* data, int size)
{
	int i;

	printf("%s ", msg);
	for (i = 0; i < size; i++)
	{
		printf("0x%02X ", *(data+i));
	}
	printf("\n");	
}

void DumpMsg(const char *msgType, int ch, const int msg_len, const unsigned char *msgBuf)
{
    int i,k,j;
    printf("\n\rDebug[%d] : %s [len=%d]  \n", ch, msgType, msg_len);
    for(k=i=1; i<=msg_len; i++)
    {
		if(i == 40)
			printf("\r\n=======HeaderEnd[39]=======\r\n");
        printf("[%02x]", msgBuf[i-1]);
        if(!(i%16))
        {
            printf("    ");
            for(;k<=i;k++)
            {
                printf("%c", ((msgBuf[k-1]>=0x20&&msgBuf[k-1]<=0x7e)?msgBuf[k-1]:'^'));
            }
            printf("\n");
        }
        else
        {
            if(!(i%8))
            {
                printf(" ");
            }
        }
    }

    if(!(msg_len%16 == 0 ))
    {
        for(j=0; j<(16-(msg_len%16)); j++)
        {
            printf("    ");
        }

        printf("    ");

        for(;k<=i;k++)
        {
            printf("%c", ((msgBuf[k-1]>=0x20&&msgBuf[k-1]<=0x7e)?msgBuf[k-1]:'^'));
        }
        printf("\n");
    }
    else
    {
        printf("    ");
        for(;k<=i;k++)
        {
            printf("%c", ((msgBuf[k-1]>=0x20&&msgBuf[k-1]<=0x7e)?msgBuf[k-1]:'^'));
        }
        printf("\n");
    }
}
void ReadQueueInit(ReadQueue *q)
{
	q->front = 0;
	q->rear = 0;
}


int ReadQueueIsEmpty(ReadQueue *q)
{
	return q->front == q->rear;
}


int ReadQueueIsFull(ReadQueue *q)
{
	return ((q->rear+1) % READ_QUEUE_SIZE) == q->front;
}


void ReadQueueEnqueue(ReadQueue *q, char c)
{
	if (ReadQueueIsFull(q)) {
	//	printf("[COMMON] Read queue is full\n");
		return;
	}

	q->rear = (q->rear+1) % READ_QUEUE_SIZE;
	q->queue[q->rear] = c;
}


char ReadQueueDequeue(ReadQueue *q)
{
	if (ReadQueueIsEmpty(q)) {
		printf("[COMMON] Read queue is empty\n");
		return 0;
	}

	q->front = (q->front+1) % READ_QUEUE_SIZE;
	return q->queue[q->front];
}


void StartPwm(ChannelType ch)
{
	CtLogGreen("start ch%d pwm with %d Hz and %d duty cycle..\n", ch, CP_PWM_FREQ, CP_PWM_DUTY_CYCLE);

	ithPwmReset(CST_CP0_PWM, CST_GPIO_CP0, ITH_GPIO_MODE2);	
	ithPwmInit(CST_CP0_PWM, CP_PWM_FREQ, 100);
	ithPwmSetDutyCycle(CST_CP0_PWM, CP_PWM_DUTY_CYCLE);	
	ithPwmEnable(CST_CP0_PWM, CST_GPIO_CP0, ITH_GPIO_MODE2);
}

int GetQueueDataNum(ReadQueue *q)
{
	int nSize = (q->rear - q->front);

	if( nSize < 0)   
		nSize += READ_QUEUE_SIZE;
	
	return nSize;
}


void StopPwm(ChannelType ch)
{	
	ithPwmSetDutyCycle(CST_CP0_PWM, 100);

	CtLogGreen("stop ch%d pwm with 100 duty cycle\n", ch);
}

#define GPIO_GFCI_MASK	(1<<(GPIO_AC220_GFCI%32))

static void DoGfciFunc(void)
{
	unsigned int regValue = ithGpioGet(GPIO_AC220_GFCI);
	if(!(regValue & GPIO_GFCI_MASK))	
	{
		regValue = ithGpioGet(GPIO_AC220_GFCI);
		
		if(!(regValue & GPIO_GFCI_MASK))
		{
			regValue = ithGpioGet(GPIO_AC220_GFCI);
			
			if(!(regValue & GPIO_GFCI_MASK))
			{
				MagneticContactorOff();		
			    ithGpioClearIntr(GPIO_AC220_GFCI);	
			}
		}
	}
}

static void CstGfciIntrHandler(unsigned int pin, void* arg)
{
	DoGfciFunc();
}

void CstGfciInit(void)
{
	int i;
	
    ithEnterCritical();

    ithGpioRegisterIntrHandler(GPIO_AC220_GFCI, CstGfciIntrHandler, NULL);
    ithGpioCtrlEnable(GPIO_AC220_GFCI, ITH_GPIO_INTR_TRIGGERFALLING);
    ithGpioEnableIntr(GPIO_AC220_GFCI);
    ithGpioSetIn(GPIO_AC220_GFCI);
    ithGpioEnable(GPIO_AC220_GFCI);
	
    ithExitCritical();
	
    printf("[COSTEL] SDL_main / CstGfciInit..[[[[[ %d ]]]]]] \n", GPIO_AC220_GFCI);
}

void CstSetEpqStatus(unsigned int nBit, bool bSet)
{ 
	int nCnt = (int)(nBit/8);
	unsigned char nResidue = nBit%8;

//	printf("CstSetEpqStatus :: [[ %d, %d =>  %d %d : %d  ==> 0x%2x]] \n", ch, nBit, nCnt, nResidue, bSet, shmDataAppInfo.eqp_status[nCnt]);

	if(bSet)	shmDataAppInfo.eqp_status[nCnt] |= (1<<nResidue);	
	else		shmDataAppInfo.eqp_status[nCnt] &= ~(1<<nResidue);

//	printf("shmDataAppInfo[%d].eqp_status[%d] :: 0x%2x \n", ch, nCnt, shmDataAppInfo.eqp_status[nCnt]);
}

bool CstGetEpqStatus(unsigned int nBit)
{ 
	int nCnt = (int)(nBit/8);
	unsigned char nResidue = nBit%8;
	bool ret = true;

	if( shmDataAppInfo.eqp_status[nCnt] & (1<<nResidue) )		ret = false;	
	else		ret = true;

	return ret;
}


void CstSetUserActiveChannel(ChannelType ch)
{
	sUserActiveChannel = ch;
}


ChannelType CstGetUserActiveChannel()
{
	return sUserActiveChannel;
}

void CstSetUsedEnergy(float energy, int time)
{
	gChUsedEnergy = energy;
	gChChargeTime = time;
}

void CstGetUsedEnergy(float *energy, int *time)
{
	*energy = gChUsedEnergy;
	*time = gChChargeTime;
}

void CstGpioSetModeState(unsigned int nGpio, int bOut, int bSet)
{
	ithGpioSetMode(nGpio, ITH_GPIO_MODE0);
	
	if(bOut == GPIO_SET_OUT )
	{	
		ithGpioSetOut(nGpio);
		
		if(bSet)		ithGpioSet(nGpio);		
		else			ithGpioClear(nGpio);
	}
	else
	{
		ithGpioSetIn(nGpio);
	}
}

unsigned int CharToInt(char *p, unsigned char len)
{
	unsigned int ret = 0, i;
	char string[32]= {0,};

	if( len > 0)
	{
		strncpy(string, p, len);	
		ret = atoi(string);  
		
	//	printf("[CstCharToInt] %s, %d => %d \n", p, len, ret);
	}
	return ret;
}

unsigned int FourByteOrder(unsigned char* buf)
{
    unsigned int ret = 0;
	bool big_endian= true;
    if(big_endian)
    {
        ret = buf[0] << 24;
        ret |= buf[1] << 16;
        ret |= buf[2] << 8;
        ret |= buf[3];
    }
    else
    {
        ret = buf[0];
        ret |= buf[1] << 8;
        ret |= buf[2] << 16;
        ret |= buf[3] << 24;
    }

    return ret;
}

void AudioAmpPowerSet(int bSet)
{
	// if(bSet)
	// {
	// 	ithGpioSet(GPIO_AUDIO_EN);
	// }
	// else
	// {
	// 	ithGpioClear(GPIO_AUDIO_EN);
	// }
	printf("Audio Amp Power [[[[  %d  ]]]]] \n", bSet);
}

void ValueOrderFourByte(unsigned char* buf, unsigned int nValue)
{
	buf[0] = nValue >> 24;
	buf[1] = nValue >> 16;
	buf[2] = nValue >> 8;
	buf[3] = nValue;
}

int CstConvertBCD(char nValue)
{
	char _temp[2];

	_temp[0] = ((nValue>>4) | 0x30);
	_temp[1] = ((nValue&0x0f) | 0x30);

	return atoi(_temp);
}

void CstBCDToInt(char *pBuf, char *pValue) // 200221 _dsahn : func name change 
{
	*pBuf = pValue[0] << 4;
	*pBuf |= (pValue[1] &= 0x0F);
	pBuf++;
	
	*pBuf = pValue[2] << 4;
	*pBuf |= (pValue[3] &= 0x0F);
	pBuf++;
	
	*pBuf = pValue[4] << 4;
	*pBuf |= (pValue[5] &= 0x0F);
}

void CstTimeValueToBCD(char *pBuf, unsigned int Value)
{
	unsigned int temp = Value;
	
	*pBuf = (((temp/10)<<4) | (temp%10));
	
	pBuf++;

	temp = (unsigned int)(temp/100);
	*pBuf = (((temp/10)<<4) | (temp%10));
	
	pBuf++;

	temp = (unsigned int)(temp/100);
	*pBuf = (((temp/10)<<4) | (temp%10));
}

bool CstCheckDoorLock(int ch)
{
	if((theConfig.devtype == BB_TYPE)|| 
		((theConfig.devtype == BC_TYPE) && (ch == CH2)) || 
		((theConfig.devtype == BC2_TYPE) && (ch==CH2)) ||
		((theConfig.devtype == B_TYPE) && (ch==CH1)) )
	{
		return true;
	}

	/// in case of BC_TYPE & CH1, C Type, HC Type, HBC Type... these type is no doorlock.
	return false;
}

bool CstCheckDoorClosed(int ch)
{
	if(CstCheckDoorLock(ch))
	{
		if(DoorLockIsClosed(ch))	return true;   // closed doorlock
		else 						return false;  // open the doorlock
	}

	/// in case of BC_TYPE & CH1, C Type, HC Type, HBC Type... these type is no doorlock.
	return true;
}


void CstFwUpdateMsg(void)
{
	// FTP Upgragde ???
	if(bFTPupgrade > FTP_FW_UD_IDLE)
	{
		printf("CstFwUpdateMsg \n");
		EmergencyDialogHide();
		EmergencyDialogShow(STEP_FTPFWUP);
	}
}

void custom_reboot(){
	printf("Reboot======>xxxx\n");
    ithWatchDogEnable();
    ithWatchDogSetReload(0);
    ithWatchDogRestart();
	while(1){
		sleep(1);
	}
}

struct WriteThis {
  const char *readptr;
  size_t sizeleft;
};
 
static size_t read_callback(char *ptr, size_t size, size_t nmemb, void *userp)
{
  struct WriteThis *upload = (struct WriteThis *)userp;
  size_t max = size*nmemb;
 
  if(max < 1)
    return 0;
 
  if(upload->sizeleft) {
    size_t copylen = max;
    if(copylen > upload->sizeleft)
      copylen = upload->sizeleft;
    memcpy(ptr, upload->readptr, copylen);
    upload->readptr += copylen;
    upload->sizeleft -= copylen;
    return copylen;
  }
 
  return 0;                          /* no more data left to deliver */
}

bool FtpLogUpload_func(void)
{
	CURL *curl;
    CURLcode res;

	char data[50];

	struct WriteThis upload;

	GetDateTime(data);
	
	upload.readptr = data;
	upload.sizeleft = strlen(data);

	char* ftpid[30];
	char* filename[150];

	memcpy(ftpid, theConfig.ftpId, sizeof(theConfig.ftpId));
	strcat(ftpid, ":");
	strcat(ftpid, theConfig.ftpPw);

	memcpy(filename, CsConfigVal.diagLogUrl, sizeof(CsConfigVal.diagLogUrl));
	strcat(filename, "/Cs0000000002_1.txt");

	curl = curl_easy_init();
	if (!curl)
	{
        printf("curl_easy_init() fail.\n");
		return true;
    }

	curl_easy_setopt(curl, CURLOPT_URL, filename);
    curl_easy_setopt(curl, CURLOPT_USERPWD, ftpid);
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
	curl_easy_setopt(curl, CURLOPT_READDATA, &upload);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)upload.sizeleft);

	res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK)
	{
      	printf("curl_easy_perform() failed: Ftp Upload\n");
		curl_easy_cleanup(curl);
		return true;
	}
 
    /* always cleanup */
    curl_easy_cleanup(curl);
	return false;
}

void FtpFwUpdate_func(void)
{
	//if(bFTPupgrade == FTP_FW_UD_WAIT_CON)
	//{	
		char upgradeFtpUrl[100] = {' ',};
		char upgradeFtpID[10] = {' ',};
		char upgradeFtpPW[16] = {' ',};
		short tmp=6;

		CstFwUpdateMsg();
		
		usleep(500*1000);
		/*				
		strcpy(upgradeFtpUrl, "ftp://");
		//strcpy(&upgradeFtpUrl[6], theConfig.ftpIp);
		strcpy(&upgradeFtpUrl[6], "1.241.77.141");
		strcat(upgradeFtpUrl, "/FW/2007A/");
		//strcat(upgradeFtpUrl, CST_FIRMWARE_NAME);
		strcat(upgradeFtpUrl, TSCT_FIRMWARE_NAME);

		strcpy(upgradeFtpID, "cptsct");

		strcpy(upgradeFtpPW, "cp01340");
		*/		
		// strcpy(upgradeFtpUrl, "ftp://");
		// strcpy(&upgradeFtpUrl[tmp], theConfig.ftpDns);
		// tmp += strlen(theConfig.ftpDns);
		strcpy(&upgradeFtpUrl[0], theConfig.ftpDns);
		// strcpy(&upgradeFtpUrl[tmp], theConfig.ftpPath);
		//strcpy(&upgradeFtpUrl[tmp], "/fw/2007a/kproc.sys");

		strcpy(upgradeFtpID, theConfig.ftpId);

		strcpy(upgradeFtpPW, theConfig.ftpPw);
		
		UpgradeSetUrl(upgradeFtpUrl, upgradeFtpID, upgradeFtpPW);

		bFTPupgrade = FTP_FW_UD_WAIT_RST;		
		CstFwUpdateMsg();
		
		bUpdateFail = UpgradePackage();

		//printf("\n\n soon reset....\n\n");

		usleep(2000*1000);
/*
		usleep(2000*1000);

		custom_reboot();
		
		while(1);

*/		
	//}	
}

void FtpIMGUpdate_func(void)
{
		char upgradeFtpUrl[40] = {' ',};
		char upgradeFtpID[10] = {' ',};
		char upgradeFtpPW[16] = {' ',};
		short tmp=6;
		
		usleep(500*1000);
				
		strcpy(upgradeFtpUrl, "ftp://");
		strcpy(&upgradeFtpUrl[tmp], "csms-ftp.tscontech.com");
		tmp += strlen(theConfig.ftpDns);
		strcpy(&upgradeFtpUrl[tmp], "/qr-code-test.png");
		//strcpy(&upgradeFtpUrl[tmp], "/fw/2007a/kproc.sys");

		strcpy(upgradeFtpID, "cptsct");

		strcpy(upgradeFtpPW, "cp01340");
		
		UpgradeSetUrl(upgradeFtpUrl, upgradeFtpID, upgradeFtpPW);
		
		bUpdateFail = UpdateImg();

		//usleep(2000*1000);
}

time_t CstGetTime()
{
	struct timeval mytime;

	gettimeofday(&mytime, NULL);

	return mytime.tv_sec;
}

time_t CstGetTime_Msec()
{
	struct timeval mytime;

	gettimeofday(&mytime, NULL);

	return mytime.tv_usec;
}

time_t CstGetTime_Msec_test()
{
	struct timeval mytime;

	gettimeofday(&mytime, NULL);

	return ((mytime.tv_sec * 1000) + (mytime.tv_usec / 1000));
}


unsigned short CalcuCRC16(const char *data, const int datacnt)
{
	unsigned short CRC16_TABLE[256] =
	{
		0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
		0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
		0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
		0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
		0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
		0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
		0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
		0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
		0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
		0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
		0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
		0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
		0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
		0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
		0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
		0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
		0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
		0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
		0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
		0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
		0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
		0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
		0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
		0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
		0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
		0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
		0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
		0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
		0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
		0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
		0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
		0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
	};

    register unsigned short x, crc;
    int i;
    crc = 0xffff;
    for(i=0; i<datacnt; ++i)
    {
        x = crc ^ data[i];
        crc = (crc >> 8) ^ CRC16_TABLE[x & 0x00ff];
    }
    return(crc);
}

void CurrentDateTime(unsigned char* buf)
{
	char temp[15]= "";
	int index = 0, n;
	time_t time = CstGetTime();
	struct tm *tm = localtime(&time);

	//printf(" 7>> %04d-%02d-%02d:%02d.%02d.%02d \n", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	
	sprintf(temp, "%d%02d%02d%02d%02d%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec );

	for( n=0; n<14; n+=2)
	{
		buf[index] = temp[n] << 4;
		buf[index] |= (temp[n+1] &= 0x0F);
		index++;
	}
}

int RS232Write(char *data, size_t size)
{
	//DumpBuffer("232 Reqeust", data, size);
	return write(CARD_READER_DEV, data, size);
}

int RS232ReadObd(char *data, size_t size)
{
	int ret = read(OBD_READER_DEV, data, size);
	
//	if (ret > 0)
//		DumpBuffer("232 Response", data, ret);
	
	return ret;
}

int RS232Read(char *data, size_t size)
{
	int ret = read(CARD_READER_DEV, data, size);
	
//	if (ret > 0)
//		DumpBuffer("232 Response", data, ret);
	
	return ret;
}

time_t evc_get_time()
{
	struct timeval mytime;

	gettimeofday(&mytime, NULL);

	return mytime.tv_sec;
}

void StartStopCharging(bool bSet, int ch)
{
	if(bSet)
	{	
		stime = evc_get_time();
		printf("[SM3] StartStopCharging Start =%d\n", stime);
	}
	else
	{
		printf("[SM3] StartStopCharging End =%d\n", (evc_get_time() - stime));
		stime = 0;
	}
}
/**
 * @brief 
 * 
 * @return Update Sucess return Zero
 */
int checkUpdate(void)
{
	return bUpdateFail;
}

/**
 * @brief Clear when send 1K
 * 
 */
void clearUpdate(void)
{
	bUpdateFail = 0x1;
}

uint16_t TsctCrc16(uint8_t *buffer, uint16_t buffer_length)
{
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