#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "openrtos/FreeRTOS.h"
#include "flower/flower.h"
#include "ite/audio.h"

void audio_flower_free(IteAudioFlower *s){
    
    if(s->Fsndwrite) ite_filter_delete(s->Fsndwrite);
    if(s->Fsndread)  ite_filter_delete(s->Fsndread);
    if(s->Fsource)   ite_filter_delete(s->Fsource);
    if(s->Fdestinat) ite_filter_delete(s->Fdestinat);
    if(s->Fudprecv)   ite_filter_delete(s->Fudprecv);
    if(s->Fudpsend) ite_filter_delete(s->Fudpsend);
	if(s->Ftcprecv)   ite_filter_delete(s->Ftcprecv);
    if(s->Ftcpsend) ite_filter_delete(s->Ftcpsend);
    if(s->Fmix)      ite_filter_delete(s->Fmix);
    if(s->Faec)      ite_filter_delete(s->Faec);
    if(s->Fdecoder)  ite_filter_delete(s->Fdecoder);
    if(s->Fencoder)  ite_filter_delete(s->Fencoder);
    if(s->Fasr)      ite_filter_delete(s->Fasr);
    //if(s->Frec_avi)  ite_filter_delete(s->Frec_avi);
    if(s->Ftee)      ite_filter_delete(s->Ftee);
    //ite_flower_deinit();
    if(s) free(s);
}

/*audio call start*/
void AudioFlowStart(IteFlower *f, int rate, AudioCodecType type, const char *rem_ip, unsigned short rem_port){

    udp_config_t udp_conf;
    IteAudioFlower *s=(IteAudioFlower*)ite_new0(IteAudioFlower,1);
    char* args[] ={"-S","-Q=32",};
       
    s->Fudprecv  = ite_filter_new(ITE_FILTER_UDP_RECV_ID);
    s->Fdecoder  = ite_filter_new(ITE_FILTER_ULAW_DEC_ID);
    s->Fsndwrite = ite_filter_new(ITE_FILTER_SNDWRITE_ID);
    
    s->Fsndread = ite_filter_new(ITE_FILTER_SNDREAD_ID);
    
	if(f->videostream->Frec_avi)
		s->Frec_avi = f->videostream->Frec_avi;
	
    s->Fencoder = ite_filter_new(ITE_FILTER_ULAW_ENC_ID);
    s->Fudpsend = ite_filter_new(ITE_FILTER_UDP_SEND_ID);
    
    if(CFG_AEC_ENABLE) {
        s->Faec     = ite_filter_new(ITE_FILTER_AEC_ID);
        s->Fsource  = ite_filter_new(ITE_FILTER_LOOPBACK_ID);
    }

    memset(&udp_conf,'\0',sizeof(udp_config_t));
    udp_conf.remote_port = rem_port;
    udp_conf.cur_socket = -1;
    udp_conf.c_type = AUDIO_INPUT;
    memset(udp_conf.group_ip,'\0',16);
    udp_conf.remote_ip = NULL;
    ite_filter_call_method(s->Fudprecv, ITE_FILTER_UDP_RECV_SET_PARA, (void*)&udp_conf);
        
    memset(&udp_conf,'\0',sizeof(udp_config_t));
	udp_conf.remote_port = rem_port;
	udp_conf.cur_socket = -1;
	udp_conf.c_type = AUDIO_OUTPUT;	
	memset(udp_conf.group_ip,'\0',16);
	udp_conf.remote_ip = rem_ip;
    ite_filter_call_method(s->Fudpsend,ITE_FILTER_UDP_SEND_SET_PARA ,(void*)&udp_conf);
    
    Castor3snd_reinit_for_diff_rate(rate,16,1);

    ite_filterChain_build(&s->fc, "FC 1");
    ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);
    
    ite_filterChain_build(&s->fcc, "FC 2");
    ite_filterChain_setConfig(&s->fcc, ARRAY_COUNT_OF(args), args);
    
    ite_filterChain_build(&s->fccc, "FC 3");
    ite_filterChain_setConfig(&s->fccc, ARRAY_COUNT_OF(args), args);
    
/*   FC 1: A->B->C     */
/*                     */
/*   FC 2: D->[AEC]->F */
/*              |      */
/*   FC 3: E--->*      */       

    ite_filterChain_link(&s->fc ,-1,s->Fudprecv, -1);
    if(s->Fdecoder) ite_filterChain_link(&s->fc , 0,s->Fdecoder ,  0);
    ite_filterChain_link(&s->fc , 0,s->Fsndwrite,  0);
    
    ite_filterChain_link(&s->fcc,-1,s->Fsndread , -1);
    if(s->Faec) {
        ite_filterChain_link(&s->fcc, 0,s->Faec     ,  1);
        ite_filterChain_link(&s->fccc,-1,s->Fsource , -1);
        ite_filterChain_link(&s->fccc, 0,s->Faec    , 0);
    }
    if(s->Fencoder) ite_filterChain_link(&s->fcc, 0,s->Fencoder ,    0);
    ite_filterChain_link(&s->fcc, 0,s->Fudpsend,  0);

	if(s->Frec_avi) ite_filterChain_A_link_B(false, &s->fcc, s->Fsndread, 1, s->Frec_avi, 1);

    //ite_filterChain_print(&s->fc );
    //ite_filterChain_print(&s->fcc);

    ite_filterChain_run(&s->fc );
    ite_filterChain_run(&s->fcc);
    ite_filterChain_run(&s->fccc);

    f->audiocase=AudioNetCom;
    f->audiostream = s;
}

void AudioFlowStop(IteFlower *f){
    IteAudioFlower *s=f->audiostream;
    if(f->audiocase!=AudioNetCom) return;
    
    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc ,-1,s->Fudprecv, -1);
    if(s->Fdecoder) ite_filterChain_unlink(&s->fc , 0,s->Fdecoder ,  0);
    ite_filterChain_unlink(&s->fc , 0,s->Fsndwrite,  0);
    ite_filterChain_delete(&s->fc); 
    
    ite_filterChain_stop(&s->fcc);
    ite_filterChain_stop(&s->fccc);
    if(s->Fsndread) ite_filterChain_unlink(&s->fcc,  -1,s->Fsndread , -1);
    if(s->Faec)     {
        ite_filterChain_unlink(&s->fcc, 0,s->Faec     ,  1);
        ite_filterChain_unlink(&s->fccc,-1,s->Fsource , -1);
        ite_filterChain_unlink(&s->fccc, 0,s->Faec    , 0);
    }
    if(s->Fencoder) ite_filterChain_unlink(&s->fcc, 0,s->Fencoder ,  0);
    if(s->Fudpsend) ite_filterChain_unlink(&s->fcc, 0,s->Fudpsend ,  0);
	if(s->Frec_avi) ite_filterChain_A_unlink_B(false, &s->fcc, s->Fsndread, 1, s->Frec_avi, 1);
    ite_filterChain_delete(&s->fcc);
    ite_filterChain_delete(&s->fccc);
    
    audio_flower_free(s);
}
/*sound AudioFlowStop end*/


/*audio tcp call start*/
void AudioTcpFlowStart(IteFlower *f, int rate, AudioCodecType type, const char *rem_ip, unsigned short rem_port){

    tcp_config_t tcp_conf;
    IteAudioFlower *s=(IteAudioFlower*)ite_new0(IteAudioFlower,1);
    char* args[] ={"-S","-Q=32",};
       
    s->Ftcprecv  = ite_filter_new(ITE_FILTER_TCP_RECV_ID);
    s->Fdecoder  = ite_filter_new(ITE_FILTER_ULAW_DEC_ID);
    s->Fsndwrite = ite_filter_new(ITE_FILTER_SNDWRITE_ID);
    
    s->Fsndread = ite_filter_new(ITE_FILTER_SNDREAD_ID);

	if(f->videostream->Frec_avi)
		s->Frec_avi = f->videostream->Frec_avi;
	
    s->Fencoder = ite_filter_new(ITE_FILTER_ULAW_ENC_ID);
    s->Ftcpsend = ite_filter_new(ITE_FILTER_TCP_SEND_ID);
    
    if(CFG_AEC_ENABLE) {
        s->Faec     = ite_filter_new(ITE_FILTER_AEC_ID);
        s->Fsource  = ite_filter_new(ITE_FILTER_LOOPBACK_ID);
    }

    memset(&tcp_conf,'\0',sizeof(tcp_config_t));
    tcp_conf.remote_port = rem_port;
    tcp_conf.cur_socket = -1;
    tcp_conf.c_type = AUDIO_INPUT;
    tcp_conf.remote_ip = rem_ip;
    ite_filter_call_method(s->Ftcprecv, ITE_FILTER_TCP_RECV_SET_PARA, (void*)&tcp_conf);
        
    memset(&tcp_conf,'\0',sizeof(tcp_config_t));
	tcp_conf.remote_port = rem_port;
	tcp_conf.cur_socket = -1;
	tcp_conf.c_type = AUDIO_OUTPUT;	
	tcp_conf.remote_ip = rem_ip;
    ite_filter_call_method(s->Ftcpsend,ITE_FILTER_TCP_SEND_SET_PARA ,(void*)&tcp_conf);
    
    Castor3snd_reinit_for_diff_rate(rate,16,1);

    ite_filterChain_build(&s->fc, "FC 1");
    ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);
    
    ite_filterChain_build(&s->fcc, "FC 2");
    ite_filterChain_setConfig(&s->fcc, ARRAY_COUNT_OF(args), args);
    
    ite_filterChain_build(&s->fccc, "FC 3");
    ite_filterChain_setConfig(&s->fccc, ARRAY_COUNT_OF(args), args);
    
/*   FC 1: A->B->C     */
/*                     */
/*   FC 2: D->[AEC]->F */
/*              |      */
/*   FC 3: E--->*      */       

    ite_filterChain_link(&s->fc ,-1,s->Ftcprecv, -1);
    if(s->Fdecoder) ite_filterChain_link(&s->fc , 0,s->Fdecoder ,  0);
    ite_filterChain_link(&s->fc , 0,s->Fsndwrite,  0);
    
    ite_filterChain_link(&s->fcc,-1,s->Fsndread , -1);
    if(s->Faec) {
        ite_filterChain_link(&s->fcc, 0,s->Faec     ,  1);
        ite_filterChain_link(&s->fccc,-1,s->Fsource , -1);
        ite_filterChain_link(&s->fccc, 0,s->Faec    , 0);
    }
    if(s->Fencoder) ite_filterChain_link(&s->fcc, 0,s->Fencoder ,    0);
    ite_filterChain_link(&s->fcc, 0,s->Ftcpsend,  0);

	if(s->Frec_avi) ite_filterChain_A_link_B(false, &s->fcc, s->Fsndread, 1, s->Frec_avi, 1);

    //ite_filterChain_print(&s->fc );
    //ite_filterChain_print(&s->fcc);

    ite_filterChain_run(&s->fc );
    ite_filterChain_run(&s->fcc);
    ite_filterChain_run(&s->fccc);

    f->audiocase=AudioNetCom;
    f->audiostream = s;
}

void AudioTcpFlowStop(IteFlower *f){
    IteAudioFlower *s=f->audiostream;
    if(f->audiocase!=AudioNetCom) return;
    
    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc ,-1,s->Ftcprecv, -1);
    if(s->Fdecoder) ite_filterChain_unlink(&s->fc , 0,s->Fdecoder ,  0);
    ite_filterChain_unlink(&s->fc , 0,s->Fsndwrite,  0);
    ite_filterChain_delete(&s->fc); 
    
    ite_filterChain_stop(&s->fcc);
    ite_filterChain_stop(&s->fccc);
    if(s->Fsndread) ite_filterChain_unlink(&s->fcc,  -1,s->Fsndread , -1);
    if(s->Faec)     {
        ite_filterChain_unlink(&s->fcc, 0,s->Faec     ,  1);
        ite_filterChain_unlink(&s->fccc,-1,s->Fsource , -1);
        ite_filterChain_unlink(&s->fccc, 0,s->Faec    , 0);
    }
    if(s->Fencoder) ite_filterChain_unlink(&s->fcc, 0,s->Fencoder ,  0);
    if(s->Ftcpsend) ite_filterChain_unlink(&s->fcc, 0,s->Ftcpsend ,  0);
	if(s->Frec_avi) ite_filterChain_A_unlink_B(false, &s->fcc, s->Fsndread, 1, s->Frec_avi, 1);
    ite_filterChain_delete(&s->fcc);
    ite_filterChain_delete(&s->fccc);
    
    audio_flower_free(s);
}
/*sound AudioTcpFlowStop end*/

/*analog audio call start*/
void AnalogFlowStart(IteFlower *f,int rate){
    IteAudioFlower *s=(IteAudioFlower*)ite_new0(IteAudioFlower,1);
    char* args[] ={"-S","-Q=32",};
    
    Castor3snd_reinit_for_diff_rate(rate,16,2);
      
    s->Fsndread  = ite_filter_new(ITE_FILTER_SNDREAD_ID);
    s->Fencoder  = ite_filter_new(ITE_FILTER_SEPARATE_ID);
    s->Fdecoder  = ite_filter_new(ITE_FILTER_MERGE_ID);
    s->Fsndwrite = ite_filter_new(ITE_FILTER_SNDWRITE_ID);
    
    //if(CFG_AEC_ENABLE) s->Faec     = ite_filter_new(ITE_FILTER_AEC_ID);
    if(s->Faec) {//**note: if enable aec ,data size(128*2) will be reshape
        int in_channels=2;
        
        s->Fsource  = ite_filter_new(ITE_FILTER_LOOPBACK_ID);
        s->Ftee     = ite_filter_new(ITE_FILTER_CHADAPT_ID); //2 channel to 1 channel
        ite_filter_call_method(s->Ftee,ITE_FILTER_SET_VALUE,&in_channels);
    }
    
    if(s->Faec){
        ite_filterChain_build(&s->fcc, "FC 2");
        ite_filterChain_setConfig(&s->fcc, ARRAY_COUNT_OF(args), args);
        
        ite_filterChain_link(&s->fcc, -1, s->Fsource, -1);
        ite_filterChain_link(&s->fcc, 0, s->Ftee, 0);
        ite_filterChain_link(&s->fcc, 0, s->Faec, 0);
        ite_filterChain_run(&s->fcc);
        
    }
    
    ite_filterChain_build(&s->fc, "FC 1");
    ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);
    
    ite_filterChain_link(&s->fc, -1, s->Fsndread, -1);
    ite_filterChain_link(&s->fc,  0, s->Fencoder, 0);
    if(s->Faec){
        ite_filterChain_A_link_B(true, &s->fc, s->Fencoder, 0, s->Faec, 1);
        ite_filterChain_A_link_B(true, &s->fc, s->Faec, 0, s->Fdecoder, 0);
    }else{
        ite_filterChain_A_link_B(true, &s->fc, s->Fencoder, 0, s->Fdecoder, 0);
    }
    ite_filterChain_A_link_B(true, &s->fc, s->Fencoder, 1, s->Fdecoder, 1);
    ite_filterChain_A_link_B(true, &s->fc, s->Fdecoder, 0, s->Fsndwrite, 0);

    ite_filterChain_run(&s->fc);
    
    f->audiocase=AnalogLocalCom;
    f->audiostream = s;
}

void AnalogFlowStop(IteFlower *f){

    IteAudioFlower *s=f->audiostream;
    if(f->audiocase!=AnalogLocalCom) return;
    
    if(s->Faec){
        ite_filterChain_stop(&s->fcc);
        ite_filterChain_unlink(&s->fcc, -1, s->Fsource, -1);
        ite_filterChain_unlink(&s->fcc, 0, s->Ftee, 0);
        ite_filterChain_unlink(&s->fcc, 0, s->Faec, 0);
        ite_filterChain_delete(&s->fcc);
    }
    
    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc, -1, s->Fsndread, -1);
    ite_filterChain_unlink(&s->fc,  0, s->Fencoder, 0);
    if(s->Faec){
        ite_filterChain_A_unlink_B(true, &s->fc, s->Fencoder, 0, s->Faec, 1);
        ite_filterChain_A_unlink_B(true, &s->fc, s->Faec, 0, s->Fdecoder, 0);
    }else{
        ite_filterChain_A_unlink_B(true, &s->fc, s->Fencoder, 0, s->Fdecoder, 0);
    }
    ite_filterChain_A_unlink_B(true, &s->fc, s->Fencoder, 1, s->Fdecoder, 1);
    ite_filterChain_A_unlink_B(true, &s->fc, s->Fdecoder, 0, s->Fsndwrite, 0);
    ite_filterChain_delete(&s->fc);
    
    audio_flower_free(s);
}
/*analog SndrwFlowStop start*/

/*audio call start*/
void AnalogNetFlowStart(IteFlower *f, int rate, AudioCodecType type, const char *rem_ip, unsigned short rem_port){
    udp_config_t udp_conf;
    IteAudioFlower *s=(IteAudioFlower*)ite_new0(IteAudioFlower,1);
    char* args[] ={"-S","-Q=32",};
       
    s->Fudprecv  = ite_filter_new(ITE_FILTER_UDP_RECV_ID);
    s->Fdecoder  = ite_filter_new(ITE_FILTER_ULAW_DEC_ID);
    s->Fsndwrite = ite_filter_new(ITE_FILTER_SNDWRITE_ID);
    
    s->Fsndread = ite_filter_new(ITE_FILTER_SNDREAD_ID);

	
    s->Fencoder = ite_filter_new(ITE_FILTER_ULAW_ENC_ID);
    s->Fudpsend = ite_filter_new(ITE_FILTER_UDP_SEND_ID);

    memset(&udp_conf,'\0',sizeof(udp_config_t));
    udp_conf.remote_port = rem_port;
    udp_conf.cur_socket = -1;
    udp_conf.c_type = AUDIO_INPUT;
    memset(udp_conf.group_ip,'\0',16);
    udp_conf.remote_ip = NULL;
    ite_filter_call_method(s->Fudprecv, ITE_FILTER_UDP_RECV_SET_PARA, (void*)&udp_conf);
        
    memset(&udp_conf,'\0',sizeof(udp_config_t));
	udp_conf.remote_port = rem_port;
	udp_conf.cur_socket = -1;
	udp_conf.c_type = AUDIO_OUTPUT;	
	memset(udp_conf.group_ip,'\0',16);
	udp_conf.remote_ip = rem_ip;
    ite_filter_call_method(s->Fudpsend,ITE_FILTER_UDP_SEND_SET_PARA ,(void*)&udp_conf);
    
    Castor3snd_reinit_for_diff_rate(rate,16,1);
    i2s_DAC_channel_switch(1,0);//change right as main channel
    i2s_DAC_channel_switch(1,0);//change right as main channel

    ite_filterChain_build(&s->fc, "FC 1");
    ite_filterChain_setConfig(&s->fc, ARRAY_COUNT_OF(args), args);
    
    ite_filterChain_build(&s->fcc, "FC 2");
    ite_filterChain_setConfig(&s->fcc, ARRAY_COUNT_OF(args), args);
    
/*   FC 1: A->*->C   */
/*            B      */
/*   FC 2: D->*->F   */

    ite_filterChain_link(&s->fc ,-1,s->Fudprecv, -1);
    if(s->Fdecoder) ite_filterChain_link(&s->fc , 0,s->Fdecoder ,  0);
    ite_filterChain_link(&s->fc , 0,s->Fsndwrite,  0);
    
    ite_filterChain_link(&s->fcc,-1,s->Fsndread , -1);
    if(s->Fencoder) ite_filterChain_link(&s->fcc, 0,s->Fencoder ,    0);
    ite_filterChain_link(&s->fcc, 0,s->Fudpsend,  0);

	//if(s->Frec_avi) ite_filterChain_A_link_B(false, &s->fcc, s->Fsndread, 1, s->Frec_avi, 1);

    ite_filterChain_run(&s->fc );
    ite_filterChain_run(&s->fcc);
    
    f->audiocase=AnalogNetCom;
    f->audiostream = s;
}

void AnalogNetFlowStop(IteFlower *f){
    IteAudioFlower *s=f->audiostream;
    if(f->audiocase!=AnalogNetCom) return;
    
    ite_filterChain_stop(&s->fc);
    ite_filterChain_unlink(&s->fc ,-1,s->Fudprecv, -1);
    if(s->Fdecoder) ite_filterChain_unlink(&s->fc , 0,s->Fdecoder ,  0);
    ite_filterChain_unlink(&s->fc , 0,s->Fsndwrite,  0);
    ite_filterChain_delete(&s->fc); 
    
    ite_filterChain_stop(&s->fcc);
    if(s->Fsndread) ite_filterChain_unlink(&s->fcc,  -1,s->Fsndread , -1);
    if(s->Fencoder) ite_filterChain_unlink(&s->fcc,   0,s->Fencoder ,  0);
    if(s->Fudpsend) ite_filterChain_unlink(&s->fcc,   0,s->Fudpsend ,  0);
	//if(s->Frec_avi) ite_filterChain_A_unlink_B(false, &s->fcc, s->Fsndread, 1, s->Frec_avi, 1);
    ite_filterChain_delete(&s->fcc);
    
    audio_flower_free(s);
}
/*sound AnalogFlowNetStop end*/

void AudioStreamCancel(IteFlower *f){
    //check audiostream be NULL;
    if(f->audiostream!=NULL){
        PlayFlowStop(f);
        RecFlowStop(f);
		QuickPlayFlowStop(f);
		if(f->audiostream->Ftcprecv || f->audiostream->Ftcpsend)
			AudioTcpFlowStop(f);
		else
        	AudioFlowStop(f);
        AnalogFlowStop(f);
        AnalogNetFlowStop(f);
        SndrwFlowStop(f);
        #ifdef CFG_BUILD_ASR
        ResmplePlayFlowStop(f);
        #endif
        StreamFlowStop(f);
        f->audiocase=AudioIdel;
        f->audiostream=NULL;
    }
}