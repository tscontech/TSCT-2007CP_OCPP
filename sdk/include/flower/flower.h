/*
 * Copyright (c) 2004 ITE Technology Corp. All Rights Reserved.
 */
/** @file
 * Flower.
 *
 * @author
 * @version 1.0
 */
#ifndef FLOWER_H
#define FLOWER_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <semaphore.h>
#include <stdbool.h>
#include <pthread.h>
#include "flower/ite_queue.h"
#include "flower/ite_buffer.h"
#include "flower/fliter_priv_def.h"

#define DEBUG_PRINT(...)
//#define DEBUG_PRINT     printf
#define STREAM_QUEUE_SIZE      32

#define ite_new(type,count)    (type*)malloc(sizeof(type)*(count))
#define ite_new0(type,count)   (type*)calloc(1,sizeof(type)*(count))
#define ARRAY_COUNT_OF(array)            (sizeof(array) / sizeof(array[0]))

typedef void (*FilterFunc)(struct _IteFilter *f);
typedef void (*MethodFunc)(struct _IteFilter *f, void *arg);

//=============================================================================
//                              Structure Definition
//=============================================================================

/**
 * Filter ID
 */
typedef enum _IteFilterId {
    ITE_FILTER_A_ID = 1,
    ITE_FILTER_B_ID,
    ITE_FILTER_C_ID,
    ITE_FILTER_D_ID,
    ITE_FILTER_E_ID,
    ITE_FILTER_F_ID,
    ITE_FILTER_SNDREAD_ID,
    ITE_FILTER_SNDWRITE_ID,
    ITE_FILTER_FILEPLAY_ID,
    ITE_FILTER_FILEPLAYMGR_ID,
    ITE_FILTER_STREAMMGR_ID,
    ITE_FILTER_PLAYM4A_ID,
    //ITE_FILTER_STREAMM4A_ID,
    ITE_FILTER_FILEREC_ID,
    ITE_FILTER_FILEMIX_ID,
    ITE_FILTER_SEPARATE_ID,
    ITE_FILTER_MERGE_ID,
    ITE_FILTER_AEC_ID,
    ITE_FILTER_ASR_ID,
    ITE_FILTER_HDT_ID,
    ITE_FILTER_ULAW_ENC_ID,
    ITE_FILTER_ULAW_DEC_ID,
    ITE_FILTER_ALAW_ENC_ID,
    ITE_FILTER_ALAW_DEC_ID,
    ITE_FILTER_ADPCM_ENC_ID,
    ITE_FILTER_ADPCM_DEC_ID,
	ITE_FILTER_CAP_ID,
	ITE_FILTER_H264DEC_ID,
	ITE_FILTER_MJPEGDEC_ID,
    ITE_FILTER_DISPLAY_ID,
    ITE_FILTER_DISPLAY_CAM_ID,
    ITE_FILTER_JPEG_WRITER_ID,
    ITE_FILTER_FILE_WRITER_ID,
    ITE_FILTER_REC_AVI_ID,
    ITE_FILTER_MJPEG_REC_AVI_ID,
    ITE_FILTER_UVC_ID,
    ITE_FILTER_IPCAM_ID,
    ITE_FILTER_UDP_SEND_ID,
    ITE_FILTER_UDP_RECV_ID,
    ITE_FILTER_TCP_SEND_ID,
    ITE_FILTER_TCP_RECV_ID,
    ITE_FILTER_VOID_ID,
    ITE_FILTER_TEE_ID,
    ITE_FILTER_DENOISE_ID,
    ITE_FILTER_VOLUME_ID,
    ITE_FILTER_RESAMPLE_ID,
    ITE_FILTER_CHADAPT_ID,
	ITE_FILTER_MIX_ID,
	ITE_FILTER_MICMIX_ID,
    ITE_FILTER_LOOPBACK_ID,
    ITE_FILTER_EQUALIZER_ID,
    ITE_FILTER_QWRITE_ID,
	ITE_FILTER_QUICKPLAY_ID,
} IteFilterId;

/**
 * Method fucntion ID
 */
typedef enum _IteMethodId {
    ITE_FILTER_A_Method = 1,
    ITE_FILTER_B_Method,
    ITE_FILTER_C_Method,
    ITE_FILTER_D_Method,
    ITE_FILTER_E_Method,
    ITE_FILTER_F_Method,
    /*video*/
	ITE_FILTER_CAP_GETFRAMERATE,
	ITE_FILTER_CAP_GETINTERLANCED,
	ITE_FILTER_CAP_GETERROR,
	ITE_FILTER_CAP_SETSENSORINIT,
	ITE_FILTER_CAP_GETSENSORSTABLE,	
	ITE_FILTER_JPEG_SNAPSHOT,
	ITE_FILTER_REC_AVI_OPEN,
	ITE_FILTER_REC_AVI_CLOSE,
	ITE_FILTER_MJPEG_REC_AVI_OPEN,
	ITE_FILTER_MJPEG_REC_AVI_CLOSE,
	ITE_FILTER_UVC_SETTING,
    /*audio*/
    //ITE_FILTER_PLAYEROPEN,
    ITE_FILTER_SET_FILEPATH,
    ITE_FILTER_LOOPPLAY,
    ITE_FILTER_FILEOPEN,
    ITE_FILTER_FILECLOSE,
    ITE_FILTER_PAUSE,
    ITE_FILTER_SETRATE,
    ITE_FILTER_GETRATE,
    ITE_FILTER_SET_CB,
    ITE_FILTER_SET_VALUE,
    ITE_FILTER_GET_VALUE,
    ITE_FILTER_SET_BYPASS,
    ITE_FILTER_INPUT_RATE,
    ITE_FILTER_OUTPUT_RATE,
    ITE_FILTER_SET_NCHANNELS,
    ITE_FILTER_GET_NCHANNELS,
    ITE_FILTER_GET_STATUS,
    ITE_FILTER_SET_GAIN,
    /*udp*/
    ITE_FILTER_UDP_SEND_SET_PARA,
    ITE_FILTER_UDP_SEND_GET_SOCKET,
    ITE_FILTER_UDP_RECV_SET_PARA,
    ITE_FILTER_UDP_RECV_GET_SOCKET,
	/*tcp*/
    ITE_FILTER_TCP_SEND_SET_PARA,
    ITE_FILTER_TCP_SEND_GET_SOCKET,
    ITE_FILTER_TCP_RECV_SET_PARA,
    ITE_FILTER_TCP_RECV_GET_SOCKET,
    /*end*/
}IteMethodId;
typedef enum _IteAudioCase {
    AudioIdel=0,
    AudioPlayFile,
    AudioRecFile,
    AudioNetCom,
    AnalogNetCom,
    AnalogLocalCom,
    AnalogLocalBroadcast,
    AudioBroadcastSend,
    AudioBroadcastRecv,
    AResamplePlayFile,
    StreamPlay,
	QuickPlay,
}IteAudioCase;

/**
 * Method fucntion description
 */
struct _IteMethodDes {
    IteMethodId id;
    MethodFunc method_func;
};
typedef struct _IteMethodDes IteMethodDes;

/**
 * Filter function description
 */
struct _IteFilterDes {
    IteFilterId id;
    FilterFunc init;
    FilterFunc uninit;
    FilterFunc process;
    IteMethodDes *method;
};
typedef struct _IteFilterDes IteFilterDes;

/**
 * Filter's input & output data structure
 */
struct _IteFilterParm {
    QueueHandle_t Qhandle;
    sem_t semHandle;
};
typedef struct _IteFilterParm IteFilterParm;

/**
 * Filter definition
 */
struct _IteFilter {
    IteFilterParm input[2];
    IteFilterParm output[2];
    IteFilterDes filterDes;
    pthread_t tID;
    bool run;
    bool inputSemBind;
    void *data;
};
typedef struct _IteFilter IteFilter;

/**
 * Filter chain list
 */
struct _IteFChain {
    IteFilter *filter;
    struct _IteFChain *nextPtr;
};
typedef struct _IteFChain IteFChain;

/**
 * Filter chain's configure
 */
struct _IteFcConf {
    char *name;
    bool Semaphore;
    int QSize;
};
typedef struct _IteFcConf IteFcConf;

/**
 * Flow list definition
 */
struct _IteFlowerList {
    IteFChain *fc;
    IteFcConf config;
};
typedef struct _IteFlowerList IteFlowerList;

typedef struct _IteAudioFlower {
    IteFilter *Fsndwrite;
    IteFilter *Fsndread;
    IteFilter *Fsource;
    IteFilter *Fdestinat;
	IteFilter *Frec_avi;
    IteFilter *Fmix;
    IteFilter *Faec;
    IteFilter *Fdecoder;
    IteFilter *Fencoder;
    IteFilter *Fasr;
    IteFilter *Fudpsend;
    IteFilter *Fudprecv;
	IteFilter *Ftcpsend;
	IteFilter *Ftcprecv;
	IteFilter *Ftee;
	IteFilter *Fsrc; //for resample
    IteFChain fc;
    IteFChain fcc;
	IteFChain fccc;
}IteAudioFlower;

typedef struct _IteVideoFlower {
	IteFilter *Fcap;
    IteFilter *Fsource;
    IteFilter *Fdestinat;
	IteFilter *Fh264dec;
	IteFilter *Fdisplay;
	IteFilter *Fdisplaycam;
	IteFilter *Fjpegwriter;
	IteFilter *Ffilewriter;
	IteFilter *Frec_avi;
	IteFilter *Fipcam;
	IteFilter *Fudprecv;
	IteFilter *Ftcprecv;
    IteFilter *Fuvc;
    IteFilter *Fjpegdec;
    IteFChain fc;
}IteVideoFlower;

typedef struct _IteFlower{
    IteAudioFlower *audiostream;
    IteAudioFlower *asrstream;
    IteVideoFlower *videostream;
    IteAudioCase audiocase;
}IteFlower;

typedef struct
{
    bool call_ready;
    bool call_start;
    bool call_end;
    char call_ip[16];
    
}Call_info;

typedef enum _VideoStreamDir {
    VideoStreamSendRecv,
    VideoStreamSendOnly,
    VideoStreamRecvOnly
} VideoStreamDir;
typedef enum _PlaySoundCase{
    Normal = -1,
    Repeat,
    Eofsound,
    Eofmixsound,
    Asrevent,
    /*ASR*/
    ASR_FAIL,
    ASR_SUCCESS_ARG,
    /*play file*/
    PLAY_EOF_FILE,
    MIX_EOF_FILE,
}PlaySoundCase;

typedef enum _AudioCodecType{
    PCM = 1,
    ALAW,
    ULAW
}AudioCodecType;

/*asr tag*/
typedef struct _asrStruct{
    int index;
    int rs;
    char* text;
    float score;
}asrStruct;

typedef struct _VideoMemoInfo
{
	char videomemo_file[256];
	int video_width;
	int video_height;
	int video_fps;
}VideoMemoInfo;

//=============================================================================
//                              Global Data Definition
//=============================================================================

extern IteFilterDes *gFilterDesSet[];


//=============================================================================
//                              Public Function Definition
//=============================================================================
typedef void (*FfilewriterCallback)(void *arg);

/**
 * Malloc a new filter.
 *
 * @param id The new filter ID
 * @return Filter
 */
IteFilter *ite_filter_new(IteFilterId id);

/**
 * Free a filter
 *
 * @param f The filter you want to free
 */
void ite_filter_delete(IteFilter *f);

/**
 * Call filter's method function
 *
 * @param f The filter
 * @param MId The method function ID
 * @param arg The method function's argument
 */
void ite_filter_call_method(IteFilter *f, int MId, void *arg);

/**
 * Sets when all input of filter use the same semaphore
 *
 * @param f The filter
 */
void ite_filter_set_semBind(IteFilter *f);

/**
 * Sets filter chain configure
 *
 * @param fc The filter chain
 * @param param_size The number of configure parameter
 * @param param The configure parameter
 */
void ite_filterChain_setConfig(IteFChain *fc, int param_size, char **param);

/**
 * Build the filter chain
 *
 * @param helper The filter chain
 * @param name The filter chain name
 */
void ite_filterChain_build(IteFChain *helper, char *name);

/**
 * Print all filter in the filter chain
 *
 * @param helper The filter chain
 */
void ite_filterChain_print(IteFChain *helper);

/**
 * Stop all threads of filter chain
 *
 * @param helper The filter chain
 */
void ite_filterChain_stop(IteFChain *helper);

/**
 * Delete the filter chain
 *
 * @param helper The filter chain
 */
void ite_filterChain_delete(IteFChain *helper);


/**
 * Link filter to filter chain
 *
 * @param bee The inf filter's thread run after filter chain run all filter threads.
 * @param helper the filter chain
 * @param outf The filter that want to send output data to @inf
 * @param preFoutPin The output Pin number of filter in filter chain
 * @param inf The filter that want to receive input data from @outf
 * @param inputPin The input Pin number of filter you want to add to link
 */
void ite_filterChain_A_link_B(bool bee, IteFChain *helper, IteFilter *outf, int preFoutPin, IteFilter *inf, int inputPin);

/**
 * Link filter to filter chain
 *
 * @param helper the filter chain
 * @param preFoutPin The output Pin number of filter in filter chain
 * @param f The filter you want to link
 * @param inputPin The input Pin number of filter you want to add to link
 */
void ite_filterChain_link(IteFChain *helper, int preFoutPin, IteFilter *f, int inputPin);

/**
 * Unlink filter from filter chain
 *
 * @param bee The inf filter stop thread before filter chain stop all filter threads
 * @param helper the filter chain
 * @param outf the filter that output data to @inf
 * @param preFoutPin The output Pin number of filter
 * @param inf The filter that input data from @outf
 * @param inputPin The input Pin number of filter you want to unlink
 */
void ite_filterChain_A_unlink_B(bool bee, IteFChain *helper, IteFilter *outf, int preFoutPin, IteFilter *inf, int inputPin);

/**
 * Unlink filter from filter chain
 *
 * @param helper the filter chain
 * @param preFoutPin The output Pin number of filter
 * @param f The filter you want to unlink
 * @param inputPin The input Pin number of filter you want to unlink
 */
void ite_filterChain_unlink(IteFChain *helper, int preFoutPin, IteFilter *f, int inputPin);

/**
 * Create all threads in filter chain to run
 *
 * @param helper The filter chain
 */
void ite_filterChain_run(IteFChain *helper);

/**
 * Init flower list
 *
 */
void ite_flower_init(void);

/**
 * Reset flower list
 *
 */
void ite_flower_reset(void);


/**
 * Add a filter chain in flower list
 *
 * @param fc The filter chain
 * @param name The filter chain name
 */
void ite_flower_add(IteFChain *fc, char *name);

/**
 * Get queue size of filter chain
 *
 * @param fc The filter chain
 * @return Queue size
 */
int ite_flower_findFChainQSize(IteFChain *fc);

/**
 * Get semaphore option of filter chain
 *
 * @param fc The filter chain
 * @return if use semaphore or not
 */
bool ite_flower_findFChainUseSem(IteFChain *fc);

/**
 * Get configure of filter chain
 *
 * @param fc The filter chain
 * @return The filter chain configure
 */
IteFcConf *ite_flower_findFChainConfig(IteFChain *fc);

/**
 * Print all filter chains in flower list
 *
 * @param f The filter
 */
void ite_flower_print(void);

/**
 * Remove a filter chain from the flower list
 *
 * @param fc The filter chain
 */
void ite_flower_delete(IteFChain *fc);

/**
 * Deinit flower list
 *
 */
void ite_flower_deinit(void);

/*flower_stream*/
typedef void (*cb_sound_t)(PlaySoundCase event_id, void *arg);
typedef void (*Callback_t)(int state,void *arg);
IteFlower *IteStreamInit(void);
void sound_playback_event_handler(PlaySoundCase nEventID, void *arg);
void flow_start_sound_play(IteFlower *f,const char* filepath,PlaySoundCase mode,Callback_t func);
void flow_stop_sound_play(IteFlower *f);
void flow_start_sound_record(IteFlower *f,const char* filepath,int rate);
void flow_stop_sound_record(IteFlower *f);
#ifdef CFG_BUILD_ASR
void flow_start_asr(IteFlower *f,Callback_t);
void flow_stop_asr(IteFlower *f);
#endif
uint8_t flow_start_udp_videostream(IteFlower *f, Call_info *call_list, unsigned short rem_port, VideoStreamDir dir);
uint8_t flow_start_tcp_videostream(IteFlower *f, Call_info *call_list, unsigned short rem_port, VideoStreamDir dir);
uint8_t flow_start_udp_audiostream(IteFlower *f, int rate, AudioCodecType type, const char *rem_ip, unsigned short rem_port);
uint8_t flow_start_tcp_audiostream(IteFlower *f, int rate, AudioCodecType type, const char *rem_ip, unsigned short rem_port);
uint8_t flow_stop_udp_videostream(IteFlower *f, VideoStreamDir dir);
uint8_t flow_stop_tcp_videostream(IteFlower *f, VideoStreamDir dir);
uint8_t flow_stop_udp_audiostream(IteFlower *f);
uint8_t flow_stop_tcp_audiostream(IteFlower *f);
uint8_t flow_start_recv_ipcamstream(IteFlower *f);
uint8_t flow_stop_recv_ipcamstream(IteFlower *f);

#ifdef CFG_UVC_ENABLE
uint8_t flow_start_uvc_mjpeg(IteFlower *f, int width, int height, int fps);
uint8_t flow_stop_uvc_mjpeg(IteFlower *f);
void flow_start_uvc_mjpeg_record(IteFlower *f, char* file_path, int width, int height, int fps);
void flow_stop_uvc_mjpeg_record(IteFlower *f);
uint8_t flow_start_uvc_h264(IteFlower *f, int width, int height, int fps);
uint8_t flow_stop_uvc_h264(IteFlower *f);
void flow_start_uvc_h264_record(IteFlower *f, char* file_path, int width, int height, int fps);
void flow_stop_uvc_h264_record(IteFlower *f);
#endif

void flow_start_quick_play(IteFlower *f,const char* filepath, PlaySoundCase mode,Callback_t func);
void QuickPlayInsert(IteFlower *f,const char* filepath);
void QuickPlayFlowPause(IteFlower *f,bool pause);
void flow_stop_quick_play(IteFlower *f);







/**/
#ifdef __cplusplus
}
#endif

#endif /* ITE_STREAMER_H */
