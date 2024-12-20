#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"
#include "audio/include/g711.h"

//=============================================================================
//                              Constant Definition
//=============================================================================

//=============================================================================
//                              Private Function Declaration
//=============================================================================
typedef struct _UlawEncData{
    rbuf_ite *Buf;
    int sizeofpcm;
    uint32_t ts;
} UlawEncData;


static void ulaw_enc_init(IteFilter *f)
{
    UlawEncData *s=(UlawEncData *)ite_new(UlawEncData,1);
    s->sizeofpcm = 160; //  160/(rate*channel) ms
    s->ts =0;
    s->Buf =ite_rbuf_init(s->sizeofpcm*16);
    f->data=s;
}

static void ulaw_enc_uninit(IteFilter *f)
{
    UlawEncData *s=(UlawEncData*)f->data;  
    ite_rbuf_free(s->Buf);
    free(s);
}

static void ulaw_enc_process(IteFilter *f)
{
    UlawEncData *s=(UlawEncData*)f->data;  
    IteQueueblk blk ={0};

    int nbytes = s->sizeofpcm*2;
    unsigned char buffer[1028];
    int ret=0;
    
    while(f->run) {    
        
        //sem_wait(&f->input[0].semHandle);
        while(ite_queue_get(f->input[0].Qhandle, &blk) == 0){
            mblk_ite *om=blk.datap;
            
            ret=ite_rbuf_put(s->Buf,om->b_rptr,om->size);
            if(om) freemsg_ite(om);
            
            while(ite_rbuf_get(buffer,s->Buf,nbytes)){
                mblk_ite *o;
                int i;
                o=allocb_ite(nbytes/2);
                
                for (i=0;i<nbytes/2;i++){
                    *o->b_wptr=s16_to_ulaw(((int16_t*)buffer)[i]);
                    o->b_wptr++;
                }
                s->ts+=s->sizeofpcm;
                blk.datap = o;
                ite_queue_put(f->output[0].Qhandle, &blk);
                usleep(20000);
            }
            
        }
        //sem_post(&f->output[0].semHandle);
    }
    
    return NULL;

}

static void ulaw_enc_method(IteFilter *f, void *arg)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
}

static IteMethodDes ulaw_methods[] = {
    {ITE_FILTER_A_Method, ulaw_enc_method},
    {0, NULL}
};

IteFilterDes FilterUlawEnc = {
    ITE_FILTER_ULAW_ENC_ID,
    ulaw_enc_init,
    ulaw_enc_uninit,
    ulaw_enc_process,
    ulaw_methods
};

/* dec */
static void ulaw_dec_init(IteFilter *f){/*do nothing*/}
static void ulaw_dec_uninit(IteFilter *f){/*do nothing*/}

static void ulaw_dec_process(IteFilter *f)
{
    UlawEncData *s=(UlawEncData*)f->data;  
    IteQueueblk blk = {0};
    mblk_ite *om;
    
    while(f->run) {      
        
        //sem_wait(&f->input[0].semHandle);
        while(ite_queue_get(f->input[0].Qhandle, &blk) == 0){
            mblk_ite *o;
            om=blk.datap;
            o=allocb_ite(om->size*2);/*m->b_wptr-m->b_rptr*/
            for(;om->b_rptr<om->b_wptr;om->b_rptr++,o->b_wptr+=2){
                *((int16_t*)(o->b_wptr))=ulaw_to_s16(*om->b_rptr);
            }

            blk.datap = o;
            ite_queue_put(f->output[0].Qhandle, &blk);
            if(om) freemsg_ite(om);
            usleep(20000);
        }
        //sem_post(&f->output[0].semHandle);
    }

}

IteFilterDes FilterUlawDec = {
    ITE_FILTER_ULAW_DEC_ID,
    ulaw_dec_init,
    ulaw_dec_uninit,
    ulaw_dec_process,
    ulaw_methods
};

