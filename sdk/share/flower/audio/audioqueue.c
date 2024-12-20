#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "openrtos/FreeRTOS.h"
#include "flower/flower.h"
#include "include/audioqueue.h"
/*duplicate mp return new mblk_ite*/
mblk_ite *dupmblk(mblk_ite* mp)
{   
    mblk_ite *newm;
    newm=allocb_ite(mp->size);
    memcpy(newm->b_wptr,mp->b_rptr,newm->size);
    newm->b_wptr+=mp->size;
  
    return newm;
}
/*init Ringbuff*/
rbuf_ite *ite_rbuf_init(size_t size)
{   
    return allocb_ite(size);
}
/*uninit Ringbuff*/
void ite_rbuf_free(rbuf_ite *m){

    freemsg_ite(m);
}
/*get Ringbuff avail data size*/
unsigned int ite_rbuf_get_avail_size(rbuf_ite *m){

    unsigned size;
    if(m->b_wptr>=m->b_rptr){
        size = m->b_wptr-m->b_rptr;
        return size;
    }else{
        size = m->b_wptr - m->b_rptr + m->size;
        return size;
    }    
}
/*put data into Ringbuff */
/*return 0 : ringbuffer full ,src data do not put to (miss data)*/
int ite_rbuf_put(rbuf_ite *mp,char *src,int sample){

    int wp=mp->b_wptr-mp->b_datap;
    int mpsize=mp->size;
    
    if(ite_rbuf_get_avail_size(mp)+sample >= mpsize) {
        printf("ring buffer full\n");
        return 0;
    }
     
    if(wp+sample < mpsize){
        memcpy(mp->b_wptr,src,sample);
        mp->b_wptr += sample;
    }else{
       int szsec0 = mpsize - wp;
       int szsec1 = sample - szsec0;
       if(szsec0) memcpy(mp->b_wptr,src,szsec0);
       memcpy(mp->b_datap,src + szsec0,szsec1);
       mp->b_wptr = mp->b_datap+szsec1;
    }
    
    return sample;
 
}
/*get data from Ringbuff */
/*return 0 : ringbuffer avail size less than sample(size),dst do not get any data*/
int ite_rbuf_get(char *dst,rbuf_ite *mp,int sample){
    
    int rp=mp->b_rptr-mp->b_datap;
    int mpsize=mp->size;
    
    if(ite_rbuf_get_avail_size(mp)<sample) return 0;
    
    if(rp+sample < mpsize){
        memcpy(dst,mp->b_rptr,sample);
        mp->b_rptr += sample;
    }else{
       int szsec0 = mpsize - rp;
       int szsec1 = sample - szsec0;
       if(szsec0) memcpy(dst, mp->b_rptr, szsec0);
       memcpy(dst + szsec0, mp->b_datap, szsec1);
       mp->b_rptr = mp->b_datap+szsec1;
    }
    return sample;
    
}
    /*flush Ringbuff data*/
void ite_rbuf_flush(rbuf_ite *mp){
    mp->b_rptr=mp->b_wptr=mp->b_datap;
    memset(mp->b_datap,0,mp->size);
}

void mblk_init(mblk_ite *mp){
    mp->b_prev=mp->b_next=NULL;
    mp->b_rptr=mp->b_wptr=mp->b_datap=NULL;
    mp->size=0;
}
 /*mblkqueue list init*/
void mblkqinit(mblkq *q){
    mblk_init(&q->qstop);
    q->qstop.b_next=&q->qstop;
    q->qstop.b_prev=&q->qstop;
    q->qcount=0;
}
/*put mp to mblkqueue list */
void putmblkq(mblkq *q, mblk_ite *mp){
    q->qstop.b_prev->b_next=mp;
    mp->b_prev=q->qstop.b_prev;
    mp->b_next=&q->qstop;
    q->qstop.b_prev=mp;
    q->qcount++;    
}
/*get mp from mblkqueue list */
mblk_ite *getmblkq(mblkq *q){
    mblk_ite *tmp;
    tmp=q->qstop.b_next;
    if (tmp==&q->qstop) return NULL;
    q->qstop.b_next=tmp->b_next;
    tmp->b_next->b_prev=&q->qstop;
    tmp->b_prev=NULL;
    tmp->b_next=NULL;
    q->qcount--;
    return tmp;    
}
/*flush mblkqueue list all data*/
void flushmblkq(mblkq *q){
    mblk_ite *mp;

    while ((mp=getmblkq(q))!=NULL)
    {
        freemsg_ite(mp);
    }    
}
/*get mblkqueue avail count*/
int getmblkqavail(mblkq *q){
    return q->qcount;
}
/*mblkQ reshape list init*/
void mblkQShapeInit(mblkq *q,int bufsize){
    mblkqinit(q);
    q->rb=ite_rbuf_init(bufsize);
}
/*mblkQ reshape flush*/
void mblkQShapeFlush(mblkq *q){
    flushmblkq(q);
    ite_rbuf_flush(q->rb);
}
/*mblkQ reshape list uninit*/
void mblkQShapeUninit(mblkq *q){
    flushmblkq(q);
    ite_rbuf_free(q->rb);
}
/*shape assign length(resize) & put to Q(q)*/
void mblkQShapePut(mblkq *q,mblk_ite *m,int resize){
    char tmp[4096];
    if(m->size==resize){
        putmblkq(q,m);
    }else{
        ite_rbuf_put(q->rb,m->b_rptr,m->size);
        while(ite_rbuf_get(tmp,q->rb,resize)){
            mblk_ite *om=allocb_ite(resize);
            memcpy(om->b_wptr,tmp,resize);
            om->b_wptr+=resize;
            putmblkq(q,om);
        }
        if(m) freemsg_ite(m);        
    }
}

void srcQShapePut(mblkq *q,unsigned char *inptr,int length,int resize){
    int availSize=ite_rbuf_get_avail_size(q->rb);
    int count=0;
    int offset=0;
    int remain=0;
    int residual=length;
    if(length==0) {//eof : put 0 to Q
        if(availSize){//rbuf with data
            mblk_ite *om=allocb_ite(availSize);
            ite_rbuf_get(om->b_wptr,q->rb,availSize);
            om->b_wptr+=availSize;
            putmblkq(q,om);
        }  
        mblk_ite *om=allocb_ite(length);//size==0
        putmblkq(q, om);
        printf("put Zeor Q\n");
        return;
    }
    if(availSize+residual>=resize){
        mblk_ite *om=allocb_ite(resize);
        if(availSize){//rbuf with data
            remain=resize-availSize;
            ite_rbuf_get(om->b_wptr,q->rb,availSize);
            om->b_wptr+=availSize;
            memcpy(om->b_wptr,inptr,remain);
            om->b_wptr+=remain;        
            putmblkq(q, om);
            residual-=remain;
        }
        while(residual>resize){
            offset=count*resize+remain;
            mblk_ite *om=allocb_ite(resize);
            memcpy(om->b_wptr,inptr+offset,resize);
            om->b_wptr+=resize;
            putmblkq(q, om);
            count++;
            residual-=resize;
        }                
    }
    if(residual>0){
        offset=count*resize+remain;
        ite_rbuf_put(q->rb,inptr+offset,residual);
    }    
    
}

/*get mblk from mblkQ & put to next filter*/
void ite_queue_put_from_mblkQ(IteQueueblk *blk,IteFilter *f,int pin,mblkq *tmpQ){ 
    while(ite_queue_get_size(f->output[pin].Qhandle)<32){
        blk->datap=getmblkq(tmpQ);
        if(blk->datap!=NULL){
            ite_queue_put(f->output[pin].Qhandle, blk);
        }else{
            break;
        }
    }  
}
/*get mblk from filter & put to shapeSize mblkQ*/
void ite_queue_put_to_mblkQShape(IteQueueblk *blk,IteFilter *f,int pin,mblkq *tmpQ,int resize){
    while(ite_queue_get(f->input[pin].Qhandle, blk) == 0){
        mblkQShapePut(tmpQ,blk->datap,resize);
    }
}

/*Queue control :make sure int f->output[pin].Qhandle queue number between : min> Qnum >max*/
int IteAudioQueueController(IteFilter *f,int pin,int max,int min){
    int timeout = 0;
    if(ite_queue_get_size(f->output[pin].Qhandle)>=max){
        while(ite_queue_get_size(f->output[pin].Qhandle)>min){
            usleep(50*1000);
            timeout++;
            if(timeout>50) {
                printf("IteAudioQueueController timeout\n");
                return -1;
            }
        }
        return 1;
    }
    return 0;
}