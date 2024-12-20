/**
*       @file
*               cstemergencybutton.h
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2017.12.05 <br>
*               author: dyhwang <br>
*               description: <br>
*/
#ifndef __CSTEMERGENCYBUTTON_H__
#define __CSTEMERGENCYBUTTON_H__

typedef void (*EMBListener)(bool);

void EmergencyButtonInit(void);
void EmergencyButtonSetListener(EMBListener listener);
void EmergencyButtonStartMonitoring(EMBListener listener);


#endif
