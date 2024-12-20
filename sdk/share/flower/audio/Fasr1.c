#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"

//=============================================================================
//                              Constant Definition
//=============================================================================
#define MEASUREMENTS

typedef struct ASRstate{
    IteFilter *msF;
    rbuf_ite *Buf;
    int framesize;
    cb_sound_t fn_cb;
	bool isbypass;
    
}ASRstate;

static void asr_process(IteFilter *f){
    ASRstate *s=(ASRstate*)f->data;

    IteQueueblk blk = {0};
#ifdef MEASUREMENTS
	uint32_t start_cnt = 0;
    uint32_t count=1;
    uint64_t diff=0;
#endif
    int nbytes    = s->framesize;
    float score;
    int rs;
    const char *text;
    uint16_t PcmBuf1[480] = {0};
	int err1 = 0;
	
	while(f->run){
        
		if(ite_queue_get(f->input[0].Qhandle, &blk) == 0){
			mblk_ite *om=blk.datap;
			int ret=0;
		    
            ret=ite_rbuf_put(s->Buf,om->b_rptr,om->size);
			if(om) freemsg_ite(om);

            while(ite_rbuf_get(PcmBuf1,s->Buf,nbytes*2)){
	            #ifdef MEASUREMENTS
	            start_cnt = itpGetTickCount();
	            #endif                    
	            rs = Wanson_ASR_Recog((short*)PcmBuf1, nbytes, &text, &score); 
	            #ifdef MEASUREMENTS
	            count++;
	            diff += itpGetTickDuration(start_cnt);
	            if(count % 200==0){
	                printf("ASR 200 time %d bufsize=%d\n",(int)diff,ite_rbuf_get_avail_size(s->Buf));
	                diff = 0;
	            }                
                #endif               
				if (rs == 1) {
	                asrStruct data;
	                data.rs=rs;
	                data.text=text;
	                data.score=score;
	                if(s->fn_cb) s->fn_cb(Asrevent,(void*)&data);
	                printf("ASR Result: %s\n", text);
	            }
            }
		}
        usleep(5000);
	}
	
	ite_mblk_queue_flush(f->input[0].Qhandle); 

    return NULL;
	
}

static void asr_init(IteFilter *f)
{
    ASRstate *s=(ASRstate *)ite_new(ASRstate,1);
    f->data=s;
    s->framesize = 480;
    s->Buf =ite_rbuf_init(s->framesize*12);
    s->fn_cb = NULL;
	s->isbypass = false;
    s->msF = f;
	Wanson_ASR_Init();
    printf("asr_init\n");   
}

static void asr_uninit(IteFilter *f)
{
    ASRstate *s=(ASRstate*)f->data;
	Wanson_ASR_Release();
    ite_rbuf_free(s->Buf);
    free(s);
}

static void asr_set_cb(IteFilter *f, cb_sound_t fnc)
{
    ASRstate *s=(ASRstate*)f->data;
    s->fn_cb=fnc;
	
	if(!s->isbypass){
		Castor3snd_reinit_for_diff_rate(16000,16,1);
	}
}

static void asr_set_bypass(IteFilter *f, void* arg)
{
	ASRstate *s=(ASRstate*)f->data;
	s->isbypass=true;
}


static IteMethodDes asr_methods[] = {
    {ITE_FILTER_SET_CB, asr_set_cb},
    {ITE_FILTER_SET_BYPASS, asr_set_bypass},
    {0, NULL}
};

IteFilterDes FilterAsr = {
    ITE_FILTER_ASR_ID,
    asr_init,
    asr_uninit,
    asr_process,
    asr_methods
};



