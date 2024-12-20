#include <stdio.h>
#include "flower/flower.h"
#include "flower/ite_queue.h"
#include "include/fileheader.h"
#include "include/audioqueue.h"

//#define THREAD_RECV
//=============================================================================
//                              struct Definition
//=============================================================================
typedef struct _Mixdata{
	rbuf_ite *Buf;
    unsigned int end;
    float mixlevel;
    float baselevel;
    bool fadingIN;
    bool fadingOUT;
	bool flag;
	mblkq insert_q;
	int insert_len;
	int mix_len;
	int mix_blk_count;
	bool run;
	pthread_t recv_thread;

}Mixdata;
//=============================================================================
//                              Function Declaration
//=============================================================================
static inline int16_t saturate(int val) {
	return (val>32767) ? 32767 : ( (val<-32767) ? -32767 : val);
}



static void *rec_mix_data_process(void *arg)
{

	IteFilter *f = (IteFilter*)arg;
	Mixdata *d = (Mixdata*)f->data;
	IteQueueblk blk1 ={0};
	while(d->run){

		if(d->flag == true)
		{
			usleep(50000);//50 ms
			continue;
		}
		if(f->input[1].Qhandle == NULL){
			usleep(10000);//10 ms
			continue;
		}
		
			
		if(ite_queue_get(f->input[1].Qhandle, &blk1) == 0){
			putmblkq(&d->insert_q, blk1.datap);
			d->insert_len = d->insert_len + blk1.datap->size;
			d->mix_blk_count++;
			
			if(blk1.private2 == Eofmixsound && d->flag == false){
				int i = 0;
				mblk_ite *insert_m;
				d->mix_len = d->insert_len;
				d->Buf = ite_rbuf_init(d->mix_len + blk1.datap->size);
				for(i = 0; i < d->mix_blk_count; i++){
					insert_m = getmblkq(&d->insert_q);
					ite_rbuf_put(d->Buf,insert_m->b_rptr,insert_m->size);
					if(insert_m) freemsg_ite(insert_m);
				}
				
				d->flag = true;
				d->fadingIN  =true;
				d->fadingOUT =false;
				d->baselevel= 1.0;
				d->insert_len = 0;
				d->mix_blk_count = 0;
			}
		}
		usleep(20000);
	}
	
return NULL;
}


static void start_recv_process(IteFilter *f)
{
	Mixdata *d=(Mixdata*)f->data;
	
 	if(pthread_create(&d->recv_thread, NULL, rec_mix_data_process, (void *)f) != 0) {
    	printf("Create recv thread failed!!\n");
    }
	d->run = true;
}

static void stop_recv_process(Mixdata *d){

    d->run = false;
    pthread_join(d->recv_thread,NULL);      
}

static void mix_process(IteFilter *f)
{
	Mixdata *d=(Mixdata*)f->data;

	IteQueueblk blk1 ={0};
	IteQueueblk blk ={0};
	mblk_ite *m;
	while(f->run){
		if(d->run == false)
			d->run = true;
		memset(&blk1, 0, sizeof(IteQueueblk));
		if(ite_queue_get_size(f->output[0].Qhandle)>30){
			usleep(200000);
			continue;
        }

		if(d->flag){
            if(ite_queue_get(f->input[0].Qhandle, &blk) == 0){
                mblk_ite *o;
                m=blk.datap;

                o=allocb_ite(m->size);
                if(d->fadingIN){
                    for(;m->b_rptr<m->b_wptr;m->b_rptr+=2,o->b_wptr+=2){
                        *((int16_t*)(o->b_wptr))=(d->baselevel)*((int)*(int16_t*)m->b_rptr);
                    }
                    d->baselevel-=0.05;
                    if(d->baselevel<=1-d->mixlevel) {
                        d->baselevel=1-d->mixlevel;
                        d->fadingIN=false;
                    }
                }else if(!d->fadingIN && !d->fadingOUT){
                    for(;m->b_rptr<m->b_wptr;m->b_rptr+=2,o->b_wptr+=2,d->end+=2){
                        if(d->end < d->mix_len){//mixsound
                        *((int16_t*)(o->b_wptr))=saturate(*((int16_t*)(d->Buf->b_datap+d->end)))*(d->mixlevel)+(d->baselevel)*((int)*(int16_t*)m->b_rptr);
                        }else{//mixsound over
                                *((int16_t*)(o->b_wptr))=(d->baselevel)*((int)*(int16_t*)m->b_rptr);
                        }
                    }
                }else{
                    for(;m->b_rptr<m->b_wptr;m->b_rptr+=2,o->b_wptr+=2){
                        *((int16_t*)(o->b_wptr))=(d->baselevel)*((int)*(int16_t*)m->b_rptr);
                    }
                    d->baselevel+=0.05;
                    if(d->baselevel>=1) {
                        d->baselevel=1;
                        d->flag=false;
                    }                    
                }
            
                if(d->end >= d->mix_len){
                    d->end=0;
                    d->fadingOUT=true;
                    blk.private1=Eofmixsound;
                }
                free(m);
                blk.datap = o;
                ite_queue_put(f->output[0].Qhandle, &blk);
            }
        
        }else{
            if(ite_queue_get(f->input[0].Qhandle, &blk) == 0){
            
                ite_queue_put(f->output[0].Qhandle, &blk);
            }
        }
		
		usleep(20000);
	}

	stop_recv_process(d);
	flushmblkq(&d->insert_q);
	
	ite_mblk_queue_flush(f->input[0].Qhandle);
	ite_mblk_queue_flush(f->input[1].Qhandle);
	ite_mblk_queue_flush(f->output[0].Qhandle);
	
	return NULL;
}

static void mix_init(IteFilter *f)
{
     Mixdata *d=(Mixdata*)ite_new(Mixdata,1);
    d->fadingIN =false;
    d->fadingOUT =false;
    d->end = 0;
	d->Buf = NULL;
    d->mixlevel = 1.0;
    d->baselevel= 1.0;
	d->flag = false;

	mblkqinit(&d->insert_q);
	d->mix_len = 0;
	d->mix_blk_count = 0;
	d->run = false;
    f->data=d;
	
	start_recv_process(f);
}

static void mix_uninit(IteFilter *f)
{
    Mixdata *d=(Mixdata*)f->data;
    d->end = 0;
	if(d->Buf){
		ite_rbuf_free(d->Buf);
		d->Buf = NULL;
	}
    free(d);
}


static int mix_get_status(IteFilter *f, void *arg){
    Mixdata *d = (Mixdata*)f->data;
    *(bool*)arg = d->flag;
    return 0;
}




static IteMethodDes mix_methods[] = {
	{ITE_FILTER_GET_STATUS, mix_get_status},
	{0, NULL},

};

IteFilterDes FilterMix = {
    ITE_FILTER_MIX_ID,
    mix_init,
    mix_uninit,
    mix_process,
    mix_methods
};



