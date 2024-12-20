/**
*       @file
*               cstled.c
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2017.12.05 <br>
*               author: dyhwang <br>
*               description: <br>
*/
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "ite/ith.h"
#include "ctrlboard.h"
#include "tsctcfg.h"

//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------
static pthread_t sChBlinkTask;
static bool sLedOn = false;
static bool sBlinkRunning = false;

unsigned int ledPin = GPIO_LED_RAMP1; 
	
//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------
void LEDOn(void)
{
	ithGpioSet(ledPin);
	sLedOn = true;	
}

void LEDOff(void)
{
	ithGpioClear(ledPin);
	sLedOn = false;		
}

static void* ChBlinkTask(void* arg)
{
	while(1)
	{
		if(sBlinkRunning)
		{
			if (sLedOn) 	LEDOff();
			else			LEDOn();
		}
		sleep(1);
	}

	CtLogYellow("[LED#0] exit blink thread..\n");
}

void LEDStartBlink(void)
{
	sBlinkRunning = true;
	CtLogYellow("[LED#] Start blink LED ..\n");
}

void LEDStopBlink(void)
{
	sBlinkRunning = false;
	LEDOff();
	CtLogYellow("[LED#] Stop blink LED.\n");
}

void LEDInit(void)
{
	ithGpioSetMode(ledPin, ITH_GPIO_MODE0);
	ithGpioSetOut(ledPin);
	ithGpioClear(ledPin);
	sLedOn = false;	

	// ithGpioSetMode(ledPin[CH2], ITH_GPIO_MODE0);
	// ithGpioSetOut(ledPin[CH2]);
	// ithGpioClear(ledPin[CH2]);
	// sLedOn[CH2] = false;	

	CtLogYellow("[LED] create blink thread..\n");
	pthread_create(&sChBlinkTask, NULL, ChBlinkTask, NULL);
	pthread_detach(sChBlinkTask);
}

