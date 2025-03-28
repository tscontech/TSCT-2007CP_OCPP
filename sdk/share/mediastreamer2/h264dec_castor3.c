/*
   mediastreamer2 library - modular sound and video processing and streaming
   Copyright (C) 2010  Belledonne Communications SARL
   Author: Simon Morlat <simon.morlat@linphone.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/rfc3984.h"
#include "mediastreamer2/msvideo.h"
#include "mediastreamer2/msticker.h"

#ifdef CFG_BUILD_FFMPEG
#include "ffmpeg-priv.h"
#endif
#include "ortp/b64.h"

/* iTE related */
#include "ite/itp.h"
#include "ith/ith_video.h"
#include "ite/itv.h"
#include "isp/mmp_isp.h"
#include "fc_external.h"

/* ffmpeg */
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
#else
#include "avc_video_decoder.h"
#endif

#define LCD_OUTPUT         1
#define FRAME_BUFFER_COUNT 4
#define _DEBUG             0

extern int itv_set_pb_mode(int pb_mode);

typedef struct _DecData {
    mblk_t            *yuv_msg;
    mblk_t            *sps, *pps;
    Rfc3984Context    unpacker;
    MSPicture         outbuf;
    //struct SwsContext *sws_ctx;
#ifdef CFG_BUILD_FFMPEG	
    AVCodecContext    av_context;
#endif
    unsigned int      packet_num;
    uint8_t           *bitstream;
    int               bitstream_size;
    uint64_t          last_error_reported_time;
    /* hack for RMI display */
    mblk_t            *frame;
    /* hack for Packetization-mode */
    int               mode;
    /* temp solution for preventing video sample brust size larger than 1M */
    mblk_t            *bak_im;
} DecData;
#ifdef CFG_BUILD_FFMPEG
static void ffmpeg_init()
{
    static bool_t done = FALSE;
    if (!done)
    {
        avcodec_init();
        avcodec_register_all();
        done = TRUE;
#if _DEBUG
        av_log_set_level(AV_LOG_DEBUG);
#endif
    }
}

static void dec_open(DecData *d)
{
    AVCodec *codec;
    int     error;

    codec = avcodec_find_decoder(CODEC_ID_H264);
    if (codec == NULL)
        ms_fatal("Could not find H264 decoder in ffmpeg.");
    avcodec_get_context_defaults(&d->av_context);

    error = avcodec_open(&d->av_context, codec);
    if (error != 0)
    {
        ms_fatal("avcodec_open() failed.");
    }
}
#endif
static void dec_init(MSFilter *f)
{
#ifdef CFG_VIDEO_ENABLE
    DecData *d = (DecData *)ms_new(DecData, 1);
#ifdef CFG_BUILD_FFMPEG
    ffmpeg_init();
#endif
    d->yuv_msg                  = NULL;
    d->sps                      = NULL;
    d->pps                      = NULL;
    //d->sws_ctx                  = NULL;
    d->mode                     = 1; // make sure Packetization-mode is Fregmentation Unit (FU)
    rfc3984_init(&d->unpacker);
    rfc3984_set_mode(&d->unpacker, d->mode);
    d->packet_num               = 0;
#ifdef CFG_BUILD_FFMPEG
    dec_open(d);
#else
	iTE_h264_Dtv_decode_init();
#endif
    d->outbuf.w                 = 0;
    d->outbuf.h                 = 0;
    d->bitstream_size           = 65536;
    d->bitstream                = ms_malloc0(d->bitstream_size);
    d->last_error_reported_time = 0;
    /* hack for RMI display */
    d->frame                    = NULL;
    f->data                     = d;
    d->bak_im                   = NULL;
#endif	
}

#ifdef CFG_BUILD_FFMPEG
static void dec_reinit(DecData *d)
{
#ifdef CFG_VIDEO_ENABLE
    avcodec_close(&d->av_context);
    dec_open(d);
#endif	
}
#endif
static void dec_uninit(MSFilter *f)
{
#ifdef CFG_VIDEO_ENABLE
    DecData *d = (DecData *)f->data;
    rfc3984_uninit(&d->unpacker);
#ifdef CFG_BUILD_FFMPEG
    avcodec_close(&d->av_context);
#else
	iTE_h264_Dtv_decode_end();
#endif
    if (d->yuv_msg)
        freemsg(d->yuv_msg);
    if (d->sps)
        freemsg(d->sps);
    if (d->pps)
        freemsg(d->pps);
    ms_free(d->bitstream);
    ms_free(d);
#endif	
}
#ifdef CFG_BUILD_FFMPEG
mblk_t *ms_yuv_msg_alloc(YuvBuf *buf)
{
    int       size    = sizeof(AVFrame);
    const int padding = 16;
    mblk_t    *msg    = allocb(size + padding, 0);
    msg->b_wptr += size;
    return msg;
}

static mblk_t *get_as_yuvmsg(MSFilter *f, DecData *s, AVFrame *orig)
{
#if 1 /* hack for RMI display */
    AVFrame *output;
    if (!s->yuv_msg)
    {
        s->yuv_msg = ms_yuv_msg_alloc(&s->outbuf);
    }
    output = (AVFrame *)(s->yuv_msg->b_rptr);
    memcpy(output, orig, sizeof(AVFrame));
    return dupmsg(s->yuv_msg);
#else
    AVCodecContext *ctx = &s->av_context;

    if (s->outbuf.w != ctx->width || s->outbuf.h != ctx->height)
    {
        if (s->sws_ctx != NULL)
        {
            sws_freeContext(s->sws_ctx);
            s->sws_ctx = NULL;
            freemsg(s->yuv_msg);
            s->yuv_msg = NULL;
        }
        ms_message("Getting yuv picture of %ix%i", ctx->width, ctx->height);
        s->yuv_msg  = ms_yuv_buf_alloc(&s->outbuf, ctx->width, ctx->height);
        s->outbuf.w = ctx->width;
        s->outbuf.h = ctx->height;
        s->sws_ctx  = sws_getContext(ctx->width, ctx->height, ctx->pix_fmt,
                                     ctx->width, ctx->height, PIX_FMT_YUV420P, SWS_FAST_BILINEAR,
                                     NULL, NULL, NULL);
    }
    if (sws_scale(s->sws_ctx, (const uint8_t *const *)orig->data, orig->linesize, 0,
                  ctx->height, s->outbuf.planes, s->outbuf.strides) < 0)
    {
        ms_error("%s: error in sws_scale().", f->desc->name);
    }
    return dupmsg(s->yuv_msg);
#endif
}
#else
mblk_t *ms_yuv_msg_alloc(YuvBuf *buf)
{
    int       size    = sizeof(AVC_FRAME);
    const int padding = 16;
    mblk_t    *msg    = allocb(size + padding, 0);
    msg->b_wptr += size;
    return msg;
}

static mblk_t *get_as_yuvmsg(MSFilter *f, DecData *s, AVC_FRAME *orig)
{
    AVC_FRAME *output;
    if (!s->yuv_msg)
    {
        s->yuv_msg = ms_yuv_msg_alloc(&s->outbuf);
    }
    output = (AVC_FRAME *)(s->yuv_msg->b_rptr);
    memcpy(output, orig, sizeof(AVC_FRAME));
    return dupmsg(s->yuv_msg);
}
#endif
static void update_sps(DecData *d, mblk_t *sps)
{
    if (d->sps)
        freemsg(d->sps);
    d->sps = dupb(sps);
}

static void update_pps(DecData *d, mblk_t *pps)
{
    if (d->pps)
        freemsg(d->pps);
    if (pps)
        d->pps = dupb(pps);
    else
        d->pps = NULL;
}

static bool_t check_sps_change(DecData *d, mblk_t *sps)
{
    bool_t ret = FALSE;
    if (d->sps)
    {
        ret = (msgdsize(sps) != msgdsize(d->sps)) || (memcmp(d->sps->b_rptr, sps->b_rptr, msgdsize(sps)) != 0);
        if (ret)
        {
            ms_message("SPS changed ! %i,%i", msgdsize(sps), msgdsize(d->sps));
            update_sps(d, sps);
            update_pps(d, NULL);
        }
    }
    else
    {
        ms_message("Receiving first SPS");
        update_sps(d, sps);
    }
    return ret;
}

static bool_t check_pps_change(DecData *d, mblk_t *pps)
{
    bool_t ret = FALSE;
    if (d->pps)
    {
        ret = (msgdsize(pps) != msgdsize(d->pps)) || (memcmp(d->pps->b_rptr, pps->b_rptr, msgdsize(pps)) != 0);
        if (ret)
        {
            ms_message("PPS changed ! %i,%i", msgdsize(pps), msgdsize(d->pps));
            update_pps(d, pps);
        }
    }
    else
    {
        ms_message("Receiving first PPS");
        update_pps(d, pps);
    }
    return ret;
}

static void enlarge_bitstream(DecData *d, int new_size)
{
    d->bitstream_size = new_size;
    d->bitstream      = ms_realloc(d->bitstream, d->bitstream_size);
}

static int nalusToFrame(DecData *d, MSQueue *naluq, bool_t *new_sps_pps)
{
    mblk_t  *im;
    uint8_t *dst          = d->bitstream, *src, *end;
    int     nal_len;
    bool_t  start_picture = TRUE;
    uint8_t nalu_type;
    uint32_t totoal_size = 0;
   
    *new_sps_pps = FALSE;
    end          = d->bitstream + d->bitstream_size;
            
    if (d->bak_im != NULL)
    {
    	  src     = d->bak_im->b_rptr;
        nal_len = d->bak_im->b_wptr - src;        
        
        totoal_size += nal_len;
                
        if (dst + nal_len + 100 > end)
        {
            int pos = dst - d->bitstream;
            enlarge_bitstream(d, d->bitstream_size + nal_len + 100);
            dst = d->bitstream + pos;
            end = d->bitstream + d->bitstream_size;            
        }
        if (src[0] == 0 && src[1] == 0 && src[2] == 0 && src[3] == 1)
        {
            int size = d->bak_im->b_wptr - src;
            /*workaround for stupid RTP H264 sender that includes nal markers */
            memcpy(dst, src, size);
            dst += size;
        }
        else
        {
            nalu_type = (*src) & ((1 << 5) - 1);
            if (nalu_type == 7)
                *new_sps_pps = check_sps_change(d, d->bak_im) || *new_sps_pps;
            if (nalu_type == 8)
                *new_sps_pps = check_pps_change(d, d->bak_im) || *new_sps_pps;
            if (start_picture || nalu_type == 7 /*SPS*/ || nalu_type == 8 /*PPS*/)
            {
                *dst++        = 0;
                start_picture = FALSE;
            }

            /*prepend nal marker*/
            *dst++ = 0;
            *dst++ = 0;
            *dst++ = 1;
            *dst++ = *src++;
            while (src < (d->bak_im->b_wptr - 3))
            {
                if (src[0] == 0 && src[1] == 0 && src[2] < 3)
                {
                    *dst++ = 0;
                    *dst++ = 0;
                    *dst++ = 3;
                    src   += 2;
                }
                *dst++ = *src++;
            }
            *dst++ = *src++;
            *dst++ = *src++;
            *dst++ = *src++;
        }
        freemsg(d->bak_im);
        d->bak_im = NULL; 
    }
    
    while ((im = ms_queue_get(naluq)) != NULL)
    {
        src     = im->b_rptr;
        nal_len = im->b_wptr - src;        
        
        totoal_size += nal_len;
        
        if (totoal_size + 100 > 1*1024*1024)
        {
        	  //printf("Error ~~~~~~~~~~~~~~~~~~%d\n",totoal_size);
        	  d->bak_im = im;
        	  break;
        }
        
        if (dst + nal_len + 100 > end)
        {
            int pos = dst - d->bitstream;
            enlarge_bitstream(d, d->bitstream_size + nal_len + 100);
            dst = d->bitstream + pos;
            end = d->bitstream + d->bitstream_size;
        }
        if (src[0] == 0 && src[1] == 0 && src[2] == 0 && src[3] == 1)
        {
            int size = im->b_wptr - src;
            /*workaround for stupid RTP H264 sender that includes nal markers */
            memcpy(dst, src, size);
            dst += size;
        }
        else
        {
            nalu_type = (*src) & ((1 << 5) - 1);
            if (nalu_type == 7)
                *new_sps_pps = check_sps_change(d, im) || *new_sps_pps;
            if (nalu_type == 8)
                *new_sps_pps = check_pps_change(d, im) || *new_sps_pps;
            if (start_picture || nalu_type == 7 /*SPS*/ || nalu_type == 8 /*PPS*/)
            {
                *dst++        = 0;
                start_picture = FALSE;
            }

            /*prepend nal marker*/
            *dst++ = 0;
            *dst++ = 0;
            *dst++ = 1;
            *dst++ = *src++;
            while (src < (im->b_wptr - 3))
            {
                if (src[0] == 0 && src[1] == 0 && src[2] < 3)
                {
                    *dst++ = 0;
                    *dst++ = 0;
                    *dst++ = 3;
                    src   += 2;
                }
                *dst++ = *src++;
            }
            *dst++ = *src++;
            *dst++ = *src++;
            *dst++ = *src++;
        }
        freemsg(im);                      
    }
    return dst - d->bitstream;
}

static void dec_preprocess(MSFilter *f)
{
#ifdef CFG_VIDEO_ENABLE
    DecData *d = (DecData *) f->data;
#ifdef CFG_BUILD_FFMPEG	
    d->frame = allocb(sizeof(AVFrame), 0);
#else
	d->frame = allocb(sizeof(AVC_FRAME), 0);
#endif
    printf("[MSH264Dec] preprocess\n");
	//itv_set_video_window(0, 0, ithLcdGetWidth(), ithLcdGetHeight());
    itv_set_pb_mode(1);
#endif	
}

static uint32_t PalGetClock(void)
{
    struct timeval tv;

    if (gettimeofday(&tv, NULL) != 0)
        printf("gettimeofday failed!\n");
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static uint32_t PalGetDuration(uint32_t clock)
{
    struct timeval tv;

    if (gettimeofday(&tv, NULL) != 0)
        printf("gettimeofday failed!\n");
    return (unsigned int)(tv.tv_sec * 1000 + tv.tv_usec / 1000) - clock;
}

//static uint32_t     lastTick, gap_tick;

static void dec_process(MSFilter *f)
{
#ifdef CFG_VIDEO_ENABLE
    DecData *d = (DecData *)f->data;
    mblk_t  *im;
    MSQueue nalus;
#ifdef CFG_BUILD_FFMPEG  	
    AVFrame orig;
#else
	AVC_FRAME orig;
#endif
    ms_queue_init(&nalus);

    while ((im = ms_queue_get(f->inputs[0])) != NULL)
    {
    	if(d->frame == NULL)
    	{
    		freemsg(im);
			break;
        }
		/*push the sps/pps given in sprop-parameter-sets if any*/
        if (d->packet_num == 0 && d->sps && d->pps)
        {
            mblk_set_timestamp_info(d->sps, mblk_get_timestamp_info(im));
            mblk_set_timestamp_info(d->pps, mblk_get_timestamp_info(im));
            rfc3984_unpack(&d->unpacker, d->sps, &nalus);
            rfc3984_unpack(&d->unpacker, d->pps, &nalus);
            d->sps = NULL;
            d->pps = NULL;
        }
        rfc3984_unpack(&d->unpacker, im, &nalus);
        if (!ms_queue_empty(&nalus))
        {
            int     size;
            uint8_t *p, *end;
            bool_t  need_reinit = FALSE;
            size = nalusToFrame(d, &nalus, &need_reinit);
#ifdef CFG_BUILD_FFMPEG            
            if (need_reinit)
                dec_reinit(d);
#endif			
            p    = d->bitstream;
            end  = d->bitstream + size;
            while (end - p > 0)
            {
                int      len;
                int      got_picture = 0;
                /* hack for RMI display */
#ifdef CFG_BUILD_FFMPEG				
                uint32_t rdIdx       = -1;				
                AVPacket pkt;
                avcodec_get_frame_defaults(&orig);
                av_init_packet(&pkt);

                /* hack for RMI display */
                orig.opaque = (void *)&rdIdx;
                pkt.data    = p;
                pkt.size    = end - p;
#else
				memset(&orig, 0, sizeof(AVC_FRAME));
#endif				
#if 0           //_DEBUG
                printf("pkt %p len %d", pkt.data, pkt.size);
                for (len = 0; len < pkt.size; len++)
                {
                    if ((len & 0xF) == 0)
                        printf("\n");
                    printf("0x%02X ", pkt.data[len]);
                }
                printf("<-- dump end\n");
#endif
				//printf("<-- H264 dec start\n");
				//gap_tick = itpGetTickCount();
				/*clear h264 encoder interrupt*/
				//ithWriteRegA(0xb2000004, ithReadRegA(0xb2000004));
				//gap_tick = itpGetTickCount();
#ifdef CFG_BUILD_FFMPEG				
                len = avcodec_decode_video2(&d->av_context, &orig, &got_picture, &pkt);
				//if(itpGetTickDuration(lastTick) > 50)
				//	printf("H264 dec time=%d(%d) ms\n", itpGetTickDuration(lastTick), itpGetTickDuration(gap_tick));
				//lastTick = itpGetTickCount();
				//printf("<-- H264 dec stop\n");
				if (len <= 0)
                {
                    ms_warning("ms_AVdecoder_process: error %i.", len);
                    if ((f->ticker->time - d->last_error_reported_time) > 5000 || d->last_error_reported_time == 0)
                    {
                        d->last_error_reported_time = f->ticker->time;
                        ms_filter_notify_no_arg(f, MS_VIDEO_DECODER_DECODING_ERRORS);
                    }
                    break;
                }
#else
				len = iTE_h264_Dtv_decode_frame(&orig, &got_picture, p, end - p);
#endif				
                if (got_picture)
                {
#ifdef CFG_BUILD_FFMPEG                  
                    orig.opaque = (void *)&rdIdx;
#endif
                    ms_queue_put(f->outputs[0], get_as_yuvmsg(f, d, &orig));					
                }
                p += len;
            }
        }
        d->packet_num++;
    }
#endif	
}

static int dec_postprocess(MSFilter *f)
{
#ifdef CFG_VIDEO_ENABLE
    DecData *d = (DecData *) f->data;
    itv_set_pb_mode(0);
    if (d->frame)
    {
        freemsg(d->frame);
        d->frame = NULL;
    }
    printf("[MSH264Dec] postprocess\n");
#endif	
}

static int dec_add_fmtp(MSFilter *f, void *arg)
{
    DecData    *d    = (DecData *)f->data;
    const char *fmtp = (const char *)arg;
    char       value[256];
    if (fmtp_get_value(fmtp, "sprop-parameter-sets", value, sizeof(value)))
    {
        char *b64_sps = value;
        char *b64_pps = strchr(value, ',');
        if (b64_pps)
        {
            *b64_pps        = '\0';
            ++b64_pps;
            ms_message("Got sprop-parameter-sets : sps=%s , pps=%s", b64_sps, b64_pps);
            d->sps          = allocb(sizeof(value), 0);
            d->sps->b_wptr += b64_decode(b64_sps, strlen(b64_sps), d->sps->b_wptr, sizeof(value));
            d->pps          = allocb(sizeof(value), 0);
            d->pps->b_wptr += b64_decode(b64_pps, strlen(b64_pps), d->pps->b_wptr, sizeof(value));
        }
    }
    return 0;
}

static MSFilterMethod h264_dec_methods[] = {
    {   MS_FILTER_ADD_FMTP, dec_add_fmtp      },
    {                    0, NULL              }
};

#ifndef _MSC_VER

MSFilterDesc ms_h264_dec_desc = {
    .id          = MS_H264_DEC_ID,
    .name        = "MSH264Dec",
    .text        = "A H264 decoder",
    .category    = MS_FILTER_DECODER,
    .enc_fmt     = "H264",
    .ninputs     =                                         1,
    .noutputs    =                                         1,
    //.noutputs=0,
    .init        = dec_init,
    .preprocess  = dec_preprocess,
    .process     = dec_process,
    .postprocess = dec_postprocess,
    .uninit      = dec_uninit,
    .methods     = h264_dec_methods
};

#else

MSFilterDesc ms_h264_dec_desc = {
    MS_H264_DEC_ID,
    "MSH264Dec",
    "A H264 decoder",
    MS_FILTER_DECODER,
    "H264",
    1,
    1,
    dec_init,
    dec_preprocess,
    dec_process,
    dec_postprocess,
    dec_uninit,
    h264_dec_methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_h264_dec_desc)