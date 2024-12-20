/**
 * @file tsctfaultmanage.c
 * @author GYJ (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-03-30
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <sys/times.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "SDL/SDL.h"
//#include <time.h>
#include <strings.h>    /* for bzero, strcasecmp, and strncasecmp */
#include "scene.h"
//#include "tsctclient.h"
#include "tsctcommon.h"
#include "tsctcfg.h"
#include "ctrlboard.h"
//#include "tsctpacket.h"

pthread_t sFaultManageTask = 0;

extern bool TouchI2cErrFlg;

extern bool bAmiErrChk;

// static void ReportEvent(SERVER_EVENT_CODE EVENT_CODE)
// {
    // uint8_t reportWait_cnt = 0;
    // printf("[ReportEvent] Report Event %d\r\n", EVE_TOUCH);
    // //shmDataAppInfo.app_order = APP_ORDER_ERR_REBOOT;
    // EventCode_buf |= 1 << EVENT_CODE;
    // if(EVENT_CODE == EVE_TOUCH)
    //     ShowWhmErrorDialogBox(ERR_TOUCH);
    // else if(EVENT_CODE == EVE_RF)
    //     ShowWhmErrorDialogBox(ERR_RFID);
    // else if(EVENT_CODE == EVE_AMI)
    //     ShowWhmErrorDialogBox(ERR_AMI);
    // while((reportWait_cnt++ < 10) && (EventCode_buf&(1 << EVENT_CODE))){  // wait for Event Report (c1)
    //     sleep(1);
    // }
    // printf("\n\n soon reset....\n\n");
    // custom_reboot();
    // while(1);
// }

static void* FaultManageThread(void* arg)
{
    uint8_t tmp_cnt = 0, touchFaultCheck_cnt[2] = {0,0};
    sleep(5);
    while(1)
    {
        sleep(1);

        if(bGloAdminStatus) continue;

        // if(tmp_cnt++ == 30){
        //     tmp_cnt = 0;
		// 	if(touch_getDummy() < 0){
        //         touchFaultCheck_cnt[0]++;
        //         if(touchFaultCheck_cnt[0] > 5)                
        //             ReportEvent(EVE_TOUCH);
        //     }    
        //     else touchFaultCheck_cnt[0] = 0;            
		// }
        // else if(tmp_cnt == 20 && shmDataAppInfo.app_order == APP_ORDER_WAIT){
        //     rfredercheck = false;
        //     RFIDCardReaderCheck();
        //     printf("[FaultManageThread] RFID Check %d\r\n", rfredercheck);
        //     if(rfredercheck)
        //     {
        //         rfredercheck = false;	
        //         ReportEvent(EVE_RF);
        //         //ShowWhmErrorDialogBox(EVENT_CODE);					    
        //         //CstErrMsgBoxShow(ERR_MSG_RFID, 0);
        //     }
        // }
        // else if (tmp_cnt == 10){
        //     if(bAmiErrChk){
        //         touchFaultCheck_cnt[1]++;
        //         if(touchFaultCheck_cnt[1] > 5)                
        //             ReportEvent(EVE_AMI);
        //     }    
        //     else touchFaultCheck_cnt[1] = 0;   
        // }
    }
    sFaultManageTask = 0;
}

void FaultManageInit(void)
{
	if (sFaultManageTask == 0)
	{
		CtLogYellow(" create Fault Management thread..\n");
		pthread_create(&sFaultManageTask, NULL, FaultManageThread, NULL);
		pthread_detach(sFaultManageTask);
	}    
}