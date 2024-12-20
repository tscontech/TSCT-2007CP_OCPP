#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "openrtos/FreeRTOS.h"
#include "flower/flower.h"
#include "ite/audio.h"


/*MixerPlay start*/
void MixerPlayFlowStart(IteFlower *f,const char* filepath,PlaySoundCase mode,cb_sound_t cb){

    //IteFChain fc1=s->fc;
    IteAudioFlower *s=(IteAudioFlower*)ite_new0(IteAudioFlower,1);
    char* args[] ={"-S","-Q=32",};

    //fc: file->mix->spk
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

    s->Fmix      = ite_filter_new(ITE_FILTER_MIX_ID);
    s->Fsndwrite = ite_filter_new(ITE_FILTER_SNDWRITE_ID);

    ite_filter_call_method(s->Fsource,ITE_FILTER_FILEOPEN ,(void*)filepath);
    ite_filter_call_method(s->Fsource,ITE_FILTER_LOOPPLAY,&mode);  
    if(cb) ite_filter_call_method(s->Fsndwrite,ITE_FILTER_SET_CB,cb);	
	
    
    ite_filterChain_build(&s->fc, "FC 1");
    ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);
    
    ite_filterChain_link(&s->fc, -1, s->Fsource, -1);
    ite_filterChain_link(&s->fc, 0, s->Fmix, 0);
    ite_filterChain_link(&s->fc, 0, s->Fsndwrite, 0);

    ite_filterChain_run(&s->fc);  
    
    //f->audiocase=AudioMixerFile;
    f->audiostream = s;
}

void MixerPlayMixStop(IteFlower *f){
	IteAudioFlower *s=f->audiostream;
    bool mixpass=true;
    ite_filter_call_method(s->Fmix,ITE_FILTER_SET_BYPASS,&mixpass); //stop mix
    if(s->Fsrc){
        ite_filterChain_stop(&s->fcc);
        ite_filterChain_unlink(&s->fcc, -1, s->Fsrc    , -1);
        ite_filterChain_unlink(&s->fcc,  0, s->Fencoder,  0);
        ite_filterChain_unlink(&s->fcc,  0, s->Fdecoder,  0);
        ite_filterChain_unlink(&s->fcc,  0, s->Fmix, 1);
        ite_filterChain_delete(&s->fcc);          
        ite_filter_delete(s->Fsrc);
        ite_filter_delete(s->Fencoder);
        ite_filter_delete(s->Fdecoder);
        s->Fsrc=NULL;
        s->Fencoder=NULL;
        s->Fdecoder=NULL;
    }
 
}

void MixerPlayMixStart(IteFlower *f,const char* filepath, PlaySoundCase mode, cb_sound_t cb){
    IteAudioFlower *s=f->audiostream;
    char* args[] ={"-S","-Q=32",};
	int out_rate = 0;
    int out_channel= 0;
    int in_rate = 0;
    int in_channel =0;    
    bool mixpass=false;
    bool newfilter=false;
    if(f->audiostream==NULL) {
        printf("main music not exit\n");
        return;
    }
    if(!s->Fsrc){
        s->Fsrc     = ite_filter_new(ITE_FILTER_FILEPLAY_ID);//wav
        s->Fdecoder = ite_filter_new(ITE_FILTER_RESAMPLE_ID);// 
        s->Fencoder = ite_filter_new(ITE_FILTER_CHADAPT_ID);
        newfilter=true;
    }
    ite_filter_call_method(s->Fsrc,ITE_FILTER_SET_BYPASS,NULL);
    ite_filter_call_method(s->Fsrc,ITE_FILTER_SET_FILEPATH ,(void*)filepath);
    ite_filter_call_method(s->Fsrc,ITE_FILTER_LOOPPLAY,&mode);
	ite_filter_call_method(s->Fsrc,ITE_FILTER_GETRATE,&in_rate);
	ite_filter_call_method(s->Fsrc,ITE_FILTER_GET_NCHANNELS,&in_channel);

	ite_filter_call_method(s->Fsource,ITE_FILTER_GETRATE,&out_rate);
	ite_filter_call_method(s->Fsource,ITE_FILTER_GET_NCHANNELS,&out_channel);

    ite_filter_call_method(s->Fencoder,ITE_FILTER_SET_VALUE,&in_channel);
    ite_filter_call_method(s->Fencoder,ITE_FILTER_SET_NCHANNELS,&out_channel);
    
    ite_filter_call_method(s->Fdecoder,ITE_FILTER_INPUT_RATE,&in_rate);
    ite_filter_call_method(s->Fdecoder,ITE_FILTER_OUTPUT_RATE,&out_rate);
    ite_filter_call_method(s->Fdecoder,ITE_FILTER_SET_NCHANNELS,&out_channel);

    if(newfilter){
        ite_filterChain_build(&s->fcc, "FC 2");
        ite_filterChain_setConfig(&s->fcc, ARRAY_COUNT_OF(args), args);

        ite_filterChain_link(&s->fcc, -1, s->Fsrc    , -1);
        ite_filterChain_link(&s->fcc,  0, s->Fencoder,  0);
        ite_filterChain_link(&s->fcc,  0, s->Fdecoder,  0);
        ite_filterChain_link(&s->fcc,  0, s->Fmix, 1);
        ite_filterChain_run(&s->fcc);
    }

    f->audiostream = s;
    
    ite_filter_call_method(s->Fmix,ITE_FILTER_SET_BYPASS,&mixpass); //start mix
}

void MixerPlayFlowStop(IteFlower *f){
    IteAudioFlower *s=f->audiostream;
    //if(f->audiocase!=AudioMixerFile) return;
    
    MixerPlayMixStop(f);

    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsource, -1);
    ite_filterChain_unlink(&s->fc, 0, s->Fmix, 0);
    ite_filterChain_unlink(&s->fc, 0, s->Fsndwrite, 0);
    ite_filterChain_delete(&s->fc);
    
    audio_flower_free(s);
    f->audiostream=NULL;
    f->audiocase=AudioIdel;
}
/*MixerPlay end*/

/*MixerMicer start*/
void MicMixerFlowStart(IteFlower *f,const char* filepath,PlaySoundCase mode,cb_sound_t cb){
    IteAudioFlower *s=(IteAudioFlower*)ite_new0(IteAudioFlower,1);
    char* args[] ={"-S","-Q=32",};

    if(audiomgrCodecType(filepath)==ITE_WAV_DECODE){
        s->Fsource   = ite_filter_new(ITE_FILTER_SNDREAD_ID);//wav
    }else {
        #ifdef CFG_BUILD_FFMPEG
        if(_isffmpeg(filepath)) 
            s->Fsource   = ite_filter_new(ITE_FILTER_PLAYM4A_ID);//m4a
        else
        #endif
            s->Fsource   = ite_filter_new(ITE_FILTER_FILEPLAYMGR_ID);//mp3 aac ...
    }

    s->Fmix      = ite_filter_new(ITE_FILTER_MIX_ID);
    s->Fsndwrite = ite_filter_new(ITE_FILTER_SNDWRITE_ID);

    ite_filter_call_method(s->Fsource,ITE_FILTER_FILEOPEN ,(void*)filepath);
    ite_filter_call_method(s->Fsource,ITE_FILTER_LOOPPLAY,&mode);  
    if(cb) ite_filter_call_method(s->Fsndwrite,ITE_FILTER_SET_CB,cb);	
	
    
    ite_filterChain_build(&s->fc, "FC 1");
    ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);
    
    ite_filterChain_link(&s->fc, -1, s->Fsource, -1);
    ite_filterChain_link(&s->fc, 0, s->Fmix, 0);
    ite_filterChain_link(&s->fc, 0, s->Fsndwrite, 0);

    ite_filterChain_run(&s->fc);  
    
    //f->audiocase=AudioMicMixer;
    f->audiostream = s;
}

void MicMixerMixStop(IteFlower *f){
	IteAudioFlower *s=f->audiostream;
    bool mixpass=true;
    ite_filter_call_method(s->Fmix,ITE_FILTER_SET_BYPASS,&mixpass); //stop mix
    if(s->Fsrc){
        ite_filterChain_stop(&s->fcc);
        ite_filterChain_unlink(&s->fcc, -1, s->Fsrc    , -1);
        ite_filterChain_unlink(&s->fcc,  0, s->Fmix    ,  1);
        ite_filterChain_delete(&s->fcc);          
        ite_filter_delete(s->Fsrc);
        s->Fsrc=NULL;
    }
 
}

void MicMixerMixStart(IteFlower *f){
    IteAudioFlower *s=f->audiostream;
    char* args[] ={"-S","-Q=32",};
    bool mixpass=false;
    
	s->Fsrc     = ite_filter_new(ITE_FILTER_SNDREAD_ID);//wav     
	
    ite_filterChain_build(&s->fcc, "FC 2");
    ite_filterChain_setConfig(&s->fcc, ARRAY_COUNT_OF(args), args);
    
    ite_filterChain_link(&s->fcc, -1, s->Fsrc    , -1);
    ite_filterChain_link(&s->fcc,  0, s->Fmix    ,  1);
    ite_filterChain_run(&s->fcc);  
    
    f->audiostream = s;
    
    ite_filter_call_method(s->Fmix,ITE_FILTER_SET_BYPASS,&mixpass); //start mix
}

void MicMixerFlowStop(IteFlower *f){
    IteAudioFlower *s=f->audiostream;
    //if(f->audiocase!=AudioMicMixer) return;
    
    MicMixerMixStop(f);

    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsource, -1);
    ite_filterChain_unlink(&s->fc, 0, s->Fmix, 0);
    ite_filterChain_unlink(&s->fc, 0, s->Fsndwrite, 0);
    ite_filterChain_delete(&s->fc);
    
    audio_flower_free(s);
    f->audiostream=NULL;
    f->audiocase=AudioIdel;
}
/*MixerMIC end*/


//*Quick play: insert short sound*//
void QuickPlayFlowStart(IteFlower *f,const char* filepath, PlaySoundCase mode, cb_sound_t cb){
    IteAudioFlower *s=(IteAudioFlower*)ite_new0(IteAudioFlower,1);
    char* args[] ={"-S","-Q=32",};
    
    s->Fsource   = ite_filter_new(ITE_FILTER_QUICKPLAY_ID);//mp3 aac ...
    ite_filter_call_method(s->Fsource,ITE_FILTER_FILEOPEN ,(void*)filepath);
	if(cb) ite_filter_call_method(s->Fsource,ITE_FILTER_SET_CB,cb);
	ite_filter_call_method(s->Fsource,ITE_FILTER_LOOPPLAY , &mode);
	
    ite_filterChain_build(&s->fc, "FC 1");
    ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);
    
    ite_filterChain_link(&s->fc, -1, s->Fsource, -1);
    ite_filterChain_run(&s->fc);  
    
    f->audiocase=QuickPlay;
    f->audiostream = s;
}

void QuickPlayFlowPause(IteFlower *f,bool pause){
    IteAudioFlower *s=f->audiostream;
    IteFilter *F1=s->Fsource;
    if(F1->filterDes.id==ITE_FILTER_QUICKPLAY_ID){
        ite_filter_call_method(F1,ITE_FILTER_PAUSE,&pause);
    }
}

void QuickPlayInsertStop(IteFlower *f,bool resume){
	IteAudioFlower *s=f->audiostream;
    if(s->Fmix){
        ite_filterChain_stop(&s->fcc);
        ite_filterChain_unlink(&s->fcc, -1, s->Fmix, -1);
        ite_filterChain_delete(&s->fcc);
        ite_filter_delete(s->Fmix);
        s->Fmix=NULL;
    }
    if(resume) QuickPlayFlowPause(f,false);//main musuc rusume;  
}

void QuickPlayInsertStart(IteFlower *f,const char* filepath, PlaySoundCase mode, cb_sound_t cb){
    IteAudioFlower *s=f->audiostream;
    char* args[] ={"-S","-Q=32",};
    
    if(f->audiostream==NULL && f->audiocase!=QuickPlay) {
        QuickPlayFlowStart(f,filepath,mode,cb);
        printf("main music not exit\n");
        return;
    }
    
    QuickPlayInsertStop(f,false);//force previously insert sound stop;
    QuickPlayFlowPause(f,true);//pause main music pause
    
    s->Fmix   = ite_filter_new(ITE_FILTER_QUICKPLAY_ID);//mp3 aac ...
    ite_filter_call_method(s->Fmix,ITE_FILTER_FILEOPEN ,(void*)filepath);
	if(cb) ite_filter_call_method(s->Fmix,ITE_FILTER_SET_CB,cb);
	ite_filter_call_method(s->Fmix,ITE_FILTER_LOOPPLAY , &mode);
    ite_filter_call_method(s->Fmix,ITE_FILTER_SET_BYPASS,NULL);
	
    ite_filterChain_build(&s->fcc, "FC 2");
    ite_filterChain_setConfig(&s->fcc, ARRAY_COUNT_OF(args), args);
    
    ite_filterChain_link(&s->fcc, -1, s->Fmix, -1);
    ite_filterChain_run(&s->fcc);  
    
    f->audiostream = s;
}

void QuickPlayFlowStop(IteFlower *f){
	IteAudioFlower *s=f->audiostream;
    if(f->audiocase!=QuickPlay) return;
    
    QuickPlayInsertStop(f,false);//force insert sound stop;

    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsource, -1);
    ite_filterChain_delete(&s->fc);

    audio_flower_free(s);
    f->audiostream=NULL;
    f->audiocase=AudioIdel;
}

/*end Quick play*/