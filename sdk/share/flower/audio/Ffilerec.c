#include <stdio.h>
#include <pthread.h>
#include "flower/flower.h"
#include "flower/ite_queue.h"
#include "include/fileheader.h"

//=============================================================================
//                              struct Definition
//=============================================================================
typedef struct RecState{
	int fd;
    pthread_mutex_t lock;
	int rate;
    int channels;
    unsigned int size;
    RecorderState state;
} RecState;
//=============================================================================
//                              Private Function Declaration
//=============================================================================

static void write_wav_header(int fd, int rate,int channels,int size){
	wave_header_t header;
	memcpy(&header.riff_chunk.riff,"RIFF",4);
	header.riff_chunk.len=le_uint32(size+36);
	memcpy(&header.riff_chunk.wave,"WAVE",4);

	memcpy(&header.format_chunk.fmt,"fmt ",4);
	header.format_chunk.len=le_uint32(0x10);
	header.format_chunk.type=le_uint16(0x1);
	header.format_chunk.channel=le_uint16(channels);
	header.format_chunk.rate=le_uint32(rate);
    header.format_chunk.bps=le_uint32(rate*2);
	header.format_chunk.blockalign=le_uint16(2);
	header.format_chunk.bitpspl=le_uint16(16);

	memcpy(&header.data_chunk.data,"data",4);
	header.data_chunk.len=le_uint32(size);
	lseek(fd,0,SEEK_SET);
	if (write(fd,&header,sizeof(header))!=sizeof(header)){
		printf("Fail to write wav header.");
	}
}

static int rec_close(IteFilter *f, void *arg){
	RecState *s=(RecState*)f->data;
	pthread_mutex_lock(&s->lock);
	s->state=RecorderClosed;
	if (s->fd>=0){
	    write_wav_header(s->fd,s->rate,s->channels,s->size);
		close(s->fd);
		s->fd=-1;
	}
	pthread_mutex_unlock(&s->lock);
	return 0;
}

static int rec_open(IteFilter *f, void *arg){
	RecState *s=(RecState*)f->data;
	const char *filename=(const char*)arg;
	if (s->fd>=0) rec_close(f,NULL);
	pthread_mutex_lock(&s->lock);
	s->fd=open(filename,O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
	if (s->fd<0){
		printf("Cannot open %s\n",filename);
		pthread_mutex_unlock(&s->lock);
		return -1;
	}
	s->state=RecorderPaused;
	pthread_mutex_unlock(&s->lock);
	return 0;
}

static int rec_start(IteFilter *f, void *arg){
	RecState *s=(RecState*)f->data;
	pthread_mutex_lock(&s->lock);
	s->state=RecorderRunning;
	pthread_mutex_unlock(&s->lock);
	return 0;
}

static int rec_stop(IteFilter *f, void *arg){
	RecState *s=(RecState*)f->data;
	pthread_mutex_lock(&s->lock);
	s->state=RecorderPaused;
	pthread_mutex_unlock(&s->lock);
	return 0;
}

static void set_rate(IteFilter *f, void *arg)
{
    RecState *s=(RecState*)f->data;
    s->rate=*((int*)arg);
}

static void set_channels(IteFilter *f, void *arg)
{
    RecState *s=(RecState*)f->data;
    s->channels=*((int*)arg);
}

static void set_state(IteFilter *f, void *arg)
{   
	RecState *s=(RecState*)f->data;
	pthread_mutex_lock(&s->lock);
    s->state=*((int*)arg);
	pthread_mutex_unlock(&s->lock);
}

static void get_state(IteFilter *f, void *arg)
{
    RecState *s=(RecState*)f->data;
    *((RecorderState*)arg)=s->state;
}

//=============================================================================
//                              filter flow
//=============================================================================
static void rec_init(IteFilter *f)
{
    RecState *s=ite_new(RecState,1);
    s->fd=-1;
    s->rate=8000;
    s->channels=1;
	s->size=0;
	s->state=RecorderClosed;
    pthread_mutex_init(&s->lock, NULL);
    f->data=s;
}

static void rec_uninit(IteFilter *f)
{
    RecState *s=(RecState*)f->data;
    if (s->fd>=0) rec_close(f,NULL);
    pthread_mutex_destroy(&s->lock);
    free(s);
}

static void rec_process(IteFilter *f)
{
    RecState *s=(RecState*)f->data;
    IteQueueblk blk={0};
    int err;
    if(s->state!=RecorderClosed) rec_start(f,NULL);
    while(f->run) {
        //sem_wait(&f->input[0].semHandle);
        if(ite_queue_get(f->input[0].Qhandle, &blk) == 0){
            mblk_ite *om=blk.datap;
            
            if(s->state==RecorderRunning){
                int len = om->size;
                if ((err=write(s->fd,om->b_rptr,len))!=len){
                    if (err<0)
                        printf("fail to write %i\n",len);
                }
                s->size+=len;
            }
            
            if(om) freemsg_ite(om);
            
        }else{
            usleep(20000);
        }
    }
}



static IteMethodDes rec_methods[] = {
    //{ITE_FILTER_REC, rec_method},
    {ITE_FILTER_FILEOPEN , rec_open},
    {ITE_FILTER_FILECLOSE, rec_close},
    {ITE_FILTER_SETRATE , set_rate},
    //{ITE_FILTER_GET_STATE, get_state},
    //{ITE_FILTER_SET_STATE, set_state},
    {0, NULL}
};

IteFilterDes FilterFileRec = {
    ITE_FILTER_FILEREC_ID,
    rec_init,
    rec_uninit,
    rec_process,
    rec_methods
};
