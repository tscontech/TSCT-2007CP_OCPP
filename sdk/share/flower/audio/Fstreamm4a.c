#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"
#include "include/fileheader.h"
#include "i2s/i2s.h"
#include "ite/audio.h"

/*RISC_USER_DEFINED_REG_BASE : 0xB0200080*/
#define DrvDumpPCM_RdPtr        (0xB0200080|0x18)
extern int gbytes;
typedef struct _PktData{
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
    
    AVCodecContext  *avctx;
    AVFormatContext *format_context;
    AVFrame *frame;

}PktData;

//=============================================================================
//                              Private Function Declaration
//=============================================================================

static int streamm4a_stop(IteFilter *f, void *arg){
    PktData *d=(PktData*)f->data;
    
    return 0;
}

static int streamm4a_close(IteFilter *f, void *arg){
    PktData *d=(PktData*)f->data;
    d->state=Closed; 

    return 0;
}

static mblk_ite *get_codec_data(PktData *d){
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
    PktData *d=(PktData*)f->data;
    int got_frame = 0;
    AVPacket pk1, *pk = &pk1;
    QGetPkt(pk,0);    

    IteQueueblk blk ={0};
    if(pk){
        int bufSize =0;
        if(pk->size==0) {
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
        }while(bufSize<=pk->size);
            
        avcodec_decode_audio4(d->avctx, d->frame, &got_frame, pk);      
        av_free_packet(pk);
    }        

}

static int streamm4a_open(IteFilter *f, void *arg){
    PktData *d=(PktData*)f->data;
    AVCodec *codec;
    int tmp=0, cnt=0;    
    
    codec = avcodec_find_decoder(CODEC_ID_AAC);
 	if (codec == NULL)
        printf("Could not find CODEC_ID_AAC decoder in ffmpeg.\n");
    
    avcodec_get_context_defaults(d->avctx);
    
    if (avcodec_open(d->avctx, codec) != 0)
        printf("avcodec_open() failed.\n");
     
    d->codecType=ITE_AAC_DECODE;
    d->streamRead=true; //must be true 
    //iteAudioOpenEngine(d->codecType);
    iteAudioCodecSetPcmIdx(d->WP);
    ithWriteRegA(DrvDumpPCM_RdPtr,d->RP);/*record PCM RP */
    iteAudioSetMusicCodecDump(1);
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

static int streamm4a_start(IteFilter *f, void *arg){
    PktData *d=(PktData*)f->data;
    if (d->state==Paused)
        d->state = Playing;
    else{
        d->state = Dummy;
        printf("MSdummyPlaying scilent\n");
    }
    return 0;
}

static int streamm4a_pause(IteFilter *f, void *arg){
    PktData *d=(PktData*)f->data;
    bool pause=*((bool*)arg);
    if(pause)
        d->state=Paused;
    else
        d->state=Playing;
    return 0;
}

static int streamm4a_IISbypass(IteFilter *f, void *arg){
    PktData *d=(PktData*)f->data;
    d->bypass=true;
    return 0;
}

static int streamm4a_loop(IteFilter *f, void *arg){
    PktData *d=(PktData*)f->data;
    d->loop_after=*((int*)arg);
    return 0;
}

static int streamm4a_get_rate(IteFilter *f, void *arg){
    PktData *d=(PktData*)f->data;
    *(int*)arg = d->rate;
    return 0;
}

static int streamm4a_get_channels(IteFilter *f, void *arg){
    PktData *d=(PktData*)f->data;
    *(int*)arg = d->nchannels;
    return 0;
}

static void streamm4a_init(IteFilter *f)
{
    PktData *d=(PktData*)ite_new(PktData,1);
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
    d->codecType=ITE_AAC_DECODE;
    d->frame = avcodec_alloc_frame();
    d->avctx = av_malloc(sizeof(AVCodecContext));
	avcodec_init();
    avcodec_register_all();
}

static void streamm4a_uninit(IteFilter *f)
{
    PktData *d=(PktData*)f->data;
    av_free(d->frame);  
    av_free(d->avctx);
    //iteAudioStopQuick();
    free(d);
    iteAudioSetMusicCodecDump(0);
}

static void streamm4a_process(IteFilter *f)
{
    IteQueueblk blk ={0};
    PktData *d=(PktData*)f->data;
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

static IteMethodDes streamm4a_methods[] = {
    {ITE_FILTER_FILEOPEN   , streamm4a_open },
    {ITE_FILTER_LOOPPLAY   , streamm4a_loop },
    {ITE_FILTER_PAUSE      , streamm4a_pause },
    {ITE_FILTER_SET_BYPASS , streamm4a_IISbypass},
    {ITE_FILTER_GETRATE    , streamm4a_get_rate},
    {ITE_FILTER_GET_NCHANNELS, streamm4a_get_channels},
    {0, NULL}
};

IteFilterDes FilterStreamM4a = {
    ITE_FILTER_STREAMM4A_ID,
    streamm4a_init,
    streamm4a_uninit,
    streamm4a_process,
    streamm4a_methods
};



