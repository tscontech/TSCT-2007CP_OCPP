/*
read file(mp3 aac wma(0) ,wav(x)),and decode to pcm data
*/
#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"
#include "include/fileheader.h"
#include "i2s/i2s.h"
#include "ite/audio.h"

/*RISC_USER_DEFINED_REG_BASE : 0xB0200080*/
#define DrvDumpPCM_RdPtr        (0xB0200080+0x18)
#define BUFSIZE  (1024)//512
extern int gbytes;

typedef struct _PlayerMgrData{
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
    char serialpath[256];
    int  offset;
}PlayerMgrData;
//=============================================================================
//                              Private Function Declaration
//=============================================================================

int audiomgrCodecType(char* filename)
{
    char*                    ext;

    ext = strrchr(filename, '.');
    if (!ext)
    {
        printf("Invalid file name: %s\n", filename);
        return -1;
    }
    ext++;

    if (stricmp(ext, "wav") == 0)
    {
        return ITE_WAV_DECODE;     
    }
    else if (stricmp(ext, "mp3") == 0)
    {
        return ITE_MP3_DECODE;
    }
    else if (stricmp(ext, "wma") == 0)
    {
        return ITE_WMA_DECODE;
    }
    else if (stricmp(ext, "aac") == 0 || 
             stricmp(ext, "m4a") == 0)
    {
        return ITE_AAC_DECODE;
    }    
    else if (stricmp(ext, "flac") == 0)
    {
        return ITE_FLAC_DECODE;
    }
    else
    {
        printf("Unsupport file format: %s\n", ext);
        return -1;
    }
    return -1;
}

static int playermgr_stop(IteFilter *f, void *arg){
    PlayerMgrData *d=(PlayerMgrData*)f->data;
    
    return 0;
}

static int playermgr_close(IteFilter *f, void *arg){
    PlayerMgrData *d=(PlayerMgrData*)f->data;
    d->state=Closed; 
    if (d->fd)
    {
        fclose(d->fd);
        //castor3snd_reinit_for_video_memo_play();
    }
    d->fd=NULL;
    
    if(i2s_get_DA_running()){
        iteAudioStopQuick();
        i2s_deinit_DAC();
    }
    return 0;
}

static mblk_ite *codec_data(PlayerMgrData *d){
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

static int check_data_enough(PlayerMgrData *d){
    /*special case :file data from network,
      when network is unstable we need buffer time wait enough data to play
    */
    if(d->state==Playing && d->wait) {
        I2S_DA32_WAIT_RP_EQUAL_WP(1);
        i2s_pause_DAC(1);
        sleep(2);
        fseek(d->fd,0,SEEK_CUR);
        i2s_pause_DAC(0);
        return 1;
    }
    return 0;
}

static void fileplay_do(IteFilter *f){
    int availSize =0;
    char buf[BUFSIZE];
    PlayerMgrData *d=(PlayerMgrData*)f->data;
    IteQueueblk blk ={0};
    int rbytes=fread(buf,1,BUFSIZE,d->fd);

    if(rbytes==0) {
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
        iteAudioUpdateMessageQ();   
    }while(availSize<=rbytes);
    iteAudioWriteStream(buf, rbytes);//write data for decoder
}

static int playermgr_open(IteFilter *f, void *arg){
    PlayerMgrData *d=(PlayerMgrData*)f->data;
    const char *file=(const char*)arg;
    int tmp=0, cnt=0;
    playermgr_close(f,NULL);
    d->fd = fopen(file, "rb");
    if(d->fd==NULL) {
        printf("%s openfile error\n",file);
        d->state=Dummy;
        return 0;
    }
    d->codecType=audiomgrCodecType(file);
    iteAudioOpenEngine(d->codecType);//load codec engine
    iteAudioCodecSetPcmIdx(d->WP);
    ithWriteRegA(DrvDumpPCM_RdPtr,d->RP);
    iteAudioSetMusicCodecDump(1);//set dump flag
    iteAudioSetAttrib(ITE_AUDIO_I2S_INIT,&tmp);
    
    if(d->codecType==ITE_MP3_DECODE)
        parsingMp3IsID3v2(d->fd,&d->offset);

    
    while(tmp==0){//wait codec parsing i2s info;
    
        fileplay_do(f);
        
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
    d->fileRead=true;
    return 0;
}

static int playermgr_set_filepath(IteFilter *f, void *arg){
    PlayerMgrData *d=(PlayerMgrData*)f->data;
    char path[256];
    sprintf(d->serialpath,(char *)arg);
    printf("serial play %s\n",d->serialpath);
    const char *file_name;
    file_name = strtok(d->serialpath," ");
    playermgr_open(f,(void*)file_name);
    file_name = strtok(NULL,"");
    if(file_name==NULL){
        strcpy(d->serialpath,"end");
    }else{
        strcpy(d->serialpath,file_name);
     }
    return 0;
}

static int playermgr_start(IteFilter *f, void *arg){
    PlayerMgrData *d=(PlayerMgrData*)f->data;
    if (d->state==Paused)
        d->state = Playing;
    else{
        d->state = Dummy;
        printf("MSdummyPlaying scilent\n");
    }
    return 0;
}

static int playermgr_pause(IteFilter *f, void *arg){
    PlayerMgrData *d=(PlayerMgrData*)f->data;
    bool pause=*((bool*)arg);
    if(pause)
        d->state=Paused;
    else
        d->state=Playing;
    return 0;
}

static int playermgr_IISbypass(IteFilter *f, void *arg){
    PlayerMgrData *d=(PlayerMgrData*)f->data;
    d->bypass=true;
    return 0;
}

static int playermgr_loop(IteFilter *f, void *arg){
    PlayerMgrData *d=(PlayerMgrData*)f->data;
    d->loop_after=*((int*)arg);
    return 0;
}

static int playermgr_get_rate(IteFilter *f, void *arg){
    PlayerMgrData *d=(PlayerMgrData*)f->data;
    *(int*)arg = d->rate;
    return 0;
}

static int playermgr_get_channels(IteFilter *f, void *arg){
    PlayerMgrData *d=(PlayerMgrData*)f->data;
    *(int*)arg = d->nchannels;
    return 0;
}

static int playermgr_set_wait(IteFilter *f, void *arg){
    PlayerMgrData *d=(PlayerMgrData*)f->data;
    d->wait=*((int*)arg);
    return 0;
}

static void _excute_serialpath(IteFilter *f){
    PlayerMgrData *d=(PlayerMgrData*)f->data;
    I2S_DA32_WAIT_RP_EQUAL_WP(1);
    i2s_pause_DAC(1);
    d->WP=0;
    d->RP=0;
    playermgr_set_filepath(f,(void*)d->serialpath);
    I2S_DA32_SET_WP(I2S_DA32_GET_RP());
    i2s_pause_DAC(0);
}
    
//=============================================================================
//                              filter flow
//=============================================================================

static void playermgr_init(IteFilter *f)
{
    PlayerMgrData *d=(PlayerMgrData*)ite_new(PlayerMgrData,1);
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
    d->wait=false;
    d->offset=0;
    sprintf(d->serialpath,"end");
}

static void playermgr_uninit(IteFilter *f)
{
    PlayerMgrData *d=(PlayerMgrData*)f->data;
	iteAudioStopEngine();
    playermgr_close(f,NULL);
    iteAudioSetMusicCodecDump(0);
    free(d);
}

static void playermgr_process(IteFilter *f)
{
    PlayerMgrData *d=(PlayerMgrData*)f->data;
    IteQueueblk blk ={0};
    gbytes =20*(2*d->rate*d->nchannels)/1000;//20ms data 

    while(f->run) {
        
        if(IteAudioQueueController(f,0,30,5)==-1) continue;
        if (d->state==Playing){
            if(d->fileRead){
                fileplay_do(f);
            }else{
                /*get data from codec*/
                iteAudioUpdateMessageQ();
                d->WP=iteAudioCodecGetPcmIdx();
                if(d->RP!=d->WP){
                    mblk_ite *om=codec_data(d);
                    blk.datap = om;
                    ite_queue_put(f->output[0].Qhandle, &blk);
                }
                
                if(GET_DA_RW_GAP<gbytes){//end of play;
                
                    if(d->loop_after<0){
                        if(strcmp(d->serialpath,"end")!=0){//serial wav file play
                            _excute_serialpath(f);
                        }else{
                            printf("play eof\n");
                            d->state=Eof;                            
                        }
                    }else{
                        printf("play repeat\n");
                        fseek(d->fd,d->offset,SEEK_SET);
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

static IteMethodDes playermgr_methods[] = {
    {ITE_FILTER_FILEOPEN   , playermgr_open },
    {ITE_FILTER_LOOPPLAY   , playermgr_loop },
    {ITE_FILTER_PAUSE      , playermgr_pause },
    {ITE_FILTER_SET_BYPASS , playermgr_IISbypass},
    {ITE_FILTER_GETRATE    , playermgr_get_rate},
    {ITE_FILTER_GET_NCHANNELS, playermgr_get_channels},
    {ITE_FILTER_SET_VALUE  , playermgr_set_wait},
    {ITE_FILTER_SET_FILEPATH ,playermgr_set_filepath},
    {0, NULL}
};

IteFilterDes FilterFilePlayMgr = {
    ITE_FILTER_FILEPLAYMGR_ID,
    playermgr_init,
    playermgr_uninit,
    playermgr_process,
    playermgr_methods
};



