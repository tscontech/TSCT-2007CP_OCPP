#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "openrtos/FreeRTOS.h"
#include "flower/flower.h"
static bool audiostream_release = false;
pthread_mutex_t AudioFlow_mutex      = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t VideoFlow_mutex = PTHREAD_MUTEX_INITIALIZER;

static Callback_t asr_cb= NULL;
static Callback_t sound_cb= NULL;

static void flow_filter_callbackfunc(int state,void *arg)
{
    if (sound_cb)
    {
        sound_cb(state,arg);
    }
    
    if (asr_cb)
    {
        asr_cb(state,arg);
    }
    
}

static void video_flower_free(IteVideoFlower *s){

    if(s->Fudprecv)   	ite_filter_delete(s->Fudprecv);
	if(s->Ftcprecv)   	ite_filter_delete(s->Ftcprecv);
    if(s->Fh264dec)   	ite_filter_delete(s->Fh264dec);
	if(s->Fdisplay)   	ite_filter_delete(s->Fdisplay);
	if(s->Fjpegwriter)	ite_filter_delete(s->Fjpegwriter);
	if(s->Ffilewriter)	ite_filter_delete(s->Ffilewriter);
	if(s->Frec_avi)		ite_filter_delete(s->Frec_avi);
	if(s->Fipcam)		ite_filter_delete(s->Fipcam);
	if(s->Fuvc)         ite_filter_delete(s->Fuvc);
	if(s->Fjpegdec)     ite_filter_delete(s->Fjpegdec);

    if(s) free(s);
 
}

void sound_playback_event_handler(PlaySoundCase nEventID, void *arg)
{
    switch(nEventID)
    {
        case PLAY_EOF_FILE:
        case Eofsound:
            printf("Eof sound eof\n");
            pthread_mutex_lock(&AudioFlow_mutex);
            audiostream_release=true;
            pthread_mutex_unlock(&AudioFlow_mutex);
            flow_filter_callbackfunc(PLAY_EOF_FILE,arg);
            break;
        case Eofmixsound:
            printf("Eof mix sound\n");
            flow_filter_callbackfunc(MIX_EOF_FILE,arg);
            break;
        case Asrevent:
            printf("asr event\n");
            flow_filter_callbackfunc(ASR_SUCCESS_ARG,arg);

            break;
    }
}

static void *_background_iterate(IteFlower *f){
    while(1){    
        pthread_mutex_lock(&AudioFlow_mutex);
        if (f->audiostream && audiostream_release){
            //flow_stop_sound_play(f);
            AudioStreamCancel(f);
            sound_cb=NULL;
            audiostream_release= false;
        }
        pthread_mutex_unlock(&AudioFlow_mutex);
        usleep(500000);
    }
}
/*Init Flow*/
IteFlower *IteStreamInit(void){
    audio_init_AD();
    audio_init_DA();
    IteFlower *f= (IteFlower*)ite_new0(IteFlower,1);
    ite_flower_init();
    {
        pthread_t Thread;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&Thread, &attr, _background_iterate, f);    
    }
    f->audiostream=NULL;
    f->asrstream=NULL;
    f->videostream=NULL;
    f->audiocase=AudioIdel;

    return f;
}
/*play api*/
void flow_start_sound_play(IteFlower *f,const char* filepath,PlaySoundCase mode,Callback_t func){


    pthread_mutex_lock(&AudioFlow_mutex);
    AudioStreamCancel(f);
//    pthread_mutex_unlock(&AudioFlow_mutex);
    
//    pthread_mutex_lock(&AudioFlow_mutex);
    sound_cb=func;
    PlayFlowStart(f,filepath,mode,sound_playback_event_handler);
    audiostream_release= false;
    
    pthread_mutex_unlock(&AudioFlow_mutex);

}

void flow_stop_sound_play(IteFlower *f){
    pthread_mutex_lock(&AudioFlow_mutex);
    AudioStreamCancel(f);
    sound_cb=NULL;
    audiostream_release= false;
    pthread_mutex_unlock(&AudioFlow_mutex);
}

//mp3 mix wav///

void flow_start_sound_play_mp3(IteFlower *f,const char* filepath,PlaySoundCase mode){


    pthread_mutex_lock(&AudioFlow_mutex);
    AudioStreamCancel(f);
//    pthread_mutex_unlock(&AudioFlow_mutex);
    
//    pthread_mutex_lock(&AudioFlow_mutex);

	PlayFlowStartMix(f,filepath,mode,sound_playback_event_handler); //file mix file 
   
    audiostream_release= false;
    
    pthread_mutex_unlock(&AudioFlow_mutex);

}

void flow_stop_sound_play_mp3(IteFlower *f){

	PlayFlowStopMix(f);//file mix file 
	audiostream_release= false;
}

void flow_start_mix_file_and_mic(IteFlower *f,const char* filepath,PlaySoundCase mode){


    pthread_mutex_lock(&AudioFlow_mutex);
    AudioStreamCancel(f);
//    pthread_mutex_unlock(&AudioFlow_mutex);
    
//    pthread_mutex_lock(&AudioFlow_mutex);

	PlayFlowStartMix2(f,filepath,mode,sound_playback_event_handler); // file mix mic sound
   
    audiostream_release= false;
    
    pthread_mutex_unlock(&AudioFlow_mutex);

}

void flow_stop_mix_file_and_mic(IteFlower *f){

	PlayFlowStopMix2(f);// file mix mic sound
	audiostream_release= false;
}

void flow_mix_sound_play_mp3(IteFlower *f,const char *filepath){//mix wav sound
    if(f->audiostream!=NULL){
        PlayFlowMixSound(f,filepath);
    }
}



void flow_pause_sound_play(IteFlower *f,bool pause){//pause play
    if(f->audiostream!=NULL){
        PlayFlowPause(f,pause);
    }
}

void flow_mix_sound_play(IteFlower *f,const char *filepath){//mix wav sound
    if(f->audiostream!=NULL){
        PlayFlowMix(f,filepath);
    }
}

//ITE_FILTER_QUICKPLAY_ID
void flow_start_quick_play(IteFlower *f,const char* filepath, PlaySoundCase mode,Callback_t func){
	pthread_mutex_lock(&AudioFlow_mutex);
	sound_cb=func;
    QuickPlayFlowStart(f,filepath, mode, sound_playback_event_handler);
	audiostream_release= false;
   	pthread_mutex_unlock(&AudioFlow_mutex);
}

void flow_stop_quick_play(IteFlower *f){
    
	pthread_mutex_lock(&AudioFlow_mutex);
	QuickPlayFlowStop(f);
    sound_cb=NULL;
    audiostream_release= false;
    pthread_mutex_unlock(&AudioFlow_mutex);
}


/*player end*/
/*record api*/
void flow_start_sound_record(IteFlower *f,const char* filepath,int rate){
    
    pthread_mutex_lock(&AudioFlow_mutex);
    AudioStreamCancel(f);
    pthread_mutex_unlock(&AudioFlow_mutex);
    
    RecFlowStart(f,filepath,rate);
    
}

void flow_stop_sound_record(IteFlower *f){
    AudioStreamCancel(f);
}
/*record end*/
/*asr api*/
void flow_start_asr(IteFlower *f,Callback_t func){
    
    pthread_mutex_lock(&AudioFlow_mutex);
    if(f->asrstream!=NULL) flow_stop_asr(f);
    pthread_mutex_unlock(&AudioFlow_mutex);
    asr_cb=func;
    AsrFlowStart(f,sound_playback_event_handler);
    
}

void flow_stop_asr(IteFlower *f){

    if(f->asrstream!=NULL){
        AsrFlowStop(f);
        asr_cb=NULL;
        f->asrstream=NULL;
    }
}
/*asr*/
/*audio call api*/
void flow_start_audioflow(IteFlower *f){
    
    pthread_mutex_lock(&AudioFlow_mutex);
    AudioStreamCancel(f);
    pthread_mutex_unlock(&AudioFlow_mutex);
    
    SndrwFlowStart(f,8000,ULAW);
    
}

void flow_stop_audioflow(IteFlower *f){
    AudioStreamCancel(f);
}
/*audio call end*/
/*mic & spk work*/
void flow_start_soundrw(IteFlower *f){
    
    pthread_mutex_lock(&AudioFlow_mutex);
    AudioStreamCancel(f);
    pthread_mutex_unlock(&AudioFlow_mutex);
    
    SndrwFlowStart(f,8000,ULAW);
    
}

void flow_stop_soundrw(IteFlower *f){
    AudioStreamCancel(f);
}
/*end*/

/*video udp stream*/
uint8_t flow_start_udp_videostream(IteFlower *f, Call_info *call_list, unsigned short rem_port, VideoStreamDir dir){

    IteVideoFlower *vstream = (IteVideoFlower *)ite_new0 (IteVideoFlower, 1);
    char* args[] ={"-Q=32"};
    udp_config_t udp_conf;


	if(dir == VideoStreamRecvOnly)
    {
        vstream->Fudprecv = ite_filter_new(ITE_FILTER_UDP_RECV_ID);
		vstream->Fh264dec = ite_filter_new(ITE_FILTER_H264DEC_ID);
		vstream->Fdisplay = ite_filter_new(ITE_FILTER_DISPLAY_ID);
		vstream->Fjpegwriter = ite_filter_new(ITE_FILTER_JPEG_WRITER_ID);
		vstream->Ffilewriter = ite_filter_new(ITE_FILTER_FILE_WRITER_ID);
		vstream->Frec_avi = ite_filter_new(ITE_FILTER_REC_AVI_ID);
		
        memset(&udp_conf,'\0',sizeof(udp_config_t));
    	udp_conf.remote_port = rem_port;
    	udp_conf.cur_socket = -1;
    	udp_conf.c_type = VIDEO_INPUT;	

        ite_filter_call_method(vstream->Fudprecv, ITE_FILTER_UDP_RECV_SET_PARA, (void*)&udp_conf);

        ite_filterChain_build(&vstream->fc, "FC 1");
        ite_filterChain_setConfig(&vstream->fc, ARRAY_COUNT_OF(args), args);
        
        ite_filterChain_link(&vstream->fc, -1, vstream->Fudprecv, -1);
       	ite_filterChain_link(&vstream->fc, 0, vstream->Fh264dec, 0);
		ite_filterChain_link(&vstream->fc, 0, vstream->Fdisplay, 0);

		//avi recoder
		ite_filterChain_A_link_B(false, &vstream->fc, vstream->Fudprecv, 1, vstream->Frec_avi, 0);
		
		//snapshot
		ite_filterChain_A_link_B(false, &vstream->fc, vstream->Fh264dec, 1, vstream->Fjpegwriter, 0);
		ite_filterChain_link(&vstream->fc, 0, vstream->Ffilewriter, 0);
        
        //printf("===Filter Chain run===\n");
        ite_filterChain_run(&vstream->fc);

        f->videostream = vstream;
    }
}

/*video tcp stream*/
uint8_t flow_start_tcp_videostream(IteFlower *f, Call_info *call_list, unsigned short rem_port, VideoStreamDir dir){

    IteVideoFlower *vstream = (IteVideoFlower *)ite_new0 (IteVideoFlower, 1);
    char* args[] ={"-Q=32"};
    tcp_config_t tcp_conf;


	if(dir == VideoStreamRecvOnly)
    {
        vstream->Ftcprecv = ite_filter_new(ITE_FILTER_TCP_RECV_ID);
		vstream->Fh264dec = ite_filter_new(ITE_FILTER_H264DEC_ID);
		vstream->Fdisplay = ite_filter_new(ITE_FILTER_DISPLAY_ID);
		vstream->Fjpegwriter = ite_filter_new(ITE_FILTER_JPEG_WRITER_ID);
		vstream->Ffilewriter = ite_filter_new(ITE_FILTER_FILE_WRITER_ID);
		vstream->Frec_avi = ite_filter_new(ITE_FILTER_REC_AVI_ID);
		
		memset(&tcp_conf,'\0',sizeof(tcp_config_t));
		tcp_conf.remote_port = rem_port;
		tcp_conf.remote_ip = call_list;
		tcp_conf.cur_socket = -1;
		tcp_conf.c_type = VIDEO_INPUT;

        ite_filter_call_method(vstream->Ftcprecv, ITE_FILTER_TCP_RECV_SET_PARA, (void*)&tcp_conf);

        ite_filterChain_build(&vstream->fc, "FC 1");
        ite_filterChain_setConfig(&vstream->fc, ARRAY_COUNT_OF(args), args);
        
        ite_filterChain_link(&vstream->fc, -1, vstream->Ftcprecv, -1);
       	ite_filterChain_link(&vstream->fc, 0, vstream->Fh264dec, 0);
		ite_filterChain_link(&vstream->fc, 0, vstream->Fdisplay, 0);

		//avi recoder
		ite_filterChain_A_link_B(false, &vstream->fc, vstream->Ftcprecv, 1, vstream->Frec_avi, 0);
		
		//snapshot
		ite_filterChain_A_link_B(false, &vstream->fc, vstream->Fh264dec, 1, vstream->Fjpegwriter, 0);
		ite_filterChain_link(&vstream->fc, 0, vstream->Ffilewriter, 0);
        
        //printf("===Filter Chain run===\n");
        ite_filterChain_run(&vstream->fc);

        f->videostream = vstream;
    }
}

uint8_t flow_start_udp_audiostream(IteFlower *f, int rate, AudioCodecType type, const char *rem_ip, unsigned short rem_port){

    pthread_mutex_lock(&AudioFlow_mutex);
    AudioStreamCancel(f);
    pthread_mutex_unlock(&AudioFlow_mutex);
   
    AudioFlowStart(f,rate,type,rem_ip,rem_port);
}

/*start audio tcp stream*/
uint8_t flow_start_tcp_audiostream(IteFlower *f, int rate, AudioCodecType type, const char *rem_ip, unsigned short rem_port){

    pthread_mutex_lock(&AudioFlow_mutex);
    AudioStreamCancel(f);
    pthread_mutex_unlock(&AudioFlow_mutex);
   
    AudioTcpFlowStart(f,rate,type,rem_ip,rem_port);
}

/*stop video udp stream*/
uint8_t flow_stop_udp_videostream(IteFlower *f, VideoStreamDir dir){
    IteVideoFlower *s=f->videostream;

    ite_filterChain_stop(&s->fc);
    if(dir == VideoStreamRecvOnly)
    {
        if(s->Fudprecv) ite_filterChain_unlink(&s->fc,-1,s->Fudprecv, -1);
		if(s->Fh264dec) ite_filterChain_unlink(&s->fc, 0,s->Fh264dec,  0);
		if(s->Fdisplay) ite_filterChain_unlink(&s->fc, 0,s->Fdisplay,  0);
		if(s->Frec_avi) ite_filterChain_A_unlink_B(false, &s->fc, s->Fudprecv, 1, s->Frec_avi, 0);
		if(s->Fjpegwriter) ite_filterChain_A_unlink_B(false, &s->fc, s->Fh264dec, 1, s->Fjpegwriter, 0);
		if(s->Ffilewriter) ite_filterChain_unlink(&s->fc, 0,s->Ffilewriter,  0);
    }
    ite_filterChain_delete(&s->fc);
    
    video_flower_free(s);
}

/*stop video tcp stream*/
uint8_t flow_stop_tcp_videostream(IteFlower *f, VideoStreamDir dir){
    IteVideoFlower *s=f->videostream;

    ite_filterChain_stop(&s->fc);
    if(dir == VideoStreamRecvOnly)
    {
        if(s->Ftcprecv) ite_filterChain_unlink(&s->fc,-1,s->Ftcprecv, -1);
		if(s->Fh264dec) ite_filterChain_unlink(&s->fc, 0,s->Fh264dec,  0);
		if(s->Fdisplay) ite_filterChain_unlink(&s->fc, 0,s->Fdisplay,  0);
		if(s->Frec_avi) ite_filterChain_A_unlink_B(false, &s->fc, s->Ftcprecv, 1, s->Frec_avi, 0);
		if(s->Fjpegwriter) ite_filterChain_A_unlink_B(false, &s->fc, s->Fh264dec, 1, s->Fjpegwriter, 0);
		if(s->Ffilewriter) ite_filterChain_unlink(&s->fc, 0,s->Ffilewriter,  0);
    }
    ite_filterChain_delete(&s->fc);
    
    video_flower_free(s);
}

/*stop audio udp stream*/
uint8_t flow_stop_udp_audiostream(IteFlower *f){
    AudioStreamCancel(f);
}

/*stop audio tcp stream*/
uint8_t flow_stop_tcp_audiostream(IteFlower *f){
    AudioStreamCancel(f);
}

uint8_t flow_start_recv_ipcamstream(IteFlower *f)
{
	IteVideoFlower *vstream = (IteVideoFlower *)ite_new0 (IteVideoFlower, 1);
	char* args[] ={"-Q=32"};
	
	vstream->Fipcam = ite_filter_new(ITE_FILTER_IPCAM_ID);
	vstream->Fh264dec = ite_filter_new(ITE_FILTER_H264DEC_ID);
	vstream->Fdisplay = ite_filter_new(ITE_FILTER_DISPLAY_ID);
	vstream->Fjpegwriter = ite_filter_new(ITE_FILTER_JPEG_WRITER_ID);
	vstream->Ffilewriter = ite_filter_new(ITE_FILTER_FILE_WRITER_ID);
	vstream->Frec_avi = ite_filter_new(ITE_FILTER_REC_AVI_ID);

	ite_filterChain_build(&vstream->fc, "FC 1");
	ite_filterChain_setConfig(&vstream->fc, ARRAY_COUNT_OF(args), args);

	ite_filterChain_link(&vstream->fc, -1, vstream->Fipcam, -1);
	ite_filterChain_link(&vstream->fc, 0, vstream->Fh264dec, 0);
	ite_filterChain_link(&vstream->fc, 0, vstream->Fdisplay, 0);

	//avi recoder
	ite_filterChain_A_link_B(false, &vstream->fc, vstream->Fipcam, 1, vstream->Frec_avi, 0);
	
	//snapshot
	ite_filterChain_A_link_B(false, &vstream->fc, vstream->Fh264dec, 1, vstream->Fjpegwriter, 0);
	ite_filterChain_link(&vstream->fc, 0, vstream->Ffilewriter, 0);

	ite_filterChain_run(&vstream->fc);
	f->videostream = vstream;
}

uint8_t flow_stop_recv_ipcamstream(IteFlower *f)
{
	IteVideoFlower *s=f->videostream;
	
	ite_filterChain_stop(&s->fc);
	
	if(s->Fipcam) ite_filterChain_unlink(&s->fc,-1,s->Fipcam, -1);
	if(s->Fh264dec) ite_filterChain_unlink(&s->fc, 0,s->Fh264dec,  0);
	if(s->Fdisplay) ite_filterChain_unlink(&s->fc, 0,s->Fdisplay,  0);
	if(s->Frec_avi) ite_filterChain_A_unlink_B(false, &s->fc, s->Fipcam, 1, s->Frec_avi, 0);
	if(s->Fjpegwriter) ite_filterChain_A_unlink_B(false, &s->fc, s->Fh264dec, 1, s->Fjpegwriter, 0);
	if(s->Ffilewriter) ite_filterChain_unlink(&s->fc, 0,s->Ffilewriter,  0);
	
	ite_filterChain_delete(&s->fc);

	video_flower_free(s);
}

#ifdef CFG_UVC_ENABLE

/*MJPEG Control flow*/
uint8_t flow_start_uvc_mjpeg(IteFlower *f, int width, int height, int fps)
{

    IteUVCStream* uvc_ptr = (IteUVCStream*)ite_new0(IteUVCStream, 1);

	IteVideoFlower *vstream = (IteVideoFlower *)ite_new0 (IteVideoFlower, 1);
	char* args[] ={"-Q=32"};
	
	vstream->Fuvc = ite_filter_new(ITE_FILTER_UVC_ID);
	vstream->Fjpegdec = ite_filter_new(ITE_FILTER_MJPEGDEC_ID);
	vstream->Fdisplay = ite_filter_new(ITE_FILTER_DISPLAY_ID);
    vstream->Frec_avi = ite_filter_new(ITE_FILTER_MJPEG_REC_AVI_ID);

	ite_filterChain_build(&vstream->fc, "FC UVC");
	ite_filterChain_setConfig(&vstream->fc, ARRAY_COUNT_OF(args), args);


    uvc_ptr->UVC_format = FLOWER_UVC_FRAME_FORMAT_MJPEG;
    uvc_ptr->UVC_Width  = width;
    uvc_ptr->UVC_Height = height;
    uvc_ptr->UVC_FPS    = fps;

    ite_filter_call_method(vstream->Fuvc, ITE_FILTER_UVC_SETTING, (void*)uvc_ptr);
    if(uvc_ptr)
        free(uvc_ptr);

	ite_filterChain_link(&vstream->fc, -1, vstream->Fuvc, -1);
	ite_filterChain_link(&vstream->fc, 0, vstream->Fjpegdec, 0);
	ite_filterChain_link(&vstream->fc, 0, vstream->Fdisplay, 0);

 	//MJPEG to AVI recoder
	ite_filterChain_A_link_B(false, &vstream->fc, vstream->Fuvc, 1, vstream->Frec_avi, 0);   

    ite_filterChain_run(&vstream->fc);
    
    f->videostream = vstream;
}

uint8_t flow_stop_uvc_mjpeg(IteFlower *f)
{
	IteVideoFlower *s=f->videostream;
	
	ite_filterChain_stop(&s->fc);
	
	if(s->Fuvc) ite_filterChain_unlink(&s->fc,-1,s->Fuvc, -1);
	if(s->Fjpegdec) ite_filterChain_unlink(&s->fc, 0,s->Fjpegdec,  0);
	if(s->Fdisplay) ite_filterChain_unlink(&s->fc, 0,s->Fdisplay,  0);
    //MJPEG to AVI recoder
    if(s->Frec_avi) ite_filterChain_A_unlink_B(false, &s->fc, s->Fuvc, 1, s->Frec_avi, 0);

   	ite_filterChain_delete(&s->fc);

	video_flower_free(s);

}

void flow_start_uvc_mjpeg_record(IteFlower *f, char* file_path, int width, int height, int fps){

	pthread_mutex_lock(&VideoFlow_mutex);
	
	VideoMemoInfo *info = (VideoMemoInfo*)ite_new0(VideoMemoInfo,1);
    strcpy(info->videomemo_file, file_path);
	info->video_width = width;
	info->video_height = height;
	info->video_fps = fps;
	ite_filter_call_method(f->videostream->Frec_avi, ITE_FILTER_MJPEG_REC_AVI_OPEN, (void*)info);
	if(info)
		free(info);	
	pthread_mutex_unlock(&VideoFlow_mutex);
}

void flow_stop_uvc_mjpeg_record(IteFlower *f){

	pthread_mutex_lock(&VideoFlow_mutex);
	ite_filter_call_method(f->videostream->Frec_avi, ITE_FILTER_MJPEG_REC_AVI_CLOSE, NULL);
	pthread_mutex_unlock(&VideoFlow_mutex);
}
/*MJPEG Control flow End*/

/*H264 Control flow*/
uint8_t flow_start_uvc_h264(IteFlower *f, int width, int height, int fps)
{
    IteUVCStream* uvc_ptr = (IteUVCStream*)ite_new0(IteUVCStream, 1);

	IteVideoFlower *vstream = (IteVideoFlower *)ite_new0 (IteVideoFlower, 1);
	char* args[] ={"-Q=32"};
	
	vstream->Fuvc = ite_filter_new(ITE_FILTER_UVC_ID);
	vstream->Fh264dec = ite_filter_new(ITE_FILTER_H264DEC_ID);
	vstream->Fdisplay = ite_filter_new(ITE_FILTER_DISPLAY_ID);
	vstream->Frec_avi = ite_filter_new(ITE_FILTER_REC_AVI_ID);


	ite_filterChain_build(&vstream->fc, "FC UVC");
	ite_filterChain_setConfig(&vstream->fc, ARRAY_COUNT_OF(args), args);


    uvc_ptr->UVC_format = FLOWER_UVC_FRAME_FORMAT_H264;
    uvc_ptr->UVC_Width  = width;
    uvc_ptr->UVC_Height = height;
    uvc_ptr->UVC_FPS    = fps;

    ite_filter_call_method(vstream->Fuvc, ITE_FILTER_UVC_SETTING, (void*)uvc_ptr);
    if(uvc_ptr)
        free(uvc_ptr);

	ite_filterChain_link(&vstream->fc, -1, vstream->Fuvc, -1);
	ite_filterChain_link(&vstream->fc, 0, vstream->Fh264dec, 0);
	ite_filterChain_link(&vstream->fc, 0, vstream->Fdisplay, 0);

 	//MJPEG to AVI recoder
	ite_filterChain_A_link_B(false, &vstream->fc, vstream->Fuvc, 1, vstream->Frec_avi, 0);   

    ite_filterChain_run(&vstream->fc);
    
    f->videostream = vstream;

}


uint8_t flow_stop_uvc_h264(IteFlower *f)
{
	IteVideoFlower *s=f->videostream;
	
	ite_filterChain_stop(&s->fc);
	
	if(s->Fuvc) ite_filterChain_unlink(&s->fc,-1,s->Fuvc, -1);
	if(s->Fh264dec) ite_filterChain_unlink(&s->fc, 0,s->Fjpegdec,  0);
	if(s->Fdisplay) ite_filterChain_unlink(&s->fc, 0,s->Fdisplay,  0);
    //MJPEG to AVI recoder
    if(s->Frec_avi) ite_filterChain_A_unlink_B(false, &s->fc, s->Fuvc, 1, s->Frec_avi, 0);

   	ite_filterChain_delete(&s->fc);

	video_flower_free(s);

}

void flow_start_uvc_h264_record(IteFlower *f, char* file_path, int width, int height, int fps){

	pthread_mutex_lock(&VideoFlow_mutex);
	
	VideoMemoInfo *info = (VideoMemoInfo*)ite_new0(VideoMemoInfo,1);
    strcpy(info->videomemo_file, file_path);
	info->video_width = width;
	info->video_height = height;
	info->video_fps = fps;
	ite_filter_call_method(f->videostream->Frec_avi, ITE_FILTER_REC_AVI_OPEN, (void*)info);
	if(info)
		free(info);	
	pthread_mutex_unlock(&VideoFlow_mutex);
}

void flow_stop_uvc_h264_record(IteFlower *f){

	pthread_mutex_lock(&VideoFlow_mutex);
	ite_filter_call_method(f->videostream->Frec_avi, ITE_FILTER_REC_AVI_CLOSE, NULL);
	pthread_mutex_unlock(&VideoFlow_mutex);
}

/*H264 Control flow End*/

#endif
