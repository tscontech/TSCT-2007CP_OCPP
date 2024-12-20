#include <stdio.h>
#include "flower/flower.h"

//#define DEF_SWITCH

#ifdef DEF_SWITCH
#include "include/volumeapi.h"
#endif 

typedef struct _MergeData{
    void *datap;
    bool cswap;
    #ifdef DEF_SWITCH
    Volume *vL;
    Volume *vR;
    #endif 
} MergeData;

static void merge_init(IteFilter *f){
    MergeData *s=(MergeData *)ite_new(MergeData,1);
    s->cswap=true;
    #ifdef DEF_SWITCH
    s->vL = VOICE_INIT();
    s->vR = VOICE_INIT();
    #endif
    f->data=s;
}
static void merge_uninit(IteFilter *f){
    MergeData *s=(MergeData*)f->data;  
    #ifdef DEF_SWITCH
    VOICE_UNINIT(s->vL);
    VOICE_UNINIT(s->vR);
    #endif
    free(s); 
}

static void merge_process(IteFilter *f)
{
    MergeData *s=(MergeData*)f->data;  
    IteQueueblk blkR = {0};
    IteQueueblk blkL = {0};
    IteQueueblk blk  = {0};
    bool linein=true;
    while(f->run) {      

        if(ite_queue_get(f->input[0].Qhandle, &blkL) == 0 && 
           ite_queue_get(f->input[1].Qhandle, &blkR) == 0 ){
            mblk_ite *iL=blkL.datap;
            mblk_ite *iR=blkR.datap;
            mblk_ite *o;
            int nbyte=iL->size;
            if(iL->size!=iR->size) printf("error size iL:%d iR:%d\n",iL->size,iR->size);
            o=allocb_ite(nbyte*2);
            #ifdef DEF_SWITCH
            if(VOICE_vaddetect(s->vL,iL)){
                //switch to line out;
                if(linein) linein=false;
                memset(iR->b_rptr,0,iR->size);
            }else{
                //switch to line in;
                if(!linein) linein=true;
                if(VOICE_vaddetect(s->vR,iR)){
                    s->vL->vad_dur=0;
                }
                
                memset(iL->b_rptr,0,iL->size);
            }
            printf("vL=%f vR=%f [%d]\n",s->vL->energy,s->vR->energy,linein);
            #endif
            
            for(;iL->b_rptr<iL->b_wptr;o->b_wptr+=4,iL->b_rptr+=2,iR->b_rptr+=2){
                if(s->cswap){
                    *((int16_t*)(o->b_wptr+2))=(int)*(int16_t*)iL->b_rptr;
                    *((int16_t*)(o->b_wptr  ))=(int)*(int16_t*)iR->b_rptr;
                }else{
                    *((int16_t*)(o->b_wptr  ))=(int)*(int16_t*)iL->b_rptr;
                    *((int16_t*)(o->b_wptr+2))=(int)*(int16_t*)iR->b_rptr;      
                }
            }
            

            if(iL) freemsg_ite(iL);
            if(iR) freemsg_ite(iR);
            
            blk.datap = o;
            ite_queue_put(f->output[0].Qhandle, &blk);
        }
    }
    ite_mblk_queue_flush(f->input[0].Qhandle);
    ite_mblk_queue_flush(f->input[1].Qhandle);
    ite_mblk_queue_flush(f->output[0].Qhandle);

}

static void merge_set_method(IteFilter *f, void *arg)
{

}

static IteMethodDes merge_methods[] = {
    {ITE_FILTER_A_Method, merge_set_method},
    {0, NULL}
};

IteFilterDes FilterCMerge = {
    ITE_FILTER_MERGE_ID,
    merge_init,
    merge_uninit,
    merge_process,
    merge_methods
};

