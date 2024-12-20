/**
*       @file
*               cstcontrolpilot.h
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2017.12.05 <br>
*               author: dyhwang <br>
*               description: <br>
*/
#ifndef __CSTCONTROLPILOT_H__
#define __CSTCONTROLPILOT_H__

typedef enum CPVoltage_t {
	CP_VOLTAGE_UNKNOWN,
    CP_VOLTAGE_12V,
	CP_VOLTAGE_9V,
	CP_VOLTAGE_6V,
	CP_VOLTAGE_0V,
} CPVoltage;

void ControlPilotInit(void);
void ControlPilotExit(void);

typedef void (*CPListener)(int, unsigned char, int);
void CPListenerOnAhead(int ch, unsigned char nAdcValue, CPVoltage voltage);
void ControlPilotSetListener(ChannelType ch, CPListener listener);
void ControlPilotDisablePower(ChannelType ch);
void ControlPilotEnablePower(ChannelType ch);
unsigned char ADCReadConversion(ChannelType ch);

#endif
