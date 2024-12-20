#include <stdio.h>
#include "include/audioqueue.h"
#include "include/fileheader.h"
PacketQueue g;
/* packet queue handling */
static void packetQ_init(PacketQueue *q)
{
    memset(q, 0, sizeof(PacketQueue));
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
    //packet_queue_put(q, &flush_pkt);
}

static void packetQ_flush(PacketQueue *q)
{
    AVPacketList *pkt, *pkt1;
    if (q->mutex)
    {
        pthread_mutex_lock(&q->mutex);
        for (pkt = q->first_pkt; pkt != NULL; pkt = pkt1)
        {
            pkt1 = pkt->next;
            av_free_packet(&pkt->pkt);
            av_freep(&pkt);
        }
        q->last_pkt   = NULL;
        q->first_pkt  = NULL;
        q->nb_packets = 0;
        q->size       = 0;
        pthread_mutex_unlock(&q->mutex);
    }
}

static void packetQ_uninit(PacketQueue *q)
{
    if (q->mutex)
    {
        packetQ_flush(q);
        pthread_mutex_destroy(&q->mutex);
        pthread_cond_destroy(&q->cond);
        memset(q, 0, sizeof(PacketQueue));
    }
}

static int packetQ_put(PacketQueue *q, AVPacket *pkt)
{
    AVPacketList *pkt1;

    /* duplicate the packet */
    if (q->mutex)
    {
        if (av_dup_packet(pkt) < 0)
            return -1;

        pkt1       = av_malloc(sizeof(AVPacketList));
        if (!pkt1)
            return -1;
        pkt1->pkt  = *pkt;
        pkt1->next = NULL;

        pthread_mutex_lock(&q->mutex);

        if (!q->last_pkt)
            q->first_pkt = pkt1;
        else
            q->last_pkt->next = pkt1;
        q->last_pkt = pkt1;
        q->nb_packets++;
        q->size    += pkt1->pkt.size + sizeof(*pkt1);
        q->lastPts  = pkt->pts;
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

static void packetQ_abort(PacketQueue *q)
{
    if (q->mutex)
    {
        pthread_mutex_lock(&q->mutex);
        q->abort_request = 1;
        pthread_cond_signal(&q->cond);
        pthread_mutex_unlock(&q->mutex);
    }
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet received. */
static int packetQ_get(PacketQueue *q, AVPacket *pkt, int block)
{
    AVPacketList *pkt1;
    int          ret;
    if (q->mutex)
    {
        pthread_mutex_lock(&q->mutex);

        for (;;)
        {
            if (q->abort_request)
            {
                ret = -1;
                break;
            }

            pkt1 = q->first_pkt;
            if (pkt1)
            {
                q->first_pkt = pkt1->next;
                if (!q->first_pkt)
                    q->last_pkt = NULL;
                q->nb_packets--;
                q->size -= pkt1->pkt.size + sizeof(*pkt1);
                *pkt     = pkt1->pkt;
                av_free(pkt1);
                ret      = 1;
                break;
            }
            else if (!block)
            {
                ret = 0;
                break;
            }
            else
            {
                av_log(NULL, AV_LOG_DEBUG, "queue empty, condition wait %lld\n", av_gettime());
                pthread_cond_wait(&q->cond, &q->mutex);
                av_log(NULL, AV_LOG_DEBUG, "leave condition wait %lld\n", av_gettime());
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

void QPktInit(void){
    packetQ_init(&g);
}

void QPktUninit(void){
    packetQ_uninit(&g);
}

void QPutPkt(AVPacket *pkt){
    if(pkt)
        packetQ_put(&g,pkt);
    else{
        AVPacket pkt1;
        pkt1.size=0;
        packetQ_put(&g,&pkt1);
    }
}

void QPutToPkt(unsigned char *inbuf,int size){
           
    if(size>0){
        uint8_t *buf=NULL;            
        AVPacket pkt;
        av_init_packet(&pkt);
        buf = malloc(size);
        memcpy(buf,inbuf,size);
        pkt.data = buf;
        pkt.size = size;       
        pkt.destruct = av_destruct_packet;         
        packetQ_put(&g,&pkt);
    }
    else{
        AVPacket pkt;
        pkt.size=0;
        packetQ_put(&g,&pkt);
    }
}

int QGetPkt(AVPacket *pkt, int block){
    return packetQ_get(&g,pkt,block);
}

int QPktCount(void){
    PacketQueue *tmp;
    tmp=&g;
    return tmp->nb_packets;
}

void QPktflush(void){
    packetQ_flush(&g);
}