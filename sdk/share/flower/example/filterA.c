#include <stdio.h>
#include "flower/flower.h"

//=============================================================================
//                              Constant Definition
//=============================================================================

//=============================================================================
//                              Private Function Declaration
//=============================================================================

static void test_init(IteFilter *f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
}

static void test_uninit(IteFilter *f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
}

static void test_process(IteFilter *f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

#if 1
    while(f->run) {
        printf("[%s] Filter(%d). thread run\n", __FUNCTION__, f->filterDes.id);
        sleep(3);
    }
#endif

#if 0
    IteQueueblk blk;
    unsigned char data;
    
    data = 0x1;
    blk.datap = (mblk_ite *) malloc(sizeof(mblk_ite));
    blk.datap->b_datap = &data;
   
    while(f->run) {
        if(ite_queue_put(f->output[0].Qhandle, &blk) == 0) {
            printf("[%s] FilterA put data\n", __FUNCTION__);
            
            sem_post(&f->output[0].semHandle);
        }
        sleep(3);
    }
#endif

#if 0
    IteQueueblk blk;
    uint8_t Day[7][10] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
    int i = 0;

    memset(blk.streamBuffer, 0, sizeof(uint8_t) * 20);
    blk.data = 0x0;
  
    while(f->run) {
        memcpy(blk.streamBuffer, Day[i], sizeof(uint8_t) * strlen(Day[i]));
        blk.data = blk.data + 1;
        printf("[FilterA] PUT buffer=%s size=%d\n", blk.streamBuffer, strlen(blk.streamBuffer));

        ite_queue_put(f->output[0].Qhandle, &blk);
        
        i++;
        if(i == 6)
            i = 0;

        memset(blk.streamBuffer, 0, sizeof(uint8_t) * 20);
        
        sleep(25);      
    }
#endif

    DEBUG_PRINT("[%s] Filter(%d) end\n", __FUNCTION__, f->filterDes.id);
    
    return NULL;
}

static void test_method(IteFilter *f, void *arg)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
}

static IteMethodDes FilterA_methods[] = {
    {ITE_FILTER_A_Method, test_method},
    {0, NULL}
};

IteFilterDes FilterA = {
    ITE_FILTER_A_ID,
    test_init,
    test_uninit,
    test_process,
    FilterA_methods
};



