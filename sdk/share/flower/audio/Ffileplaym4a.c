#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"
#include "include/fileheader.h"
#include "i2s/i2s.h"
#include "ite/audio.h"

/*RISC_USER_DEFINED_REG_BASE : 0xB0200080*/
#define DrvDumpPCM_RdPtr        (0xB0200080+0x18)
extern int gbytes;
typedef struct _M4AData{
    void *fd;
    PlayerState state;
    int rate;
    int bitsize;
    int nchannels;
    int loop_after;
    int codecType;

    uint8_t *codecbuf;
    int codelen;
    unsigned int RP;
    unsigned int WP;
    bool bypass;
    bool fileRead;
    bool wait;
    
    bool isM4A;
    AVCodecContext  *avctx;
    AVFormatContext *format_context;
    AVFrame *frame;

}M4AData;
//=============================================================================
//                              Private Function Declaration
//=============================================================================
int _isffmpeg(char* filename)
{
    char* ext;
    ext = strrchr(filename, '.');
    ext++;
    if (stricmp(ext, "m4a") == 0) {
        av_register_all();
        avcodec_register_all();
        return true;
    }else{
        return false;
    }
}

static int m4amgr_stop(IteFilter *f, void *arg){
    M4AData *d=(M4AData*)f->data;
    
    return 0;
}

static int m4amgr_close(IteFilter *f, void *arg){
    M4AData *d=(M4AData*)f->data;
    d->state=Closed; 
    if (d->fd)
    {
        //avcodec_close(d->avctx);
        av_close_input_file(d->format_context);
        //castor3snd_reinit_for_video_memo_play();
    }
    d->fd=NULL;
    d->format_context=NULL;
    if(i2s_get_DA_running()){
        iteAudioStopQuick();
        i2s_deinit_DAC();
    }
    return 0;
}

static mblk_ite *codec_data(M4AData *d){
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

static int check_data_enough(M4AData *d){
    /*special case :file data from network,
      when network is unstable we need buffer time wait enough data to play
    */
    if(d->state==Playing && d->wait) {
        I2S_DA32_WAIT_RP_EQUAL_WP(1);
        i2s_pause_DAC(1);
        sleep(2);
        i2s_pause_DAC(0);
        return 1;
    }
    return 0;
}

static void m4aplay_do(IteFilter *f){
    M4AData *d=(M4AData*)f->data;
    int availSize =0;
    IteQueueblk blk ={0};
    AVPacket pkt1, *pkt = &pkt1;
    int ret = av_read_frame(d->format_context, pkt);
    if(ret<0) {
        if(check_data_enough(d)) return ; //file from network
        d->fileRead=false;
        printf("finish file read\n");
        return;
    }    
    
    do{//wait codec available buff size
        iteAudioGetAvailableBufferLength(ITE_AUDIO_OUTPUT_BUFFER, &availSize);
        d->WP=iteAudioCodecGetPcmIdx();
        if(d->RP!=d->WP){
            if(d->state==Closed){//wait codec parsing i2s info 
                d->RP=d->WP;
                ithWriteRegA(DrvDumpPCM_RdPtr,d->RP);
            }else{//get pcm data from codec;
                mblk_ite *om=codec_data(d);
                blk.datap = om;
                ite_queue_put(f->output[0].Qhandle, &blk);
            }
        }
        usleep(1000);        
    }while(availSize<=pkt->size);
    
    
    int got_frame = 0;
    avcodec_decode_audio4(d->avctx, d->frame, &got_frame, pkt);
    av_free_packet(pkt);

}

static int m4amgr_open(IteFilter *f, void *arg){
    M4AData *d=(M4AData*)f->data;
    const char *file=(const char*)arg;
    int tmp=0, cnt=0;
    d->isM4A = _isffmpeg(file);
    int idx;
    AVPacket pkt1, *pkt = &pkt1;
    AVCodec *codec;
    m4amgr_close(f,NULL);
    avformat_open_input(&d->format_context, file, NULL, NULL);  
    d->fd=d->format_context;
    if(d->fd==NULL){
        printf("%s openfile error\n",file);
        d->state=Dummy;
        return 0;
    }
    idx = av_find_best_stream(d->format_context, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    d->avctx = d->format_context->streams[idx]->codec; 
    codec = avcodec_find_decoder(d->avctx->codec_id);
    if (!codec || avcodec_open2(d->avctx, codec, NULL) < 0){
        printf("[err] %s %d\n",__FUNCTION__,__LINE__);
        while(1);
    }
    //iteAudioOpenEngine(d->codecType);//load codec engine
   // d->codecType=audiomgrCodecType(file);
    iteAudioCodecSetPcmIdx(d->WP);
    ithWriteRegA(DrvDumpPCM_RdPtr,d->RP);
    iteAudioSetMusicCodecDump(1);//set dump flag
    iteAudioSetAttrib(ITE_AUDIO_I2S_INIT,&tmp);   
    
    
    while(tmp==0){//wait codec parsing i2s info;
        m4aplay_do(f);
        
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

    d->state=Playing;

    return 0;
}

static int m4amgr_start(IteFilter *f, void *arg){
    M4AData *d=(M4AData*)f->data;
    if (d->state==Paused)
        d->state = Playing;
    else{
        d->state = Dummy;
        printf("MSdummyPlaying scilent\n");
    }
    return 0;
}

static int m4amgr_pause(IteFilter *f, void *arg){
    M4AData *d=(M4AData*)f->data;
    bool pause=*((bool*)arg);
    if(pause)
        d->state=Paused;
    else
        d->state=Playing;
    return 0;
}

static int m4amgr_IISbypass(IteFilter *f, void *arg){
    M4AData *d=(M4AData*)f->data;
    d->bypass=true;
    return 0;
}

static int m4amgr_loop(IteFilter *f, void *arg){
    M4AData *d=(M4AData*)f->data;
    d->loop_after=*((int*)arg);
    return 0;
}

static int m4amgr_get_rate(IteFilter *f, void *arg){
    M4AData *d=(M4AData*)f->data;
    *(int*)arg = d->rate;
    return 0;
}

static int m4amgr_get_channels(IteFilter *f, void *arg){
    M4AData *d=(M4AData*)f->data;
    *(int*)arg = d->nchannels;
    return 0;
}

static int m4amgr_set_wait(IteFilter *f, void *arg){
    M4AData *d=(M4AData*)f->data;
    d->wait=*((int*)arg);
    return 0;
}

static void m4amgr_init(IteFilter *f)
{
    M4AData *d=(M4AData*)ite_new(M4AData,1);
    d->fd=NULL;
    d->state=Closed;
    d->rate=8000;
    d->bitsize = 16;
    d->nchannels=1;
    d->loop_after=-1; /*by default, don't loop*/
    d->RP=0;
    d->WP=0;
    d->bypass=false;
    f->data=d;
    d->codecType=-1;
    d->fileRead=true;
    d->wait= false;
    d->isM4A = false;
    d->format_context=NULL;
    d->frame = avcodec_alloc_frame();
    av_register_all();
    avcodec_register_all();
}

static void m4amgr_uninit(IteFilter *f)
{
    M4AData *d=(M4AData*)f->data;
    m4amgr_close(f,NULL);
    av_free(d->frame);
    iteAudioSetMusicCodecDump(0);
    free(d);
}

static void m4amgr_process(IteFilter *f)
{
    M4AData *d=(M4AData*)f->data;
    IteQueueblk blk ={0};
    gbytes =20*(2*d->rate*d->nchannels)/1000;//20ms data 

    while(f->run) {
        
        if(IteAudioQueueController(f,0,30,5)==-1) continue;
        iteAudioUpdateMessageQ();
        if (d->state==Playing){
            if(d->fileRead){
                m4aplay_do(f);
            }else{
                /*get data from codec*/
                d->WP=iteAudioCodecGetPcmIdx();
                if(d->RP!=d->WP){
                    mblk_ite *om=codec_data(d);
                    blk.datap = om;
                    ite_queue_put(f->output[0].Qhandle, &blk);
                }
                
                if(GET_DA_RW_GAP<256){//end of play;
                    if(d->loop_after<0){
                        printf("play eof\n");
                        d->state=Eof;
                    }else{
                        printf("play repeat\n");
                        avformat_seek_file(d->format_context, -1, INT64_MIN, AV_NOPTS_VALUE, INT64_MAX, 0);
                        //fseek(d->fd,0,SEEK_SET);
                        d->fileRead=true;
                        continue;
                    }                    
                }
            }

        }
        
        if(d->state == Dummy){
            mblk_ite *om=allocb_ite(gbytes);
            memset(om->b_wptr,0,gbytes);
            om->b_wptr+=gbytes;
            blk.datap = om;
            blk.private1=Eofsound;
            ite_queue_put(f->output[0].Qhandle, &blk);
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
        usleep(100*GET_DA_RW_GAP/gbytes);
    }
    ite_mblk_queue_flush(f->output[0].Qhandle);
    
    return NULL;

}

static IteMethodDes m4amgr_methods[] = {
    {ITE_FILTER_FILEOPEN   , m4amgr_open },
    {ITE_FILTER_LOOPPLAY   , m4amgr_loop },
    {ITE_FILTER_PAUSE      , m4amgr_pause },
    {ITE_FILTER_SET_BYPASS , m4amgr_IISbypass},
    {ITE_FILTER_GETRATE    , m4amgr_get_rate},
    {ITE_FILTER_GET_NCHANNELS, m4amgr_get_channels},
    {ITE_FILTER_SET_VALUE, m4amgr_set_wait},
    {0, NULL}
};

IteFilterDes FilterPlayM4a = {
    ITE_FILTER_PLAYM4A_ID,
    m4amgr_init,
    m4amgr_uninit,
    m4amgr_process,
    m4amgr_methods
};



