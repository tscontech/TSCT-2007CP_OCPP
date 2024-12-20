#include <stdio.h>
#include "flower/flower.h"
#include "flower/fliter_priv_def.h"
#include "capture/capture_9860/mmp_capture.h"

//=============================================================================
//                              Constant Definition
//=============================================================================
#define SIGNALCHECK_TIMEOUT 100//100*10ms
#define SENSORCHECK_TIMEOUT 300//300*10ms


CAPTURE_HANDLE   gCapDev0 = {0};
/*memory mode max : 1080P*/
static int       gMaxWidth  = 1920;
static int       gMaxHeight = 1080;
static unsigned short   gFramerate  = 0;
static unsigned short   ginterlaced = 0;
static bool             gIsError    = false; //false : no error   true: error
static QueueHandle_t    gCapQueue;
static unsigned char 	gInputCH    = 0;

//=============================================================================
//                              Private Function Declaration
//=============================================================================

static void _CAP0ISR(void *arg)
{
    uint32_t     capture0state              = 0;
	uint32_t 	 capwrindex                 = 0;
    BaseType_t   gHigherPriorityTaskWoken   = (BaseType_t)0;

    capture0state = ithCapGetEngineErrorStatus(&gCapDev0, MMP_CAP_LANE0_STATUS);

    if (capture0state >> 31)
    {
        if ((capture0state >> 8) & 0xF)
        {
        	ithPrintf("[Error]ErrorCode = 0x%x\n",(capture0state >> 8) & 0xF);
            //clear cap0 interrupt and reset error status
            ithCapClearInterrupt(&gCapDev0, MMP_TRUE);
        }
        else
        {
            //clear cap0 interrupt
            ithCapClearInterrupt(&gCapDev0, MMP_FALSE);
            capwrindex = ithCapReturnWrBufIndex(&gCapDev0);
            xQueueSendToBackFromISR(gCapQueue, &capwrindex, &gHigherPriorityTaskWoken);			
        }
    }
    portYIELD_FROM_ISR(gHigherPriorityTaskWoken);
}

static int _SIGNALCHECK_FIRE(CAPTURE_HANDLE *ptDev)
{
    int         timeout  = 0;
    while ((ithCapGetEngineErrorStatus(ptDev, MMP_CAP_LANE0_STATUS) & 0xe) != 0xe)
    {
        if (++timeout > SIGNALCHECK_TIMEOUT)
        {
        	printf("Wait Capture Lock timeout\n");
            return 1;
        }
        DEBUG_PRINT("Hsync or Vsync not stable!\n");
        usleep(10000);
    }

    ithCapFire(ptDev, MMP_TRUE);
    DEBUG_PRINT("Capture Fire! (%d)\n", ptDev->cap_id);

    return 0;
}

static int _SENSOR_SIGNALCHECK(void)
{
    int timeout = 0;
    while (ithCapDeviceIsSignalStable() != true)
    {
        //printf("Sensor not stable!\n");
        if (++timeout > SENSORCHECK_TIMEOUT)
        {
            printf("Wait Sensor stable timeout\n");
            return 1;
        }
        usleep(10000);
    } 
    return 0;
}

static void f_capture_init(IteFilter *f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    /*capture clk on*/
    ithCapPowerUp();
    /*capture init*/
    ithCapInitialize();

    gCapQueue = xQueueCreate(20, (unsigned portBASE_TYPE) sizeof(uint32_t));
}

static void f_capture_uninit(IteFilter *f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);		
    ithCapPowerDown();
#if defined(CFG_SENSOR_ENABLE)
    ithCapDeviceLEDON(0);
    ithCapDeviceTerminate();
#endif		
	vQueueDelete(gCapQueue);

}

static void f_capture_process(IteFilter *f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
	IteQueueblk    blk = {0};
	IteSensorStream* output    = NULL;
	uint32_t      capturestate = 0;
	uint32_t	  rec_wrindex  = 0;

	CAPTURE_SETTING mem_analog_set     = {MMP_CAP_DEV_ANALOG_DECODER, MMP_FALSE,  MMP_TRUE, gMaxWidth, gMaxHeight};
	CAPTURE_SETTING mem_digital_set    = {MMP_CAP_DEV_SENSOR, MMP_FALSE,  MMP_TRUE, gMaxWidth, gMaxHeight};	
	
    if (gMaxWidth != 0 && gMaxHeight != 0)
    {
    	if(ithCapDeviceGetProperty(DEVICES_IS_DECODER) == 1)
        	ithCapConnect(&gCapDev0, mem_analog_set);
		else
			ithCapConnect(&gCapDev0, mem_digital_set);

		ithCapRegisterIRQ(_CAP0ISR, &gCapDev0);
    }
    else
    {
        DEBUG_PRINT("[%s] Filter(%d) Width or Height Error \n", __FUNCTION__, f->filterDes.id);
    }

	
    while (f->run)
    {
    
		if (ithCapDeviceIsSignalStable())
		{
			if (ithCapIsFire(&gCapDev0) == false)
			{
		    	ithCapGetDeviceInfo(&gCapDev0);
		    	ithCapParameterSetting(&gCapDev0);
		    	_SIGNALCHECK_FIRE(&gCapDev0);
			}
		}

		if (xQueueReceive(gCapQueue, &rec_wrindex, 0))
		{
			//printf("rec_wrindex = %d\n",rec_wrindex);
		    output = malloc(sizeof(IteSensorStream));
			output->Width       = gCapDev0.cap_info.outinfo.OutWidth;
			output->Height      = gCapDev0.cap_info.outinfo.OutHeight;
			output->Framerate   = gCapDev0.cap_info.ininfo.framerate;
			output->Interlanced = gCapDev0.cap_info.ininfo.Interleave;
			output->DataAddrY   = gCapDev0.cap_info.OutAddrY[rec_wrindex];
			output->DataAddrU   = gCapDev0.cap_info.OutAddrUV[rec_wrindex];
			output->DataAddrV   = gCapDev0.cap_info.OutAddrUV[rec_wrindex];
			output->PitchY      = gCapDev0.cap_info.ininfo.PitchY;
			output->PitchUV     = gCapDev0.cap_info.ininfo.PitchUV;

			blk.private1 = (void*)output;
			ite_queue_put(f->output[0].Qhandle, &blk);				
		}

		usleep(16*1000);

    }
	ithCapTerminate();
	ithCapDisConnect(&gCapDev0);
	gIsError = false;

}

static void f_cap_getframerate(IteFilter *f, void *arg)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

	_SENSOR_SIGNALCHECK();

	gFramerate = ithCapDeviceGetProperty(DEVICES_FRAMETRATE);
	ginterlaced = ithCapDeviceGetProperty(DEVICES_ISINTERLANCED);

    switch (gFramerate)
    {
	    case 2500:
	        gFramerate = MMP_CAP_FRAMERATE_25HZ;
	        break;
	    case 3000:
	        gFramerate = MMP_CAP_FRAMERATE_30HZ;
	        break;
	    case 5000:
			if(ginterlaced)
				gFramerate = MMP_CAP_FRAMERATE_25HZ;
			else
	        	gFramerate = MMP_CAP_FRAMERATE_50HZ;
	        break;
	    case 5994:
	    case 6000:
			if(ginterlaced)
				gFramerate = MMP_CAP_FRAMERATE_30HZ;
			else			
	        	gFramerate = MMP_CAP_FRAMERATE_60HZ;
	        break;
	    default:
	        gFramerate = MMP_CAP_FRAMERATE_25HZ;
			printf("[Error]unknow frame rate!\n");
	        break;
    }

    *(unsigned short*)arg = gFramerate;
	//printf("gFramerate = %x  addr = %x\n", gFramerate , *(unsigned short*)arg);

}

static void f_cap_getinterlanced(IteFilter *f, void *arg)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
	
	_SENSOR_SIGNALCHECK();

	ginterlaced = ithCapDeviceGetProperty(DEVICES_ISINTERLANCED);

    *(unsigned short*)arg = ginterlaced;
}

static void f_cap_geterror(IteFilter *f, void *arg)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    *(bool*)arg = gIsError;
}


static void f_cap_sensor_init(IteFilter *f, void *arg)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
	gInputCH = (unsigned char*) arg;
#if defined(CFG_SENSOR_ENABLE)	
    /*Sensor init*/
    ithCapDeviceInitialize();
	ithCapDeviceCHSwitch(gInputCH);
    ithCapDeviceLEDON(1);
#endif	
}

static void f_cap_getsensorstable(IteFilter *f, void *arg)
{
	if(ithCapDeviceIsSignalStable())
	{
		*(bool*)arg = true;
	}
	else
	{
		*(bool*)arg = false;
	}

}

static IteMethodDes Filter_CAP_methods[] = {
    {ITE_FILTER_CAP_GETFRAMERATE  , f_cap_getframerate},
    {ITE_FILTER_CAP_GETINTERLANCED, f_cap_getinterlanced},
    {ITE_FILTER_CAP_GETERROR      , f_cap_geterror},
    {ITE_FILTER_CAP_SETSENSORINIT , f_cap_sensor_init},
    {ITE_FILTER_CAP_GETSENSORSTABLE, f_cap_getsensorstable},
};

IteFilterDes        FilterCapture = {
    ITE_FILTER_CAP_ID,
    f_capture_init,
    f_capture_uninit,
    f_capture_process,
    Filter_CAP_methods
};
