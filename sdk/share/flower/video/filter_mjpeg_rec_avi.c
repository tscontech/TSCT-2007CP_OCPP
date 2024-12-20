#include <stdio.h>
#include "flower/flower.h"
#include "flower/fliter_priv_def.h"
#include "ite_avienc.h"

#define MAX_QUEUE_NUM	2

typedef struct {
    pthread_mutex_t mutex;
	int q_index;
    char filepath[DEF_FileStream_Name_LENGTH];	
	int width;
	int height;
	int fps;
	struct ite_avi_t *avi;
	int frame_count;
	bool bGetFirstSPSPPS;
	bool bGetFirstIFrame;
	bool bGetFirstAudio;
} RecorderAVI;

typedef void (*pfPktReleaseCb) (void *pkt);

typedef struct VideoInputPkt {
    uint8_t *pInputBuffer;
    uint32_t bufferSize;
} VideoInputPkt;

typedef struct AudioInputPkt {
    uint8_t *pInputBuffer;
    uint32_t bufferSize;
} AudioInputPkt;

typedef struct QueuePktList {
    void* pkt;
    struct QueuePktList *next;
} QueuePktList;

typedef struct PktQueue {
    QueuePktList *firstPkt, *lastPkt;
    int numPackets;
    int maxPackets;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pfPktReleaseCb pfPktRelease;
} PktQueue;

static pthread_t tid;
static bool b_RECORDING = false;
static bool b_OpenFileReady = false;
static int gAudioStartPoint = 0;
PktQueue gVideoInputQueue[MAX_QUEUE_NUM];
PktQueue gAudioInputQueue[MAX_QUEUE_NUM];
static int q_index = 0;

static pfPktReleaseCb
_videoInputPktRelease(
    void* pkt)
{
    VideoInputPkt *ptVideoInputPkt = (VideoInputPkt*)pkt;
    if (ptVideoInputPkt && ptVideoInputPkt->pInputBuffer && ptVideoInputPkt->bufferSize)
    {
        free(ptVideoInputPkt->pInputBuffer);
    }
    free(pkt);
}

static pfPktReleaseCb
_audioInputPktRelease(
    void* pkt)
{
    AudioInputPkt *ptAudioInputPkt = (AudioInputPkt*)pkt;
    if (ptAudioInputPkt && ptAudioInputPkt->pInputBuffer && ptAudioInputPkt->bufferSize)
    {
        free(ptAudioInputPkt->pInputBuffer);
    }
    free(pkt);
}

static int
_packetQueuePut(
    PktQueue *q,
    void *pkt)
{
    QueuePktList *pkt1;

    /* duplicate the packet */
    if (q->mutex)
    {
    	while(q->numPackets > q->maxPackets)
		{
			printf("queue is full: cur: %u, max: %u\n", q->numPackets, q->maxPackets);
			usleep(1000*100);
		}
        pkt1 = malloc(sizeof(QueuePktList));
        if (!pkt1) return -1;
        pkt1->pkt =  pkt;
        pkt1->next = NULL;

        pthread_mutex_lock(&q->mutex);

        if (!q->lastPkt)
            q->firstPkt = pkt1;
        else
            q->lastPkt->next = pkt1;
        q->lastPkt = pkt1;
        q->numPackets++;

        /* XXX: should duplicate packet data in DV case */
        pthread_cond_signal(&q->cond);
        pthread_mutex_unlock(&q->mutex);
        return 0;
    }
    else
    {
        return -1;
    }
}

static int
_packetQueueGet(
    PktQueue *q,
    void **pkt,
    int block)
{
    QueuePktList *pkt1;
    int ret;
    if (q->mutex)
    {
        pthread_mutex_lock(&q->mutex);

        for (;;)
        {
            pkt1 = q->firstPkt;
            if (pkt1)
            {
                q->firstPkt = pkt1->next;
                if (!q->firstPkt)
                    q->lastPkt = NULL;
                q->numPackets--;
                *pkt = pkt1->pkt;
                free(pkt1);
                ret = 1;
                break;
            }
            else if (!block)
            {
                ret = 0;
                break;
            }
            else
            {
                pthread_cond_wait(&q->cond, &q->mutex);
            }
        }
        pthread_mutex_unlock(&q->mutex);
        return ret;
    }
    else
    {
        return -1;
    }
}


/* packet queue handling */
static void
_packetQueueInit(
    PktQueue *q,
    void* pfPktRelease,
    int maxPackets)
{
    memset(q, 0, sizeof(PktQueue));
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
    q->maxPackets = maxPackets;
    q->pfPktRelease = pfPktRelease;
}

static void
_packetQueueFlush(
    PktQueue *q)
{
    QueuePktList *pkt, *pkt1;
    if (q->mutex)
    {
        pthread_mutex_lock(&q->mutex);
        for (pkt = q->firstPkt; pkt != NULL; pkt = pkt1) {
            pkt1 = pkt->next;
            if (q->pfPktRelease)
            {
                q->pfPktRelease(pkt->pkt);
            }
            free(pkt);
        }
        q->lastPkt = NULL;
        q->firstPkt = NULL;
        q->numPackets = 0;
        pthread_mutex_unlock(&q->mutex);
    }
}

static void
_packetQueueEnd(
    PktQueue *q)
{
    if (q->mutex)
    {
        _packetQueueFlush(q);
        pthread_mutex_destroy(&q->mutex);
        pthread_cond_destroy(&q->cond);
        memset(q, 0, sizeof(PktQueue));
    }
}

static void *AVMuxToAVI(void *arg)
{
	//char* filepath = (char*) arg;
	RecorderAVI *s = (RecorderAVI *)arg;
	struct ite_avi_t *avi;
	struct ite_avi_audio_t auds;
	VideoInputPkt *ptVideoPkt = NULL;
	AudioInputPkt *ptAudioPkt = NULL;
	int frame_count = 0;
	char filepath[DEF_FileStream_Name_LENGTH];
	int width, height, fps, q_index;

	strcpy(filepath, s->filepath);
	width = s->width;
	height = s->height;
	fps = s->fps;
	q_index = s->q_index;
	
	auds.bits = 16;
    auds.channels = 1;
    auds.samples_per_second = 8000;  

	avi = ite_avi_open(filepath, width, height, "MJPG", fps, &auds);

	printf("Encode to AVI Start, Filepath = %s\n", filepath);
	usleep(3000);
	b_OpenFileReady = true;
    while(1)
    {
    	if(_packetQueueGet(&gVideoInputQueue[q_index], (void**) &ptVideoPkt, 0) > 0)
        {
			//if((ptVideoPkt->pInputBuffer[4] & 0xf) == 0x5 || (ptVideoPkt->pInputBuffer[4] & 0xf) == 0x1)
            frame_count++;
            ite_avi_add_frame(avi, ptVideoPkt->pInputBuffer, ptVideoPkt->bufferSize);
            _videoInputPktRelease(ptVideoPkt);      
            if((frame_count%fps == 0 && frame_count != 0) && 
				(frame_count > gAudioStartPoint && gAudioStartPoint != 0))
            {
                while(1)
                {
                    if (_packetQueueGet(&gAudioInputQueue[q_index], (void**) &ptAudioPkt, 0) > 0)
                    {
                        ite_avi_add_audio(avi, ptAudioPkt->pInputBuffer, ptAudioPkt->bufferSize);
                        _audioInputPktRelease(ptAudioPkt);
                        break;
                    }
					usleep(1000);
                }
            }
			else if(frame_count%fps == 0 && frame_count != 0)
			{
				char *fake_audio = malloc(16000);
				memset(fake_audio, 0, 16000);
				ite_avi_add_audio(avi, fake_audio, 16000);
				free(fake_audio);
			}
        }
        if(b_RECORDING == false && gVideoInputQueue[q_index].numPackets == 0)
        {
            break;
        }
    	usleep(1000);
    }
	ite_avi_close(avi);
    printf("Mux to AVI File Finish.\n");
	usleep(100*1000);
	_packetQueueEnd(&gVideoInputQueue[q_index]);
	_packetQueueEnd(&gAudioInputQueue[q_index]);
	
    pthread_exit(NULL);
}

static void rec_avi_open(IteFilter *f, void *arg)
{
	RecorderAVI *s = (RecorderAVI *)f->data;
	VideoMemoInfo *info = (VideoMemoInfo  *)arg;

    pthread_mutex_lock(&s->mutex);
	printf("##### AVI recorder start , q_index = %d######\n", q_index);
	
	_packetQueueInit(&gVideoInputQueue[q_index], _videoInputPktRelease, 150);
	_packetQueueInit(&gAudioInputQueue[q_index], _audioInputPktRelease, 50);

	s->q_index = q_index;
    strcpy(s->filepath, (char*)info->videomemo_file);
	s->width = info->video_width;
	s->height = info->video_height;
	s->fps = info->video_fps;

 	s->bGetFirstSPSPPS = s->bGetFirstIFrame = s->bGetFirstAudio = false;
	s->frame_count = 0;	
	b_RECORDING = true;
	gAudioStartPoint = 0;
	pthread_create(&tid, NULL, AVMuxToAVI, s);
	pthread_detach(tid);
    pthread_mutex_unlock(&s->mutex);
}

static void rec_avi_close(IteFilter *f)
{
	RecorderAVI *s = (RecorderAVI *)f->data;
	pthread_mutex_lock(&s->mutex);
	b_RECORDING = false;
	b_OpenFileReady = false;
	q_index = !q_index;
	pthread_mutex_unlock(&s->mutex);
	//ite_avi_close(s->avi);
	//pthread_join(tid, NULL);
	printf("##### AVI recorder stop ######\n");
}

static void mjpeg_rec_avi_init(IteFilter *f)
{
	RecorderAVI *s = (RecorderAVI *)ite_new(RecorderAVI,1);
	DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
	
	pthread_mutex_init(&s->mutex, NULL);
	
	f->data = s;
}

static void mjpeg_rec_avi_uninit(IteFilter *f)
{
	RecorderAVI *s = (RecorderAVI *)f->data;
	DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

	pthread_mutex_destroy(&s->mutex); 
    free(s);
}

static void mjpeg_rec_avi_process(IteFilter *f)
{
	RecorderAVI *s = (RecorderAVI *)f->data;
	IteQueueblk blk_input0 = {0};
	IteQueueblk blk_input1 = {0};
	mblk_ite *m = NULL;
	char *v_bitstream;
	char *a_bitstream;
	int v_bitstream_size = 0;
	int a_bitstream_size = 0;
	DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
	while (f->run)
    {
    	pthread_mutex_lock(&s->mutex);
    	if(ite_queue_get(f->input[0].Qhandle, &blk_input0) == 0) 
		{
			m = (mblk_ite*)blk_input0.datap;
			//printf("%x %x %x %x %x\n", m->b_rptr[0], m->b_rptr[1], m->b_rptr[2], m->b_rptr[3], m->b_rptr[4]);
			if(b_OpenFileReady)
			{
				v_bitstream = malloc(m->b_wptr - m->b_rptr);
				memcpy(v_bitstream, m->b_rptr, m->b_wptr - m->b_rptr);
				v_bitstream_size = m->b_wptr - m->b_rptr;

				if(v_bitstream)
				{
					VideoInputPkt *videoPkt = (VideoInputPkt*) malloc(sizeof(VideoInputPkt));
			        if (videoPkt)
			        {
			            videoPkt->pInputBuffer =v_bitstream;
			            videoPkt->bufferSize = v_bitstream_size;
			            _packetQueuePut(&gVideoInputQueue[s->q_index], videoPkt);
			        }

				}
				s->frame_count++;

				if (m)
            		freemsg_ite(m);
				m = NULL;
			}
			else
			{
				if (m)
            		freemsg_ite(m);
				m = NULL;
			}
    	}
		if(f->input[1].Qhandle)
		{		
			if(ite_queue_get(f->input[1].Qhandle, &blk_input1) == 0)
			{
				m = (mblk_ite*)blk_input1.datap;
				//printf("%x %x %x %x %x, size = %d\n", m->b_rptr[0], m->b_rptr[1], m->b_rptr[2], m->b_rptr[3], m->b_rptr[4], m->b_wptr - m->b_rptr);
				if(b_OpenFileReady && s->frame_count > 0)
				{
					if(s->bGetFirstAudio == false)
					{
						gAudioStartPoint = s->frame_count;
						s->bGetFirstAudio = true;
					}
					
					if(a_bitstream_size == 0)
					{
						a_bitstream = malloc(16000);
					}
					memcpy(a_bitstream + a_bitstream_size, m->b_rptr, m->b_wptr - m->b_rptr);
					a_bitstream_size += m->b_wptr - m->b_rptr;

					//put into audio packetqueue
					if(a_bitstream_size == 16000)
					{
						AudioInputPkt *audioPkt = (AudioInputPkt*) malloc(sizeof(AudioInputPkt));
				        if (audioPkt)
				        {
				            audioPkt->pInputBuffer =a_bitstream;
				            audioPkt->bufferSize = a_bitstream_size;
				            _packetQueuePut(&gAudioInputQueue[s->q_index], audioPkt);
							a_bitstream_size = 0;
						}

					}	
				
					if (m)
            			freemsg_ite(m);
					m = NULL;
				}
				else
				{
					if(a_bitstream_size > 0)
					{
						AudioInputPkt *audioPkt = (AudioInputPkt*) malloc(sizeof(AudioInputPkt));
				        if (audioPkt)
				        {
				            audioPkt->pInputBuffer = a_bitstream;
				            audioPkt->bufferSize = a_bitstream_size;
				            _packetQueuePut(&gAudioInputQueue[s->q_index], audioPkt);
							a_bitstream_size = 0;
				        }
					}
					if (m)
	            		freemsg_ite(m);
					m = NULL;
				}
    		}
		}
		pthread_mutex_unlock(&s->mutex);
		usleep(1000);
	}
}

static IteMethodDes mjpeg_rec_avi_methods[] = {
    {ITE_FILTER_MJPEG_REC_AVI_OPEN , rec_avi_open},
    {ITE_FILTER_MJPEG_REC_AVI_CLOSE , rec_avi_close},
    {0, NULL}
};

IteFilterDes FilterMJpegRecAVI = {
    ITE_FILTER_MJPEG_REC_AVI_ID,
    mjpeg_rec_avi_init,
    mjpeg_rec_avi_uninit,
    mjpeg_rec_avi_process,
    mjpeg_rec_avi_methods,
};


