#include "sbc_aec_api.h"
#include "ite/audio.h"

//#if (CFG_NET_ETHERNET && CFG_NET_WIFI)
//#define AEC_RUN_IN_ARM
//#endif

#ifdef AEC_RUN_IN_ARM
    #include "type_def.h"
    #include "aecm_core.h"
    #include "basic_op.h"
    #include "hd_aec.h"
    #include "rfft_256.h"
#endif
//#define EC_MEASUREMENTS
//#define ENABLE_DUMP_AEC_DATA

#ifdef ENABLE_DUMP_AEC_DATA
#define EC_DUMP_ITE
#include "ite/itp.h"
static char *Get_Storage_path(void)
{
    ITPDriveStatus* driveStatusTable;
    ITPDriveStatus* driveStatus = NULL;
    int i;

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    for (i = 0; i < ITP_MAX_DRIVE; i++)
    {
        driveStatus = &driveStatusTable[i];
		 if (driveStatus->disk >= ITP_DISK_MSC00 && driveStatus->disk <= ITP_DISK_MSC17)
        {
            if (driveStatus->avail )
            {
                printf("USB #%d inserted: %s\n", driveStatus->disk - ITP_DISK_MSC00, driveStatus->name[0]);
                return driveStatus->name[0];
            }
        }
    }
    return NULL;
} 
#endif

SbcAECState* AEC_ITE_INIT(int SAMPLINGRATE,int FRAMESIZE,int DELAY_MS,bool_t BYPASS){
    
    SbcAECState *s=(SbcAECState *)ms_new(SbcAECState,1);
    ms_bufferizer_init(&s->delayed_ref);
    ms_bufferizer_init(&s->echo);
    ms_bufferizer_init(&s->ref);
    ms_bufferizer_init(&s->hw_echo);
    ms_bufferizer_init(&s->hw_ref);
    ms_bufferizer_init(&s->hw_clean);
    s->samplerate=SAMPLINGRATE;    
    s->delay_ms=DELAY_MS;
    s->framesize=FRAMESIZE;
    s->bypass_mode = BYPASS;
    s->echostarted=FALSE;
    s->delayset=FALSE;
    s->echoruning=FALSE;
    ms_mutex_init(&s->mutex,NULL);
    s->hw_start=FALSE;    
    if (s->bypass_mode)
        return s; // In bypass we need not init AEC thread return
#ifdef EC_DUMP_ITE
    qinit(&s->echo_copy_q);
    qinit(&s->ref_copy_q);
    qinit(&s->clean_copy_q);
#endif
#ifdef AEC_RUN_IN_ARM
    AEC_IN_ARM_INIT(&s->AECframesize);//AECframesize = 128
    printf("ARM\n");
#else
    iteAecCommand(AEC_CMD_INIT, 0, 0, 0, 0, &s->AECframesize);//AECframesize=144
    printf("armLite version\n");
#endif
    printf("s->AECframesize=%d\n",s->AECframesize);
    sbc_aec_start_hwthread(s);
    AEC_set_delay(s,DELAY_MS,FALSE);
    
    return s;
}

void AEC_ITE_UNINIT(SbcAECState *s){
    
#ifdef EC_DUMP_ITE
    FILE *echofile;
    FILE *reffile;
    FILE *cleanfile;
    mblk_t *echo;
    mblk_t *ref;
    mblk_t *clean;
    int hw_nbytes = s->AECframesize*2;
    static int index = 0;
    char *fname;
    char USBPATH = Get_Storage_path();
    if(USBPATH == NULL){
        flushq(&s->echo_copy_q,0);
        flushq(&s->ref_copy_q,0);
        flushq(&s->clean_copy_q,0);
        printf("USB not insert \n");
        return ;
    }
    
    printf("save audio data in USB %c:/ \n",USBPATH);
    fname=ms_strdup_printf("%c:/echo%03d.raw",USBPATH,index);
    echofile=fopen(fname,"w");
    ms_free(fname);
    fname=ms_strdup_printf("%c:/ref%03d.raw",USBPATH,index);
    reffile=fopen(fname,"w");
    ms_free(fname);
    fname=ms_strdup_printf("%c:/clean%03d.raw",USBPATH,index);
    cleanfile=fopen(fname,"w");
    ms_free(fname);
    index++;
    
    while (1)
    {
        echo=ref=NULL;
        echo=getq(&s->echo_copy_q);
        ref=getq(&s->ref_copy_q);
        clean=getq(&s->clean_copy_q);
        if (echo && ref && clean)
        {
            fwrite(echo->b_rptr,hw_nbytes,1,echofile);
            freemsg(echo);            
            fwrite(ref->b_rptr,hw_nbytes,1,reffile);
            freemsg(ref);
            fwrite(clean->b_rptr,hw_nbytes,1,cleanfile);
            freemsg(clean);
        }
        else
        {
            flushq(&s->echo_copy_q,0);
            flushq(&s->ref_copy_q,0);
            flushq(&s->clean_copy_q,0);
            fclose(echofile);
            fclose(reffile);
            fclose(cleanfile);
            break;
        }
    }
#endif

    if(!s->bypass_mode)
        sbc_aec_stop_hwthread(s);

#ifdef AEC_RUN_IN_ARM 
    AEC_IN_ARM_UNINIT();
#endif    
    
    ms_bufferizer_uninit (&s->delayed_ref);
    ms_bufferizer_uninit (&s->echo);
    ms_bufferizer_uninit (&s->ref);
    ms_bufferizer_uninit (&s->hw_echo);
    ms_bufferizer_uninit (&s->hw_ref);    
    ms_bufferizer_uninit (&s->hw_clean); 
    ms_mutex_destroy(&s->mutex);
    ms_free(s);     
}

int AEC_applyTX(SbcAECState *s,mblk_t *TxIn_echo,mblk_t *TxOut_clean,int nbytes){
    mblk_t *ref,*echo;
    
    if(s->bypass_mode){
        ms_bufferizer_put(&s->echo,TxIn_echo);       
        if(ms_bufferizer_read(&s->echo,TxOut_clean->b_wptr,nbytes)){
            TxOut_clean->b_wptr += nbytes;
            return nbytes;
        }
        else
            return 0;
    }
    
    if (!s->echostarted) s->echostarted=TRUE;
    //if (!s->delayset) return 0;
    if(s->echoruning){
        
        ms_bufferizer_put(&s->echo,TxIn_echo);

        ref=allocb(nbytes,0); 
        echo=allocb(nbytes,0); 
        if (ms_bufferizer_read(&s->echo,echo->b_wptr,nbytes)>=nbytes){
            int avail = 0;
            ms_mutex_lock(&s->mutex);
            if ((avail=ms_bufferizer_get_avail(&s->delayed_ref))<nbytes+(s->delay_ref_samples*2)){
                mblk_t *refm;
                /*we don't have enough to read in a reference signal buffer, inject silence instead*/
                refm=allocb(nbytes,0);
                memset(refm->b_wptr,-10,nbytes);
                refm->b_wptr+=nbytes;
                
                ms_bufferizer_put(&s->delayed_ref,refm);
                //ms_bufferizer_put(&s->ref,dupmsg(refm));
                printf("add scilent\n");
            }        
            

            if (ms_bufferizer_read(&s->delayed_ref,ref->b_wptr,nbytes)==0){
                printf("error Should never happen (delayed_ref) \n");
                ms_fatal("Should never happen");
            }

            
            echo->b_wptr+=nbytes;
            ref->b_wptr+=nbytes;
        
            ms_bufferizer_put(&s->hw_echo,echo);//put to buffer
            ms_bufferizer_put(&s->hw_ref,ref);  //put to buffer
            ms_mutex_unlock(&s->mutex);        
        }else{
            if (echo) freemsg(echo);
            if (ref) freemsg(ref);
        }        
        
        //printf("&s->delayed_ref = %d ",ms_bufferizer_get_avail(&s->delayed_ref));
        //printf("&s->echo = %d \n",ms_bufferizer_get_avail(&s->echo));
            ms_mutex_lock(&s->mutex); 
        if(ms_bufferizer_read(&s->hw_clean,TxOut_clean->b_wptr,nbytes)){
            TxOut_clean->b_wptr += nbytes;
            ms_mutex_unlock(&s->mutex);            
            return nbytes;
        }else
            ms_mutex_unlock(&s->mutex); 
            return 0;
        
    }else{
        memcpy(TxOut_clean->b_wptr,TxIn_echo->b_rptr,nbytes);
        TxOut_clean->b_wptr += nbytes;
        if(TxIn_echo) freemsg(TxIn_echo);
        return nbytes;
    }

}

int AEC_applyRX(SbcAECState *s,mblk_t *RxIn_ref,mblk_t *RxOut_ref,int nbytes){
    //uint8_t *ref,*echo;

    if(s->bypass_mode){
        ms_bufferizer_put(&s->ref,RxIn_ref);       
        if(ms_bufferizer_read(&s->ref,RxOut_ref->b_wptr,nbytes)){
            RxOut_ref->b_wptr += nbytes;
            return nbytes;
        }
        else
            return 0;
    }    
    
    if (!s->echostarted) return 0;
    
    ms_mutex_lock(&s->mutex);
    ms_bufferizer_put(&s->delayed_ref,dupmsg(RxIn_ref));
    ms_bufferizer_put(&s->ref,RxIn_ref);
    ms_mutex_unlock(&s->mutex);
        
    
    if (ms_bufferizer_read(&s->ref,RxOut_ref->b_wptr,nbytes)){
        RxOut_ref->b_wptr+=nbytes;
        return nbytes;  
    }else{
        return 0;
    }
      
    
}
void AEC_set_delay(SbcAECState *s,int delay,bool_t setdelay_flag){
    mblk_t *m;
    int delay_samples=0;
    s->delay_ms = delay;
    delay_samples=s->delay_ms*s->samplerate/1000;
    s->delay_ref_samples = delay_samples;
    m=allocb(delay_samples*2,0);
    memset(m->b_wptr,10,delay_samples*2);
    m->b_wptr+=delay_samples*2;
    
    ms_mutex_lock(&s->mutex);
    ms_bufferizer_put(&s->delayed_ref,m);
    ms_mutex_unlock(&s->mutex);

    if(setdelay_flag)
        s->delayset = setdelay_flag;
}

void AEC_set_delay_flag(SbcAECState *s,bool_t yesno){
    ms_bufferizer_flush(&s->echo);
    s->delayset = yesno;
}

static void sbc_aec_start_hwthread(SbcAECState *e){
    //ms_mutex_init(&e->hw_mutex,NULL);
    
    if (e->hw_start == FALSE){
        pthread_attr_t attr;
        struct sched_param param;
        e->hw_start=TRUE;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 25*1024);       
        param.sched_priority = sched_get_priority_min(0) + 1;
        pthread_attr_setschedparam(&attr, &param);
        ms_thread_create(&e->hw_thread, &attr, sbc_aec_hw_engine_thread, e);
        
    }
}

static void sbc_aec_stop_hwthread(SbcAECState *e){
    e->hw_start=FALSE;
    //ms_mutex_destroy(e->hw_mutex);
    ms_thread_join(e->hw_thread,NULL);      
}

static void *sbc_aec_hw_engine_thread(void *arg) {
    
    SbcAECState *s=(SbcAECState*)arg;

    int hw_nbytes = s->AECframesize*2;
    int nbytes    = s->framesize*2;
#ifdef EC_MEASUREMENTS
	MSTimeSpec begin,end;
    uint32_t count=1;
    uint64_t diff=0;
#endif	   

    while(s->hw_start){
        mblk_t *ref,*echo;
        int err1,err2;
        
        ref=allocb(hw_nbytes, 0);
        echo=allocb(hw_nbytes, 0);
        ms_mutex_lock(&s->mutex);
        err1 = ms_bufferizer_read(&s->hw_echo,echo->b_wptr,hw_nbytes);
        err2 = ms_bufferizer_read(&s->hw_ref,ref->b_wptr,hw_nbytes);
        ms_mutex_unlock(&s->mutex);

        if (err1 && err2) {
#ifdef EC_MEASUREMENTS
            ms_get_cur_time(&begin); 
#endif
            mblk_t *oecho = allocb(hw_nbytes, 0);
            memset(oecho->b_wptr, 0, hw_nbytes);
            echo->b_wptr+=hw_nbytes;
            ref->b_wptr+=hw_nbytes;
#ifdef AEC_RUN_IN_ARM
        #if(CFG_AUDIO_SAMPLING_RATE == 16000)
            //PAES_Process_Block(&aecm,echo->b_rptr, ref->b_rptr, oecho->b_wptr); //16K
            FreqWrapping(echo->b_rptr, ref->b_rptr, oecho->b_wptr, &aec_config[0]);
        #else
            //aecm_core(echo->b_rptr, ref->b_rptr, oecho->b_wptr, &aec_config[0]);
            EchoPlayBack(&aec_config[0], ref->b_rptr);
            EchoCapture(&aec_config[0], echo->b_rptr, oecho->b_wptr, FRAME_LEN);
            //TimeStretch(echo->b_rptr, oecho->b_wptr, &sndstrh_config[0]);
        #endif          
#else
            iteAecCommand(AEC_CMD_PROCESS,(unsigned int) echo->b_rptr,(unsigned int) ref->b_rptr, (unsigned int) oecho->b_wptr,hw_nbytes, 0);            
#endif
            oecho->b_wptr += hw_nbytes;
            
            ms_mutex_lock(&s->mutex);
            ms_bufferizer_put(&s->hw_clean,oecho); 
            ms_mutex_unlock(&s->mutex);            
#ifdef EC_DUMP_ITE
            ms_mutex_lock(&s->mutex);
            putq(&s->echo_copy_q, dupmsg(echo));
            putq(&s->ref_copy_q, dupmsg(ref));
            putq(&s->clean_copy_q, dupmsg(oecho));
            ms_mutex_unlock(&s->mutex); 
#endif
#ifdef EC_MEASUREMENTS
            ms_get_cur_time(&end);
            count++;
            diff += (end.tv_sec-begin.tv_sec)*1000 + (end.tv_nsec-begin.tv_nsec)/1000000;
            if(count % 100==0){
                printf("AEC 100 time %d\n",(int)diff);
                diff = 0;
            }
#endif            
        }else{
           usleep(20000);
        }
        if (echo) freemsg(echo);
        if (ref) freemsg(ref);
        
        if(s->hw_start==FALSE){
            break;
        }
    }

    return NULL;
}

#ifdef AEC_RUN_IN_ARM 
static void AEC_IN_ARM_INIT(int *AECframesize){
    AEC_Init(&aec_config[0]);
    NR_Create(&anr_config[0], 1);
    AEC_Create(&aec_config[0]);
    TimeStretch_Create(&sndstrh_config[0]);
    #if(CFG_AUDIO_SAMPLING_RATE == 16000)
    *AECframesize = 256;
    #else
    *AECframesize = 128;
    #endif    
}

static void AEC_IN_ARM_UNINIT(){
    AEC_Destroy(&aec_config[0]);
    TimeStretch_Destroy(&sndstrh_config[0]);    
}
#endif