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
#define DrvDumpPCM_RdPtr        (0xB0200080|0x18)
#define BUFSIZE  (512)//512
extern int gbytes;

typedef struct _QuickPlayData{
    void *fd;
    PlayerState state;
    int rate;
    int nchannels;
    int loop_after;
    int codecType;
	bool is_insert;
	bool is_busy;
	int availSize;
	cb_sound_t fn_cb;

    bool fileRead;
    char serialpath[256];
	char fileName[256];
}QuickPlayData;
//=============================================================================
//                              Private Function Declaration
//=============================================================================

static int audioCodecType(char* filename)
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

static int quickplay_stop(IteFilter *f, void *arg){
    QuickPlayData *d=(QuickPlayData*)f->data;
    
    return 0;
}

static int quickplay_close(IteFilter *f, void *arg){
    QuickPlayData *d=(QuickPlayData*)f->data;
    d->state=Closed;
    if (d->fd)
    {
        fclose(d->fd);
        //castor3snd_reinit_for_video_memo_play();
    }
    d->fd=NULL;
	memset(d->fileName, 0, 256*sizeof(char));

    return 0;
}

 

static void fileplay_do(IteFilter *f){
    int availSize =0;
    char buf[BUFSIZE];
    QuickPlayData *d=(QuickPlayData*)f->data;
    int rbytes= 0;
	
	if(d->fd)
		rbytes = fread(buf,1,BUFSIZE,d->fd);

    if(rbytes==0) {
        d->fileRead=false;
        printf("finish file read\n");
		fseek(d->fd,0,SEEK_SET);

		if(d->loop_after < 0){
			quickplay_close(f,NULL);//d->state=Closed;
			iteAudioStopQuick();
			i2s_deinit_DAC();
			if(d->fn_cb){
				if(d->is_insert == false)
					d->fn_cb(Eofsound, NULL);
				else
					d->fn_cb(Eofmixsound, NULL);
			}
			usleep(20000);
		}
        return;
    }
		
    do{//wait codec available buff size
        iteAudioGetAvailableBufferLength(ITE_AUDIO_OUTPUT_BUFFER, &availSize);
		d->availSize = availSize;
        usleep(1000);
        iteAudioUpdateMessageQ();

    }while(availSize<=rbytes);

    iteAudioWriteStream(buf, rbytes);//write data for decoder

}

static int quickplay_codec_init(IteFilter *f, void *arg){

	QuickPlayData *d=(QuickPlayData*)f->data;
    const char *file=(const char*)arg;

	if(d->fd == NULL){printf("d->fd == NULL\n"); return 0;}	

	d->codecType=audioCodecType(file);
	iteAudioStopQuick();
	i2s_deinit_DAC();
	iteAudioOpenEngine(d->codecType);//load codec engine
	iteAudioSetMusicCodecDump(0);//set dump flag
	usleep(20000);
	d->state=Playing;
	d->fileRead=true;

	return 0;

}

static int quickplay_open(IteFilter *f, void *arg){
    QuickPlayData *d=(QuickPlayData*)f->data;
    const char *file=(const char*)arg;
	
    int tmp=0, cnt=0;
    quickplay_close(f,NULL);
    d->fd = fopen(file, "rb");
    if(d->fd==NULL) {
        printf("openfile error\n");
        return 0;
    }
	sprintf(d->fileName,file);

	quickplay_codec_init(f,file);
	
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

static int quickplay_pause(IteFilter *f, void *arg){
    QuickPlayData *d=(QuickPlayData*)f->data;
    bool pause=*((bool*)arg);
    if(pause){
    	d->state=Paused;
    }
	else{
		 //d->state=Playing;
		iteAudioOpenEngine(d->codecType);
		//usleep(20000);
		d->state=Playing;
		  
	}
   
    return 0;
}

static int quickplay_set_insert(IteFilter *f, void *arg){
    QuickPlayData *d=(QuickPlayData*)f->data;
	d->is_insert = true;
   // d->bypass=true;
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

static int quickplay_get_status(IteFilter *f, void *arg){
    QuickPlayData *d=(QuickPlayData*)f->data;
	//printf("d->is_busy = %d  d->availSize %d\n", d->is_busy, d->availSize);
    *(bool*)arg = d->is_busy;
    return 0;
}


//=============================================================================
//                              filter flow
//=============================================================================

static void quickplay_init(IteFilter *f)
{
    QuickPlayData *d=(QuickPlayData*)ite_new(QuickPlayData,1);
    d->fd=NULL;
    d->state=Closed;
    d->rate=8000;
    d->nchannels=1;
    d->loop_after=-1; /*by default, don't loop*/
    //d->bypass=false;
    f->data=d;
    d->codecType=-1;
    d->fileRead=true;
	d->is_insert=false;
	d->is_busy = false;
	d->fn_cb = NULL;
    sprintf(d->serialpath,"end");
}

static void quickplay_uninit(IteFilter *f)
{
    QuickPlayData *d=(QuickPlayData*)f->data;
    quickplay_close(f,NULL);
	iteAudioStopQuick();
	i2s_deinit_DAC();
    free(d);
}

static void quickplay_process(IteFilter *f)
{
    QuickPlayData *d=(QuickPlayData*)f->data;
    gbytes =20*(2*d->rate*d->nchannels)/1000;//20ms data 

    while(f->run) {
        
        //if(IteAudioQueueController(f,0,30,5)==-1) continue;
        if (d->state==Playing){
			d->is_busy = true;
            if(d->fileRead){
                fileplay_do(f);
				
            }else{
            	if(d->loop_after >= 0){
	            	printf("play repeat\n");
	                //fseek(d->fd,0,SEEK_SET);
	                d->fileRead=true;
	                continue;
            	}
            }

        }
		else if(d->state==Paused){
			d->is_busy = false;
		}

        usleep(1000*GET_DA_RW_GAP/gbytes);
    }
    
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
    {ITE_FILTER_SET_CB, quickplay_set_cb},
    {ITE_FILTER_GET_STATUS, quickplay_get_status},
    {0, NULL}
};

IteFilterDes FilterQuickPlay = {
    ITE_FILTER_QUICKPLAY_ID,
    quickplay_init,
    quickplay_uninit,
    quickplay_process,
    quickplay_methods
};



