#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"
#include "include/volumeapi.h"


static void volume_init(IteFilter *f)
{
    Volume *v=VOICE_INIT();    
    f->data=v;
}

static void volume_uninit(IteFilter *f)
{
    Volume *v=(Volume*)f->data;
    VOICE_UNINIT(v);

}

static void volume_process(IteFilter *f)
{
    Volume *v=(Volume*)f->data;

    IteQueueblk blk ={0};
    float target_gain;
    
    while(f->run) {    
        
        IteAudioQueueController(f,0,30,5);
        if(ite_queue_get(f->input[0].Qhandle, &blk) == 0){
            mblk_ite *om=blk.datap;
            VOICE_applyProcess(v,om);
            blk.datap = om;
            ite_queue_put(f->output[0].Qhandle, &blk);
            
        }
        usleep(10000);
    }
    ite_mblk_queue_flush(f->input[0].Qhandle);
    ite_mblk_queue_flush(f->output[0].Qhandle);
}

static void volume_dB_gain(IteFilter *f, void *arg)
{
    Volume *v=(Volume*)f->data;
    float dBgain=*((float*)arg);
    VOICE_set_dB_gain(v,dBgain);
}

static void volume_ng_enable(IteFilter *f, void *arg)
{
    Volume *v=(Volume*)f->data;
    bool yesno=*((bool*)arg);
    VOICE_set_noise_gate(v,yesno);
}

static void volume_agc_enable(IteFilter *f, void *arg)
{
    Volume *v=(Volume*)f->data;
    bool yesno=*((bool*)arg);
    VOICE_set_agc(v,yesno);
}

static IteMethodDes volume_methods[] = {
    {ITE_FILTER_SET_VALUE, volume_dB_gain},
    {0, NULL}
};

IteFilterDes FilterVolume = {
    ITE_FILTER_VOLUME_ID,
    volume_init,
    volume_uninit,
    volume_process,
    volume_methods
};



