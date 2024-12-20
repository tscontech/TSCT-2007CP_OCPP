#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"

//=============================================================================
//                              Constant Definition
//=============================================================================
#define MEASUREMENTS

//=============================================================================
//                              Private Function Declaration
//=============================================================================
typedef struct Fvad Fvad;
typedef struct C_Compound_Type C_Compound_Type;
extern C_Compound_Type *ITE_WFST_Init(Fvad **vs);
extern int ITE_WFST_Decoder(Fvad **vs, short *in, C_Compound_Type *asr);
extern void ITE_WFST_Destroy(Fvad **vs, C_Compound_Type *asr);
Fvad *vs;

typedef struct ASRstate
{
    IteFilter *msF;
    int framesize;
    cb_sound_t fn_cb;
    bool isbypass;
    bool threadstart;
    C_Compound_Type *asr;
    pthread_t asr_thread;	   
    pthread_mutex_t mutex;
    mblkq dataQ;
    
} ASRstate;

static void *asr_thread(void *arg) {
#ifdef MEASUREMENTS
    uint32_t start_cnt = 0;
    uint32_t count = 1;
    uint64_t diff = 0;
#endif
    ASRstate *s=(ASRstate*)arg;
    int nbytes = s->framesize;    
    while(s->threadstart){
        mblk_ite *m;
        
        if(s->dataQ.qcount>400){//Qdata over 400*80(sample)/8000 = 4sec flush Q
            pthread_mutex_lock(&s->mutex);
            flushmblkq(&s->dataQ);
            pthread_mutex_unlock(&s->mutex);
        }
            
        pthread_mutex_lock(&s->mutex);
        m=getmblkq(&s->dataQ);
        pthread_mutex_unlock(&s->mutex);
        
        if(m){
            #ifdef MEASUREMENTS
            start_cnt = itpGetTickCount();
            #endif
            ITE_WFST_Decoder(&vs, m->b_rptr, s->asr);
            #ifdef MEASUREMENTS
            count++;
            diff += itpGetTickDuration(start_cnt);
            if (count % 200 == 0)
            {
                printf("ASR 200 time %d nQ=%d\n", (int)diff,s->dataQ.qcount);
                diff = 0;
            }
            #endif
            if(m) freemsg_ite(m);
        }
        usleep(2000);
    }
}

static void init_asr_thread(ASRstate *e){
    pthread_attr_t attr;
    struct sched_param param;
    e->asr = ITE_WFST_Init(&vs);
    e->threadstart=true;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 25*1024);       
    param.sched_priority = sched_get_priority_min(0);
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&e->asr_thread, &attr, asr_thread, e);
}

static void asr_stop_hwthread(ASRstate *e){
    e->threadstart=false;
    pthread_join(e->asr_thread,NULL);
    ITE_WFST_Destroy(&vs, e->asr);
}

static void asr_process(IteFilter *f)
{
    ASRstate *s = (ASRstate *)f->data;
    IteQueueblk blk = {0};

    while (f->run)
    {
        if (ite_queue_get(f->input[0].Qhandle, &blk) == 0)
        {
            mblk_ite *om = blk.datap;
            pthread_mutex_lock(&s->mutex);
            mblkQShapePut(&s->dataQ,om,80*2);
            pthread_mutex_unlock(&s->mutex);
        }
        usleep(20000);
    }

    ite_mblk_queue_flush(f->input[0].Qhandle);
    return NULL;
}

static void asr_init(IteFilter *f)
{
    ASRstate *s = (ASRstate *)ite_new(ASRstate, 1);
    f->data = s;
    s->framesize = 80;
    s->fn_cb = NULL;
    s->isbypass = false;
    s->msF = f;
    mblkQShapeInit(&s->dataQ,s->framesize * 12);
    //Wanson_ASR_Init();
    pthread_mutex_init(&s->mutex,NULL);
    init_asr_thread(s);
    printf("asr_init\n");
}

static void asr_uninit(IteFilter *f)
{
    ASRstate *s = (ASRstate *)f->data;
    asr_stop_hwthread(s);
    pthread_mutex_destroy(&s->mutex);
    mblkQShapeUninit(&s->dataQ);
    free(s);
}

static void asr_set_cb(IteFilter *f, cb_sound_t fnc)
{
    ASRstate *s = (ASRstate *)f->data;
    s->fn_cb = fnc;
    if (!s->isbypass)
    {
        Castor3snd_reinit_for_diff_rate(8000, 16, 1);
    }
}

static void asr_set_bypass(IteFilter *f, void *arg)
{
    ASRstate *s = (ASRstate *)f->data;
    s->isbypass = true;
}

static IteMethodDes asr_methods[] = {
    {ITE_FILTER_SET_CB, asr_set_cb},
    {ITE_FILTER_SET_BYPASS, asr_set_bypass},
    {0, NULL}};

IteFilterDes FilterAsr = {
    ITE_FILTER_ASR_ID,
    asr_init,
    asr_uninit,
    asr_process,
    asr_methods};
