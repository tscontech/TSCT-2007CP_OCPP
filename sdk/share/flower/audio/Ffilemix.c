#include <stdio.h>
#include "flower/flower.h"
#include "flower/ite_queue.h"
#include "include/fileheader.h"

#define max_file 10
//=============================================================================
//                              struct Definition
//=============================================================================
typedef struct _Mixdata{
    unsigned char* pcm_buf;
    char mixfile[256];
    unsigned int mix_size;
    unsigned int end;
    float mixlevel;
    float baselevel;
    int repeatN;
    bool flag;
    bool fadingIN;
    bool fadingOUT;
    pthread_mutex_t lock;
}Mixdata;
//=============================================================================
//                              Function Declaration
//=============================================================================
static inline int16_t saturate(int val) {
	return (val>32767) ? 32767 : ( (val<-32767) ? -32767 : val);
}

static int mix_fileopen(IteFilter *f, void *arg){
    Mixdata *d=(Mixdata*)f->data;
    char filepath[128];  
    FILE *fp[max_file];
    const char *file_name;
    unsigned int wpidx=0;    
    int data_size[max_file];    
    int file_num=0;
    int i,j;
    sprintf(filepath,(char *)arg);
    pthread_mutex_lock(&d->lock);
    if(strcmp(d->mixfile,filepath)==0){
        d->flag = true;
        d->fadingIN  =true;
        d->fadingOUT =false;
        d->baselevel= 1.0;
        pthread_mutex_unlock(&d->lock);
        return 0;
    }
    //d->mixfile = filepath;
    strcpy(d->mixfile, filepath);
    d->mix_size = 0;
    file_name = strtok(filepath, " ");
    while (file_name != NULL){
        char* buf;
        fp[file_num] = fopen(file_name,"rb");
        
        if( NULL == fp[file_num] ){
            printf( "[mixerror] %s open failure\n",file_name);
            pthread_mutex_unlock(&d->lock);
            return 0;
        }
        
        buf = malloc(50);
        fread(buf, 1, 44, fp[file_num]);
        data_size[file_num] = *((int*)&buf[40]);
        d->mix_size += data_size[file_num];
        file_name = strtok(NULL, " ");
        free(buf);
        file_num++;
    }
    if(d->pcm_buf) free(d->pcm_buf);
    d->pcm_buf = (unsigned char*)malloc(d->mix_size);
    for(j=0;j<file_num;j++){
        unsigned char* file_pcm;
        file_pcm = (unsigned char*)malloc(data_size[j]);
        fread(file_pcm, 1, data_size[j], fp[j]);
        memmove(d->pcm_buf+wpidx,file_pcm,data_size[j]);
        wpidx += data_size[j];
        free(file_pcm);
        fclose(fp[j]);
    }
    
    d->flag = true;
    d->fadingIN  =true;
    d->fadingOUT =false;
    d->baselevel= 1.0;
    pthread_mutex_unlock(&d->lock);
    return 0;
} 

static int mix_set_level(IteFilter *f, void *arg){
    Mixdata *d=(Mixdata*)f->data;
    pthread_mutex_lock(&d->lock);
    d->mixlevel=*((double*)arg);
    pthread_mutex_unlock(&d->lock);  
    return 0;
}

static int mix_set_repeat(IteFilter *f, void *arg){
    Mixdata *d=(Mixdata*)f->data;
    if(d->flag) d->flag=false;
    pthread_mutex_lock(&d->lock);
    d->repeatN=*((int*)arg);//repeatN=-1 :repeat forever ,1:NO repeat ,>0 repeat N times
    if(d->repeatN==0){
        d->end=0;
        d->fadingOUT=true;
    }
    pthread_mutex_unlock(&d->lock);    
    return 0;    
}

static void mix_init(IteFilter *f)
{
     Mixdata *d=(Mixdata*)ite_new(Mixdata,1);
    d->flag =false;
    d->fadingIN =false;
    d->fadingOUT =false;
    d->end = 0;
    d->mix_size = 0;
    d->pcm_buf = NULL;
    d->mixlevel = 1.0;
    d->baselevel= 1.0;
    d->repeatN  = 1;
    pthread_mutex_init(&d->lock, NULL);
    //d->mixfile = "null";
    f->data=d;
}

static void mix_uninit(IteFilter *f)
{
    Mixdata *d=(Mixdata*)f->data;
    d->flag =false;
    d->end = 0;
    d->mix_size = 0;
    d->repeatN = 0;
    free(d->pcm_buf);
    d->pcm_buf = NULL;
    pthread_mutex_destroy(&d->lock);
    free(d);
}

static void mix_process(IteFilter *f)
{
    IteQueueblk blk ={0};
    Mixdata *d=(Mixdata*)f->data;
    mblk_ite *m;

    while(f->run){
    
        IteAudioQueueController(f,0,30,5);
    
        if(d->flag){
            //sem_wait(&f->input[0].semHandle);
            if(ite_queue_get(f->input[0].Qhandle, &blk) == 0){
                mblk_ite *o;
                m=blk.datap;

                o=allocb_ite(m->size);
                if(d->fadingIN){
                    //printf("size %d ",m->b_wptr-m->b_rptr);
                    for(;m->b_rptr<m->b_wptr;m->b_rptr+=2,o->b_wptr+=2){
                        *((int16_t*)(o->b_wptr))=(d->baselevel)*((int)*(int16_t*)m->b_rptr);
                    }
                    d->baselevel-=0.005;
                    if(d->baselevel<=1-d->mixlevel) {
                        d->baselevel=1-d->mixlevel;
                        d->fadingIN=false;
                    }
                }else if(!d->fadingIN && !d->fadingOUT){
                    for(;m->b_rptr<m->b_wptr;m->b_rptr+=2,o->b_wptr+=2,d->end+=2){
                        if(d->end < d->mix_size){//mixsound
                        //*((int16_t*)(o->b_wptr))=*((int16_t*)(d->pcm_buf+d->end));
                        *((int16_t*)(o->b_wptr))=saturate(*((int16_t*)(d->pcm_buf+d->end)))*(d->mixlevel)+(d->baselevel)*((int)*(int16_t*)m->b_rptr);
                        }else{//mixsound over
                                *((int16_t*)(o->b_wptr))=(d->baselevel)*((int)*(int16_t*)m->b_rptr);
                        }
                    }
                }else{
                    //printf("size %d ",m->b_wptr-m->b_rptr);
                    for(;m->b_rptr<m->b_wptr;m->b_rptr+=2,o->b_wptr+=2){
                        *((int16_t*)(o->b_wptr))=(d->baselevel)*((int)*(int16_t*)m->b_rptr);
                    }
                    d->baselevel+=0.005;
                    if(d->baselevel>=1) {
                        d->baselevel=1;
                        d->flag=false;
                    }                    
                }
            
                if(d->end >= d->mix_size){
                    if(d->repeatN == 1){//mix 1 time
                        d->end=0;
                        d->fadingOUT=true;
                        blk.private1=Eofmixsound;
                    }else if(d->repeatN < 0){//repeat mix
                        d->end=0;
                    }else{
                        d->end=0;//mix mixlevel times
                        d->repeatN--;
                    }
                };
                free(m);
                blk.datap = o;
                ite_queue_put(f->output[0].Qhandle, &blk);
                //sem_post(&f->output[0].semHandle);
            }
        
        }else{
            //sem_wait(&f->input[0].semHandle);
            if(ite_queue_get(f->input[0].Qhandle, &blk) == 0){
            
                ite_queue_put(f->output[0].Qhandle, &blk);
            }
            //sem_post(&f->output[0].semHandle);
        }

    }
    
    ite_mblk_queue_flush(f->input[0].Qhandle);
    ite_mblk_queue_flush(f->output[0].Qhandle);

    
    return NULL;

}

static IteMethodDes mix_methods[] = {
    {ITE_FILTER_FILEOPEN, mix_fileopen},
    {0, NULL}
};

IteFilterDes FilterFileMix = {
    ITE_FILTER_FILEMIX_ID,
    mix_init,
    mix_uninit,
    mix_process,
    mix_methods
};



