/**
*       @file
*               cstwatthourmeter.h
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2017.12.05 <br>
*               author: dyhwang <br>
*               description: <br>
*/
#ifndef __CSTWATTHOURMETER_H__
#define __CSTWATTHOURMETER_H__

typedef void (*WHMListener)(int, float, float, uint32_t);
void WattHourMeterInit(void);
void WattHourMeterStartMonitoring(ChannelType ch, WHMListener listener);
void WattHourMeterStopMonitoring(ChannelType ch);
// bool WattHourMeterCheck(ChannelType ch);

#endif
