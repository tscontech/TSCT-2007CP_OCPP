#include <stdio.h>
#include "flower/flower.h"
#include "flower/ite_queue.h"
#include "include/fileheader.h"
#include "include/audioqueue.h"
#include "ite/itp.h"
#include "i2s/i2s.h"
extern STRC_I2S_SPEC spec_da;
extern STRC_I2S_SPEC spec_ad;
extern int gbytes;
#define SHAPESIZE 2000
//=============================================================================
//                              struct Definition
//=============================================================================
typedef struct _Mixdata{
	int framesize;
	mblkq dataQ0;
	mblkq dataQ1;
    int bypass;
	
}Mixdata;
//=============================================================================
//                              Function Declaration
//=============================================================================
static inline int16_t saturate(int val) {
	return (val>32767) ? 32767 : ( (val<-32767) ? -32767 : val);
}

static void _check_eof_info(IteQueueblk *blk,mblkq *tmpQ){
    if(blk->private1==Eofsound){
        mblk_ite *om=allocb_ite(0);//eof info
        putmblkq(tmpQ, om);
        blk->private1=Normal;
    }
}

static void mix_process(IteFilter *f)
{
	Mixdata *d=(Mixdata*)f->data;
    int bytes=20*(2*spec_ad.sample_rate*spec_ad.channels)/1000;//20ms data byte;    
    //bytes*=2;
	IteQueueblk blk ={0};
	IteQueueblk blk0 ={0};
	IteQueueblk blk1 ={0};
 
	while(f->run){
        int input0,input1;
        if(IteAudioQueueController(f,0,30,5)==-1) continue;
        
        if(d->bypass){
            if(getmblkqavail(&d->dataQ0)>0){
                ite_queue_put_from_mblkQ(&blk0,f,0,&d->dataQ0);
            }else{
                if(ite_queue_get(f->input[0].Qhandle, &blk0) == 0){
                    ite_queue_put(f->output[0].Qhandle, &blk0);
                }                
            }    
         
        }else{
            
            if(getmblkqavail(&d->dataQ0)<64) ite_queue_put_to_mblkQShape(&blk0,f,0,&d->dataQ0,bytes);
            if(getmblkqavail(&d->dataQ1)<64) ite_queue_put_to_mblkQShape(&blk1,f,1,&d->dataQ1,bytes);
        
            _check_eof_info(&blk0,&d->dataQ0);//check eof info
            _check_eof_info(&blk1,&d->dataQ1);//check eof info
        
            while((getmblkqavail(&d->dataQ0)>0) && (getmblkqavail(&d->dataQ1)>0)){
                mblk_ite *m0,*m1,*o;            
                m0=getmblkq(&d->dataQ0);
                m1=getmblkq(&d->dataQ1);
                o=allocb_ite(bytes);
                if(m0->size==0 || m1->size==0){//eof mix
                    memcpy(o->b_wptr,m0->b_wptr,bytes);
                    o->b_wptr+=bytes;
                    
                    blk.private1=Eofmixsound;
                    blk.datap = o;
                    ite_queue_put(f->output[0].Qhandle, &blk);
                    
                    if(m0) freemsg_ite(m0);
                    if(m1) freemsg_ite(m1);
                    blk.private1=Normal;
                    d->bypass=true;
                    
                    break;
                }
            
                for(;m0->b_rptr<m0->b_wptr;m0->b_rptr+=2,m1->b_rptr+=2, o->b_wptr+=2)
                    *((int16_t*)(o->b_wptr))=(0.1)*((int)*(int16_t*)m0->b_rptr)+(0.9)*((int)*(int16_t*)m1->b_rptr);
                if(m0) freemsg_ite(m0);
                if(m1) freemsg_ite(m1);
                blk.datap = o;
                ite_queue_put(f->output[0].Qhandle, &blk);            
            }      
        }
        usleep(10000);
	}
	ite_mblk_queue_flush(f->input[0].Qhandle);
    ite_mblk_queue_flush(f->input[1].Qhandle);
	ite_mblk_queue_flush(f->output[0].Qhandle);
	
	return NULL;
}


static void mix_init(IteFilter *f)
{
    Mixdata *d=(Mixdata*)ite_new(Mixdata,1);

	d->framesize = 80;
    d->bypass=true;
	mblkQShapeInit(&d->dataQ0,d->framesize * 360);
	mblkQShapeInit(&d->dataQ1,d->framesize * 360);

	f->data=d;
	
}

static void mix_uninit(IteFilter *f)
{
    Mixdata *d=(Mixdata*)f->data;
    d->bypass=true;
	mblkQShapeUninit(&d->dataQ0);
	mblkQShapeUninit(&d->dataQ1);
    free(d);
}

static void mix_set_bypass(IteFilter *f, void *arg)
{
	Mixdata *d=(Mixdata*)f->data;
    d->bypass=*((bool*)arg);
}

static IteMethodDes mix_methods[] = {
    {ITE_FILTER_SET_BYPASS,mix_set_bypass},
	{0, NULL},

};

IteFilterDes FilterMix = {
    ITE_FILTER_MIX_ID,
    mix_init,
    mix_uninit,
    mix_process,
    mix_methods
};



