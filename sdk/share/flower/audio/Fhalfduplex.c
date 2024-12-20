#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"
#include "include/volumeapi.h"


typedef struct HDTstate{
    //IteFilter *msF;
    mblkq refQ;
    mblkq ecoQ;
    Volume *vref;
    Volume *veco;
    //int framesize;
	//int samplerate;
    bool bypass_mode;
}HDTstate;

static inline int16_t saturate(int val) {
	return (val>32767) ? 32767 : ( (val<-32767) ? -32767 : val);
}

static void hdt_init(IteFilter *f){
    HDTstate *s=(HDTstate *)ite_new(HDTstate,1);
	f->data=s;
    mblkQShapeInit(&s->refQ,256*6);
    mblkQShapeInit(&s->ecoQ,256*6);
    s->vref = VOICE_INIT();
    s->veco = VOICE_INIT();
    s->bypass_mode=false;
    //s->msF = f;

}



static void hdt_uninit(IteFilter *f)
{     HDTstate *s=(HDTstate*)f->data;
    mblkQShapeUninit(&s->refQ);
    mblkQShapeUninit(&s->ecoQ);
    VOICE_UNINIT(s->vref);
    VOICE_UNINIT(s->veco);
    free(s);
}

/*  input[0]= reference signal from far end (sent to soundcard)
 *  input[1]= near speech & echo signal    (read from soundcard)
 *  output[0]=  is a copy of inputs[0] to be sent to soundcard
 *  output[1]=  near end speech, echo removed - towards far end
*/
static void hdt_process(IteFilter *f){
    HDTstate *s=(HDTstate*)f->data;
    IteQueueblk blkref={0};
    IteQueueblk blkeco={0};

	bool linein=true;
	while(f->run){
 
        if (s->bypass_mode) {
            if(ite_queue_get(f->input[0].Qhandle, &blkref) == 0){
                ite_queue_put(f->output[0].Qhandle, &blkref);
            }
            if(ite_queue_get(f->input[1].Qhandle, &blkeco) == 0){
            ite_queue_put(f->output[1].Qhandle, &blkeco);
            }
            usleep(20000);
            continue;
        }else{
            int input0,input1;
            /*while(ite_queue_get(f->input[0].Qhandle, &blkref) == 0){
                mblkQShapePut(&s->refQ,blkref.datap,256);
            }
            while(ite_queue_get(f->input[1].Qhandle, &blkeco) == 0){
                mblkQShapePut(&s->ecoQ,blkeco.datap,256);
            }*/
            ite_queue_put_to_mblkQShape(&blkref,f,0,&s->refQ,256);
            ite_queue_put_to_mblkQShape(&blkeco,f,1,&s->ecoQ,256);

            input0=getmblkqavail(&s->refQ);
            input1=getmblkqavail(&s->ecoQ);
            
            while(input0>0&&input1>0){
                mblk_ite *iref,*ieco;
                
                iref=getmblkq(&s->refQ);
                ieco=getmblkq(&s->ecoQ);
                
                if(VOICE_vaddetect(s->vref,iref)){
                    //switch to line out;
                    if(linein) linein=false;
                    memset(ieco->b_rptr,0,ieco->size);
                }else{
                    //switch to line in;
                    if(!linein) linein=true;
                    if(VOICE_vaddetect(s->veco,ieco)){
                        s->vref->vad_dur=0;
                    }
                    memset(iref->b_rptr,0,iref->size);
                }
                ite_queue_put(f->output[0].Qhandle, &blkref);
                ite_queue_put(f->output[1].Qhandle, &blkeco);
                input0--;
                input1--;
                usleep(10000);
            }
            usleep(10000);
        }
	}
    
    ite_mblk_queue_flush(f->input[0].Qhandle);  
    ite_mblk_queue_flush(f->input[1].Qhandle);  
    ite_mblk_queue_flush(f->output[0].Qhandle);  
    ite_mblk_queue_flush(f->output[1].Qhandle);  

	return NULL;
}

static void hdt_set_bypass(IteFilter *f, void *arg)
{
    HDTstate *s=(HDTstate*)f->data;
    s->bypass_mode=*((int*)arg);
}

static IteMethodDes hdt_methods[] = {
    {ITE_FILTER_SET_VALUE, hdt_set_bypass},
    {0, NULL}
};


IteFilterDes FilterHDT = {
    ITE_FILTER_HDT_ID,
    hdt_init,
    hdt_uninit,
    hdt_process,
    hdt_methods
};

