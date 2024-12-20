/**
*       @file
*               cstcardreader.h
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2017.12.05 <br>
*               author: dyhwang <br>
*               description: <br>
*/
#ifndef __CSTCARDREADER_H__
#define __CSTCARDREADER_H__

typedef void (*CardReaderListener)(char*, int size);

void CardReaderStopMonitoring(void);
void CardReaderStartMonitoring(CardReaderListener listener);
void CardReaderInit(void);
void RequestPollingStart(void);
void RequestPollingStop(void);

bool RFIDCardReaderCheck(void);

#endif

