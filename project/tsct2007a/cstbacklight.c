/**
*       @file
*               cstbacklight.c
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

//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------
#define CHECK_TASK_DELAY				10 * 1000 // ms
#define GND_FAULT_MON_TASK_DELAY		1000

//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------
void BacklightOn(void)
{
	ithGpioSet(GPIO_BACKLIGHT);
	LEDOff();
}

void BacklightOff(void)
{
	ithGpioClear(GPIO_BACKLIGHT);
	LEDOn();
}

void BacklightInit(void)
{
	ithGpioSetMode(GPIO_BACKLIGHT, ITH_GPIO_MODE0);
	ithGpioSetOut(GPIO_BACKLIGHT);
	ithGpioSet(GPIO_BACKLIGHT);
}



