/**
*       @file
*               cstbuzzer.c
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


//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------

void BuzzerInit(void)
{
	ithGpioSetMode(GPIO_BUZZER, ITH_GPIO_MODE0);
	ithGpioSetOut(GPIO_BUZZER);
	ithGpioClear(GPIO_BUZZER);
}

void BuzzerOn(void)
{
	ithGpioSet(GPIO_BUZZER);
}

void BuzzerOff(void)
{
	ithGpioClear(GPIO_BUZZER);
}

void BuzzerBeep(void)
{
	ithGpioSet(GPIO_BUZZER);
	usleep(10*1000);
	ithGpioClear(GPIO_BUZZER);
}

