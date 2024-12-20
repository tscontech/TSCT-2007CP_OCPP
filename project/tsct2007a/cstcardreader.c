/**
*       @file
*               cstcardreader.c
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
#include "tsctcommon.h"
#include "cstcardreader.h"
#include "tsctcfg.h"

#include "ite/ith.h"
#include "ite/itp.h"
// #include "openrtos/queue.h"
#include <errno.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
// #include "openrtos/FreeRTOS.h"
// #include "openrtos/queue.h"
// #include "ite/ith.h"
// #include "ite/itp.h"




//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------
#define READ_232_TASK_DELAY		100 // ms
#define READ_ID_TASK_DELAY		100 // ms

//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------

static pthread_t sReadTask;
static ReadQueue sReadQueue;
static bool sReadRunning = true;

static pthread_t sMonitoringTask1;
static bool sMonitoringTaskExit = false;
static CardReaderListener sListener = NULL;

enum{
	UID_CARD=1,
	TMONEY_CARD,
	OTHER_CARD
};

//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------

//********************* RX Function*********************************** */

static void* RS232ReadTask(void* arg)
{
	ReadQueueInit(&sReadQueue);

    while(1)
    {
		int i;
		char buffer[256];

		if(sReadRunning){
			memset(buffer, 0, 256);
			int bufsize = RS232Read(buffer, 256);
			for (i = 0; i < bufsize; i++)
				ReadQueueEnqueue(&sReadQueue, buffer[i]);	
		}
        usleep(READ_232_TASK_DELAY * 800);
	}
		
	sReadTask = 0;
	CtLogYellow("[RFID] exit read rs232 thread\n");
}

//********************* TX Function*********************************** */

// Not Use
static void RequestAutoReadPolling(void) //auto read
{
	char request[] = {0x02,0x8A,0x00,0x00, 0x8A, 0x03};

	request[4] = (char)(request[1] + request[2]+request[3]);
	write(CARD_READER_DEV, request, 6);

	//	DumpBuffer("232 Reqeust", request, 12);
		
	// sleep enough for RS232ReadTask to get response packet
    usleep(300*1000);

	int size = 0;
	char response[256];
	memset(response, 0, 256);
	while (!ReadQueueIsEmpty(&sReadQueue)) {
		response[size++] = ReadQueueDequeue(&sReadQueue);
	}

	DumpBuffer("Auto#1 Response", response, size);	
}

// Not Use
static void RequestAutoReadBasic(void) //auto read
{
	char request[] = {0x02,0x0A,0x00,0x00, 0x00, 0x03};

	request[4] = (char)(request[1] + request[2]+request[3]);
	write(CARD_READER_DEV, request, 6);


	// sleep enough for RS232ReadTask to get response packet
    usleep(300*1000);

	int size = 0;
	char response[256];

	memset(response, 0, 256);

	while (!ReadQueueIsEmpty(&sReadQueue)) {
		response[size++] = ReadQueueDequeue(&sReadQueue);
	}

	DumpBuffer("Auto#2 Response", response, size);	
}


void RequestPollingStart(void)
{
	char request[] = {0x02,0x0E,0x00,0x00, 0x00, 0x03};

	request[4] = (char)(request[1] + request[2]+request[3]);

	write(CARD_READER_DEV, request, 6);
	
	// sleep enough for RS232ReadTask to get response packet
	usleep(300*1000);

	int size = 0;
	char response[256];

	memset(response, 0, 256);

	while (!ReadQueueIsEmpty(&sReadQueue)) {
		response[size++] = ReadQueueDequeue(&sReadQueue);
	}
	
//	DumpBuffer("PollingStart Response", response, size);	
}

void RequestPollingStop(void)
{
	char request[] = {0x02, 0x8F, 0x00, 0x00, 0x8F, 0x03};	

	write(CARD_READER_DEV, request, 6);

	// sleep enough for RS232ReadTask to get response packet
    usleep(300*1000);

	int size = 0;
	char response[256];

	memset(response, 0, 256);

	while (!ReadQueueIsEmpty(&sReadQueue)) {
		response[size++] = ReadQueueDequeue(&sReadQueue);
	}

	DumpBuffer("PollingStop Response", response, size);	
}


// Not Use
static char *IntToHexASCII_16(char *pBuf, char len, char *pOut)
{
	int i, j=0;
	char lowByte, highByte;

	printf(" [[[ ");
	for(i=0; i<len; i++)
	{
		highByte = (*pBuf) >> 4;
		lowByte = (*pBuf)&0x0F;

		highByte = (highByte % 10);
		lowByte = (lowByte % 10);
		
		pOut[j++] = (highByte + 0x30);	
		pOut[j++] = (lowByte + 0x30);			
	
		pBuf++;
	}
	
	for(i=0; i<len*2; i++)
		printf(" %c,", pOut[i]);
	
	printf(" ]]]\n");
	
	return pOut;
}

// Not Use
static char *IntToHexASCII_8(char *pBuf, char len, char *pOut)
{
	int i;

	printf(" [[[ ");
	for(i=0; i<len; i++)
	{
		pOut[i] = (*pBuf + 0x30);	
		
		if(*pBuf > 9 )	pOut[i] = pOut[i] +1;
		
		printf(" %c,", pOut[i]);
		pBuf++;
	}
	printf(" ]]]\n");
	
	return pOut;
}


static char *HexToASCII_8(char *pBuf, char len, char *pOut)
{
	int i;

	printf(" [[[ ");
	for(i=0; i<len*2; i++)
	{
		char byteLow = (char)((*pBuf)&0x0F);
		char byteHigh = (char)(((*pBuf)>>4)&0x0F);
		
		//pOut[i] = (byteLow + 0x30);	
		//pOut[i+1] = (byteHigh + 0x30);	

		pOut[i] = (byteHigh + 0x30);	
		pOut[i+1] = (byteLow + 0x30);	

		
		if(byteHigh > 9 )	pOut[i] = pOut[i] +0x7;
		if(byteLow > 9 )	pOut[i+1] = pOut[i+1] +0x7;
		
		printf(" %c%c,", pOut[i+1], pOut[i]);
		pBuf++;
		i++;
	}
	printf(" ]]]\n");
	
	return pOut;
}

static char *HexToASCII_16(char *pBuf, char len, char *pOut)
{
	int i;

	printf(" [[[ ");
	for(i=0; i<len*4; i++)
	{
		char byteInt0 = (char)(((*pBuf)&0x0F)%10);
		char byteInt1 = (char)(((*pBuf)&0x0F)/10);
		char byteInt2 = (char)((((*pBuf)>>4)&0x0F)%10);
		char byteInt3 = (char)((((*pBuf)>>4)&0x0F)/10);
		
		/*
		pOut[i] = (byteInt0 + 0x30);	
		pOut[i+1] = (byteInt1 + 0x30);	
		pOut[i+2] = (byteInt2 + 0x30);	
		pOut[i+3] = (byteInt3 + 0x30);	
		*/
		pOut[i*4] = 	(byteInt3 + 0x30);	
		pOut[i*4+1] = 	(byteInt2 + 0x30);	
		pOut[i*4+2] = 	(byteInt1 + 0x30);	
		pOut[i*4+3] = 	(byteInt0 + 0x30);

		printf(" %c%c%c%c,", pOut[i+3], pOut[i+2], pOut[i+1], pOut[i]);
		pBuf++;
		//i++;
	}
	printf(" ]]]\n");
	
	return pOut;
}


static void* UidAutoReadTask(void* arg)
{
	int size = 0, nCnt=0;
	char response[256]={0,};
	char bReceived;

	RequestPollingStart();

	usleep(1000);

	while (!sMonitoringTaskExit)
	{
		bReceived = 0;
		size = 0;

		memset(response, 0, 256);		
		while (!ReadQueueIsEmpty(&sReadQueue)) {
			response[size++] = ReadQueueDequeue(&sReadQueue);
		}
		
		if (response[0] == 0x02 && response[2] == 0x01 ) 
		{
			if (response[1] == 0x17)
				bReceived = UID_CARD;
			else if (response[1] == 0x3d)
				bReceived = TMONEY_CARD;
			else if (response[1] == 0x3e)
				bReceived = OTHER_CARD;
		}

		
		if(bReceived > 0)
		{			
			char Out[16];

			DumpBuffer("232 Response", response, size);
			
			if (sListener)
			{
				if(bReceived == UID_CARD)
					(*sListener)(HexToASCII_16(&response[6], 4, Out), 16); 
				else
					(*sListener)(HexToASCII_8(&response[5], 8, Out), 16);
			}
			else
			{
				DumpBuffer("Card Info", &response[6], (response[4]-1));
				HexToASCII_16(&response[6], 4, Out);
			}
			
			RequestPollingStop();
		}

		if(nCnt++ > 100)
		{
			nCnt = 0;
			RequestPollingStart();
			usleep(300 * 1000);
		}

		usleep(READ_ID_TASK_DELAY * 1000);
	}
	
	sMonitoringTask1 = 0;
	CtLogYellow("Out UidAutoReadTask [%d]", sMonitoringTask1);
}


void CardReaderStopMonitoring(void)
{
	printf("CardReaderStopMonitoring : \n");

	RequestPollingStop();
	
	sMonitoringTaskExit = true;

	sReadRunning = false;
}

void CardReaderStartMonitoring(CardReaderListener listener)
{
	sListener = listener;

	CtLogYellow("CardReaderStartMonitoring %d", sMonitoringTask1);

	sReadRunning = true;

	if (sMonitoringTask1 == 0)
	{
		sMonitoringTaskExit = false;
		CtLogYellow("[RFID] create uid auto read thread..\n");

		pthread_create(&sMonitoringTask1, NULL, UidAutoReadTask, NULL);
		pthread_detach(sMonitoringTask1);
	}
}


//********************* Test Function*********************************** */

static int RequestTest(void)
{
	char request[] = {0x02,0x0F,0x00,0x00, 0x0F, 0x03};

	request[4] = (char)(request[1] + request[2]+request[3]);
	write(CARD_READER_DEV, request, 6);

	usleep(500*1000);

	int size = 0;
	char response[256];
	memset(response, 0, 256);
	while (!ReadQueueIsEmpty(&sReadQueue)) {
		response[size++] = ReadQueueDequeue(&sReadQueue);
	}
	
	return size;
}

bool RFIDCardReaderCheck(void)
{	
	int size = 0;

	sReadRunning = true;

	size = RequestTest();

	if (size > 0)
	{
		rfredercheck = false;
		CtLogGreen("RFIDReaderCheckTest.. OK\n");
	}
	else
	{
		rfredercheck = true;	
		CtLogRed("RFIDReaderCheckTest.. Fail\n");
	}
		
	sReadRunning = false;


	return rfredercheck;
}


void CardReaderInit(void)
{
	if (sReadTask==0)
	{
		CtLogYellow("[RFID] create rs232 read thread..\n");
		pthread_create(&sReadTask, NULL, RS232ReadTask, NULL);
		pthread_detach(sReadTask);
	}		

	RFIDCardReaderCheck();
}

