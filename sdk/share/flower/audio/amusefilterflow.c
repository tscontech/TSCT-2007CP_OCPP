#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "openrtos/FreeRTOS.h"
#include "flower/flower.h"
#include "ite/audio.h"

/*play sound start*/
void PlayFlowStart(IteFlower *f,const char* filepath,PlaySoundCase mode,cb_sound_t cb){

    //IteFChain fc1=s->fc;
    IteAudioFlower *s=(IteAudioFlower*)ite_new0(IteAudioFlower,1);
    char* args[] ={"-S","-Q=32",};
    
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

    s->Fmix      = ite_filter_new(ITE_FILTER_FILEMIX_ID);
    s->Fsndwrite = ite_filter_new(ITE_FILTER_SNDWRITE_ID);

    //ite_filter_call_method(s->Fsource,ITE_FILTER_SET_FILEPATH ,(void*)filepath);//serial path="a:/test1.wav a:/test2.wav"
    ite_filter_call_method(s->Fsource,ITE_FILTER_FILEOPEN ,(void*)filepath);
    ite_filter_call_method(s->Fsource,ITE_FILTER_LOOPPLAY,&mode); 
    //ite_filter_call_method(s->Fmix,ITE_FILTER_FILEOPEN,(void*)fileinsert); 
    if(cb) ite_filter_call_method(s->Fsndwrite,ITE_FILTER_SET_CB,cb);
    
    ite_filterChain_build(&s->fc, "FC 1");
    ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);
    
    ite_filterChain_link(&s->fc, -1, s->Fsource, -1);
    if(s->Fmix) ite_filterChain_link(&s->fc, 0, s->Fmix, 0);
    ite_filterChain_link(&s->fc, 0, s->Fsndwrite, 0);

    ite_filterChain_run(&s->fc);  
    
    f->audiocase=AudioPlayFile;
    f->audiostream = s;

}

void PlayFlowStop(IteFlower *f){
    IteAudioFlower *s=f->audiostream;
    if(f->audiocase!=AudioPlayFile) return;

    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsource, -1);
    if(s->Fmix) ite_filterChain_unlink(&s->fc, 0, s->Fmix, 0);
    ite_filterChain_unlink(&s->fc, 0, s->Fsndwrite, 0);
    ite_filterChain_delete(&s->fc);
    audio_flower_free(s);
}

void PlayFlowPause(IteFlower *f,bool pause){
    IteAudioFlower *s=f->audiostream;
    IteFilter *F1=s->Fsource;
    IteFilter *F2=s->Fsndwrite;
    if(F1->filterDes.id==ITE_FILTER_FILEPLAY_ID && 
       F2->filterDes.id==ITE_FILTER_SNDWRITE_ID ){
        ite_filter_call_method(F1,ITE_FILTER_PAUSE,&pause);
        ite_filter_call_method(F2,ITE_FILTER_PAUSE,&pause);
    }
}

void PlayFlowMix(IteFlower *f,const char *fileinsert){
    IteAudioFlower *s=f->audiostream;
    IteFilter *F1=s->Fmix;
    if(F1->filterDes.id==ITE_FILTER_FILEMIX_ID){
        ite_filter_call_method(F1,ITE_FILTER_FILEOPEN,(void*)fileinsert); 
    }
}

void PlayFlowWait(IteFlower *f,int wait){
    /*special case :file data from network*/
    IteAudioFlower *s=f->audiostream;
    if(s->Fsource)
        ite_filter_call_method(s->Fsource,ITE_FILTER_SET_VALUE ,&wait);
}


void PlayFlowGetTime(IteFlower *f,int *timestamp){
    IteAudioFlower *s=f->audiostream;
    IteFilter *F2=s->Fsndwrite;
    if(F2->filterDes.id==ITE_FILTER_SNDWRITE_ID ){
        ite_filter_call_method(F2,ITE_FILTER_GET_VALUE, timestamp);
    }
}

//
/*void QuickPlayFlowStart(IteFlower *f,const char* filepath, PlaySoundCase mode, cb_sound_t cb){
   //IteFChain fc1=s->fc;
    IteAudioFlower *s=(IteAudioFlower*)ite_new0(IteAudioFlower,1);
    char* args[] ={"-S","-Q=32",};
    
    //ite_flower_init();
    s->Fsource   = ite_filter_new(ITE_FILTER_QUICKPLAY_ID);//mp3 aac ...
    ite_filter_call_method(s->Fsource,ITE_FILTER_FILEOPEN ,(void*)filepath);
	if(cb) ite_filter_call_method(s->Fsource,ITE_FILTER_SET_CB,cb);
	ite_filter_call_method(s->Fsource,ITE_FILTER_LOOPPLAY , &mode);
	
    
    ite_filterChain_build(&s->fc, "FC 1");
    ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);
    
    ite_filterChain_link(&s->fc, -1, s->Fsource, -1);


	//insert
	s->Fmix= ite_filter_new(ITE_FILTER_QUICKPLAY_ID);//mp3 aac ...
	ite_filter_call_method(s->Fmix,ITE_FILTER_SET_BYPASS , NULL);
	if(cb) ite_filter_call_method(s->Fmix,ITE_FILTER_SET_CB,cb);
	ite_filterChain_build(&s->fcc, "FC 2");
    ite_filterChain_setConfig(&s->fcc, ARRAY_COUNT_OF(args), args);
    
    ite_filterChain_link(&s->fcc, -1, s->Fmix, -1);
	

    ite_filterChain_run(&s->fc);  
    
    f->audiocase=QuickPlay;
    f->audiostream = s;
}

void QuickPlayFlowStop(IteFlower *f){
	IteAudioFlower *s=f->audiostream;
    if(f->audiocase!=QuickPlay) return;

    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsource, -1);
    ite_filterChain_delete(&s->fc);

	ite_filterChain_stop(&s->fcc);
    ite_filterChain_unlink(&s->fcc, -1, s->Fmix, -1);
    ite_filterChain_delete(&s->fcc);
    audio_flower_free(s);
}

void QuickPlayFlowPause(IteFlower *f,bool pause){
    IteAudioFlower *s=f->audiostream;
    IteFilter *F1=s->Fsource;
    if(F1->filterDes.id==ITE_FILTER_QUICKPLAY_ID){
        ite_filter_call_method(F1,ITE_FILTER_PAUSE,&pause);
    }
}

void QuickPlayInsert(IteFlower *f,const char* filepath){
	IteAudioFlower *s=f->audiostream;
	IteFilter *F1=s->Fsource;
	IteFilter *F2=s->Fmix;
	static bool run = false; 
    if(F2->filterDes.id==ITE_FILTER_QUICKPLAY_ID){
		if(F1->filterDes.id==ITE_FILTER_QUICKPLAY_ID){
			bool is_busy = true;
			do{
				ite_filter_call_method(F1,ITE_FILTER_GET_STATUS ,&is_busy);
				
			}while(is_busy == true);
         ite_filter_call_method(F2,ITE_FILTER_FILEOPEN ,(void*)filepath);
		}
		
		if(F2->run == false)
		{
			printf("RUN RUN RUN\n");
			ite_filterChain_run(&s->fcc);
		}
    }
	
	
	
}

*/
/*play sound end*/

/*record sound start*/
void RecFlowStart(IteFlower *f,const char* filename,int rate){
    
    IteAudioFlower *s=(IteAudioFlower*)ite_new0(IteAudioFlower,1);
    char* args[] ={"-S","-Q=32",};
    
    //ite_flower_init();
    
    s->Fsndread   = ite_filter_new(ITE_FILTER_SNDREAD_ID);
	s->Fmix        = ite_filter_new(ITE_FILTER_DENOISE_ID);
    s->Fdestinat  = ite_filter_new(ITE_FILTER_FILEREC_ID);
    
    ite_filter_call_method(s->Fdestinat,ITE_FILTER_FILEOPEN ,(void*)filename);
    ite_filter_call_method(s->Fdestinat,ITE_FILTER_SETRATE ,&rate);
    
    ite_filterChain_build(&s->fc, "FC 1");
    ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);
    
    ite_filterChain_link(&s->fc, -1, s->Fsndread, -1);
	ite_filterChain_link(&s->fc, 0, s->Fmix, 0);
    ite_filterChain_link(&s->fc, 0, s->Fdestinat, 0);

    //ite_filterChain_print(&s->fc);

    ite_filterChain_run(&s->fc);
    f->audiocase=AudioRecFile;
    f->audiostream = s;
}

void RecFlowStop(IteFlower *f){
    IteAudioFlower *s=f->audiostream;
    if(f->audiocase!=AudioRecFile) return;
    
    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsndread, -1);
	ite_filterChain_unlink(&s->fc, 0, s->Fmix, 0);
    ite_filterChain_unlink(&s->fc, 0, s->Fdestinat, 0);
    ite_filterChain_delete(&s->fc);
    
    audio_flower_free(s);
}


/*record sound end*/

/*sound RW start*/
void SndrwFlowStart(IteFlower *f,int rate,AudioCodecType type){
    
    IteAudioFlower *s=(IteAudioFlower*)ite_new0(IteAudioFlower,1);
    char* args[] ={"-S","-Q=32",};
    
    //ite_flower_init();
    
    s->Fsndread  = ite_filter_new(ITE_FILTER_SNDREAD_ID);
	s->Fmix      = ite_filter_new(ITE_FILTER_FILEMIX_ID);
    //s->Fencoder  = ite_filter_new(ITE_FILTER_ADPCM_ENC_ID);
    //s->Fdecoder  = ite_filter_new(ITE_FILTER_ADPCM_DEC_ID);
    s->Fsndwrite = ite_filter_new(ITE_FILTER_SNDWRITE_ID);
    
    Castor3snd_reinit_for_diff_rate(rate,16,1);

    ite_filterChain_build(&s->fc, "FC 1");
    ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);
    
    ite_filterChain_link(&s->fc, -1, s->Fsndread, -1);
    if(s->Fencoder) ite_filterChain_link(&s->fc, 0, s->Fencoder, 0);
    if(s->Fdecoder) ite_filterChain_link(&s->fc, 0, s->Fdecoder, 0);
	if(s->Fmix) ite_filterChain_link(&s->fc, 0, s->Fmix, 0);
	ite_filterChain_link(&s->fc, 0, s->Fsndwrite, 0);

    //ite_filterChain_print(&s->fc);

    ite_filterChain_run(&s->fc);
    
    f->audiocase=AnalogLocalBroadcast;
    f->audiostream = s;
}

void SndrwFlowStop(IteFlower *f){
    IteAudioFlower *s=f->audiostream;
    if(f->audiocase!=AnalogLocalBroadcast) return;
    
    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsndread, -1);
    if(s->Fencoder) ite_filterChain_unlink(&s->fc, 0, s->Fencoder, 0);
    if(s->Fdecoder) ite_filterChain_unlink(&s->fc, 0, s->Fdecoder, 0);
	if(s->Fmix) ite_filterChain_unlink(&s->fc, 0, s->Fmix, 0);
    ite_filterChain_unlink(&s->fc, 0, s->Fsndwrite, 0);
    ite_filterChain_delete(&s->fc);
    
    audio_flower_free(s);
}
/*sound RW end*/

/**/
void StreamFlowStart(IteFlower *f,cb_sound_t cb,const char* filepath){

    //IteFChain fc1=s->fc;
    IteAudioFlower *s=(IteAudioFlower*)ite_new0(IteAudioFlower,1);
    char* args[] ={"-S","-Q=32",};

    s->Fsource   = ite_filter_new(ITE_FILTER_STREAMMGR_ID);//mp3 aac ...
    s->Fsndwrite = ite_filter_new(ITE_FILTER_SNDWRITE_ID);

    ite_filter_call_method(s->Fsource,ITE_FILTER_FILEOPEN ,NULL);
    if(cb) ite_filter_call_method(s->Fsndwrite,ITE_FILTER_SET_CB,cb);
    
    ite_filterChain_build(&s->fc, "FC 1");
    ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);
    
    ite_filterChain_link(&s->fc, -1, s->Fsource, -1);
    ite_filterChain_link(&s->fc, 0, s->Fsndwrite, 0);

    ite_filterChain_run(&s->fc);  
    
    f->audiocase=StreamPlay;
    f->audiostream = s;

}

void StreamFlowStop(IteFlower *f){

    IteAudioFlower *s=f->audiostream;

    if(f->audiocase!=StreamPlay) return;
    
    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsource, -1);
    ite_filterChain_unlink(&s->fc, 0, s->Fsndwrite, 0);
    ite_filterChain_delete(&s->fc);
    
    audio_flower_free(s);

}

void StreamFlowPause(IteFlower *f){
    IteAudioFlower *s=f->audiostream;
    int pause=1;
    if(f->audiocase!=StreamPlay) return;
    ite_filter_call_method(s->Fsource,ITE_FILTER_PAUSE ,&pause);
    ite_filter_call_method(s->Fsndwrite,ITE_FILTER_PAUSE ,&pause);
}

void StreamFlowResume(IteFlower *f){
    IteAudioFlower *s=f->audiostream;
    int pause=0;
    if(f->audiocase!=StreamPlay) return;
    ite_filter_call_method(s->Fsource,ITE_FILTER_PAUSE ,&pause);
    ite_filter_call_method(s->Fsndwrite,ITE_FILTER_PAUSE ,&pause);
   
}
/**/

void QRecFlowStart(IteFlower *f,int rate,int ch){

    //IteFChain fc1=s->fc;
    IteAudioFlower *s=(IteAudioFlower*)ite_new0(IteAudioFlower,1);
    char* args[] ={"-S","-Q=32",};
    
    s->Fsndread   = ite_filter_new(ITE_FILTER_SNDREAD_ID);//mp3 aac ...

    s->Fdestinat = ite_filter_new(ITE_FILTER_QWRITE_ID);
    
    Castor3snd_reinit_for_diff_rate(rate,16,ch);
   
    ite_filterChain_build(&s->fc, "FC 1");
    ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);
    
    ite_filterChain_link(&s->fc, -1, s->Fsndread, -1);
    ite_filterChain_link(&s->fc, 0, s->Fdestinat, 0);
	
    ite_filterChain_run(&s->fc);  
    
    f->audiostream = s;
}



void QRecFlowStop(IteFlower *f){

    IteAudioFlower *s=f->audiostream;

    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsndread, -1);
    ite_filterChain_unlink(&s->fc, 0, s->Fdestinat, 0);
    ite_filterChain_delete(&s->fc);
    
    audio_flower_free(s);

}

/*mp3 mix wav start*/
void PlayFlowStartMix(IteFlower *f,const char* filepath,PlaySoundCase mode,cb_sound_t cb){

    //IteFChain fc1=s->fc;
    IteAudioFlower *s=(IteAudioFlower*)ite_new0(IteAudioFlower,1);
    char* args[] ={"-S","-Q=32",};
    int input_rate = 0;
	int output_rate = 0;
    int output_channel= 0;
	int mic_in = 0;

    //fc: file->mix->spk
    if(audiomgrCodecType(filepath)==ITE_WAV_DECODE){
        s->Fsource   = ite_filter_new(ITE_FILTER_FILEPLAY_ID);//wav
    }else {
        s->Fsource   = ite_filter_new(ITE_FILTER_FILEPLAYMGR_ID);//mp3 aac ...
    }

   	s->Fmix      = ite_filter_new(ITE_FILTER_MICMIX_ID);
    s->Fsndwrite = ite_filter_new(ITE_FILTER_SNDWRITE_ID);

	//fcc: insert file(wav)->resample->mix->spk
	s->Fsrc = ite_filter_new(ITE_FILTER_FILEPLAY_ID);//wav
	s->Fasr = ite_filter_new(ITE_FILTER_RESAMPLE_ID);// 
	s->Ftee     = ite_filter_new(ITE_FILTER_CHADAPT_ID);
    
    ite_filter_call_method(s->Fsource,ITE_FILTER_FILEOPEN ,(void*)filepath);
    ite_filter_call_method(s->Fsource,ITE_FILTER_LOOPPLAY,&mode);
    if(cb) ite_filter_call_method(s->Fsndwrite,ITE_FILTER_SET_CB,cb);

	ite_filter_call_method(s->Fsource,ITE_FILTER_GETRATE,&output_rate);
	ite_filter_call_method(s->Fsource,ITE_FILTER_GET_NCHANNELS,&output_channel);
	
	if(s->Fasr)ite_filter_call_method(s->Fasr,ITE_FILTER_OUTPUT_RATE,&output_rate);
	if(s->Fmix){
		ite_filter_call_method(s->Fmix,ITE_FILTER_SETRATE,&output_rate);
		ite_filter_call_method(s->Fmix,ITE_FILTER_SET_NCHANNELS,&output_channel);
		ite_filter_call_method(s->Fmix,ITE_FILTER_SET_VALUE,&mic_in);
		
	}
	
    ite_filter_call_method(s->Ftee,ITE_FILTER_SET_NCHANNELS,&output_channel);
	ite_filter_call_method(s->Fsrc,ITE_FILTER_SET_BYPASS,NULL);
	
	
    
    ite_filterChain_build(&s->fc, "FC 1");
    ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);
    
    ite_filterChain_link(&s->fc, -1, s->Fsource, -1);
    if(s->Fmix) ite_filterChain_link(&s->fc, 0, s->Fmix, 0);
    ite_filterChain_link(&s->fc, 0, s->Fsndwrite, 0);

	ite_filterChain_build(&s->fcc, "FC 2");
    ite_filterChain_setConfig(&s->fcc, ARRAY_COUNT_OF(args), args);
	ite_filterChain_link(&s->fcc, -1, s->Fsrc, -1);
	if(s->Fasr)ite_filterChain_link(&s->fcc, 0, s->Fasr, 0);
    ite_filterChain_link(&s->fcc, 0, s->Ftee, 0);
	if(s->Fmix) ite_filterChain_link(&s->fcc, 0, s->Fmix, 1);
	if(s->Fmix) ite_filterChain_link(&s->fcc, 0, s->Fsndwrite, 0);

    ite_filterChain_run(&s->fc);  
    
    f->audiocase=AudioPlayFile;
    f->audiostream = s;
}

void PlayFlowStopMix(IteFlower *f){
    IteAudioFlower *s=f->audiostream;
    if(f->audiocase!=AudioPlayFile) return;

    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsource, -1);
    if(s->Fmix) ite_filterChain_unlink(&s->fc, 0, s->Fmix, 0);
    ite_filterChain_unlink(&s->fc, 0, s->Fsndwrite, 0);
    ite_filterChain_delete(&s->fc);

	//ite_filterChain_stop(&s->fcc);
    ite_filterChain_unlink(&s->fcc, -1, s->Fsrc, -1);
    if(s->Fasr) ite_filterChain_unlink(&s->fcc, 0, s->Fasr, 0);
    ite_filterChain_unlink(&s->fcc, 0, s->Ftee, 0);
    ite_filterChain_unlink(&s->fcc, 0, s->Fmix, 1);
	ite_filterChain_unlink(&s->fcc, 0, s->Fsndwrite, 0);
    ite_filterChain_delete(&s->fcc);
	

    audio_flower_free(s);
}

void PlayFlowMixSound(IteFlower *f,const char *fileinsert){
    IteAudioFlower *s=f->audiostream;
    IteFilter *F1=s->Fsrc;
	IteFilter *F2=s->Fasr;
    IteFilter *F3=s->Ftee;
	IteFilter *F4=s->Fsource;
	IteFilter *Fmix=s->Fmix;
	int inpu_rate = 0;
	int channels = 0;
	int out_channels = 0;
	static bool start = false;
	int is_busy = 0;
	
	if(F1->filterDes.id==ITE_FILTER_FILEPLAY_ID){
		while(1){
			ite_filter_call_method(F1,ITE_FILTER_GET_STATUS,&is_busy);
			if(is_busy == 0){
				//printf("insert mix data!!\n");
				break;
			}
			sleep(1);
		}
	}
	if(fileinsert == NULL){
		if(F1->filterDes.id==ITE_FILTER_FILEPLAY_ID)
			ite_filter_call_method(F1,ITE_FILTER_SET_VALUE,NULL);
	}
	else{
	    if(F1->filterDes.id==ITE_FILTER_FILEPLAY_ID){
			//ite_filter_call_method(F1,ITE_FILTER_FILEOPEN,(void*)fileinsert);//one file
			ite_filter_call_method(F1,ITE_FILTER_SET_FILEPATH,(void*)fileinsert);//files
			
			ite_filter_call_method(F1,ITE_FILTER_GETRATE,&inpu_rate);
			ite_filter_call_method(F1,ITE_FILTER_GET_NCHANNELS,&channels);
			
			if(F2->filterDes.id==ITE_FILTER_RESAMPLE_ID){
				ite_filter_call_method(F2,ITE_FILTER_INPUT_RATE,&inpu_rate);
				ite_filter_call_method(F2,ITE_FILTER_SET_NCHANNELS,&channels);
			}
	        if(F3->filterDes.id==ITE_FILTER_CHADAPT_ID){
				if(F4->filterDes.id==ITE_FILTER_FILEPLAYMGR_ID){
					ite_filter_call_method(F4,ITE_FILTER_GET_NCHANNELS,&out_channels);
				}
	            ite_filter_call_method(F3,ITE_FILTER_SET_VALUE,&channels);//set input channel
	            ite_filter_call_method(F3,ITE_FILTER_SET_NCHANNELS,&out_channels);//set output channel
	        }
	        
	    }
	}
	if(start == false){
		ite_filterChain_run(&s->fcc);
		start = true;
	}
	
}
/*mp3 mix wav end*/

/*audio file mix mic sound start*/
void PlayFlowStartMix2(IteFlower *f,const char* filepath,PlaySoundCase mode,cb_sound_t cb){

    //IteFChain fc1=s->fc;
    IteAudioFlower *s=(IteAudioFlower*)ite_new0(IteAudioFlower,1);
    char* args[] ={"-S","-Q=32",};
    int input_rate = 0;
	int output_rate = 0;
    int output_channel= 0;
	int mic_in = 1;

    //fc: file->mix->spk
    if(audiomgrCodecType(filepath)==ITE_WAV_DECODE){
        s->Fsource   = ite_filter_new(ITE_FILTER_FILEPLAY_ID);//wav
    }else {
        s->Fsource   = ite_filter_new(ITE_FILTER_FILEPLAYMGR_ID);//mp3 aac ...
    }

    s->Fmix      = ite_filter_new(ITE_FILTER_MICMIX_ID);
    s->Fsndwrite = ite_filter_new(ITE_FILTER_SNDWRITE_ID);

	//fcc: mic->mix->spk
	s->Fsrc = ite_filter_new(ITE_FILTER_SNDREAD_ID);
	s->Faec = ite_filter_new(ITE_FILTER_AEC_ID);

    //fccc: loopback->->AEC
	s->Fencoder = ite_filter_new(ITE_FILTER_LOOPBACK_ID);
	
    ite_filter_call_method(s->Fsource,ITE_FILTER_FILEOPEN ,(void*)filepath);
    ite_filter_call_method(s->Fsource,ITE_FILTER_LOOPPLAY,&mode);
    if(cb) ite_filter_call_method(s->Fsndwrite,ITE_FILTER_SET_CB,cb);

	ite_filter_call_method(s->Fsource,ITE_FILTER_GETRATE,&output_rate);
	ite_filter_call_method(s->Fsource,ITE_FILTER_GET_NCHANNELS,&output_channel);
	if(s->Fmix){
		ite_filter_call_method(s->Fmix,ITE_FILTER_SETRATE,&output_rate);
		ite_filter_call_method(s->Fmix,ITE_FILTER_SET_NCHANNELS,&output_channel);
		ite_filter_call_method(s->Fmix,ITE_FILTER_SET_VALUE,&mic_in);
	}
 
    ite_filterChain_build(&s->fc, "FC 1");
    ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);
    
    ite_filterChain_link(&s->fc, -1, s->Fsource, -1);
    if(s->Fmix) ite_filterChain_link(&s->fc, 0, s->Fmix, 0);
    ite_filterChain_link(&s->fc, 0, s->Fsndwrite, 0);

	ite_filterChain_build(&s->fcc, "FC 2");
    ite_filterChain_setConfig(&s->fcc, ARRAY_COUNT_OF(args), args);
	ite_filterChain_link(&s->fcc, -1, s->Fsrc, -1);
	ite_filterChain_link(&s->fcc, 0, s->Faec, 1);
	ite_filterChain_link(&s->fcc, 0, s->Fmix, 1);

	ite_filterChain_build(&s->fccc, "FC 3");
	ite_filterChain_setConfig(&s->fccc, ARRAY_COUNT_OF(args), args);
	ite_filterChain_link(&s->fccc, -1, s->Fencoder, -1);
	ite_filterChain_link(&s->fccc, 0, s->Faec, 0);
	
    ite_filterChain_run(&s->fc);
	ite_filterChain_run(&s->fcc);
	ite_filterChain_run(&s->fccc);
    
    f->audiocase=AudioPlayFile;
    f->audiostream = s;
}


void PlayFlowStopMix2(IteFlower *f){
    IteAudioFlower *s=f->audiostream;
    if(f->audiocase!=AudioPlayFile) return;

    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsource, -1);
    if(s->Fmix) ite_filterChain_unlink(&s->fc, 0, s->Fmix, 0);
    ite_filterChain_unlink(&s->fc, 0, s->Fsndwrite, 0);
    ite_filterChain_delete(&s->fc);

	ite_filterChain_stop(&s->fcc);
    ite_filterChain_unlink(&s->fcc, -1, s->Fsrc, -1);
	if(s->Faec) ite_filterChain_unlink(&s->fcc, 0, s->Faec, 1);
    if(s->Fmix) ite_filterChain_unlink(&s->fcc, 0, s->Fmix, 1);
 
    ite_filterChain_delete(&s->fcc);

	ite_filterChain_stop(&s->fccc);
	ite_filterChain_unlink(&s->fccc, -1, s->Fencoder, -1);
	ite_filterChain_unlink(&s->fccc, 0, s->Faec, 0);
	ite_filterChain_delete(&s->fccc);

    audio_flower_free(s);
}

