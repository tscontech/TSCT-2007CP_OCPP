/**
*       @file
*               cstemergencybutton.c
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2017.12.05 <br>
*               author: dyhwang <br>
*               description: <br>
*/
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "ite/audio.h"
#include "audio_mgr.h"
#include "ctrlboard.h"
#include "scene.h"
#include "tsctcfg.h"

//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------
#define MON_TASK_DELAY		500 // ms

//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------

static pthread_t sMonitoringTask;
static bool sMonitoringRunning = false;

static EMBListener sListener = NULL;

//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------
static void* EMBMonitoringTask(void* arg)
{
	ithGpioSetMode(GPIO_EMBUTTON, ITH_GPIO_MODE0);
	ithGpioSetIn(GPIO_EMBUTTON);

	while (sMonitoringRunning)
	{ 
		//if((theConfig.devtype < HBC_TYPE) || (theConfig.devtype == BC2_TYPE)) /// Stand Type...
		//{
			bool pressed = ithGpioGet(GPIO_EMBUTTON); ///y
			//printf("EMBMonitoringTask [%d]\n ",pressed);
			if (sListener != NULL)
						(*sListener)(pressed);
		//}	
		usleep(MON_TASK_DELAY * 1000);
	}
	sMonitoringTask = 0;
	CtLogYellow("[EMB] exit button monitoring thread\n");
}


void EmergencyButtonSetListener(EMBListener listener)
{
	sListener = listener;
}


void EmergencyButtonStartMonitoring(EMBListener listener)
{
	sListener = listener;

	if (sMonitoringTask == 0)
	{
		sMonitoringRunning = true;
		CtLogYellow("[EMB] create monitoring thread..\n");
		pthread_create(&sMonitoringTask, NULL, EMBMonitoringTask, NULL);
		pthread_detach(sMonitoringTask);
	}
}

