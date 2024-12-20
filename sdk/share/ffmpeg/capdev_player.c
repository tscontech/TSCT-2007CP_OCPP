/*
 * iTE castor3 media player for camera capture
 *
 * @file:capdev_player.c
 * @version 3.0.0
 * @How to use:
 *
 *
 *  1.set play window size 
 *     itv_set_video_window(x, y, width, height);
 *  2.init
 *     mtal_pb_init(EventHandler);
 *     mtal_spec.camera_in = CAPTURE_IN;
 *     mtal_pb_select_file(&mtal_spec);
 *  3.start play
 *     mtal_pb_play();
 *  4.stop
 *     mtal_pb_stop();
 *  5.destory
 *     mtal_pb_exit();
 *
 *  6.deine setting
 *     CAPDEV_SENSOR_MAX_WIDTH , according sensor max Image resolution
 *     CAPDEV_SENSOR_MAX_HEIGHT, according sensor max Image resolution
 *     CAPDEV_MS_PER_FRAME         ,  capture thread update frame pre ms. ex: 60fps => 16ms
 */
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>
#include <unistd.h>
#include <assert.h>
#include "ite/itp.h"
#include "ith/ith_video.h"
#include "ite/itv.h"

#ifdef __OPENRTOS__
    #include "openrtos/FreeRTOS.h"
    #include "openrtos/queue.h"
#endif
#include "file_player.h"
#include "isp/mmp_isp.h"
#include "capture/capture_9860/mmp_capture.h"
#ifdef CFG_SENSOR_ENABLE
    #include "sensor/mmp_sensor.h"
#endif
///////////////////////////////////////////////////////////////////////////////////////////
// Definitions and type
///////////////////////////////////////////////////////////////////////////////////////////

#define CAPDEVQUEUESIZE          10
#define CAPDEV_SENSOR_MAX_WIDTH  1920
#define CAPDEV_SENSOR_MAX_HEIGHT 1080
#define CAPDEV_MS_PER_FRAME      16
#define CAPDEVTIMEOUT            100//100 * 10ms
#define SENSORTIMEOUT            100//100 * 10ms

typedef struct SPlayerInstance {
    int             abort_request;
} SPlayerInstance;

typedef struct SPlayerProps {
    pthread_t      read_tid;
    cb_handler_t   callback;
    int            instCnt;
    bool           is_thread_create;
	bool           capturerror;
	bool           is_memory_mode; //memory control.
	int            devices_type;
    SPlayerInstance *inst;
} SPlayerProps;

///////////////////////////////////////////////////////////////////////////////////////////
// Global Value
//
///////////////////////////////////////////////////////////////////////////////////////////
static pthread_mutex_t player_mutex;
static SPlayerProps     *global_player_prop = NULL;
static CAPTURE_HANDLE  gCapDev0;

#ifdef __OPENRTOS__
QueueHandle_t          gCapInfoQueue; // Queue use to interrupt
#endif
///////////////////////////////////////////////////////////////////////////////////////////
// Private Functions
//
///////////////////////////////////////////////////////////////////////////////////////////
static void
_CAP_ISR0(
    void *arg)
{
#ifdef __OPENRTOS__
    uint32_t       capture0state = 0, interrupt_status = 0;
    BaseType_t     gHigherPriorityTaskWoken = (BaseType_t) 0;
    CAPTURE_HANDLE *ptDev                   = (CAPTURE_HANDLE *) arg;

    capture0state = ithCapGetEngineErrorStatus(&gCapDev0, MMP_CAP_LANE0_STATUS);

    if (capture0state >> 31)
    {
        if ((capture0state >> 8) & 0xF)
        {
            //ithPrintf("cap0_isr err\n");
            ithPrintf("ErrorCode = 0x%x\n",(capture0state >> 8) & 0xF);
            interrupt_status = 1;
            xQueueSendToBackFromISR(gCapInfoQueue, (void *)&interrupt_status, &gHigherPriorityTaskWoken);
            //clear cap0 interrupt and reset error status
            ithCapClearInterrupt(&gCapDev0, MMP_TRUE);
        }
        else
        {
            //ithPrintf("cap0_isr frame end\n");
            interrupt_status = 0;
            xQueueSendToBackFromISR(gCapInfoQueue, (void *)&interrupt_status, &gHigherPriorityTaskWoken);
            //clear cap0 interrupt
            ithCapClearInterrupt(&gCapDev0, MMP_FALSE);
        }
    }
    portYIELD_FROM_ISR(gHigherPriorityTaskWoken);
#endif
    return;
}

static int _SIGNALCHECK_FIRE(CAPTURE_HANDLE *ptDev)
{
    int         timeout  = 0;
    CAP_CONTEXT *Capctxt = &ptDev->cap_info;

    while ((ithCapGetEngineErrorStatus(ptDev, MMP_CAP_LANE0_STATUS) & 0xe) != 0xe)
    {
        if (++timeout > CAPDEVTIMEOUT)
            return 1;
        printf("Hsync or Vsync not stable!\n");
        usleep(10 * 1000);
    }

    ithCapFire(ptDev, MMP_TRUE);
    printf("Capture Fire! (%d)\n", ptDev->cap_id);

    return 0;
}

static int _SENSOR_SIGNALCHECK(void)
{
    int timeout = 0;
    while (ithCapDeviceIsSignalStable() != true)
    {
        //printf("Sensor not stable!\n");
        if (++timeout > SENSORTIMEOUT)
        {
            printf("Wait Sensor stable timeout\n");
            return 1;
        }
        usleep(1000 * 10);
    } 
    return 0;
}
static void CaptureGetNewFrame(CAPTURE_HANDLE *ptDev, ITE_CAP_VIDEO_INFO *Outdata)
{
    int         cap_idx;
    CAP_CONTEXT *Capctxt = &ptDev->cap_info;

    Outdata->OutHeight    = Capctxt->outinfo.OutHeight;
    Outdata->OutWidth     = Capctxt->outinfo.OutWidth;
    Outdata->IsInterlaced = Capctxt->ininfo.Interleave;
    Outdata->PitchY       = Capctxt->ininfo.PitchY;
    Outdata->PitchUV      = Capctxt->ininfo.PitchUV;
    Outdata->OutMemFormat = Capctxt->outinfo.OutMemFormat;

    cap_idx               = ithCapReturnWrBufIndex(ptDev);

    switch (cap_idx)
    {
    case 0:
        Outdata->DisplayAddrY = (uint8_t *)Capctxt->OutAddrY[0];
        Outdata->DisplayAddrU = (uint8_t *)Capctxt->OutAddrUV[0];
        Outdata->DisplayAddrV = (uint8_t *)Capctxt->OutAddrUV[0];
        //Outdata->DisplayAddrOldY = (uint8_t*)Capctxt->OutAddrY[2];
        break;

    case 1:
        Outdata->DisplayAddrY = (uint8_t *)Capctxt->OutAddrY[1];
        Outdata->DisplayAddrU = (uint8_t *)Capctxt->OutAddrUV[1];
        Outdata->DisplayAddrV = (uint8_t *)Capctxt->OutAddrUV[1];
        //Outdata->DisplayAddrOldY = (uint8_t*)Capctxt->OutAddrY[0];
        break;

    case 2:
        Outdata->DisplayAddrY = (uint8_t *)Capctxt->OutAddrY[2];
        Outdata->DisplayAddrU = (uint8_t *)Capctxt->OutAddrUV[2];
        Outdata->DisplayAddrV = (uint8_t *)Capctxt->OutAddrUV[2];
        //Outdata->DisplayAddrOldY = (uint8_t*)Capctxt->OutAddrY[1];
        break;
    }

    //printf("[%d] AddrY = %x ,ADDU = %x\n",cap_idx,Outdata->DisplayAddrY,Outdata->DisplayAddrU);
}

static void ithCapdevPlayer_FlipLCD(CAPTURE_HANDLE *ptDev)
{
    uint8_t            *dbuf    = NULL;
    ITV_DBUF_PROPERTY  dbufprop = {0};
    ITE_CAP_VIDEO_INFO outdata  = {0};

#ifdef CFG_BUILD_ITV
    dbuf = itv_get_dbuf_anchor();
#endif
    if (dbuf == NULL)
    {
        printf("itv buffer full \n");
        return;
    }

    CaptureGetNewFrame(ptDev, &outdata);

    if (outdata.IsInterlaced)
    {
        itv_enable_isp_feature(MMP_ISP_DEINTERLACE);
    }
    dbufprop.src_w    = outdata.OutWidth;
    dbufprop.src_h    = outdata.OutHeight;
    dbufprop.pitch_y  = outdata.PitchY;
    dbufprop.pitch_uv = outdata.PitchUV;

    dbufprop.format   = MMP_ISP_IN_NV12;
    dbufprop.ya       = outdata.DisplayAddrY;
    dbufprop.ua       = outdata.DisplayAddrU;
    dbufprop.va       = outdata.DisplayAddrV;
#ifdef CFG_BUILD_ITV
    itv_update_dbuf_anchor(&dbufprop);
#endif
}

void *read_thread_cap(void *arg)
{
    SPlayerProps    *pprop           = (SPlayerProps *) arg;
    SPlayerInstance *is;
    int            ret              = 0;
    int            interrupt_status = 0;
    unsigned int   first_tick       = 0;
    unsigned int   diff_tick        = 0;
    int            delay_time       = 0;
	uint32_t       capturestate     = 0;

    if (!pprop)
    {
        printf("Player not exist\n");
        ret = -1;
        goto fail;
    }

    is = pprop->inst;

    for (;;)
    {
        if (is->abort_request)
        {
            printf("should not run here!!!\n");
            break;
        }
#ifdef __OPENRTOS__
        first_tick = itpGetTickCount();

		if(pprop->is_memory_mode)
		{
	        if (xQueueReceive(gCapInfoQueue, &interrupt_status, 0))
	        {
	            //printf("get queue = %d\n", intrrupt_status);
	            if (interrupt_status == 0)
	            {
	                ithCapdevPlayer_FlipLCD(&gCapDev0);
	            }
	            else
	            {
	                pprop->capturerror = true;
	                break;
	            }
	        }
		}
        if (ithCapIsFire(&gCapDev0) == false)
        {
            if (ithCapDeviceIsSignalStable())
            {
                ithCapGetDeviceInfo(&gCapDev0);
                ithCapParameterSetting(&gCapDev0);
                _SIGNALCHECK_FIRE(&gCapDev0);
                first_tick = itpGetTickCount();
            }
        }
        
        if(!pprop->is_memory_mode)
        {
        	capturestate = ithCapGetEngineErrorStatus(&gCapDev0, MMP_CAP_LANE0_STATUS);
            if(capturestate & 0x0F00)
            {
            	printf("ErrorCode = 0x%x\n",(capturestate >> 8) & 0xF);
                pprop->capturerror = true;
                break;
            }
        }

        diff_tick = itpGetTickDuration(first_tick);
        delay_time = CAPDEV_MS_PER_FRAME - diff_tick;
        if(delay_time < 1) delay_time = 1;

#endif
        //printf("CAP sleep ms = %d \n", delay_time);
        usleep(delay_time * 1000);
    }

fail:
    printf("terminate video\n");
        
    if (ret != 0)
        printf("read_thread failed, ret=%d\n", ret);
    else
        printf("read thread %x done\n", pprop->read_tid);

    if(pprop->capturerror)
    {
         pprop->callback(PLAYER_EVENT_CAPTURE_DEV, (void*)NULL);
         pprop->capturerror = false;
    }

    pthread_exit(NULL);
    return 0;
}

static void stream_close(SPlayerProps *pprop)
{
    void           *status;
    SPlayerInstance *is;

    printf("stream close\n");

    if ((!pprop) || (!pprop->inst))
        return;

    pprop->inst->abort_request = 1;
    pthread_join(pprop->read_tid, &status);

    /* free all pictures */
    is = pprop->inst;

    if (!is)
        return;

    is->abort_request = 0;

    free(pprop->inst);
    pprop->inst    = NULL;
    pprop->instCnt = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Public Functions
//
///////////////////////////////////////////////////////////////////////////////////////////

static int ithCapdevPlayer_init(cb_handler_t callback)
{
    SPlayerProps    *pprop        = NULL;
    SPlayerInstance *inst         = NULL;
    printf("call = %s\n", __FUNCTION__);

    if (global_player_prop)
        return 1;

    pprop = (SPlayerProps *) calloc(sizeof(char), sizeof(SPlayerProps));
    if (!pprop)
    {
        printf("Initialize player failed\n");
        return -1;
    }

    // player properties initial, TODO use API later
    pthread_mutex_init(&player_mutex, NULL);
    pprop->callback      = callback;

    inst                 = (SPlayerInstance *) calloc(1, sizeof(SPlayerInstance));

    if (!inst)
        printf("not enough memory\n");

    if (pprop->inst)
        free(pprop->inst);
    pprop->inst             = inst;
    pprop->instCnt          = 1;
	pprop->is_thread_create = false;
	pprop->capturerror      = false;


#ifdef __OPENRTOS__
    gCapInfoQueue      = xQueueCreate(CAPDEVQUEUESIZE, (unsigned portBASE_TYPE) sizeof(int));
#endif

    /*power on*/
    ithCapPowerUp();

	/*capture init*/
	ithCapInitialize();

    /*Sensor init */
    ithCapDeviceInitialize();
			
    global_player_prop = pprop;

    return 0;
}

static int ithCapdevPlayer_select_file(const char *filename, int level)
{
    printf("call = %s\n", __FUNCTION__);
	SPlayerProps    *pprop = global_player_prop;
	MMP_ISP_SHARE   isp_share     = {0};

	if(strcmp(filename, "CH0") == 0)
	{
		ithCapDeviceCHSwitch(0);
	}
	else if(strcmp(filename, "CH1") == 0)
	{
		ithCapDeviceCHSwitch(1);
	}
	else if(strcmp(filename, "CH2") == 0)
	{
		ithCapDeviceCHSwitch(2);
	}
	else if(strcmp(filename, "CH3") == 0)
	{
		ithCapDeviceCHSwitch(3);
	}
	else
	{
		printf("unknown channel\n");
	}

    _SENSOR_SIGNALCHECK();	

	/*IT986x default use memory mode*/
	pprop->is_memory_mode = true;
	
	if(ithCapDeviceGetProperty(DEVICES_IS_DECODER))
		pprop->devices_type = MMP_CAP_DEV_ANALOG_DECODER;
	else
		pprop->devices_type = MMP_CAP_DEV_SENSOR;
	
    if(pprop->is_memory_mode)
    {
        printf("CAPTURE MEM MODE type = %d\n", pprop->devices_type);
        CAPTURE_SETTING mem_modeset = {pprop->devices_type, MMP_FALSE,  MMP_TRUE, CAPDEV_SENSOR_MAX_WIDTH, CAPDEV_SENSOR_MAX_HEIGHT};
        ithCapConnect(&gCapDev0, mem_modeset);
        ithCapRegisterIRQ(_CAP_ISR0, &gCapDev0);
		ithCapGetDeviceInfo(&gCapDev0);
    }
    else
    {
    	CAPTURE_SETTING onfly_modeset = {pprop->devices_type, MMP_TRUE,  MMP_FALSE, CAPDEV_SENSOR_MAX_WIDTH, CAPDEV_SENSOR_MAX_HEIGHT};
		ithCapConnect(&gCapDev0, onfly_modeset);
        ithCapDisableIRQ();
		ithCapGetDeviceInfo(&gCapDev0);
        printf("CAPTURE ONFLY MODE type = %d\n", pprop->devices_type);
        isp_share.addrY      = 0;
        isp_share.addrU      = 0;
        isp_share.addrV      = 0;
        isp_share.width      = gCapDev0.cap_info.ininfo.capwidth;
        isp_share.height     = gCapDev0.cap_info.ininfo.capheight;
        isp_share.pitchY     = gCapDev0.cap_info.ininfo.PitchY;
        isp_share.pitchUv    = gCapDev0.cap_info.ininfo.PitchUV;
        isp_share.format     = MMP_ISP_IN_YUV422;   
        itv_set_isp_onfly(isp_share, MMP_FALSE);
    }

    return 0;
}

static int ithCapdevPlayer_play(void)
{
    SPlayerProps    *pprop = global_player_prop;
    SPlayerInstance *is    = NULL;
    int            rc;
    printf("call = %s\n", __FUNCTION__);
    pthread_mutex_lock(&player_mutex);

    if (!pprop)
    {
        printf("Player not exist\n");
        pthread_mutex_unlock(&player_mutex);
        return -1;
    }

    is = pprop->inst;
    if (!is)
    {
        printf("No assigned stream in player\n");
        pthread_mutex_unlock(&player_mutex);
        return -1;
    }

    if (!pprop->is_thread_create)
    {
        rc                     = pthread_create(&pprop->read_tid, NULL, read_thread_cap, (void *)pprop);
        if (rc)
        {
            printf("create thread failed %d\n", rc);
        }
    }

    pprop->is_thread_create = true;
    pthread_mutex_unlock(&player_mutex);
    return 0;
}

static int ithCapdevPlayer_stop(void)
{
    SPlayerProps    *pprop = global_player_prop;
    SPlayerInstance *is    = NULL;

    printf("call = %s\n", __FUNCTION__);
    pthread_mutex_lock(&player_mutex);

    if (!pprop)
    {
        printf("Player not exist\n");
        pthread_mutex_unlock(&player_mutex);
        return -1;
    }

    is = pprop->inst;
    stream_close(pprop);
    ithCapTerminate();

    ithCapDeviceTerminate();

    pprop->is_thread_create = false;

#ifdef CFG_BUILD_ITV
    itv_flush_dbuf();
#endif

    pthread_mutex_unlock(&player_mutex);
    return 0;
}

static int ithCapdevPlayer_deinit()
{
    SPlayerProps *pprop = global_player_prop;
    printf("call = %s\n", __FUNCTION__);

    if (!pprop)
    {
        printf("Player not exist\n");
        return -1;
    }

    global_player_prop = NULL;

    /* Release PlayerProps if any */
    if (pprop)
    {
        if (pprop->inst)
            free(pprop->inst);
        free(pprop);
        pprop = NULL;
    }

    /* Release video request memory buffers */
    printf("ithCapTerminate , Deinit\n");
    ithCapTerminate();
    ithCapDisConnect(&gCapDev0);
#ifdef __OPENRTOS__
    vQueueDelete(gCapInfoQueue);
#endif
    pthread_mutex_destroy(&player_mutex);
    return 0;
}

/* pause or resume the video */
static int ithCapdevPlayer_pause(void)
{
    printf("call = %s\n", __FUNCTION__);
    return 0;
}

static int ithCapdevPlayer_play_videoloop(void)
{
    printf("call = %s\n", __FUNCTION__);
    return 0;
}

static int ithCapdevPlayer_get_total_duration(int64_t *total_time)
{
    printf("call = %s\n", __FUNCTION__);
    return 0;
}

static int ithCapdevPlayer_get_total_duration_ext(int64_t *total_time, char *filepath)
{
    printf("call = %s\n", __FUNCTION__);
    return 0;
}

static int ithCapdevPlayer_get_current_time(int64_t *current_time)
{
    printf("call = %s\n", __FUNCTION__);
    return 0;
}

static int ithCapdevPlayer_seekto(int pos)
{
    printf("call = %s\n", __FUNCTION__);
    return 0;
}

static int ithCapdevPlayer_slow_fast_play(float speed)
{
    printf("call = %s\n", __FUNCTION__);
    return 0;
}

static int ithCapdevPlayer_get_file_pos(double *pos)
{
    printf("call = %s\n", __FUNCTION__);
    return 0;
}

static int ithCapdevPlayer_volume_up(void)
{
    printf("call = %s\n", __FUNCTION__);
    return 0;
}

static int ithCapdevPlayer_volume_down(void)
{
    printf("call = %s\n", __FUNCTION__);
    return 0;
}

static int ithCapdevPlayer_mute(void)
{
    printf("call = %s\n", __FUNCTION__);
    return 0;
}

int ithCapdevPlayer_drop_all_input_streams(void)
{
    printf("call = %s\n", __FUNCTION__);
    return 0;
}

static int video_thread_for_rtsp_client(void *arg)
{
    printf("call = %s\n", __FUNCTION__);
    return 0;
}

static int audio_thread_for_rtsp_client(void *arg)
{
    printf("call = %s\n", __FUNCTION__);
    return 0;
}

static void ithCapdevPlayer_InitAVDecodeEnv()
{
    printf("call = %s\n", __FUNCTION__);
}

static void ithCapdevPlayer_InitH264DecodeEnv()
{
    printf("call = %s\n", __FUNCTION__);
}

static void ithCapdevPlayer_InitAudioDecodeEnv(int samplerate, int num_channels, RTSPCLIENT_AUDIO_CODEC codec_id)
{
    printf("call = %s\n", __FUNCTION__);
}

static int ithCapdevPlayer_h264_decode_from_rtsp(unsigned char *inputbuf, int inputbuf_size, double timestamp)
{
    printf("call = %s\n", __FUNCTION__);
    return 0;
}

static int ithCapdevPlayer_audio_decode_from_rtsp(unsigned char *inputbuf, int inputbuf_size, double timestamp)
{
    printf("call = %s\n", __FUNCTION__);
    return 0;
}

static void ithCapdevPlayer_DeinitAVDecodeEnv()
{
    printf("call = %s\n", __FUNCTION__);
    return;
}

#if defined(_MSC_VER)
ithMediaPlayer captureplayer = {
    ithCapdevPlayer_init,
    ithCapdevPlayer_select_file,
    ithCapdevPlayer_play,
    ithCapdevPlayer_pause,
    ithCapdevPlayer_stop,
    ithCapdevPlayer_play_videoloop,
    ithCapdevPlayer_deinit,
    ithCapdevPlayer_get_total_duration,
    ithCapdevPlayer_get_total_duration_ext,
    ithCapdevPlayer_get_current_time,
    ithCapdevPlayer_seekto,
    ithCapdevPlayer_slow_fast_play,
    ithCapdevPlayer_get_file_pos,
    ithCapdevPlayer_volume_up,
    ithCapdevPlayer_volume_down,
    ithCapdevPlayer_mute,
    ithCapdevPlayer_drop_all_input_streams,
    ithCapdevPlayer_InitAVDecodeEnv,
    ithCapdevPlayer_InitH264DecodeEnv,
    ithCapdevPlayer_InitAudioDecodeEnv,
    ithCapdevPlayer_h264_decode_from_rtsp,
    ithCapdevPlayer_audio_decode_from_rtsp,
    ithCapdevPlayer_DeinitAVDecodeEnv
};
#else // no defined _MSC_VER
ithMediaPlayer captureplayer = {
    .init                   = ithCapdevPlayer_init,
    .select                 = ithCapdevPlayer_select_file,
    .play                   = ithCapdevPlayer_play,
    .pause                  = ithCapdevPlayer_pause,
    .stop                   = ithCapdevPlayer_stop,
    .play_videoloop         = ithCapdevPlayer_play_videoloop,
    .deinit                 = ithCapdevPlayer_deinit,
    .gettotaltime           = ithCapdevPlayer_get_total_duration,
    .gettotaltime_ext       = ithCapdevPlayer_get_total_duration_ext,
    .getcurrenttime         = ithCapdevPlayer_get_current_time,
    .seekto                 = ithCapdevPlayer_seekto,
    .slow_fast_play         = ithCapdevPlayer_slow_fast_play,
    .getfilepos             = ithCapdevPlayer_get_file_pos,
    .vol_up                 = ithCapdevPlayer_volume_up,
    .vol_down               = ithCapdevPlayer_volume_down,
    .mute                   = ithCapdevPlayer_mute,
    .drop_all_input_streams = ithCapdevPlayer_drop_all_input_streams,
    .InitAVDecodeEnv        = ithCapdevPlayer_InitAVDecodeEnv,
    .InitH264DecodeEnv      = ithCapdevPlayer_InitH264DecodeEnv,
    .InitAudioDecodeEnv     = ithCapdevPlayer_InitAudioDecodeEnv,
    .h264_decode_from_rtsp  = ithCapdevPlayer_h264_decode_from_rtsp,
    .audio_decode_from_rtsp = ithCapdevPlayer_audio_decode_from_rtsp,
    .DeinitAVDecodeEnv      = ithCapdevPlayer_DeinitAVDecodeEnv
};
#endif
//ithMediaPlayer *media_player = &captureplayer;
