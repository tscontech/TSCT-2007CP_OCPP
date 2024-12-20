#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"
#include "audio/include/adpcm.h"

//=============================================================================
//                              Constant Definition
//=============================================================================

//=============================================================================
//                              Private Function Declaration
//=============================================================================
typedef struct _AdpcmEncData{
    rbuf_ite *Buf;
	int size_of_pcm;
    int encInx;
    int enclastalue;
} AdpcmEncData;

typedef struct _AdpcmDecData{
	rbuf_ite *Buf;
	int size_of_pcm;
    int decInx;
    int declastalue;
} AdpcmDecData;



static void adpcm_enc_init(IteFilter *f)
{
    AdpcmEncData *s=(AdpcmEncData *)ite_new(AdpcmEncData,1);
    s->size_of_pcm = 640;
    s->Buf =ite_rbuf_init(s->size_of_pcm*16);
	s->encInx = 0;
	s->enclastalue = 0;
	adpcm_encode_init(&s->encInx,&s->enclastalue);
	
    f->data=s;
}

static void adpcm_enc_uninit(IteFilter *f)
{
    AdpcmEncData *s=(AdpcmEncData*)f->data;  
    ite_rbuf_free(s->Buf);
    free(s);
}

static void adpcm_enc_process(IteFilter *f)
{
    AdpcmEncData *s=(AdpcmEncData*)f->data;  
    IteQueueblk blk ={0};

    int nbytes = s->size_of_pcm;//640
    unsigned char buffer[1280];
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
                o=allocb_ite(nbytes/4);

				int len;
				len = adpcm_encoder(buffer,o->b_wptr,nbytes,&s->encInx,&s->enclastalue);
                o->b_wptr+=len;
                blk.datap = o;
                ite_queue_put(f->output[0].Qhandle, &blk);
                usleep(20000);
            }
            
        }
        //sem_post(&f->output[0].semHandle);
    }
    
    return NULL;

}

static void adpcm_enc_method(IteFilter *f, void *arg)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
}

static IteMethodDes adpcm_methods[] = {
    {0, NULL}
};

IteFilterDes FilterAdpcmEnc = {
    ITE_FILTER_ADPCM_ENC_ID,
    adpcm_enc_init,
    adpcm_enc_uninit,
    adpcm_enc_process,
    adpcm_methods
};

/* dec */
static void adpcm_dec_init(IteFilter *f)
{
	AdpcmDecData *s=(AdpcmDecData *)ite_new(AdpcmDecData,1);
	s->size_of_pcm = 160;
	s->Buf =ite_rbuf_init(s->size_of_pcm*16);
	s->decInx = 0;
	s->declastalue = 0;
	adpcm_decode_init(&s->decInx,&s->declastalue);
	
	f->data=s;
}

static void adpcm_dec_uninit(IteFilter *f)
{
    AdpcmDecData *s=(AdpcmDecData*)f->data;  
    ite_rbuf_free(s->Buf);
    free(s);
}

static void adpcm_dec_process(IteFilter *f)
{
    AdpcmDecData *s=(AdpcmDecData*)f->data;  
    IteQueueblk blk={0};
    mblk_ite *om;
    uint8_t buffer[320];
    int nbytes = s->size_of_pcm;//160
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
                o=allocb_ite(nbytes*4);

				int len;
				len = adpcm_decoder(buffer,o->b_wptr,nbytes,&s->decInx,&s->declastalue);
                o->b_wptr+=len;
                blk.datap = o;
                ite_queue_put(f->output[0].Qhandle, &blk);
                usleep(20000);
            }
            
        }
        //sem_post(&f->output[0].semHandle);
    }
    
    return NULL;

}

IteFilterDes FilterAdpcmDec = {
    ITE_FILTER_ADPCM_DEC_ID,
    adpcm_dec_init,
    adpcm_dec_uninit,
    adpcm_dec_process,
    adpcm_methods
};

