/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL RTC internal functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include "../itp_cfg.h"

//#define TMUS_ENABLE
#define RTC_TIMER		(CFG_INTERNAL_RTC_TIMER - 1)
#define RTC_YEAR_2106_TIMESTAMP (0xFFFFFFFF)
#define RTC_YEAR_2100_TIMESTAMP (0xF485E680)

#ifdef CFG_RTC_6_8P
#define RTC_FILE        "B:/RtcAdjust"
#define ADJUST_INTERVAL 8640
#define ADJUST_SEC      1
#else
#define RTC_FILE        "B:/RtcAdjust"
#define ADJUST_INTERVAL 0
#define ADJUST_SEC      0
#endif

//#define DEBUG_PRINT     ithPrintf
#define DEBUG_PRINT(...)


static pthread_t adjustTask, updateTask;
static sem_t updateSem, adjustSem;
static long updateSec = 0, lastUpdateSec = 0;
static long gAdjustInterval = 0;

static void RtcSecIntrHandler(void* arg)
{
#ifndef TMUS_ENABLE
    ithTimerSetCounter(RTC_TIMER, 0);  // reset counter
#endif
    ithRtcClearIntr(ITH_RTC_SEC);
}

static void RtcAdjustTask(void *arg)
{
    int h = 0, r = 0;
    int i = 0;
    long timeInterval = 0;
    int fix_h = 0, fix_r = 0;

    DEBUG_PRINT("[%s] start\n", __FUNCTION__);

    // pre-calulate fixed time interval
    fix_h = ADJUST_INTERVAL / 3600;
    fix_r = ADJUST_INTERVAL - (fix_h*3600);

    // the first loop's time interval
    sem_wait(&adjustSem);
    h = gAdjustInterval / 3600;
    r = gAdjustInterval - (h*3600);

    DEBUG_PRINT("[%s] h=%d r=%d gAjustInterval=%d\n", __FUNCTION__, h, r, gAdjustInterval);

    while(1) {
        // NOTICE: sleep(time): max value of time is 4294s(uint64)  
        for(i = 0; i < h; i++) {
            // sleep 3600s 
            sleep(3600);
        }
        sleep(r);

        // Start to Adjust RTC
        updateSec = ithRtcGetTime();
        DEBUG_PRINT("[%s] lastUpdateSec=%d, updateSec=%d gAdjustInterval=%d h=%d r=%d\n", __FUNCTION__, lastUpdateSec, updateSec, gAdjustInterval, h, r);
        timeInterval = updateSec - lastUpdateSec;
        
        if(timeInterval > gAdjustInterval) {
            // RTC is faster than swTimer
            //updateSec = updateSec - (timeInterval - gAdjustInterval);
            updateSec = updateSec - ADJUST_SEC;
            ithRtcSetTime(updateSec);
            DEBUG_PRINT("[%s] 1. updateSec=%d\n", __FUNCTION__, updateSec);
        }
#if 0
        else if(timeInterval < gAdjustInterval){
            // RTC is slower than swTimer
            //updateSec = updateSec + (gAdjustInterval - timeInterval);
            updateSec = updateSec + ADJUST_SEC;
            ithRtcSetTime(updateSec);
            DEBUG_PRINT("[%s] 2. updateSec=%d\n", __FUNCTION__, updateSec);
        }
#endif
        else if(gAdjustInterval < ADJUST_INTERVAL) {
            updateSec = updateSec - ADJUST_SEC;
            ithRtcSetTime(updateSec);
            DEBUG_PRINT("[%s] 3. updateSec=%d\n", __FUNCTION__, updateSec);
        }
        else {
            //lastUpdateSec - updateSec == ADJUST_INTERVAL
            DEBUG_PRINT("[%s] 4. updateSec=%d\n", __FUNCTION__, updateSec);           
        }
        lastUpdateSec = updateSec;

        // reinit the second-loop's time interval
        h = fix_h;
        r = fix_r;
        gAdjustInterval = ADJUST_INTERVAL;
        
        // RtcUpdateTask wakeup
        sem_post(&updateSem);
    }    
}

static void RtcUpdateTask(void *arg)
{
    FILE *fptr = NULL;
    long lastSec = 0;
    
    DEBUG_PRINT("[%s] start\n", __FUNCTION__);

    while(1) {
        // Wait for RtcAdjustTask
        sem_wait(&updateSem);
        DEBUG_PRINT("[%s] currSec=%d gAdjustInterval=%d\n", __FUNCTION__, updateSec, gAdjustInterval);
        
        //record rtc
        fptr = fopen(RTC_FILE, "w");
        fprintf(fptr, "%d %d", updateSec, gAdjustInterval);
        fclose(fptr);        
        
        //flush nor
#if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
        ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
#endif
    }
}

void RtcAdjustStopTask(void)
{
    // Destroy Sem
    sem_destroy(&updateSem);
    sem_destroy(&adjustSem);

    // Stop RtcUpdate task
    pthread_cancel(updateTask);

    // Stop RtcAdjust task
    pthread_cancel(adjustTask);
}

void RtcAdjustInitTask(void *arg)
{
    FILE *fptr = NULL;
    long currSec = 0, lastSec = 0, adjMul = 0, adjIntval = 0;

    if(access(RTC_FILE, 0)) {
        currSec = ithRtcGetTime();
        lastUpdateSec = currSec;

        // the first adjust-interval time
        gAdjustInterval = ADJUST_INTERVAL;
        sem_post(&adjustSem);

        // no RTC file in nor, create a new ones
        fptr = fopen(RTC_FILE, "w");
        if(fptr == NULL) {
            printf("[%s] fopen(%s) fail\n", __FUNCTION__, RTC_FILE);
            return;
        }
        
        fprintf(fptr, "%d %d", currSec, gAdjustInterval);
        fclose(fptr);

        DEBUG_PRINT("\n[%s] lastUpdateSec=%d gAdjustInterval=%d\n", __FUNCTION__, lastUpdateSec, gAdjustInterval);
    }
    else {
        // RTC file already exist
        fptr = fopen(RTC_FILE, "r");
        if(fptr == NULL) {
            printf("[%s] fopen(%s) fail\n", __FUNCTION__, RTC_FILE);
            return;
        }
        
        fscanf(fptr, "%d %d", &lastSec, &adjIntval);
        fclose(fptr);

        currSec = ithRtcGetTime();
        if(currSec >= lastSec + adjIntval) {
            DEBUG_PRINT("[%s] start Adjust RTC\n", __FUNCTION__);
            // Adjust RTC, and update to RTC file
            adjMul = (currSec - lastSec) / ADJUST_INTERVAL;
        
            ithRtcSetTime(currSec - (ADJUST_SEC*adjMul));
            lastUpdateSec = currSec - (ADJUST_SEC*adjMul);            

            // the first adjust-interval time
            //gAdjustInterval = (lastSec + (adjMul + 1) * ADJUST_INTERVAL) - lastUpdateSec;
            gAdjustInterval = ADJUST_INTERVAL;
            sem_post(&adjustSem);

            // Record
            fptr = fopen(RTC_FILE, "w");
            fprintf(fptr, "%d %d", lastUpdateSec, gAdjustInterval);
            fclose(fptr);

            DEBUG_PRINT("\n[%s] lastSec=%d currSec=%d adjSec=%d adjMul=%d gAdjustInterval=%d adjIntval=%d\n", __FUNCTION__, lastSec, currSec, currSec-(ADJUST_SEC*adjMul), adjMul, gAdjustInterval, adjIntval);
        }
        else {
            // the first adjust-interval time
            lastUpdateSec = currSec;
            gAdjustInterval = adjIntval - (currSec - lastSec);
            sem_post(&adjustSem);

            DEBUG_PRINT("\n[%s] lastSec=%d lastUpdateSec=%d gAdjustInterval=%d adjIntval=%d\n", __FUNCTION__, lastSec, lastUpdateSec, gAdjustInterval, adjIntval);
        }
    }

#if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
    ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
#endif

}

void itpRtcAdjustInit(void)
{
    pthread_t initTask;
    int tmp;

    // Init Sem
    sem_init(&updateSem, 0, 0);
    sem_init(&adjustSem, 0, 0);

    // Create a routine task for adusting RTC periodically
    pthread_create(&adjustTask, NULL, RtcAdjustTask, NULL);

    // Create RTC file or adjust RTC when bootup
    pthread_create(&initTask, NULL, RtcAdjustInitTask, NULL);

    // Create a task for update RtcAdjust file
    pthread_create(&updateTask, NULL, RtcUpdateTask, NULL);
}

#ifdef TMUS_ENABLE
static long RtcGetUs(void)
{
	long usec = 0x0;

	usec = (long)ithReadRegA(ITH_TIMER_BASE + ITH_TIMER_TMUS_COUNTER_REG) & 0x0FFFFFFF;

	return usec;
}
#endif

void itpRtcInit(void)
{
    //printf("RTC itpRtcInit:long size = %d, %d\n",sizeof(long),sizeof(int));
    {
        // check RTC clk is live
        uint32_t wd_counter = 0;
        uint32_t timeout = 100000000;
        
        ithSetRegBitA(ITH_WD_BASE + ITH_WD_CR_REG, 4);
        ithSetRegBitA(ITH_WD_BASE + ITH_WD_CR_REG, 0);

        wd_counter = ithWatchDogGetCounter();

        while(wd_counter == ithWatchDogGetCounter()) {
            if(timeout <= 0) {
                ithPrintf("RTC's power is dead\n");
                timeout = 100000000;
            }
            timeout--;
        }

        ithClearRegBitA(ITH_WD_BASE + ITH_WD_CR_REG, 0);
        ithClearRegBitA(ITH_WD_BASE + ITH_WD_CR_REG, 4);
    }

    ithRtcInit(CFG_RTC_EXTCLK);
    if (ithRtcEnable())
    {
        LOG_INFO "First time boot\n" LOG_END
        ithRtcSetTime(CFG_RTC_DEFAULT_TIMESTAMP);
    }

#ifdef TMUS_ENABLE
	// init TmUs counter to calc usec of gettimeofday()
	ithWriteRegA(ITH_TIMER_BASE + ITH_TIMER_TMUS_EN_REG, (ithGetBusClock()/1000000)-1);
	ithWriteRegMaskA(ITH_TIMER_BASE + ITH_TIMER_TMUS_EN_REG, 1<<31, 1<<31);
#else
	// init timer4 to calc usec of gettimeofday()
    ithTimerReset(RTC_TIMER);
    ithTimerCtrlEnable(RTC_TIMER, ITH_TIMER_UPCOUNT);
    ithTimerSetCounter(RTC_TIMER, 0);
    ithTimerEnable(RTC_TIMER);
#endif

    // init rtc sec interrupt
    ithRtcCtrlEnable(ITH_RTC_INTR_SEC);
    ithIntrRegisterHandlerIrq(ITH_INTR_RTCSEC, RtcSecIntrHandler, NULL);
    ithIntrSetTriggerModeIrq(ITH_INTR_RTCSEC, ITH_INTR_EDGE);
    ithIntrEnableIrq(ITH_INTR_RTCSEC);
    
}

long itpRtcGetTime(long* usec)
{
    long sec1, sec2;
    do 
    {
        sec1 = ithRtcGetTime();
        if (usec) {
#ifdef TMUS_ENABLE
			*usec = RtcGetUs();
#else
            *usec = ithTimerGetTime(RTC_TIMER);
#endif
        }

        sec2 = ithRtcGetTime();
        
    } while (sec1 != sec2);

    // LOG_DBG "[RTC] get time: sec=%x, usec=%x\n", sec1, *usec LOG_END

    if( ((unsigned long)sec1 < (unsigned long)CFG_RTC_DEFAULT_TIMESTAMP) || 
        ((unsigned long)sec1 > (unsigned long)RTC_YEAR_2100_TIMESTAMP) )
    {
        /* SW handle internal RTC low bettery power issue */
        /* report default timestemp when RTC < default or RTC > year 2100 */
        long stemp_def = (long)CFG_RTC_DEFAULT_TIMESTAMP;
        long stemp_2100 = (long)RTC_YEAR_2100_TIMESTAMP;
        
        LOG_ERR "[RTC] get time ERROR(ori=%x, default=%x, 2100=%x):[#%d]\n", sec1, stemp_def, stemp_2100, __LINE__ LOG_END
        sec1 = (long)CFG_RTC_DEFAULT_TIMESTAMP;
    }

    return sec1;
}

void itpRtcSetTime(long sec, long usec)
{
    FILE *fptr = NULL;
    long currSec = 0;

    LOG_DBG "[RTC] set time: sec=%x, usec=%x\n", sec, usec LOG_END
    
    if( ((unsigned long)sec < (unsigned long)CFG_RTC_DEFAULT_TIMESTAMP) || 
        ((unsigned long)sec > (unsigned long)RTC_YEAR_2100_TIMESTAMP) )
    {
        /* SW handle internal RTC low bettery power issue */
        /* set default timestemp when sec < default or sec > year 2100 */
        LOG_ERR "[RTC] set time ERROR(ori=%x, default=%x):[#%d]\n", sec, CFG_RTC_DEFAULT_TIMESTAMP, __LINE__ LOG_END
        ithRtcSetTime(CFG_RTC_DEFAULT_TIMESTAMP + (usec / 1000000));
    }
    else
    {
        ithRtcSetTime(sec + (usec / 1000000));
    }

#ifdef CFG_RTC_6_8P
    // remove the old RTC file
    pthread_cancel(adjustTask);
    remove(RTC_FILE);
    
    // Adjust_RTC thread restart
    pthread_create(&adjustTask, NULL, RtcAdjustTask, NULL);
    gAdjustInterval = ADJUST_INTERVAL;
    sem_post(&adjustSem);

    // update RTC file
    fptr = fopen(RTC_FILE, "w");
    if(fptr == NULL) {
        printf("[%s] fopen(%s) fail\n", __FUNCTION__, RTC_FILE);
        return;
    }

    currSec = ithRtcGetTime();
    lastUpdateSec = currSec;
    DEBUG_PRINT("[%s] currSec=%d\n", __FUNCTION__, currSec);
    
    fprintf(fptr, "%d %d", currSec, gAdjustInterval);    
    fclose(fptr);

#if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
    ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
#endif
    
#endif
}
