#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"

typedef struct _ChAdaptData{
	int inchannels;
	int outchannels;
} ChAdaptData;

static void chadapt_init(IteFilter *f){
    ChAdaptData *s=(ChAdaptData *)ite_new(ChAdaptData,1);
    s->inchannels=1;
    s->outchannels=1;
    f->data=s;
}
static void chadapt_uninit(IteFilter *f){
    ChAdaptData *s=(ChAdaptData*)f->data;  
    free(s);
}

static void chadapt_process(IteFilter *f)
{
    ChAdaptData *s=(ChAdaptData*)f->data;  
    IteQueueblk blk = {0};
    int length;
    
    while(f->run) {
        IteAudioQueueController(f,0,30,5);
        
        if(ite_queue_get(f->input[0].Qhandle, &blk) == 0){
            mblk_ite *im=blk.datap;
            if (s->inchannels==s->outchannels){
                ite_queue_put(f->output[0].Qhandle, &blk);
            }else if (s->inchannels==2){
                length=im->size/2;
                mblk_ite *om=allocb_ite(length);

                for (;im->b_rptr<im->b_wptr;im->b_rptr+=4,om->b_wptr+=2){
                    *(int16_t*)om->b_wptr=*(int16_t*)im->b_rptr;
                }
                blk.datap = om;
                ite_queue_put(f->output[0].Qhandle, &blk);
                freemsg_ite(im);

            }else if (s->outchannels==2){
                length=im->size*2;
                mblk_ite *om=allocb_ite(length);;
                for (;im->b_rptr<im->b_wptr;im->b_rptr+=2,om->b_wptr+=4){
                    ((int16_t*)om->b_wptr)[0]=*(int16_t*)im->b_rptr;
                    ((int16_t*)om->b_wptr)[1]=*(int16_t*)im->b_rptr;
                }
                blk.datap = om;
                ite_queue_put(f->output[0].Qhandle, &blk);
                freemsg_ite(im);                
            }
        }
		usleep(10000);
    }
    
    ite_mblk_queue_flush(f->input[0].Qhandle);  
    ite_mblk_queue_flush(f->output[0].Qhandle);  

}

static void chadapt_set_in_nchannels(IteFilter *f, void *arg)
{
    ChAdaptData *s=(ChAdaptData*)f->data;
    s->inchannels=*(int*)arg;
}
static void chadapt_set_out_nchannels(IteFilter *f, void *arg)
{
    ChAdaptData *s=(ChAdaptData*)f->data;
    s->outchannels=*(int*)arg;
}
static void chadapt_get_in_channels(IteFilter *f, void *arg)
{
    ChAdaptData *s=(ChAdaptData*)f->data;
    *(int*)arg = s->inchannels;    
}
static void chadapt_get_out_channels(IteFilter *f, void *arg)
{
    ChAdaptData *s=(ChAdaptData*)f->data;
    *(int*)arg = s->outchannels;
}

static IteMethodDes chadapt_methods[] = {
    {	ITE_FILTER_SET_NCHANNELS, chadapt_set_out_nchannels	},
    {   ITE_FILTER_GET_NCHANNELS, chadapt_get_out_channels    },
    {	ITE_FILTER_SET_VALUE    , chadapt_set_in_nchannels	},
    {   ITE_FILTER_GET_VALUE    , chadapt_get_in_channels   },
    {0, NULL}
};

IteFilterDes FilterChadapt = {
    ITE_FILTER_CHADAPT_ID,
    chadapt_init,
    chadapt_uninit,
    chadapt_process,
    chadapt_methods
};

