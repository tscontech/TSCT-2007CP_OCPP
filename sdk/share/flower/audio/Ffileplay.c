#include <stdio.h>
#include "flower/flower.h"
#include "flower/ite_queue.h"
#include "include/fileheader.h"
#include "ite/itp.h"
#include "i2s/i2s.h"
extern int gbytes;
//=============================================================================
//                              struct Definition
//=============================================================================
typedef struct _PlayerData{
    int fd;
    PlayerState state;
    int rate;
    int bitsize;
    int nchannels;
    int codectype;
    int hsize;
    int loop_after;
    int pause_time;
    bool bypass;
	bool finish;
    char serialpath[256];
}PlayerData;
//=============================================================================
//                              Private Function Declaration
//=============================================================================

static int read_wav_header(PlayerData *d){
    char header1[sizeof(riff_t)];
    char header2[sizeof(format_t)];
    char header3[sizeof(data_t)];
    int count;

    riff_t *riff_chunk=(riff_t*)header1;
    format_t *format_chunk=(format_t*)header2;
    data_t *data_chunk=(data_t*)header3;

    unsigned long len=0;

    len = read(d->fd, header1, sizeof(header1)) ;
    if (len != sizeof(header1)){
        goto not_a_wav;
    }

    if (0!=strncmp(riff_chunk->riff, "RIFF", 4) || 0!=strncmp(riff_chunk->wave, "WAVE", 4)){
        goto not_a_wav;
    }

    len = read(d->fd, header2, sizeof(header2)) ;
    if (len != sizeof(header2)){
        //ms_warning("Wrong wav header: cannot read file");
        goto not_a_wav;
    }

    d->rate=format_chunk->rate;
    d->nchannels=format_chunk->channel;
    d->codectype=format_chunk->type;
    d->bitsize=format_chunk->bitpspl;


    if (format_chunk->len-0x10>0)
    {
        lseek(d->fd,(format_chunk->len-0x10),SEEK_CUR);
    }

    d->hsize=sizeof(wave_header_t)-0x10+format_chunk->len;

    len = read(d->fd, header3, sizeof(header3)) ;
    if (len != sizeof(header3)){
        //ms_warning("Wrong wav header: cannot read file");
        goto not_a_wav;
    }
    count=0;
    while (strncmp(data_chunk->data, "data", 4)!=0 && count<30)
    {
        //ms_warning("skipping chunk=%s len=%i", data_chunk->data, data_chunk->len);
        lseek(d->fd,data_chunk->len,SEEK_CUR);
        count++;
        d->hsize=d->hsize+len+data_chunk->len;

        len = read(d->fd, header3, sizeof(header3)) ;
        if (len != sizeof(header3)){
            //ms_warning("Wrong wav header: cannot read file");
            goto not_a_wav;
        }
    }

    return 0;

    not_a_wav:
        /*rewind*/
        lseek(d->fd,0,SEEK_SET);
        d->hsize=0;
        return -1;
}

static int player_stop(IteFilter *f, void *arg){
    PlayerData *d=(PlayerData*)f->data;

    if (d->state!=Closed){
        d->state=Paused;
        lseek(d->fd,d->hsize,SEEK_SET);
    }

    return 0;
}

static int player_close(IteFilter *f, void *arg){
    PlayerData *d=(PlayerData*)f->data;
    player_stop(f,NULL);
    if (d->fd>=0)   close(d->fd);
    d->fd=-1;
    d->state=Closed;

    return 0;
}

static int player_start(IteFilter *f, void *arg){
    PlayerData *d=(PlayerData*)f->data;
    if (d->state==Paused){
        d->pause_time=0; d->state = Playing;}
    else{
        d->state = Dummy;
        printf("Dummy scilent\n");
    }
    return 0;
}

static int player_open(IteFilter *f, void *arg){
    PlayerData *d=(PlayerData*)f->data;
    int fd;
    const char *file=(const char*)arg;

    if (d->fd>=0){
        player_close(f,NULL);
    }
    if ((fd=open(file,O_RDONLY|O_BINARY))==-1){
        //ms_warning("Failed to open %s",file);
        return -1;
    }
    d->state=Paused;
    d->fd=fd;

    if (strstr(file,".wav") || strstr(file,".WAV")){
        if (read_wav_header(d)!=0)
            printf("File %s has .wav extension but wav header could be found.\n",file);
    }else if (strstr(file,".svm")){
        d->codectype = 0; //uknow codec not .wav file
    }else{
        printf("not .wav or .svm file\n");
    }
    if(!d->bypass)
        Castor3snd_reinit_for_diff_rate(d->rate,d->bitsize,d->nchannels);//chane to diff sampling rate
    //printf("%s opened: rate=%i,channel=%i\n",file,d->rate,d->nchannels);
    player_start(f,NULL);
    return 0;
}

static int player_set_filepath(IteFilter *f, void *arg){
    PlayerData *d=(PlayerData*)f->data;
    char path[256];
    sprintf(d->serialpath,(char *)arg);
    printf("serial play %s\n",d->serialpath);
    const char *file_name;
    file_name = strtok(d->serialpath," ");
    player_open(f,(void*)file_name);
    file_name = strtok(NULL,"");
    if(file_name==NULL){
        strcpy(d->serialpath,"end");
    }else{
        strcpy(d->serialpath,file_name);
     }
    return 0;
}

static int player_pause(IteFilter *f, void *arg){
    PlayerData *d=(PlayerData*)f->data;
    bool pause=*((bool*)arg);
    if(pause)
        d->state=Paused;
    else
        d->state=Playing;
    return 0;
}

static int player_IISbypass(IteFilter *f, void *arg){
    PlayerData *d=(PlayerData*)f->data;
    d->bypass=true;
    return 0;
}

static int player_loop(IteFilter *f, void *arg){
    PlayerData *d=(PlayerData*)f->data;
    int tmp=*((int*)arg);
    if(tmp<0)
        d->loop_after=-1;//play 1 time;
    else
        d->loop_after=1000;// >0 loop
    return 0;
}

static int player_get_rate(IteFilter *f, void *arg){
    PlayerData *d=(PlayerData*)f->data;
    *(int*)arg = d->rate;
    return 0;
}

static int player_get_channels(IteFilter *f, void *arg){
    PlayerData *d=(PlayerData*)f->data;
    *(int*)arg = d->nchannels;
    return 0;
}

static int player_get_status(IteFilter *f, void *arg){
    PlayerData *d=(PlayerData*)f->data;
    *(int*)arg = d->state;
    return 0;
}

static int player_set_finish(IteFilter *f, void *arg){
    PlayerData *d=(PlayerData*)f->data;
    d->finish = true;
	d->state = Dummy;
    return 0;
}

static void _excute_serialpath(IteFilter *f){
    PlayerData *d=(PlayerData*)f->data;
    I2S_DA32_WAIT_RP_EQUAL_WP(1);
    i2s_pause_DAC(1);
    player_set_filepath(f,(void*)d->serialpath);
    I2S_DA32_SET_WP(I2S_DA32_GET_RP());
    i2s_pause_DAC(0);    
}


//=============================================================================
//                              filter flow
//=============================================================================
static void player_init(IteFilter *f)
{
    PlayerData *d=(PlayerData*)ite_new(PlayerData,1);

    d->fd=-1;
    d->state=Closed;
    d->rate=8000;
    d->bitsize = 16;
    d->nchannels=1;
    d->codectype = -1;
    d->hsize=0;
    d->loop_after=-1; /*by default, don't loop*/
    d->pause_time=0;
    d->bypass=false;
	d->finish = false;
    sprintf(d->serialpath,"end");
    f->data=d;

}

static void player_uninit(IteFilter *f)
{
    PlayerData *d=(PlayerData*)f->data;
    if (d->fd>=0) player_close(f,NULL);
    free(d);
}

static void player_process(IteFilter *f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
    IteQueueblk blk ={0};
    PlayerData *d=(PlayerData*)f->data;
    int gbytes =20*(2*d->rate*d->nchannels)/1000;//20ms data 
    
    while(f->run) {
    	memset(&blk, 0, sizeof(IteQueueblk));
        IteAudioQueueController(f,0,30,5);

        //printf("%d\n",ite_queue_get_size(f->output[0].Qhandle));
        if (d->state==Playing){
            int err;
            mblk_ite *om=allocb_ite(gbytes);
            if (d->pause_time>0){
                err=gbytes;
                memset(om->b_wptr,0,gbytes);
                d->pause_time-=100;
            }else{
                err=read(d->fd,om->b_wptr,gbytes);
            }
            if (err>=0){
                if (err!=0){
                    if (err<gbytes)
                        memset(om->b_wptr+err,0,gbytes-err);
                    om->b_wptr+=gbytes;
                    blk.datap = om;
                    
                    ite_queue_put(f->output[0].Qhandle, &blk);
                    
                    //ms_queue_put(f->outputs[0],om);
                }else freemsg_ite(om);
                if (err<gbytes){
                    lseek(d->fd,d->hsize,SEEK_SET);

                    /* special value for playing file only once */
                    if (d->loop_after<0)
                    {
                        if(strcmp(d->serialpath,"end")!=0){//serial wav file play
                            _excute_serialpath(f);
                        }else{//Eof file play
                            d->state=Eof;
                            d->pause_time=50000;//50ms buffer time
                        }
                    }

                    if (d->loop_after>=0){
                        d->pause_time=d->loop_after;
                    }
                }
            }else{
                printf("error %s %d\n",__FUNCTION__,__LINE__);
            }
        }
        
        if(d->state==Eof){//add null time(data) prevent abnormal sound 
            int err;
            mblk_ite *om=allocb_ite(gbytes);
            if (d->pause_time>0){
                err=gbytes;
                memset(om->b_wptr,0,gbytes);
                om->b_wptr+=gbytes;
                d->pause_time-=100;
                blk.datap = om;
                blk.private1=Eofsound;
                ite_queue_put(f->output[0].Qhandle, &blk);
				d->state = Closed;
            }else{
                d->state=Paused;
                return;
            }
        }
        if(d->state == Dummy){
            mblk_ite *om=allocb_ite(gbytes);
            memset(om->b_wptr,0,gbytes);
            om->b_wptr+=gbytes;
            blk.datap = om;
			if(d->finish == true){
				blk.private2 = Eofmixsound;
				printf("send Eofmixsound\n");
				d->finish = false;
				d->state = Closed;
			}
            ite_queue_put(f->output[0].Qhandle, &blk);
        }

        //sem_post(&f->output[0].semHandle);
        usleep(100*GET_DA_RW_GAP/gbytes);
    }
    
    ite_mblk_queue_flush(f->output[0].Qhandle);
    
    return NULL;

}

static IteMethodDes player_methods[] = {
    {ITE_FILTER_FILEOPEN   , player_open },
    {ITE_FILTER_LOOPPLAY   , player_loop },
    {ITE_FILTER_PAUSE      , player_pause },
    {ITE_FILTER_SET_BYPASS , player_IISbypass},
    {ITE_FILTER_GETRATE    , player_get_rate},
    {ITE_FILTER_GET_NCHANNELS, player_get_channels},
    {ITE_FILTER_GET_STATUS, player_get_status},
    {ITE_FILTER_SET_VALUE, player_set_finish},
    {ITE_FILTER_SET_FILEPATH ,player_set_filepath},
    {0, NULL}
};

IteFilterDes FilterFilePlay = {
    ITE_FILTER_FILEPLAY_ID,
    player_init,
    player_uninit,
    player_process,
    player_methods
};



