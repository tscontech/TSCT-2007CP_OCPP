#include <stdlib.h>
#include <stdio.h>
#include "ite/ith.h"
#include "ite/itp.h"
#include "uart_fifo.h"

static int UartFifoObjInit(UART_OBJ *pUartObj);
static int UartFifoObjSend(UART_OBJ *pUartObj, char *ptr, int len);
static int UartFifoObjRead(UART_OBJ *pUartObj, char *ptr, int len);
static void UartFifoObjTerminate(UART_OBJ *pUartObj);

static UART_FIFO_OBJ UART_STATIC_FIFO_OBJ[6] = {};

UART_OBJ* iteNewUartFifoObj(ITHUartPort port, UART_OBJ *pUartObj)
{
	UART_OBJ* pObj = iteNewUartObj(port, pUartObj);
	if (pObj == NULL)
		return NULL;

	UART_FIFO_OBJ* pUartFifoObj = &UART_STATIC_FIFO_OBJ[UART_JUDGE_PORT(port)];

	pObj->pMode = pUartFifoObj;

	//Changing base class interface to access pMode class functions
	pObj->init = UartFifoObjInit;
	pObj->send = UartFifoObjSend;
	pObj->read = UartFifoObjRead;
	pObj->dele = UartFifoObjTerminate;

	return pObj;
}

static int UartFifoObjInit(UART_OBJ *pUartObj)
{
	UART_FIFO_OBJ* pUartFifoObj = pUartObj->pMode;

	/* Set the required protocol. */
	ithUartReset(pUartObj->port, pUartObj->baud, pUartObj->parity, 1, 8);

	ithUartSetMode(pUartObj->port, ITH_UART_DEFAULT, pUartObj->txPin, pUartObj->rxPin);

	return 0;
}

static int UartFifoObjSend(UART_OBJ *pUartObj, char *ptr, int len)
{
	UART_FIFO_OBJ* pUartFifoObj = pUartObj->pMode;
	uint32_t lasttime = 0, timeout_val = pUartObj->timeout;
	int count = 0;

#ifdef CFG_UART_FORCE_FLUSH
	ithEnterCritical();
	timeout_val = 0;
#endif
	if (timeout_val)
		lasttime = itpGetTickCount();
	while (count < len)
	{
		// Is transmitter ready?
		if (!ithUartIsTxFull(pUartObj->port))
		{
			ithUartPutChar(pUartObj->port, *ptr++);// Write character from uart
			count++;
		}
		else if (timeout_val)
		{
			if (itpGetTickDuration(lasttime) < timeout_val)
				usleep(1);
			else
				break;
		}
	}
#ifdef CFG_UART_FORCE_FLUSH
	ithExitCritical();
#endif
	return count;
}

static int UartFifoObjRead(UART_OBJ *pUartObj, char *ptr, int len)
{
	UART_FIFO_OBJ* pUartFifoObj = pUartObj->pMode;
	uint32_t lasttime = 0, timeout_val = pUartObj->timeout;
	int count = 0;

	if (timeout_val)
		lasttime = itpGetTickCount();
	while (count < len)
	{
		// Is a character waiting?
		if (ithUartIsRxReady(pUartObj->port))
		{
			*ptr++ = ithUartGetChar(pUartObj->port);// Read character from uart
			count++;
			continue;
		}
		else if (timeout_val)
		{
			if (itpGetTickDuration(lasttime) < timeout_val)
				usleep(1);
			else
				break;
		}
		else
			break;
	}

	return count;
}

static void UartFifoObjTerminate(UART_OBJ *pUartObj)
{
	UART_FIFO_OBJ* pUartFifoObj = pUartObj->pMode;

	pUartObj->init = NULL;
	pUartObj->dele = NULL;
	pUartObj->send = NULL;
	pUartObj->read = NULL;

	pUartFifoObj = NULL;

	pUartObj = NULL;
}
