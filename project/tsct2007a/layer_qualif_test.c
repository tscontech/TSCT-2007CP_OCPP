#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "scene.h"
#include "ctrlboard.h"

#include "SDL/SDL.h"
#include "tsctcfg.h"

#include "ite/itp.h"
#include "cststring.h"

static ITUBackground* sBackground;
static ITUText* mc1Status;
static ITUText* mc2Status;
static ITUText* whm;

///-------EMG temp------- ///mod
static ITUBackground* mEmgTempBackground;
static ITUText* mEmgTempText1;
static ITUText* mEmgTempText2;
static ITUIcon* mEmgTempButton;
///-------EMG temp-------

uint32_t AMIValue;
static pthread_t GetAMIVMonitoringTask;
static pthread_t GetEMGMonitoringTask;

static void* GetAMIVMonitoringTaskFuntion(void* arg);
static void* GetEMGMonitoringTaskFuntion(void* arg);

bool bMc1Check1;
bool bMc2Check1;
static bool prevEmgPressed = false;

/* widgets:
QualifTestLayer
QualifTestLayerBackground
QualifTestLayer_out3
QualifTestLayer_out2
QualifTestLayer_out1
QualifTestLayer_btn2
QualifTestLayer_btn1
QualifTestLayer_txt3
QualifTestLayer_txt2
QualifTestLayer_txt1
QualifTestLayer_icon
QualifTestLayer_btn5
QualifTestLayer_img
*/

bool qualifTestEnter(ITUWidget* widget, char* param)
{
	if (!sBackground){
		sBackground = ituSceneFindWidget(&theScene, "QualifTestLayerBackground");
        assert(sBackground);

        mc1Status = ituSceneFindWidget(&theScene, "QualifTestLayer_out1");
        assert(mc1Status);
		
        mc2Status = ituSceneFindWidget(&theScene, "QualifTestLayer_out2");
        assert(mc2Status);

		whm = ituSceneFindWidget(&theScene, "QualifTestLayer_out3");
        assert(whm);

		///-------EMG temp-------///mod
		mEmgTempBackground = ituSceneFindWidget(&theScene, "emgTempBackground");
		assert(mEmgTempBackground);

		mEmgTempText1 = ituSceneFindWidget(&theScene, "emgTempText1");
		assert(mEmgTempText1);

		mEmgTempText2 = ituSceneFindWidget(&theScene, "emgTempText2");
		assert(mEmgTempText2);

		mEmgTempButton = ituSceneFindWidget(&theScene, "emgTempButton");
		// assert(mEmgTempButton);
		///-------EMG temp-------
	}
		
	pthread_create(&GetAMIVMonitoringTask, NULL, GetAMIVMonitoringTaskFuntion, NULL);
	pthread_detach(GetAMIVMonitoringTask);

	pthread_create(&GetEMGMonitoringTask, NULL, GetEMGMonitoringTaskFuntion, NULL);
	pthread_detach(GetEMGMonitoringTask);

    return true;
}

static void* GetEMGMonitoringTaskFuntion(void* arg)
{
	ithGpioSetMode(GPIO_EMBUTTON, ITH_GPIO_MODE0);
	ithGpioSetIn(GPIO_EMBUTTON);

	while (1)
	{ 
		// if((theConfig.devtype < HBC_TYPE) || (theConfig.devtype == BC2_TYPE)) /// Stand Type...
		{
			bool pressed = ithGpioGet(GPIO_EMBUTTON);
			// if (sListener != NULL)
				// (*sListener)(pressed);

			///-------EMG temp-------///mod
				ituWidgetSetVisible(mEmgTempBackground, false);
				ituWidgetSetVisible(mEmgTempText1, false);
				ituWidgetSetVisible(mEmgTempText2, false);
				ituWidgetSetVisible(mEmgTempButton, false);
				LEDStopBlink();
			///-------EMG temp-------

			if(pressed && prevEmgPressed){
				// EmergencyDialogShow(5);
				printf("EMG Button pressed=======\n ");
				ituTextSetString(whm, "EMG Button ON");	
				OkCancelDialogShow();
				
				///-------EMG temp-------///mod
				ituWidgetSetVisible(mEmgTempBackground, true);
				ituWidgetSetVisible(mEmgTempText1, true);
				ituWidgetSetVisible(mEmgTempText2, true);
				ituWidgetSetVisible(mEmgTempButton, true);

				MagneticContactorOff();
				ituTextSetString(mc1Status, "OFF");	
				StopPwm(CH1);
				WattHourMeterStopMonitoring(CH1);	
				LEDStartBlink();
				///-------EMG temp-------

				// EmergencyDialogShow(STEP_START);
			}
			prevEmgPressed = pressed;
		}	
		usleep(500 * 1000);
	}
}

static void* GetAMIVMonitoringTaskFuntion(void* arg)
{
	while(1)
	{
		char temp[32] = {0,};
		AMIValue = GetAMIValue();

		memset(temp, 0, 32); 
		sprintf(temp, "%d0 Wh", AMIValue);

		ituTextSetString(whm, temp);	
		ituWidgetSetVisible((ITUWidget*)whm, true);
		usleep(500*1000);
	}
}

bool qualifTestLeave(ITUWidget* widget, char* param)
{
    return true;
}

bool tMc1teststartOnPress(ITUWidget* widget, char* param)
{
     if(!bMc1Check1)
	{	
		ituTextSetString(mc1Status, "ON");	
		MagneticContactorOn();
		bMc1Check1 = true;
	}
	else	
	{
		ituTextSetString(mc1Status, "OFF");	
		MagneticContactorOff();	
		bMc1Check1 = false;;	
	}
    return true;
}

bool tMc2teststartOnPress(ITUWidget* widget, char* param)
{

    return true;
}

bool tExitLayer(ITUWidget* widget, char* param)
{
	ituLayerGoto(ituSceneFindWidget(&theScene, "mainLayer"));
    return true;
}

