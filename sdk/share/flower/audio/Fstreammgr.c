#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"
#include "include/fileheader.h"
#include "i2s/i2s.h"
#include "ite/audio.h"

/*RISC_USER_DEFINED_REG_BASE : 0xB0200080*/
#define DrvDumpPCM_RdPtr        (0xB0200080|0x18)
extern int gbytes;
typedef struct _StreamMgrData{
    PlayerState state;
    int rate;
    int bitsize;
    int nchannels;
    int loop_after;
    int codecType;

    int nbytes;
    uint8_t *codecbuf;
    int codelen;
    unsigned int RP;
    unsigned int WP;
    bool bypass;
    bool streamRead;

}StreamMgrData;

typedef struct _gData{
    int gcodecType;
    mblkq dataQ;
    pthread_mutex_t mutex;
}gData;
gData g;


void streamQInit(ITE_AUDIO_ENGINE cotype){
    mblkQShapeInit(&g.dataQ,2048);
    pthread_mutex_init(&g.mutex, NULL);
    g.gcodecType=cotype;
    iteAudioSetMusicCodecDump(1);
}

void streamQUninit(void){
    pthread_mutex_lock(&g.mutex);
    mblkQShapeUninit(&g.dataQ);
    pthread_mutex_unlock(&g.mutex);
    pthread_mutex_destroy(&g.mutex);
    g.gcodecType=-1;
    iteAudioSetMusicCodecDump(0);
}

void streamQPut(unsigned char *inbuf,int size){
    pthread_mutex_lock(&g.mutex);
    srcQShapePut(&g.dataQ,inbuf,size,2048);
    pthread_mutex_unlock(&g.mutex);
}

mblk_ite *streamQGet(void){
    mblk_ite *om;
    pthread_mutex_lock(&g.mutex);
    om=getmblkq(&g.dataQ);
    pthread_mutex_unlock(&g.mutex);

    return om;
}

int streamQCount(void){
    mblkq *q=&g.dataQ;
    return q->qcount;
}

void streamQflush(void){
    pthread_mutex_lock(&g.mutex);
    mblkQShapeFlush(&g.dataQ);
    pthread_mutex_unlock(&g.mutex);
}

void streamSetCodec(ITE_AUDIO_ENGINE cotype){
    g.gcodecType=cotype;
}

//=============================================================================
//                              Private Function Declaration
//=============================================================================

static int streammgr_stop(IteFilter *f, void *arg){
    StreamMgrData *d=(StreamMgrData*)f->data;
    
    return 0;
}

static int streammgr_close(IteFilter *f, void *arg){
    StreamMgrData *d=(StreamMgrData*)f->data;
    d->state=Closed; 

    return 0;
}

static mblk_ite *get_codec_data(StreamMgrData *d){
    mblk_ite *rm = NULL;
    uint32_t bsize = 0;
    uint32_t rp,wp;
    uint8_t *codecbuf=d->codecbuf;
    uint32_t codelen=d->codelen;
    
    rp=d->RP;
    wp=d->WP;
    if (rp <= wp){
        bsize = wp - rp;
        if (bsize){
            rm = allocb_ite(bsize);
            ithInvalidateDCacheRange(codecbuf + rp, bsize);
            memcpy(rm->b_wptr, codecbuf + rp, bsize);
            rm->b_wptr += bsize;
            rp += bsize;
        }
    }else{ // rp > wp
        bsize = (codelen - rp) + wp;
        if (bsize){     
            uint32_t szsec0 = codelen - rp;
            uint32_t szsec1 = bsize - szsec0;
            rm =allocb_ite(bsize);
            if (szsec0){
                ithInvalidateDCacheRange(codecbuf + rp, szsec0);
                memcpy(rm->b_wptr, codecbuf + rp, szsec0);
            }
            ithInvalidateDCacheRange(codecbuf, szsec1);
            memcpy(rm->b_wptr + szsec0, codecbuf, szsec1);
            rm->b_wptr += bsize;
            rp = szsec1;
        }
    }
    d->RP=d->WP;
    ithWriteRegA(DrvDumpPCM_RdPtr,d->RP);/*record PCM RP */
    return rm;
}

static void streamer_do(IteFilter *f){
    StreamMgrData *d=(StreamMgrData*)f->data;
    mblk_ite *im=streamQGet();//get data from Q
    IteQueueblk blk ={0};
    if(im){
        int bufSize =0;
        if(im->size==0) {
            printf("finish stream Play\n");
            d->streamRead=false;
            return;
        }
        do{ //wait available codec buf size
            iteAudioGetAvailableBufferLength(ITE_AUDIO_OUTPUT_BUFFER, &bufSize);
            d->WP=iteAudioCodecGetPcmIdx();
            if(d->RP!=d->WP){
                
                if(d->state==Closed){
                    d->RP=d->WP;
                    ithWriteRegA(DrvDumpPCM_RdPtr,d->RP);/*record PCM RP */
                }else{
                    mblk_ite *om=get_codec_data(d);//get pcm data
                    blk.datap = om;
                    ite_queue_put(f->output[0].Qhandle, &blk);
                }
            }
            usleep(1000);
            iteAudioUpdateMessageQ(); 
        }while(bufSize<=im->size);
        iteAudioWriteStream(im->b_rptr, im->size);
        if(im) freemsg_ite(im);
    }        

}

static int streammgr_open(IteFilter *f, void *arg){
    StreamMgrData *d=(StreamMgrData*)f->data;

    int tmp=0, cnt=0;
    
    d->codecType=g.gcodecType;
    d->streamRead=true; //must be true 
    iteAudioSetMusicCodecDump(1);
    iteAudioOpenEngine(d->codecType);
    iteAudioCodecSetPcmIdx(d->WP);
    ithWriteRegA(DrvDumpPCM_RdPtr,d->RP);/*record PCM RP */
    iteAudioSetAttrib(ITE_AUDIO_I2S_INIT,&tmp);
    
    while(tmp==0){
        
        streamer_do(f);
        
        if(cnt++>200){
            printf("timeout error %s %d\n",__FUNCTION__,__LINE__);
            if(cnt>210) printf("error init i2s\n"); break;
        }
        usleep(10000);
        iteAudioGetAttrib(ITE_AUDIO_I2S_INIT, &tmp);
        iteAudioUpdateMessageQ();
    }
    iteAudioGetAttrib(  ITE_AUDIO_CODEC_SET_CHANNEL,        &d->nchannels);
    iteAudioGetAttrib(  ITE_AUDIO_CODEC_SET_SAMPLE_RATE,    &d->rate);
    iteAudioGetAttrib(  ITE_AUDIO_I2S_PTR,                  &d->codecbuf);
    iteAudioGetAttrib(  ITE_AUDIO_CODEC_SET_BUFFER_LENGTH,  &d->codelen);
    printf("%d %d %d\n",d->rate,d->nchannels,tmp);
    
    if(!d->bypass)
        Castor3snd_reinit_for_diff_rate(d->rate,16,d->nchannels);

    if(d->streamRead){
        d->state = Playing;
    }else{
        d->state = Eof;
    }
    
    return 0;
}

static int streammgr_start(IteFilter *f, void *arg){
    StreamMgrData *d=(StreamMgrData*)f->data;
    if (d->state==Paused)
        d->state = Playing;
    else{
        d->state = Dummy;
        printf("MSdummyPlaying scilent\n");
    }
    return 0;
}

static int streammgr_pause(IteFilter *f, void *arg){
    StreamMgrData *d=(StreamMgrData*)f->data;
    bool pause=*((bool*)arg);
    if(pause)
        d->state=Paused;
    else
        d->state=Playing;
    return 0;
}

static int streammgr_IISbypass(IteFilter *f, void *arg){
    StreamMgrData *d=(StreamMgrData*)f->data;
    d->bypass=true;
    return 0;
}

static int streammgr_loop(IteFilter *f, void *arg){
    StreamMgrData *d=(StreamMgrData*)f->data;
    d->loop_after=*((int*)arg);
    return 0;
}

static int streammgr_get_rate(IteFilter *f, void *arg){
    StreamMgrData *d=(StreamMgrData*)f->data;
    *(int*)arg = d->rate;
    return 0;
}

static int streammgr_get_channels(IteFilter *f, void *arg){
    StreamMgrData *d=(StreamMgrData*)f->data;
    *(int*)arg = d->nchannels;
    return 0;
}

static void streammgr_init(IteFilter *f)
{
    StreamMgrData *d=(StreamMgrData*)ite_new(StreamMgrData,1);
    d->state=Closed;
    d->rate=8000;
    d->bitsize = 16;
    d->nchannels=1;
    d->loop_after=-1; /*by default, don't loop*/
    d->RP=0;
    d->WP=0;
    d->bypass=false;
    d->streamRead=false;
    f->data=d;
    d->codecType=-1;
}

static void streammgr_uninit(IteFilter *f)
{
    StreamMgrData *d=(StreamMgrData*)f->data;
    pthread_mutex_lock(&g.mutex);
    flushmblkq(&g.dataQ);
    pthread_mutex_unlock(&g.mutex);
    iteAudioStopQuick();
    free(d);
    iteAudioSetMusicCodecDump(0);
}

static void streammgr_process(IteFilter *f)
{
    IteQueueblk blk ={0};
    StreamMgrData *d=(StreamMgrData*)f->data;
    gbytes =20*(2*d->rate*d->nchannels)/1000;//20ms data 
    //bool streamRead=true;
    
    while(f->run) {
        
        if(IteAudioQueueController(f,0,30,5)==-1) continue;
        
        if (d->state==Playing){         
            if(d->streamRead){
                streamer_do(f);
            }else{
                /*get data from codec*/
                d->WP=iteAudioCodecGetPcmIdx();
                if(d->RP!=d->WP){
                    mblk_ite *om=get_codec_data(d);
                    blk.datap = om;
                    ite_queue_put(f->output[0].Qhandle, &blk);
                }
                if(GET_DA_RW_GAP<256){
                    printf("play eof\n");
                    d->state=Eof;        
                }
            }
        }
        
        if(d->state==Eof){
            mblk_ite *om=allocb_ite(gbytes);
            memset(om->b_wptr,0,gbytes);
            om->b_wptr+=gbytes;
            blk.datap = om;
            blk.private1=Eofsound;
            ite_queue_put(f->output[0].Qhandle, &blk);
            usleep(100000);
        }
        usleep(1000*GET_DA_RW_GAP/gbytes);
    }
    ite_mblk_queue_flush(f->output[0].Qhandle);
    
    return NULL;

}

static IteMethodDes streammgr_methods[] = {
    {ITE_FILTER_FILEOPEN   , streammgr_open },
    {ITE_FILTER_LOOPPLAY   , streammgr_loop },
    {ITE_FILTER_PAUSE      , streammgr_pause },
    {ITE_FILTER_SET_BYPASS , streammgr_IISbypass},
    {ITE_FILTER_GETRATE    , streammgr_get_rate},
    {ITE_FILTER_GET_NCHANNELS, streammgr_get_channels},
    {0, NULL}
};

IteFilterDes FilterStreamMgr = {
    ITE_FILTER_STREAMMGR_ID,
    streammgr_init,
    streammgr_uninit,
    streammgr_process,
    streammgr_methods
};



