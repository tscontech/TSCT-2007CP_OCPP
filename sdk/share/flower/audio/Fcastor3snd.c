#include <stdio.h>
#include "flower/flower.h"
#include "flower/ite_queue.h"
#include "ite/itp.h"
#include "i2s/i2s.h"
//=============================================================================
//                              Constant Definition
//=============================================================================
#define I2S_DAC_BUF_LENGHT 128*1024
#define I2S_ADC_BUF_LENGHT  64*1024
STRC_I2S_SPEC spec_da = {0};
STRC_I2S_SPEC spec_ad = {0};
uint8_t *dac_buf=NULL;
uint8_t *adc_buf=NULL;
int gbytes=640; 
//=============================================================================
//                              struct Declaration
//=============================================================================
typedef struct Castor3SndData{
    int rate;
    bool started;
    bool pause;
    cb_sound_t fn_cb;
	int length;
	int timestamp;
	pthread_mutex_t mutex;
} Castor3SndData;
//=============================================================================
//                              Private Function Declaration
//=============================================================================
void audio_init_AD(void){
    
    if(!adc_buf){ 
        memset((void*)&spec_ad, 0, sizeof(STRC_I2S_SPEC));
        adc_buf = (uint8_t*)malloc(I2S_ADC_BUF_LENGHT);
        memset(adc_buf, 0, I2S_ADC_BUF_LENGHT);
        assert(d->adc_buf);
    }else{
        return;
    }
    /* ADC Spec */    
    spec_ad.channels      = 1;
    spec_ad.sample_rate   = 8000;
    spec_ad.buffer_size   = I2S_ADC_BUF_LENGHT;

	spec_ad.is_big_endian = 0;
    spec_ad.base_i2s      = adc_buf;

    spec_ad.sample_size   = 16;
    spec_ad.record_mode   = 1;
    
    spec_ad.from_MIC_IN   = 1;

    i2s_init_ADC(&spec_ad);
    i2s_pause_ADC(1);
    
    printf("ADC_INIT first time\n");
}

void audio_init_DA(void){
    
    if(!dac_buf){
        memset((void*)&spec_da, 0, sizeof(STRC_I2S_SPEC));
        dac_buf = (uint8_t*)malloc(I2S_DAC_BUF_LENGHT);
        memset(dac_buf, 0, I2S_DAC_BUF_LENGHT);
        assert(dac_buf);    
    }else{
        return;
    }
    spec_da.channels                 = 1;
    spec_da.sample_rate              = 8000;
    spec_da.buffer_size              = I2S_DAC_BUF_LENGHT;
	spec_da.is_big_endian            = 0;
    spec_da.base_i2s                 = dac_buf;

    spec_da.sample_size              = 16;
    spec_da.num_hdmi_audio_buffer    = 1;
    spec_da.is_dac_spdif_same_buffer = 1;

    spec_da.base_hdmi[0]             = dac_buf;
    spec_da.base_hdmi[1]             = dac_buf;
    spec_da.base_hdmi[2]             = dac_buf;
    spec_da.base_hdmi[3]             = dac_buf;
    spec_da.base_spdif               = dac_buf;

    spec_da.enable_Speaker           = 1;
    spec_da.enable_HeadPhone         = 1;
    spec_da.postpone_audio_output    = 1;    
    
    i2s_init_DAC(&spec_da);
    
    printf("DAC_INIT first time\n");
}

static int checkpause(bool falg){
    if(falg){
        usleep(50000);
        return 1;
    }else{
        return 0;
    }
}

//=============================================================================
//                          Write filter flow
//=============================================================================
static void write_init(IteFilter *f)
{
    Castor3SndData *d=(Castor3SndData*)ite_new(Castor3SndData,1);
    audio_init_DA();
    d->started=false;
    d->pause =false;
    d->fn_cb = NULL;
	d->length = 0;
	d->timestamp = 0;
	pthread_mutex_init(&d->mutex,NULL);
	
    f->data=d;
}

static void write_uninit(IteFilter *f)
{
    Castor3SndData *d=(Castor3SndData*)f->data;
    d->started = false;
    d->fn_cb = NULL;
	pthread_mutex_destroy(&d->mutex);
    free(d);
}

static void write_process(IteFilter *f)
{
    Castor3SndData *d=(Castor3SndData*)f->data;
    IteQueueblk blk ={0};
    uint32_t waitcount = 1;

    gbytes=20*(2*spec_da.sample_rate*spec_da.channels)/1000;//20ms data byte;
    //blk.AInfo.Eof =false;
	int bytes_per_ms = (2*spec_da.sample_rate*spec_da.channels)/1000;
	
    I2S_DA32_SET_WP(I2S_DA32_GET_RP());
    //i2s_set_direct_volperc(70);
    d->started=true;
    while (f->run)
    {  
        if(checkpause(d->pause)) continue;
        if(!d->started) break;

    //    if(GET_DA_RW_GAP>GET_DA_BASE_LEN*3/4) {
    //        usleep(1000*GET_DA_RW_GAP/bytes);//avoid put too many data into i2sbuf
    //        continue; 
    //    }else{
            //sem_wait(&f->input[0].semHandle);
            if(ite_queue_get(f->input[0].Qhandle, &blk) == 0){
                mblk_ite *om=blk.datap;
                
                while((GET_DA_RW_GAP+om->size)>=GET_DA_BASE_LEN){
                    if(waitcount>0) break;
                    usleep(1000*GET_DA_RW_GAP/gbytes);//avoid put too many data into i2sbuf
                    //printf("%d\n",1000*GET_DA_RW_GAP/bytes);
                }

				d->length += om->size;
				
                i2s_da_data_put(om->b_rptr,om->size);
                if(om) freemsg_ite(om);
                
                if(waitcount>0){
                    waitcount--;
                    if(waitcount==0) i2s_pause_DAC(0);
                }

                if(blk.private1==Eofsound) {
                    while(GET_DA_RW_GAP>gbytes){
                        usleep(1000*(GET_DA_RW_GAP/gbytes));
                    }
                    if(d->fn_cb) d->fn_cb(Eofsound,NULL);                    
                    d->started = false;
                }

                if(blk.private1==Eofmixsound) {
                    if(d->fn_cb) d->fn_cb(Eofmixsound,NULL);
                }
				pthread_mutex_lock(&d->mutex);
				d->timestamp = d->length/bytes_per_ms;
				pthread_mutex_unlock(&d->mutex);
				
            }
            usleep(1000*GET_DA_RW_GAP/gbytes);
        }
   // }
    i2s_pause_DAC(1);
    ite_mblk_queue_flush(f->input[0].Qhandle);    
    return NULL;
}    

//=============================================================================
//                          Read filter flow
//=============================================================================
static void read_init(IteFilter *f)
{
    Castor3SndData *d=(Castor3SndData*)ite_new(Castor3SndData,1);
    audio_init_AD();
    d->started=false;
    d->pause =false;
    f->data=d;
}

static void read_uninit(IteFilter *f)
{
    Castor3SndData *d=(Castor3SndData*)f->data;
    d->started=false;
    free(d);
}

static void read_process(IteFilter *f)
{
    Castor3SndData *d=(Castor3SndData*)f->data;
    IteQueueblk blk_output0 ={0};
	IteQueueblk blk_output1 ={0};
    int bytes=20*(2*spec_ad.sample_rate*spec_ad.channels)/1000;//10ms data byte;    
    bytes*=2;
    I2S_AD32_SET_RP(I2S_AD32_GET_WP());
    //i2s_ADC_set_rec_volperc(60);
    i2s_pause_ADC(0);
    d->started=true;
    //blk.AInfo.Eof =false;
    //blk.AInfo.Eofmix =false;
    while (f->run)
    {  

        if(!d->started) break;
        
        if(GET_AD_RW_GAP<bytes){//wait data
            usleep(100000);
            continue ;  
        }else{//get data
            mblk_ite *im=allocb_ite(bytes);
        
            i2s_ad_data_get(im->b_wptr,bytes);
            im->b_wptr+=bytes;
            blk_output0.datap = im;
            IteAudioQueueController(f,0,30,5);
            ite_queue_put(f->output[0].Qhandle, &blk_output0);
        
			if(f->output[1].Qhandle)
			{
				mblk_ite *data = dupmblk(im);
                blk_output1.datap = data;
                IteAudioQueueController(f,1,30,5);
                ite_queue_put(f->output[1].Qhandle, &blk_output1);
			}
            //sem_post(&f->output[0].semHandle);
            usleep(10000);            
        }

    }
    i2s_pause_ADC(1);
    ite_mblk_queue_flush(f->output[0].Qhandle);  
    return NULL;
}

static void snd_stop(IteFilter *f, void *arg)
{
    Castor3SndData *d=(Castor3SndData*)f->data;
    d->started=false;
}

static void snd_set_cb(IteFilter *f, cb_sound_t fnc)
{
    Castor3SndData *d=(Castor3SndData*)f->data;
    d->fn_cb=fnc;
}

static void snd_pause(IteFilter *f, void *arg)
{
    Castor3SndData *d=(Castor3SndData*)f->data;
    d->pause=*((bool*)arg);
    if(d->pause){
        i2s_pause_DAC(1);    
    }else{
        i2s_pause_DAC(0);
    } 
}

static void snd_get_timestamp(IteFilter *f, void *arg)
{
    Castor3SndData *d=(Castor3SndData*)f->data;
	pthread_mutex_lock(&d->mutex);
	*(int*)arg = d->timestamp;
	pthread_mutex_unlock(&d->mutex);
}


static IteMethodDes snd_methods[] = {
    //{ITE_FILTER_STOP, snd_stop},
    {ITE_FILTER_SET_CB, snd_set_cb},
    {ITE_FILTER_PAUSE , snd_pause  },
    {ITE_FILTER_GET_VALUE, snd_get_timestamp},
    {0, NULL}
};

IteFilterDes FilterSndWtire = {
    ITE_FILTER_SNDWRITE_ID,
    write_init,
    write_uninit,
    write_process,
    snd_methods
};

IteFilterDes FilterSndRead = {
    ITE_FILTER_SNDREAD_ID,
    read_init,
    read_uninit,
    read_process,
    snd_methods
};

void castor3snd_deinit_state(void)
{
    spec_ad.sample_rate = 1000;
    spec_ad.sample_size = 8;
    i2s_deinit_ADC();
    i2s_deinit_DAC();
}

void Castor3snd_reinit_for_diff_rate(int rate,int bitsize,int channel)
{
    if (spec_da.sample_rate == rate && spec_da.sample_size == bitsize && spec_da.channels == channel && 
        i2s_get_DA_running() && i2s_get_AD_running())
        return ;
    i2s_deinit_ADC();
    i2s_deinit_DAC();
    spec_da.sample_rate =  rate ;
    spec_da.sample_size =  bitsize ;
    spec_da.channels = channel;
    
    spec_ad.sample_rate = rate;
    spec_ad.sample_size = bitsize;
    spec_ad.channels = channel;
    // channel bit etc. can be changed here;
    i2s_init_DAC(&spec_da);
    i2s_init_ADC(&spec_ad);
    i2s_pause_ADC(1);
    gbytes=20*(2*rate*channel)/1000;
}

void flow_set_dac_level(int level){
    i2s_set_direct_volperc(level);
}

void flow_set_adc_level(int level){
    i2s_ADC_set_rec_volperc(level);
}

