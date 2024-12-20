#include <stdio.h>
#include "flower/flower.h"

typedef struct _separateData{
    void *datap;
} separateData;

static void separate_init(IteFilter *f){
    separateData *s=(separateData *)ite_new(separateData,1);
    f->data=s;
}
static void separate_uninit(IteFilter *f){
    separateData *s=(separateData*)f->data;
    free(s);
}

static void separate_process(IteFilter *f)
{
    separateData *s=(separateData*)f->data;  
    IteQueueblk blkR = {0};
    IteQueueblk blkL = {0};
    IteQueueblk blk  = {0};
    
    while(f->run) {      

        if(ite_queue_get(f->input[0].Qhandle, &blk) == 0){
            mblk_ite *im=blk.datap;
            mblk_ite *oL,*oR;
            int nbyte=im->size;
            oL=allocb_ite(nbyte/2);
            oR=allocb_ite(nbyte/2);
            for(;im->b_rptr<im->b_wptr;oL->b_wptr+=2,oR->b_wptr+=2,im->b_rptr+=4){
                *((int16_t*)(oL->b_wptr  ))=(int)*(int16_t*)(im->b_rptr  );
                *((int16_t*)(oR->b_wptr  ))=(int)*(int16_t*)(im->b_rptr+2);             
            } 

            blkL.datap = oL;
            blkR.datap = oR;
            ite_queue_put(f->output[0].Qhandle, &blkL);
            ite_queue_put(f->output[1].Qhandle, &blkR);

            if(im) freemsg_ite(im);
        }
    }
    ite_mblk_queue_flush(f->input[0].Qhandle);
    ite_mblk_queue_flush(f->output[0].Qhandle);
    ite_mblk_queue_flush(f->output[1].Qhandle);
}

static void separate_set_method(IteFilter *f, void *arg)
{

}

static IteMethodDes separate_methods[] = {
    {ITE_FILTER_A_Method, separate_set_method},
    {0, NULL}
};

IteFilterDes FilterCSeparate = {
    ITE_FILTER_SEPARATE_ID,
    separate_init,
    separate_uninit,
    separate_process,
    separate_methods
};

