/*
read file(mp3 aac wma wav flac m4a),and decode to pcm data
*/
#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"
#include "include/fileheader.h"
#include "i2s/i2s.h"
#include "ite/audio.h"

#define BUFSIZE  (512)//512

typedef struct _QuickPlayData{
    void *fd;
    PlayerState state;
    int rate;
    int bitsize;
    int nchannels;
    int loop_after;
    int codecType;
    int offset;
    cb_sound_t fn_cb;

    bool is_insert;
    bool fileRead;
    char serialpath[256];
    /*m4a*/
    bool isM4A;
    AVCodecContext  *avctx;
    AVFormatContext *format_context;
    AVFrame *frame;
    
    /*vedio*/
    bool is_vedio;
    int vediocodecType;
    
}QuickPlayData;

#ifdef CFG_BUILD_FFMPEG
static void vedio_pause(int pause,int *vcodecType)
{
    #if defined(__OPENRTOS__)
    if (pause){
        iteAudioGetEngineAttrib(ITE_AUDIO_ENGINE_TYPE,vcodecType);
    }else{
        iteAudioOpenEngine(*vcodecType); //reinit i2s ,clear buffer data
    }
    itp_codec_playback_mute();
    iteAudioSetAttrib(ITE_AUDIO_FFMPEG_PAUSE_AUDIO, &pause);
    printf("vedio pause(%d)\n",pause);
    #endif
}
#endif

//=============================================================================
//                              Private Function Declaration
//=============================================================================
static int quickplay_close(IteFilter *f, void *arg){
    QuickPlayData *d=(QuickPlayData*)f->data;
    int tmp = 0;
    d->state=Closed;
    if (d->fd)
    {
        if(d->isM4A){ 
            av_close_input_file(d->format_context);
        }else{
            fclose(d->fd);
        }
        //castor3snd_reinit_for_video_memo_play();
    }
    d->fd=NULL;
    d->format_context=NULL;
    
    if(i2s_get_DA_running()){
        iteAudioStopQuick();
        i2s_deinit_DAC();
    }
    iteAudioSetAttrib(ITE_AUDIO_I2S_INIT, &tmp);
    return 0;
}

static void filem4a_do(IteFilter *f){
    QuickPlayData *d=(QuickPlayData*)f->data;
    int availSize =0;
    AVPacket pkt1, *pkt = &pkt1;
    int ret = av_read_frame(d->format_context, pkt);

    if(ret<0) {
        d->fileRead=false;
        printf("finish file read\n");
        return;
    }    
    
    do{//wait codec available buff size
        iteAudioGetAvailableBufferLength(ITE_AUDIO_OUTPUT_BUFFER, &availSize);
        usleep(1000);
        iteAudioUpdateMessageQ();  
    }while(availSize<=pkt->size);

    int got_frame = 0;
    avcodec_decode_audio4(d->avctx, d->frame, &got_frame, pkt);
    av_free_packet(pkt);
}



static void fileplay_do(IteFilter *f){
    int availSize =0;
    char buf[BUFSIZE];
    QuickPlayData *d=(QuickPlayData*)f->data;
    int rbytes=fread(buf,1,BUFSIZE,d->fd);

    if(rbytes==0) {
        d->fileRead=false;
        printf("finish file read\n");
        return;
    }
    
    do{//wait codec available buff size
        iteAudioGetAvailableBufferLength(ITE_AUDIO_OUTPUT_BUFFER, &availSize);
        usleep(1000);
        iteAudioUpdateMessageQ();   
    }while(availSize<=rbytes);
    
    iteAudioWriteStream(buf, rbytes);//write data for decoder
}

static int quickplay_open(IteFilter *f, void *arg){
    QuickPlayData *d=(QuickPlayData*)f->data;
    const char *file=(const char*)arg;
    
    quickplay_close(f,NULL);
    
    d->isM4A = _isffmpeg(file);
    d->codecType=audiomgrCodecType(file);
    if(d->is_vedio) vedio_pause(1,&d->vediocodecType);
    
    if(d->isM4A){
        int idx;
        AVCodec *codec;
        avformat_open_input(&d->format_context, file, NULL, NULL);  
        d->fd=d->format_context;
        idx = av_find_best_stream(d->format_context, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
        d->avctx = d->format_context->streams[idx]->codec; 
        codec = avcodec_find_decoder(d->avctx->codec_id);
        if (!codec || avcodec_open2(d->avctx, codec, NULL) < 0){
            printf("[err] %s %d\n",__FUNCTION__,__LINE__);
            while(1);
        }
    }else{
        d->fd = fopen(file, "rb");
        if(d->fd==NULL) {
            printf("openfile error\n");
            return 0;
        }
        iteAudioOpenEngine(d->codecType);
        
        if(d->codecType==ITE_MP3_DECODE)
            parsingMp3IsID3v2(d->fd,&d->offset);        
    }
    d->state=Playing;
    d->fileRead=true;   
    return 0;
}

static int quickplay_set_filepath(IteFilter *f, void *arg){
    QuickPlayData *d=(QuickPlayData*)f->data;
    char path[256];
    sprintf(d->serialpath,(char *)arg);
    printf("serial play %s\n",d->serialpath);
    const char *file_name;
    file_name = strtok(d->serialpath," ");
    quickplay_open(f,(void*)file_name);
    file_name = strtok(NULL,"");
    if(file_name==NULL){
        strcpy(d->serialpath,"end");
    }else{
        strcpy(d->serialpath,file_name);
     }
    return 0;
}

static int quickplay_start(IteFilter *f, void *arg){
    QuickPlayData *d=(QuickPlayData*)f->data;
    if (d->state==Paused)
        d->state = Playing;
    else{
        d->state = Dummy;
        printf("MSdummyPlaying scilent\n");
    }
    return 0;
}

static int quickplay_pause(IteFilter *f, void *arg){
    QuickPlayData *d=(QuickPlayData*)f->data;
    bool pause=*((bool*)arg);
    if(pause){
        iteAudioStopQuick();
        d->state=Paused;
        i2s_pause_DAC(1);
    }else{
        iteAudioOpenEngine(d->codecType);
        d->state=Playing;
        //i2s_pause_DAC(0); 
    }
    return 0;
}

static int quickplay_set_insert(IteFilter *f, void *arg){
    QuickPlayData *d=(QuickPlayData*)f->data;
    d->is_insert=true;
    return 0;
}

static int quickplay_loop(IteFilter *f, void *arg){
    QuickPlayData *d=(QuickPlayData*)f->data;
    d->loop_after=*((int*)arg);
    return 0;
}

static int quickplay_get_rate(IteFilter *f, void *arg){
    QuickPlayData *d=(QuickPlayData*)f->data;
    *(int*)arg = d->rate;
    return 0;
}

static int quickplay_get_channels(IteFilter *f, void *arg){
    QuickPlayData *d=(QuickPlayData*)f->data;
    *(int*)arg = d->nchannels;
    return 0;
}

static void _excute_serialpath(IteFilter *f){
    QuickPlayData *d=(QuickPlayData*)f->data;
    I2S_DA32_WAIT_RP_EQUAL_WP(1);
    i2s_pause_DAC(1);
    quickplay_set_filepath(f,(void*)d->serialpath);
    I2S_DA32_SET_WP(I2S_DA32_GET_RP());
    i2s_pause_DAC(0);
}

static void quickplay_set_cb(IteFilter *f, cb_sound_t fnc)
{
    QuickPlayData *d=(QuickPlayData*)f->data;
    d->fn_cb=fnc;
}

static void update_param(QuickPlayData *d,int *pinit,int *pbytes){
    int tmp;
    iteAudioGetAttrib(ITE_AUDIO_I2S_INIT, &tmp);
    if(tmp){
        iteAudioGetAttrib(  ITE_AUDIO_CODEC_SET_CHANNEL,        &d->nchannels);
        iteAudioGetAttrib(  ITE_AUDIO_CODEC_SET_SAMPLE_RATE,    &d->rate);
        *pinit=1;
        printf("rate=%d ch=%d pbytes=%d\n",d->nchannels,d->rate,*pbytes);
    }    
}

static void quickplay_init(IteFilter *f)
{
    QuickPlayData *d=(QuickPlayData*)ite_new(QuickPlayData,1);
    d->fd=NULL;
    d->state=Closed;
    d->rate=8000;
    d->bitsize = 16;
    d->nchannels=1;
    d->loop_after=-1; /*by default, don't loop*/
    d->offset=0;
    d->is_insert=false;
    d->codecType=-1;
    d->fileRead=true;
    d->isM4A = false;
    d->format_context=NULL;
    d->frame = avcodec_alloc_frame();
    sprintf(d->serialpath,"end");
    d->is_vedio = mtal_pb_check_fileplayer_with_audio();
    f->data=d;
}

static void quickplay_uninit(IteFilter *f)
{
    QuickPlayData *d=(QuickPlayData*)f->data;
    iteAudioStopQuick();
    quickplay_close(f,NULL);
    av_free(d->frame);
    if(d->is_vedio) {
        vedio_pause(0,&d->vediocodecType);
    }
    free(d);
}

static void quickplay_process(IteFilter *f)
{
    QuickPlayData *d=(QuickPlayData*)f->data;
    int initDAC=0;
    int bytes =10*(2*d->rate*d->nchannels)/1000;//10 ms data(tmp value)

    while(f->run) {
        
        if (d->state==Playing){
            if(d->fileRead){
                if(d->isM4A){
                    filem4a_do(f);
                }else{
                    fileplay_do(f);
                }
                if(!initDAC) 
                    update_param(d,&initDAC,&bytes);
            }else{
                /*get data from codec*/
                //iteAudioUpdateMessageQ();

                if(GET_DA_RW_GAP<bytes){//end of play;
                    if(d->loop_after<0){
                        if(strcmp(d->serialpath,"end")!=0){
                            _excute_serialpath(f);//serial wav file play
                        }else{
                            I2S_DA32_WAIT_RP_EQUAL_WP(1);
                            printf("play eof\n");
                            if(d->is_insert){
                                if(d->fn_cb) d->fn_cb(MIX_EOF_FILE,NULL);
                            }else{
                                if(d->fn_cb) d->fn_cb(PLAY_EOF_FILE,NULL);                                
                            }                            
                            d->state=Eof;
                        }
                    }else{
                        printf("play repeat\n");
                        if(d->isM4A){
                            avformat_seek_file(d->format_context, -1, INT64_MIN, AV_NOPTS_VALUE, INT64_MAX, 0);
                        }else{
                            fseek(d->fd,d->offset,SEEK_SET);
                        }
                        d->fileRead=true;
                        continue;
                    }                    
                }
            }

        }
        if(d->state==Paused){
            usleep(100000);
        }
        
        if(d->state == Dummy){
            usleep(100000);
        }
        
        if(d->state==Eof){
            usleep(100000);
            break;
        }
        usleep(100*GET_DA_RW_GAP/bytes);
    }
    i2s_pause_DAC(1);
    
    return NULL;

}

static IteMethodDes quickplay_methods[] = {
    {ITE_FILTER_FILEOPEN   , quickplay_open },
    {ITE_FILTER_LOOPPLAY   , quickplay_loop },
    {ITE_FILTER_PAUSE      , quickplay_pause },
    {ITE_FILTER_SET_BYPASS , quickplay_set_insert},
    {ITE_FILTER_GETRATE    , quickplay_get_rate},
    {ITE_FILTER_GET_NCHANNELS, quickplay_get_channels},
    {ITE_FILTER_SET_FILEPATH ,quickplay_set_filepath},
    {ITE_FILTER_SET_CB     , quickplay_set_cb},
    {0, NULL}
};

IteFilterDes FilterQuickPlay = {
    ITE_FILTER_QUICKPLAY_ID,
    quickplay_init,
    quickplay_uninit,
    quickplay_process,
    quickplay_methods
};



