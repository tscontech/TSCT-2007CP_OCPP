/**
*       @file
*               cstmagneticcontactor.c
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

#include "cstled.h"

//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------
static bool MCOn = false;

//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------
void MagneticContactorOn(void)
{
//	printf("[MC#0] Magnetic contactor on\n");
	ithGpioSetMode(GPIO_MAGNET_CTL1, ITH_GPIO_MODE0);
	ithGpioSetOut(GPIO_MAGNET_CTL1);
	ithGpioSet(GPIO_MAGNET_CTL1);
	
	MCOn = true;
}

void MagneticContactorOff(void)
{
//	printf("[MC#0] Magnetic contactor off\n");
	ithGpioSetMode(GPIO_MAGNET_CTL1, ITH_GPIO_MODE0);
	ithGpioSetOut(GPIO_MAGNET_CTL1);
	ithGpioClear(GPIO_MAGNET_CTL1);

	MCOn = false;
}

bool CstGetMcstatus(void)
{	
	return MCOn;
}


void RouterContactorOn(void)
{
	printf("[Router] Router contactor on\n");
	ithGpioSetMode(GPIO_ROUTER_CTL, ITH_GPIO_MODE0);
	ithGpioSetOut(GPIO_ROUTER_CTL);
	//ithGpioSet(GPIO_ROUTER_CTL); //2.2.2T
	ithGpioClear(GPIO_ROUTER_CTL); //2.2.2
}

void RouterContactorOff(void)
{
	printf("[Router] Router contactor off\n");
	ithGpioSetMode(GPIO_ROUTER_CTL, ITH_GPIO_MODE0);
	ithGpioSetOut(GPIO_ROUTER_CTL);
	//ithGpioClear(GPIO_ROUTER_CTL);  //2.2.2T
	ithGpioSet(GPIO_ROUTER_CTL); //2.2.2
}