#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "scene.h"
#include "ctrlboard.h"

static bool sFirstEnter = true;
bool bAdminExit = false;
bool bBootFirst = true;

/* widgets:
standbyLayer
standbyBackground
*/
bool StandbyOnEnter(ITUWidget* widget, char* param) {

	if(bFTPupgrade == FTP_FW_UD_IDLE)
	{
		if(bBootFirst)		
		{
			// CstPlayAudioMsg(AUDIO_WELCOM);
			//AudioPlay("A:/sounds/welcom.wav", NULL);
			bBootFirst = false;
		}
		else
		{
			// CstPlayAudioMsg(AUDIO_TRYCHARGE);
			// AudioPlay("A:/sounds/tryCharging.wav", NULL);
		}	
	}
	else
	{
		bAdminExit = true;
		sleepOn1chCheck = false;
	}

	CtLogRed("StandbyOnEnter  ..========== [%d]\n",sleepOn1chCheck);

	if(!sleepOn1chCheck)
	{
		//CstStartGroundFaultStartMonitoring(GroundFaultListenerOnCharge);	
		
		if(bAdminExit)
		{
			bAdminExit = false;			
			GotoStartLayer();
		}
		
		if (sFirstEnter)
		{
			ControlPilotDisablePower(CH1);			
			
			//if(theConfig.devchanel == 2)
				ControlPilotDisablePower(CH2);			
			
			sleep(2);	
			
			BacklightOn();
			GotoStartLayer();
			
			sFirstEnter = false;			
		}
		else
		{		
			BacklightOn();
		}
	}
    return true;
}

bool StandbyOnLeave(ITUWidget* widget, char* param)
{
    return true;
}

bool StandbyScreenOnPress(ITUWidget* widget, char* param)
{
    return true;
}

