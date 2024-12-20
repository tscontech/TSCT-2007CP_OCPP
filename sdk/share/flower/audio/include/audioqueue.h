#include "flower/flower.h"

#ifdef CFG_BUILD_FFMPEG
#include "libavutil/avstring.h"
#include "libavutil/colorspace.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avassert.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavcodec/audioconvert.h"
#include "libavutil/opt.h"
#include "libavcodec/avfft.h"
#include "libswresample/swresample.h"
typedef struct PacketQueue {
    AVPacketList    *first_pkt, *last_pkt;
    int             nb_packets;
    int             size;
    int             abort_request;
    int64_t         lastPts;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
} PacketQueue;
#endif

typedef mblk_ite rbuf_ite;

typedef struct _mblkq
{
    mblk_ite qstop;
    int qcount;
    rbuf_ite *rb;
} mblkq;

/*ring buff*/
rbuf_ite *ite_rbuf_init(size_t size);
void ite_rbuf_free(rbuf_ite *m);
unsigned int ite_rbuf_get_avail_size(rbuf_ite *m);
int ite_rbuf_put(rbuf_ite *mp,char *src,int sample);
int ite_rbuf_get(char *dst,rbuf_ite *mp,int sample);
void ite_rbuf_flush(rbuf_ite *mp);

/*link list*/
void mblkqinit(mblkq *q);
void putmblkq(mblkq *q, mblk_ite *m);
mblk_ite *getmblkq(mblkq *q);
void flushmblkq(mblkq *q);

/*shape mblkQ*/
void mblkQShapeInit(mblkq *q,int bufsize);
void mblkQShapeUninit(mblkq *q);
void mblkQShapePut(mblkq *q,mblk_ite *m,int resize);
void ite_queue_put_from_mblkQ(IteQueueblk *blk,IteFilter *f,int pin,mblkq *tmpQ);

/*Qnumber controller*/
int IteAudioQueueController(IteFilter *f,int pin,int max,int min);