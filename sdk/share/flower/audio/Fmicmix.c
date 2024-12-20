#include <stdio.h>
#include "flower/flower.h"
#include "flower/ite_queue.h"
#include "include/fileheader.h"
#include "include/audioqueue.h"

//#define DUMP_OUTPUT
//=============================================================================
//                              struct Definition
//=============================================================================
typedef struct _Mixdata{
	int framesize;
	int sample_rate;
	int chn;
	int mic_in;
	mblkq dataQ0;
	mblkq dataQ1;
	
}Mixdata;
//=============================================================================
//                              Function Declaration
//=============================================================================
static inline int16_t saturate(int val) {
	return (val>32767) ? 32767 : ( (val<-32767) ? -32767 : val);
}


static void mix_process(IteFilter *f)
{
	Mixdata *d=(Mixdata*)f->data;

	IteQueueblk blk ={0};
	IteQueueblk blk0 ={0};
	IteQueueblk blk1 ={0};
	mblk_ite *m0;
	mblk_ite *m1;
	mblk_ite *o;
	int input0 = 0;
	int input1 = 0;
	int mix_count = 0;
	int eof_flag = 0;//0:one put, 1: mixing, 2:mixend
	int i = 0;
	int shape_size = 10* 2* d->sample_rate * d->chn / 1000; //10ms data
	while(f->run){

		
		if(getmblkqavail(&d->dataQ0) < 64){
			if (ite_queue_get(f->input[0].Qhandle, &blk0) == 0)
	        {
	            mblk_ite *om = blk0.datap;
				//printf("in0 size %d\n", om->size);
	            mblkQShapePut(&d->dataQ0,om,shape_size);
	        }
		}

		if(getmblkqavail(&d->dataQ1)<64){
			if (ite_queue_get(f->input[1].Qhandle, &blk1) == 0)
	        {
	            mblk_ite *om = blk1.datap;
				
				if(eof_flag == 0)
					eof_flag = 1;
				else if(eof_flag == 1){
					if(blk1.private1 == Eofsound){
						eof_flag = 2;
					}	
				}
					
					
				//printf("in1 size %d\n", om->size);
	            mblkQShapePut(&d->dataQ1,om,shape_size);

	        }
		}

		input0=getmblkqavail(&d->dataQ0);
		input1=getmblkqavail(&d->dataQ1);
		//printf("input0 %d  input1 %d\n",input0, input1);
		mix_count = (input0 >= input1?input1:input0);
		
		if(mix_count > 0){
			for(i = 0; i < mix_count; i++){
				m0 = NULL;
				m1 = NULL;
				m0=getmblkq(&d->dataQ0);
				m1=getmblkq(&d->dataQ1);

				if(m0 && m1){
					o=allocb_ite(m0->size);
					
					for(;m0->b_rptr<m0->b_wptr;m0->b_rptr+=2,m1->b_rptr+=2, o->b_wptr+=2)
					 *((int16_t*)(o->b_wptr))=(0.1)*((int)*(int16_t*)m0->b_rptr)+(0.9)*((int)*(int16_t*)m1->b_rptr);
					
					if(m0) freemsg_ite(m0);
					if(m1) freemsg_ite(m1);
					blk.datap = o;

					ite_queue_put(f->output[0].Qhandle, &blk);

				}
			}
			
			if(d->mic_in == 0)//insert file
				usleep(18000);//18000
			else//mic in
				usleep(10000);

			if(eof_flag == 2){//0:one put, 1: mixing, 2:mixend
				eof_flag = 0;
				while(getmblkqavail(&d->dataQ0)){
					blk.datap = getmblkq(&d->dataQ0);
					ite_queue_put(f->output[0].Qhandle, &blk);
					usleep(10000);//shape_size : 10ms data
				}
			}
		}
		else if(mix_count == 0 && d->mic_in == 0)//insert file
		{
			if(input0!= 0){
				while(getmblkqavail(&d->dataQ0)){
					blk.datap = getmblkq(&d->dataQ0);
					ite_queue_put(f->output[0].Qhandle, &blk);
					usleep(10000);//shape_size : 10ms data
				}
			}
			else if(input1!= 0){
				while(getmblkqavail(&d->dataQ1)){
					blk.datap = getmblkq(&d->dataQ1);
					ite_queue_put(f->output[0].Qhandle, &blk);
					usleep(10000);//shape_size : 10ms data
				}
			}
		}
	}
	ite_mblk_queue_flush(f->output[0].Qhandle);
	
	return NULL;
}


static void mix_init(IteFilter *f)
{
    Mixdata *d=(Mixdata*)ite_new(Mixdata,1);

	d->framesize = 80;
	d->sample_rate = 8000;
	d->chn = 1;
	d->mic_in = 0;
	mblkQShapeInit(&d->dataQ0,d->framesize * 960);
	mblkQShapeInit(&d->dataQ1,d->framesize * 960);

	f->data=d;
	
}

static void mix_uninit(IteFilter *f)
{
    Mixdata *d=(Mixdata*)f->data;
	mblkQShapeUninit(&d->dataQ0);
	mblkQShapeUninit(&d->dataQ1);
    free(d);
}

static int mix_set_input_rate(IteFilter *f, void *arg){
    Mixdata *d = (Mixdata*)f->data;
	d->sample_rate = *(int*)arg;
   
    return 0;
}

static int mix_set_input_channel(IteFilter *f, void *arg){
    Mixdata *d = (Mixdata*)f->data;
	d->chn = *(int*)arg;
   
    return 0;
}

static int is_mix_with_mic(IteFilter *f, void *arg){
    Mixdata *d = (Mixdata*)f->data;
	d->mic_in = *(int*)arg;
   
    return 0;
}



static IteMethodDes mix_methods[] = {
	{ITE_FILTER_SETRATE,mix_set_input_rate},
	{ITE_FILTER_SET_NCHANNELS, mix_set_input_channel},
	{ITE_FILTER_SET_VALUE, is_mix_with_mic},
	{0, NULL},

};

IteFilterDes FilterMicMix = {
    ITE_FILTER_MICMIX_ID,
    mix_init,
    mix_uninit,
    mix_process,
    mix_methods
};



