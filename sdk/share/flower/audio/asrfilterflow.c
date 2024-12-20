#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "openrtos/FreeRTOS.h"
#include "flower/flower.h"
#include "i2s/i2s.h"
#include "ite/audio.h"
extern STRC_I2S_SPEC spec_ad;
extern STRC_I2S_SPEC spec_da;
/*ASR start*/
void AsrFlowStart(IteFlower *f,cb_sound_t cb){
    #if CFG_BUILD_ASR
    IteAudioFlower *s=(IteAudioFlower*)ite_new0(IteAudioFlower,1);
    char* args[] ={"-S","-Q=32",};
    
    //ite_flower_init();
    
    s->Fsndread   = ite_filter_new(ITE_FILTER_SNDREAD_ID);
    s->Fasr       = ite_filter_new(ITE_FILTER_ASR_ID);

    if(s->Fasr) ite_filter_call_method(s->Fasr,ITE_FILTER_SET_CB,cb);
 
    //Castor3snd_reinit_for_diff_rate(16000,16,1);
 
    ite_filterChain_build(&s->fc, "FC 1");
    ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);
    
    ite_filterChain_link(&s->fc, -1, s->Fsndread, -1);
    ite_filterChain_link(&s->fc, 0, s->Fasr, 0);

    //ite_filterChain_print(&s->fc);

    ite_filterChain_run(&s->fc);
    
    f->asrstream = s;
    #endif
}

void AsrFlowStop(IteFlower *f){
    #if CFG_BUILD_ASR
    IteAudioFlower *s=f->asrstream;

    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsndread, -1);
    ite_filterChain_unlink(&s->fc, 0, s->Fasr, 0);
    ite_filterChain_delete(&s->fc);
    
    audio_flower_free(s);
    #endif
}


/*ASR end*/

void ResmplePlayFlowStart(IteFlower *f,const char* filepath,PlaySoundCase mode,cb_sound_t cb,int out_rate){

    //IteFChain fc1=s->fc;
    IteAudioFlower *s=(IteAudioFlower*)ite_new0(IteAudioFlower,1);
    char* args[] ={"-S","-Q=32",};
    int in_rate,channels;
    
    //ite_flower_init();

    if(audiomgrCodecType(filepath)==ITE_WAV_DECODE){
        s->Fsource   = ite_filter_new(ITE_FILTER_FILEPLAY_ID);//wav
    }else {
        #ifdef CFG_BUILD_FFMPEG
        if(_isffmpeg(filepath)) 
            s->Fsource   = ite_filter_new(ITE_FILTER_PLAYM4A_ID);//m4a
        else
        #endif
            s->Fsource   = ite_filter_new(ITE_FILTER_FILEPLAYMGR_ID);//mp3 aac ...
    }
    s->Fdecoder  = ite_filter_new(ITE_FILTER_RESAMPLE_ID);//resample
    s->Fsndwrite = ite_filter_new(ITE_FILTER_SNDWRITE_ID);

    if(s->Fdecoder) ite_filter_call_method(s->Fsource,ITE_FILTER_SET_BYPASS ,NULL);
    ite_filter_call_method(s->Fsource,ITE_FILTER_FILEOPEN ,(void*)filepath);
    ite_filter_call_method(s->Fsource,ITE_FILTER_GETRATE,&in_rate); 
    ite_filter_call_method(s->Fsource,ITE_FILTER_GET_NCHANNELS,&channels); 
    ite_filter_call_method(s->Fsource,ITE_FILTER_LOOPPLAY,&mode); 

    if(s->Fdecoder){
        ite_filter_call_method(s->Fdecoder,ITE_FILTER_INPUT_RATE,&in_rate); 
        ite_filter_call_method(s->Fdecoder,ITE_FILTER_OUTPUT_RATE,&out_rate);
        ite_filter_call_method(s->Fdecoder,ITE_FILTER_SET_NCHANNELS,&channels); 
        Castor3snd_reinit_for_diff_rate(out_rate,16,channels);
    }

    if(cb) ite_filter_call_method(s->Fsndwrite,ITE_FILTER_SET_CB,cb);
    
    ite_filterChain_build(&s->fc, "FC 1");
    ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);
    
    ite_filterChain_link(&s->fc, -1, s->Fsource, -1);
    if(s->Fdecoder) ite_filterChain_link(&s->fc, 0, s->Fdecoder, 0);
    ite_filterChain_link(&s->fc, 0, s->Fsndwrite, 0);

    //ite_filterChain_print(&s->fc);

    ite_filterChain_run(&s->fc);  
    
    f->audiocase=AResamplePlayFile;
    f->audiostream = s;
}

void ResmplePlayFlowStop(IteFlower *f){
    IteAudioFlower *s=f->audiostream;
    if(f->audiocase!=AResamplePlayFile) return;

    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsource, -1);
    if(s->Fdecoder) ite_filterChain_unlink(&s->fc, 0, s->Fdecoder, 0);
    ite_filterChain_unlink(&s->fc, 0, s->Fsndwrite, 0);
    ite_filterChain_delete(&s->fc);
    audio_flower_free(s);
}

void SynASRFlowStart(IteFlower *f,cb_sound_t cb){
 
    //IteFChain fc1=s->fc;
    IteAudioFlower *s=(IteAudioFlower*)ite_new0(IteAudioFlower,1);
    IteFilter *F1=f->audiostream->Fsndwrite;
    char* args[] ={"-S","-Q=32",};
    int in_rate,in_channels;
	int channels = 1;//asr input must be one channel
	int out_rate = 16000; //must be 16000
    
    if(F1){//play exit
        in_rate =spec_ad.sample_rate;
        in_channels=spec_ad.channels;
    }else{
        in_rate=16000;
        in_channels=1;
        Castor3snd_reinit_for_diff_rate(16000,16,1);
    }
    
#if CFG_BUILD_ASR
	s->Fsndread  = ite_filter_new(ITE_FILTER_SNDREAD_ID);
    s->Fasr      = ite_filter_new(ITE_FILTER_ASR_ID);
#endif
    s->Fsource   = ite_filter_new(ITE_FILTER_LOOPBACK_ID);//loopback
    s->Faec      = ite_filter_new(ITE_FILTER_AEC_ID);//aec
    //ite_filter_call_method(s->Faec,ITE_FILTER_SET_BYPASS,NULL);
    
    if(in_channels!=channels){
        s->Ftee = ite_filter_new(ITE_FILTER_CHADAPT_ID);//2channel ->1channel
        s->Fsrc  = ite_filter_new(ITE_FILTER_CHADAPT_ID);//2channel ->1channel
        ite_filter_call_method(s->Ftee,ITE_FILTER_SET_VALUE,&in_channels);
        ite_filter_call_method(s->Fsrc,ITE_FILTER_SET_VALUE,&in_channels);
    }
    
    if(in_rate!=out_rate){
        
        s->Fdecoder  = ite_filter_new(ITE_FILTER_RESAMPLE_ID);//resample
        s->Fencoder  = ite_filter_new(ITE_FILTER_RESAMPLE_ID);//resample
        
        ite_filter_call_method(s->Fdecoder,ITE_FILTER_INPUT_RATE,&in_rate); 
        ite_filter_call_method(s->Fdecoder,ITE_FILTER_OUTPUT_RATE,&out_rate);
        ite_filter_call_method(s->Fdecoder,ITE_FILTER_SET_NCHANNELS,&channels);
        
        ite_filter_call_method(s->Fencoder,ITE_FILTER_INPUT_RATE,&in_rate); 
        ite_filter_call_method(s->Fencoder,ITE_FILTER_OUTPUT_RATE,&out_rate);
        ite_filter_call_method(s->Fencoder,ITE_FILTER_SET_NCHANNELS,&channels);
    }

    if(s->Faec){
        
        ite_filterChain_build(&s->fcc, "FC 2");
        ite_filterChain_setConfig(&s->fcc, ARRAY_COUNT_OF(args), args);
    
        ite_filterChain_link(&s->fcc,-1, s->Fsource, -1);
        if(s->Ftee)     ite_filterChain_link(&s->fcc, 0, s->Ftee, 0);
        if(s->Fencoder) ite_filterChain_link(&s->fcc, 0, s->Fencoder, 0);
        ite_filterChain_link(&s->fcc, 0, s->Faec, 0);
    }
#if CFG_BUILD_ASR
	//fccc:mic->resample->AEC->ASR
    if(s->Fasr){
       
        ite_filter_call_method(s->Fasr,ITE_FILTER_SET_BYPASS,NULL);//must before ITE_FILTER_SET_CB method, do not init AD DA in filter function
		ite_filter_call_method(s->Fasr,ITE_FILTER_SET_CB,cb);
    
        ite_filterChain_build(&s->fc, "FC 1");
        ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);

        ite_filterChain_link(&s->fc, -1, s->Fsndread, -1);
        if(s->Fsrc)      ite_filterChain_link(&s->fc, 0, s->Fsrc, 0);
        if(s->Fdecoder) ite_filterChain_link(&s->fc, 0, s->Fdecoder, 0);
        if(s->Faec) ite_filterChain_link(&s->fc, 0, s->Faec, 1);	 
        ite_filterChain_link(&s->fc, 0, s->Fasr, 0);
    }
#endif
	if(s->Fasr) ite_filterChain_run(&s->fc);
    if(s->Faec) ite_filterChain_run(&s->fcc);
    
   // f->audiocase=AResamplePlayFile;
    f->asrstream = s;    
    
}


void SynASRFlowStop(IteFlower *f){
	IteAudioFlower *s=f->asrstream;

    ite_filterChain_stop(&s->fcc);
    ite_filterChain_unlink(&s->fcc,-1, s->Fsource,-1);
    if(s->Ftee)     ite_filterChain_link(&s->fc, 0, s->Ftee, 0);
    if(s->Fencoder) ite_filterChain_link(&s->fc, 0, s->Fencoder, 0);
    if(s->Faec) ite_filterChain_unlink(&s->fcc, 0, s->Faec, 0);
    ite_filterChain_delete(&s->fcc);	

#if CFG_BUILD_ASR
    ite_filterChain_stop(&s->fc);
	ite_filterChain_unlink(&s->fc, -1, s->Fsndread, -1);
    if(s->Fsrc)     ite_filterChain_unlink(&s->fc, 0, s->Fsrc, 0);
    if(s->Fdecoder) ite_filterChain_unlink(&s->fc, 0, s->Fdecoder, 0);
	if(s->Faec)     ite_filterChain_unlink(&s->fc, 0, s->Faec, 1);	 
	ite_filterChain_unlink(&s->fc, 0, s->Fasr, 0);
	ite_filterChain_delete(&s->fc);
#endif
	audio_flower_free(s);

}