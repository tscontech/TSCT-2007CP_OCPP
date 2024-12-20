#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"
#include "include/fileheader.h"
#include "i2s/i2s.h"
#include "ite/audio.h"


typedef struct _QWriteData{
   int blk_size;
}QWriteData;

typedef struct _qData{
    mblkq dataQ;
    pthread_mutex_t mutex;
	int blk_size;
}qData;
qData q_data;


void qwriteInit(int blk_size)
{
	q_data.blk_size = blk_size;
	//mblkqinit(&q_data.dataQ);
	mblkQShapeInit(&q_data.dataQ, q_data.blk_size * 10);
    pthread_mutex_init(&q_data.mutex, NULL);
}

void qwriteUnInit()
{
	pthread_mutex_lock(&q_data.mutex);
    //flushmblkq(&q_data.dataQ);
	mblkQShapeUninit(&q_data.dataQ);
    pthread_mutex_unlock(&q_data.mutex);
    pthread_mutex_destroy(&q_data.mutex);
}


int DataQGet(uint8_t* pdata)
{
	int ret = 0;
	mblk_ite *om;
    pthread_mutex_lock(&q_data.mutex);
    om=getmblkq(&q_data.dataQ);
    pthread_mutex_unlock(&q_data.mutex);
	if(om){
		memcpy(pdata, om->b_rptr, om->size);
		ret = om->size;
		freemsg_ite(om);
	}

	return ret;
}



//=============================================================================
//                              Private Function Declaration
//=============================================================================



static void qwrite_init(IteFilter *f)
{

}

static void qwrite_uninit(IteFilter *f)
{

}

static void qwrite_process(IteFilter *f)
{
    IteQueueblk blk ={0};
   
    while(f->run) {

#if 0
		pthread_mutex_lock(&q_data.mutex);
		if(getmblkqavail(&q_data.dataQ)<64){
			pthread_mutex_unlock(&q_data.mutex);
			
			if(ite_queue_get(f->input[0].Qhandle, &blk) == 0){
				pthread_mutex_lock(&q_data.mutex);
				//putmblkq(&q_data.dataQ, blk->datap);
				mblkQShapePut(&q_data.dataQ, blk.datap,q_data.blk_size);
				pthread_mutex_unlock(&q_data.mutex);
			}
		}
		else
			pthread_mutex_unlock(&q_data.mutex);
#else
		int s = 0;
		pthread_mutex_lock(&q_data.mutex);
		s = getmblkqavail(&q_data.dataQ);
		pthread_mutex_unlock(&q_data.mutex);
		if(s>64){
			printf("queue size %d\n", s);
		}
		

		if(ite_queue_get(f->input[0].Qhandle, &blk) == 0){
			pthread_mutex_lock(&q_data.mutex);
			//putmblkq(&q_data.dataQ, blk->datap);
			mblkQShapePut(&q_data.dataQ, blk.datap,q_data.blk_size);
			pthread_mutex_unlock(&q_data.mutex);
		}

#endif
        usleep(1000);
    }
    
    return NULL;

}

static IteMethodDes qwrite_methods[] = {
    {0, NULL}
};

IteFilterDes FilterQWrite = {
    ITE_FILTER_QWRITE_ID,
    qwrite_init,
    qwrite_uninit,
    qwrite_process,
    qwrite_methods
};



