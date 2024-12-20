#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"
#include <math.h>
#include <memory.h>
//=============================================================================
//                              Constant Definition
//=============================================================================
#define GAIN_ZERODB 20000
#define TAPS 512
//=============================================================================
//                              Private Function Declaration
//=============================================================================
extern void OLS_Filtering(int16_t *Sin, int16_t *Sout, void *cpx);

typedef struct _EqualizerState{
	rbuf_ite *buff;
    int rate;
	short* fft_cpx;
} EqualizerState;

static void equalizer_state_flatten(EqualizerState *s){
	int i;
	short val=GAIN_ZERODB/TAPS;
	s->fft_cpx[0]=val;
	for(i=1;i<TAPS;i+=2)
		s->fft_cpx[i]=val;
}

/* TODO: rate also beyond 16000 */
static EqualizerState * equalizer_state_new(){
	EqualizerState *s=(EqualizerState *)ite_new(EqualizerState,1);
	s->rate=44100;
	s->fft_cpx=(short*)malloc(sizeof(short)*TAPS);
	equalizer_state_flatten(s);
    s->buff=ite_rbuf_init(TAPS*10);
	return s;
}

static void equalizer_state_destroy(EqualizerState *s){
    ite_rbuf_flush(s->buff);
    ite_rbuf_free(s->buff);
	free(s->fft_cpx);
	free(s);
}

static int equalizer_state_hz_to_index(EqualizerState *s, int hz){
	int ret;
	if (hz<0){
		// printf("Bad frequency value %i",hz);
		return -1;
	}
	if (hz>(s->rate/2)){
		hz=(s->rate/2);
	}
	/*round to nearest integer*/
	ret=((hz*TAPS)+(s->rate/2))/s->rate;
	if (ret==TAPS/2) ret=(TAPS/2)-1;
	return ret;
}

static int equalizer_state_index2hz(EqualizerState *s, int index){
	return (index * s->rate + TAPS/2) / TAPS;
}

/* The natural peaking equalizer amplitude transfer function is multiplied to the discrete f-points.
 * Note that for PEQ no sqrt is needed for the overall calculation, applying it to gain yields the
 * same response.
 */
static float equalizer_compute_gainpoint(int f, int freq_0, float sqrt_gain, int freq_bw)
{
	float k1, k2;
	k1 = ((float)(f*f)-(float)(freq_0*freq_0));
	k1*= k1;
	k2 = (float)(f*freq_bw);
	k2*= k2;
	return (k1+k2*sqrt_gain)/(k1+k2/sqrt_gain);
}

static void equalizer_point_set(EqualizerState *s, int i, int f, float gain){
	// printf("Setting gain %f for freq_index %i (%i Hz)\n",gain,i,f);
	s->fft_cpx[1+((i-1)*2)] = (s->fft_cpx[1+((i-1)*2)]*(int)(gain*32768))/32768;
}

static void equalizer_state_set(EqualizerState *s, int freq_0, float gain, int freq_bw){
	int i, f;
	int delta_f = equalizer_state_index2hz(s, 1);
	float sqrt_gain = sqrt(gain);
	int mid = equalizer_state_hz_to_index(s, freq_0);
	freq_bw-= delta_f/2;   /* subtract a constant - compensates for limited fft steepness at low f */
	if (freq_bw < delta_f/2)
		freq_bw = delta_f/2;
	i = mid;
	f = equalizer_state_index2hz(s, i);
	equalizer_point_set(s, i, f, gain);   /* gain according to argument */
	do {	/* note: to better accomodate limited fft steepness, -delta is applied in f-calc ... */
		i++;
		f = equalizer_state_index2hz(s, i);
		gain = equalizer_compute_gainpoint(f-delta_f, freq_0, sqrt_gain, freq_bw);
		equalizer_point_set(s, i, f, gain);
	}
	while (i < TAPS/2 && (gain>1.1 || gain<0.9));
	i = mid;
	do {	/* ... and here +delta, as to  */
		i--;
		f = equalizer_state_index2hz(s, i);
		gain = equalizer_compute_gainpoint(f+delta_f, freq_0, sqrt_gain, freq_bw);
		equalizer_point_set(s, i, f, gain);
	}
	while (i>=0 && (gain>1.1 || gain<0.9));
}

static void dump_table(short *t, int len){
	int i;
	for(i=0;i<len;i++) {
        if(len%8==0) printf("\n");
		printf("[%i] %i\t",i,t[i]);
    }
    printf("\n");
}

static void equalizer_init(IteFilter *f){
	f->data=equalizer_state_new(TAPS);
}

static void equalizer_uninit(IteFilter *f){
	equalizer_state_destroy((EqualizerState*)f->data);
}

static void equalizer_process(IteFilter *f){
	// uint32_t nbytes = TAPS;
    IteQueueblk blk ={0};
	EqualizerState *s=(EqualizerState*)f->data;
    unsigned char buffer[TAPS];
    
    while(f->run){
        
        IteAudioQueueController(f,0,30,5);
    
        if((ite_queue_get(f->input[0].Qhandle, &blk)) == 0){
            mblk_ite *om=blk.datap;
            ite_rbuf_put(s->buff,om->b_rptr,om->size);
            if(om) freemsg_ite(om);
        
            while(ite_rbuf_get(buffer, s->buff, TAPS)){
                mblk_ite *out;
                out = allocb_ite(TAPS);
                OLS_Filtering(buffer, out->b_wptr, s->fft_cpx);
                out->b_wptr+=TAPS;
                blk.datap = out;
                ite_queue_put(f->output[0].Qhandle, &blk);
            }
        }
    }
    
    ite_mblk_queue_flush(f->input[0].Qhandle);
    ite_mblk_queue_flush(f->output[0].Qhandle);
    
    return NULL;
}

static int equalizer_set_gain(IteFilter *f, void *data){
	EqualizerState *s=(EqualizerState*)f->data;
	EqualizerGain *d=(EqualizerGain*)data;
	equalizer_state_set(s,d->frequency,d->gain,d->width);
    
    // dump_table(s->fft_cpx,TAPS);
	return 0;
}

static int equalizer_set_rate(IteFilter *f, void *data){
	EqualizerState *s=(EqualizerState*)f->data;
	s->rate=*(int*)data;
	return 0;
}

static IteMethodDes equalizer_methods[]={
    {	ITE_FILTER_SETRATE	    ,	equalizer_set_rate	},
	{	ITE_FILTER_SET_GAIN		,	equalizer_set_gain	},
	{	0				        ,	NULL			}
};

IteFilterDes FilterEqualizer = {
	ITE_FILTER_EQUALIZER_ID,
	equalizer_init,
    equalizer_uninit,
	equalizer_process,
	equalizer_methods
};
