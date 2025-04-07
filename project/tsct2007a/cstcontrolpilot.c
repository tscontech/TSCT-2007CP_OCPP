/**
*       @file
*               cstcontrolpilot.c
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
#include "ite/itp.h"
#include "iic/mmp_iic.h"
#include "saradc/saradc.h"

//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------
#define MON_TASK_DELAY		10  // 200

//#define CP_PWM_FREQ		1000
//#define CP_PWM_DUTY_CYCLE	47 // => 53.3%

//ADC081C027
#define CONVERSION_RESULT	0x00
#define ALERT_STATUS		0x01
#define CONFIGURATION		0x02
#define ALERT_LIMIT_UPPER	0x03
#define ALERT_LIMIT_OVER	0x04
#define ALERT_HYSTERESIS	0x05
#define LOWEST_CONVERSION	0x06
#define HIGHEST_CONVERSION	0x07
#define LSB					(3.3/(float)256)
#define CYCLE_TIME_32		1
#define CYCLE_TIME_64		2
#define CYCLE_TIME_128		3
#define CYCLE_TIME_256		4
#define CYCLE_TIME_512		5
#define CYCLE_TIME_1024		6 
#define CYCLE_TIME_2048		7

#if 1
#define CP12V_ADC_VPP		(3.1)
#define CP9V_ADC_VPP		(2.3)
#define CP6V_ADC_VPP		(1.5)


// 212 / 16 
// #define CP12V_ADC_MAX_CONV	250
// #define CP12V_ADC_MIN_CONV	200  //212

// #define CP9V_ADC_MAX_CONV	165 //150
// #define CP9V_ADC_MIN_CONV	140

// #define CP6V_ADC_MAX_CONV	95 //85
// #define CP6V_ADC_MIN_CONV	70 //
/* 220531
#define CP12V_ADC_MAX_CONV	250
#define CP12V_ADC_MIN_CONV	205  // 212

#define CP9V_ADC_MAX_CONV	180 //148
#define CP9V_ADC_MIN_CONV	120

#define CP6V_ADC_MAX_CONV	95 //85
#define CP6V_ADC_MIN_CONV	70 //
*/
#define CP12V_ADC_MAX_CONV	250
#define CP12V_ADC_MIN_CONV	195
  // Max.225 Mean.180

#define CP9V_ADC_MAX_CONV	180 //Max.168  Mean.138
#define CP9V_ADC_MIN_CONV	135

#define CP6V_ADC_MAX_CONV	125 //Max.100 Mean.84
#define CP6V_ADC_MIN_CONV	60 //



#else
#define CP12V_ADC_VPP		(3.00)
#define CP9V_ADC_VPP		(2.68)
#define CP6V_ADC_VPP		(2.35)

#define CP12V_ADC_MAX_CONV	0xFF
#define CP12V_ADC_MIN_CONV	0xE5

#define CP9V_ADC_MAX_CONV	0xDC
#define CP9V_ADC_MIN_CONV	0xC8

#define CP6V_ADC_MAX_CONV	0xBF
#define CP6V_ADC_MIN_CONV	0xA5
#endif
//-----------------------------------------------------------------------
// Local Variable
//-----------------------------------------------------------------------
static int sAdcFd = -1;

static pthread_t sCh1CPMonTask;
bool sCh1CPMonRun = false;
static CPListener sCh1CPListener = NULL;
static CPVoltage sCh1OldVoltage = CP_VOLTAGE_UNKNOWN;




//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------

static CPVoltage GetVoltage(ChannelType ch){

	if(SeccRxData.pwmvoltage >= 110 && SeccRxData.pwmvoltage <= 130)
		return CP_VOLTAGE_12V;	
	else if(SeccRxData.pwmvoltage >= 80 && SeccRxData.pwmvoltage <= 100)
		return CP_VOLTAGE_9V;
	else if(SeccRxData.pwmvoltage >= 50 && SeccRxData.pwmvoltage <= 70)
		return CP_VOLTAGE_6V;
	else if(SeccRxData.pwmvoltage >= 0 && SeccRxData.pwmvoltage <= 20)
		return CP_VOLTAGE_12V;	
	
	return CP_VOLTAGE_UNKNOWN;
}

static bool IsCh1VoltageChanged(CPVoltage voltage) {
	if (voltage != sCh1OldVoltage) 	{
		sCh1OldVoltage = voltage;
		return true;
	}	
	return false;
}



bool IsCableDiconnected(int ch) {

	if(sCh1OldVoltage == CP_VOLTAGE_12V) return true;

	return false;
}

CPVoltage 	voltage;
uint16_t 	lconv=224;
unsigned char conv=0, nSampleCount=4, conv_disp_index=0, conv_disp[20] = {0,};
uint16_t 	writeBuffer_len = 512;
uint16_t 	calibrationOutput;

static void* Ch1CPMonitoringTask(void* arg){

	printf("Ch1CPMonitoringTask Start \r\n");

    while (1)     {
		if (sCh1CPMonRun) 
		{ 	
			voltage = GetVoltage(0);

			if (IsCh1VoltageChanged(voltage))	{				
				//CtLogYellow("[lconv:%d]==>[voltage:%d] \r\n",lconv,voltage);
				switch (voltage)
				{
					case CP_VOLTAGE_12V:				
							CtLogBlue("[CP#0] 12V, cable is connected\n");
							(*sCh1CPListener)(CH1, (unsigned char) lconv, CP_VOLTAGE_12V);
						break;
					case CP_VOLTAGE_9V:
							CtLogRed("[CP#0] 9V, cable is connected\n");
							(*sCh1CPListener)(CH1, (unsigned char) lconv, CP_VOLTAGE_9V);
						break;
					case CP_VOLTAGE_6V:
							CtLogGreen("[CP#0] 6V, request charging..\n");
							(*sCh1CPListener)(CH1, (unsigned char) lconv, CP_VOLTAGE_6V);
						break;
					case CP_VOLTAGE_UNKNOWN:
							CtLogMagenta("[CP#0] unknown Voltage, request charging..\n");
							(*sCh1CPListener)(CH1, (unsigned char) lconv, CP_VOLTAGE_UNKNOWN);
						break;	
					default:
						break;
				}
			}
		}
		else if(sCh1CPMonRun==0)		lconv = 224;
		usleep(MON_TASK_DELAY * 1000);
    }
	sCh1CPMonTask = 0;
	CtLogYellow("[CP#0] exit CP monitoring thread\n");
}


void ControlPilotInit(void){
	printf("[CP] initilaize control pilot..\n");
	printf("WCLK : %d \r\n", ithGetBusClock());

    // IIC_OP_MODE iic_port0_mode = IIC_MASTER_MODE;

	// sAdcFd = open(":i2c1", O_RDWR);

	if (sCh1CPMonTask == 0)		{
		sCh1CPMonRun = false;
		CtLogYellow("[CP#0] create CP monitoring thread..\n");
		pthread_create(&sCh1CPMonTask, NULL, Ch1CPMonitoringTask, NULL);
		pthread_detach(sCh1CPMonTask);
	}
}


void ControlPilotExit(void)
{
	if (sAdcFd != -1)
		close(sAdcFd);
}


/**
 * @brief CP Level Check Function Listener
 * 
 * @param ch 
 * @param listener 
 */
void ControlPilotSetListener(ChannelType ch, CPListener listener)
{
	// bCnctLed = false;

	if(listener != NULL)
		CtLogGreen("Start CP Listener!!");
	else
		CtLogGreen("Stop CP Listener!!");

	sCh1CPListener = listener;
	sCh1OldVoltage = CP_VOLTAGE_UNKNOWN;
}

/**
 * @brief Disable CP Output
 * 
 * @param ch 
 */
void ControlPilotDisablePower(ChannelType ch)
{	
	//sCh1CPMonRun = false;
	ithGpioSetMode(GPIO_CP1_RELAY_CTL, ITH_GPIO_MODE0);
	ithGpioSetOut(GPIO_CP1_RELAY_CTL);
	ithGpioClear(GPIO_CP1_RELAY_CTL);		
	CtLogYellow("[CP#0] PWM Off\n");	
}


void ControlPilotEnablePower(ChannelType ch)
{	
	//sCh1CPMonRun = true;
	ithGpioSetMode(GPIO_CP1_RELAY_CTL, ITH_GPIO_MODE0);
	ithGpioSetOut(GPIO_CP1_RELAY_CTL);
	ithGpioSet(GPIO_CP1_RELAY_CTL);	
	CtLogYellow("[CP#0] PWM On\n");	
}
