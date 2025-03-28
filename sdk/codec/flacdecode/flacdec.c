﻿/*
 * FLAC (Free Lossless Audio Codec) decoder
 * Copyright (c) 2003 Alex Beregszaszi
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * FLAC (Free Lossless Audio Codec) decoder
 * @author Alex Beregszaszi
 * @see http://flac.sourceforge.net/
 *
 * This decoder can be used in 1 of 2 ways: Either raw FLAC data can be fed
 * through, starting from the initial 'fLaC' signature; or by passing the
 * 34-byte streaminfo structure through avctx->extradata[_size] followed
 * by data starting with the 0xFFF8 marker.
 */

//#include <limits.h>

#include "crc.h"
//#include "avcodec.h"
//#include "internal.h"
#include "get_bits.h"
#include "bytestream.h"
//#include "golomb.h"
#include "flac.h"
#include "flacdata.h"
//#include "config.h"

//#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#if defined(ENABLE_CODECS_PLUGIN)
# include "plugin.h"
#endif

static void allocate_buffers(FLACContext *s);

int avpriv_flac_is_extradata_valid(//AVCodecContext *avctx,
                               enum FLACExtradataFormat *format,
                               uint8_t **streaminfo_start)
{
#if 0
    if (!avctx->extradata || avctx->extradata_size < FLAC_STREAMINFO_SIZE) {
        printf("extradata NULL or too small.\n");
        return 0;
    }
    if (AV_RL32(avctx->extradata) != MKTAG('f','L','a','C')) {
        /* extradata contains STREAMINFO only */
        if (avctx->extradata_size != FLAC_STREAMINFO_SIZE) {
            printf("extradata contains %d bytes too many.\n",FLAC_STREAMINFO_SIZE-avctx->extradata_size);
        }
        *format = FLAC_EXTRADATA_FORMAT_STREAMINFO;
        *streaminfo_start = avctx->extradata;
    } else {
        if (avctx->extradata_size < 8+FLAC_STREAMINFO_SIZE) {
            printf("extradata too small.\n");                        
            return 0;
        }
        *format = FLAC_EXTRADATA_FORMAT_FULL_HEADER;
        *streaminfo_start = &avctx->extradata[8];
    }
#endif    
    return 1;
}

static av_cold int flac_decode_init(FLACContext *fctx)
{
    enum FLACExtradataFormat format;
    uint8_t *streaminfo;
    FLACContext *s = fctx;

    s->sample_fmt = AV_SAMPLE_FMT_S16;

    /* for now, the raw FLAC header is allowed to be passed to the decoder as
       frame data instead of extradata. */
   // if (!avctx->extradata)
   //     return 0;

    //if (!avpriv_flac_is_extradata_valid(avctx, &format, &streaminfo))
    //    return -1;

    /* initialize based on the demuxer-supplied streamdata header */
    //avpriv_flac_parse_streaminfo(avctx, (FLACStreaminfo *)s, streaminfo);
    avpriv_flac_parse_streaminfo((FLACStreaminfo *)s, streaminfo);    
    if (s->bps > 16)
        s->sample_fmt = AV_SAMPLE_FMT_S32;
    else
        s->sample_fmt = AV_SAMPLE_FMT_S16;
    allocate_buffers(s);
    s->got_streaminfo = 1;

    //avcodec_get_frame_defaults(&s->frame);
    //avctx->coded_frame = &s->frame;

    return 0;
}

//static void dump_headers(AVCodecContext *avctx, FLACStreaminfo *s)
static void dump_headers(FLACStreaminfo *s)
{
    printf("Max Blocksize: %d Max Framesize: %d Samplerate: %d Channels: %d Bits: %d \n",s->max_blocksize,s->max_framesize,s->samplerate,s->channels,s->bps);
}

static void allocate_buffers(FLACContext *s)
{
//    int i;

//    assert(s->max_blocksize);

//    for (i = 0; i < s->channels; i++) {
//        s->decoded[i] = av_realloc(s->decoded[i],
//                                   sizeof(int32_t)*s->max_blocksize);
//    }
}

//void avpriv_flac_parse_streaminfo(AVCodecContext *avctx, struct FLACStreaminfo *s,const uint8_t *buffer)
void avpriv_flac_parse_streaminfo(struct FLACStreaminfo *s, const uint8_t *buffer)
{
    GetBitContext gb;
    init_get_bits(&gb, buffer, FLAC_STREAMINFO_SIZE<<3);

    skip_bits(&gb, 16); /* skip min blocksize */
    s->max_blocksize = get_bits(&gb, 16);
    if (s->max_blocksize < FLAC_MIN_BLOCKSIZE) {
        printf("invalid max blocksize: %d\n",s->max_blocksize);        
        s->max_blocksize = 16;
    }

    skip_bits(&gb, 24); /* skip min frame size */
    s->max_framesize = get_bits_long(&gb, 24);

    s->samplerate = get_bits_long(&gb, 20);
    s->channels = get_bits(&gb, 3) + 1;
    s->bps = get_bits(&gb, 5) + 1;

    //avctx->channels = s->channels;
    //avctx->sample_rate = s->samplerate;
    //avctx->bits_per_raw_sample = s->bps;

    s->samples  = get_bits_long(&gb, 32) << 4;
    s->samples |= get_bits(&gb, 4);

    skip_bits_long(&gb, 64); /* md5 sum */
    skip_bits_long(&gb, 64); /* md5 sum */

    //dump_headers(avctx, s);
    dump_headers(s);
}

void avpriv_flac_parse_block_header(const uint8_t *block_header,
                                int *last, int *type, int *size)
{
    int tmp = bytestream_get_byte(&block_header);
    if (last)
        *last = tmp & 0x80;
    if (type)
        *type = tmp & 0x7F;
    if (size)
        *size = bytestream_get_be24(&block_header);
}

/**
 * Parse the STREAMINFO from an inline header.
 * @param s the flac decoding context
 * @param buf input buffer, starting with the "fLaC" marker
 * @param buf_size buffer size
 * @return non-zero if metadata is invalid
 */
static int parse_streaminfo(FLACContext *s, const uint8_t *buf, int buf_size)
{
    int metadata_type, metadata_size;

    if (buf_size < FLAC_STREAMINFO_SIZE+8) {
        /* need more data */
        return 0;
    }
    avpriv_flac_parse_block_header(&buf[4], NULL, &metadata_type, &metadata_size);
    if (metadata_type != FLAC_METADATA_TYPE_STREAMINFO ||
        metadata_size != FLAC_STREAMINFO_SIZE) {
        return -1;
    }
    //avpriv_flac_parse_streaminfo(s->avctx, (FLACStreaminfo *)s, &buf[8]);
    avpriv_flac_parse_streaminfo((FLACStreaminfo *)s, &buf[8]);    
    allocate_buffers(s);
    s->got_streaminfo = 1;

    return 0;
}

/**
 * Determine the size of an inline header.
 * @param buf input buffer, starting with the "fLaC" marker
 * @param buf_size buffer size
 * @return number of bytes in the header, or 0 if more data is needed
 */
static int get_metadata_size(const uint8_t *buf, int buf_size)
{
    int metadata_last, metadata_size;
    const uint8_t *buf_end = buf + buf_size;

    buf += 4;
    do {
        if (buf_end - buf < 4)
            return 0;
        avpriv_flac_parse_block_header(buf, &metadata_last, NULL, &metadata_size);
        buf += 4;
        if (buf_end - buf < metadata_size) {
            /* need more data in order to read the complete header */
            return 0;
        }
        buf += metadata_size;
    } while (!metadata_last);

    return buf_size - (buf_end - buf);
}

static int decode_residuals(FLACContext *s, int channel, int pred_order)
{
    int i, tmp, partition, method_type, rice_order;
    int sample = 0, samples;

    method_type = get_bits(&s->gb, 2);
    if (method_type > 1) {
        printf("illegal residual coding method %d\n",method_type);
        return -1;
    }

    rice_order = get_bits(&s->gb, 4);

    samples= s->blocksize >> rice_order;
    if (pred_order > samples) {
        printf("invalid predictor order: %i > %i\n",pred_order, samples);
        return -1;
    }

    sample=
    i= pred_order;
    for (partition = 0; partition < (1 << rice_order); partition++) {
        tmp = get_bits(&s->gb, method_type == 0 ? 4 : 5);
        if (tmp == (method_type == 0 ? 15 : 31)) {
            tmp = get_bits(&s->gb, 5);
            for (; i < samples; i++, sample++)
                s->decoded[channel][sample] = get_sbits_long(&s->gb, tmp);
        } else {
            for (; i < samples; i++, sample++) {
                s->decoded[channel][sample] = get_sr_golomb_flac(&s->gb, tmp, INT_MAX, 0);
            }
        }
        i= 0;
    }

    return 0;
}

static int decode_subframe_fixed(FLACContext *s, int channel, int pred_order)
{
    const int blocksize = s->blocksize;
    int32_t *decoded = s->decoded[channel];
    int av_uninit(a), av_uninit(b), av_uninit(c), av_uninit(d), i;

    /* warm up samples */
    for (i = 0; i < pred_order; i++) {
        decoded[i] = get_sbits_long(&s->gb, s->curr_bps);
    }

    if (decode_residuals(s, channel, pred_order) < 0)
        return -1;

    if (pred_order > 0)
        a = decoded[pred_order-1];
    if (pred_order > 1)
        b = a - decoded[pred_order-2];
    if (pred_order > 2)
        c = b - decoded[pred_order-2] + decoded[pred_order-3];
    if (pred_order > 3)
        d = c - decoded[pred_order-2] + 2*decoded[pred_order-3] - decoded[pred_order-4];

    switch (pred_order) {
    case 0:
        break;
    case 1:
        for (i = pred_order; i < blocksize; i++)
            decoded[i] = a += decoded[i];
        break;
    case 2:
        for (i = pred_order; i < blocksize; i++)
            decoded[i] = a += b += decoded[i];
        break;
    case 3:
        for (i = pred_order; i < blocksize; i++)
            decoded[i] = a += b += c += decoded[i];
        break;
    case 4:
        for (i = pred_order; i < blocksize; i++)
            decoded[i] = a += b += c += d += decoded[i];
        break;
    default:
        printf("illegal pred order %d\n", pred_order);
        return -1;
    }

    return 0;
}
#if !defined(_ARM_)
static __inline void MLA(int  a, int b) {
	asm volatile ("l.mac %0, %1" ::"r"(a), "r"(b));
}
#endif

static int decode_subframe_lpc(FLACContext *s, int channel, int pred_order)
{
    int i, j;
    int coeff_prec, qlevel;
    int coeffs[32];
    int32_t *decoded = s->decoded[channel];

    /* warm up samples */
    for (i = 0; i < pred_order; i++) {
        decoded[i] = get_sbits_long(&s->gb, s->curr_bps);
    }

    coeff_prec = get_bits(&s->gb, 4) + 1;
    if (coeff_prec == 16) {
        printf("invalid coeff precision\n");        
        return -1;
    }
    qlevel = get_sbits(&s->gb, 5);
    if (qlevel < 0) {
        printf("qlevel %d not supported, maybe buggy stream\n",qlevel);        
        return -1;
    }

    for (i = 0; i < pred_order; i++) {
        coeffs[i] = get_sbits(&s->gb, coeff_prec);
    }

    if (decode_residuals(s, channel, pred_order) < 0)
        return -1;

    if (s->bps > 16) {
        int64_t sum;
        for (i = pred_order; i < s->blocksize; i++) {
            sum = 0;
            for (j = 0; j < pred_order; j++)
                sum += (int64_t)coeffs[j] * decoded[i-j-1];
            decoded[i] += sum >> qlevel;
        }
    } else {

#if defined(_ARM_)
        for (i = pred_order; i < s->blocksize-1; i += 2) {
            int c;
            int d = decoded[i-pred_order];
            int s0 = 0, s1 = 0;
            for (j = pred_order-1; j > 0; j--) {
                c = coeffs[j];
                s0 += c*d;
                d = decoded[i-j];
                s1 += c*d;
            }
            c = coeffs[0];
            s0 += c*d;
            d = decoded[i] += s0 >> qlevel;
            s1 += c*d;
            decoded[i+1] += s1 >> qlevel;
        }
        if (i < s->blocksize) {
            int sum = 0;
            for (j = 0; j < pred_order; j++)
                sum += coeffs[j] * decoded[i-j-1];
            decoded[i] += sum >> qlevel;
        }
#else

/*       for (i = pred_order; i < s->blocksize-1; i += 2) {
            int c;
            int d = decoded[i-pred_order];
            int s0 = 0, s1 = 0;
            for (j = pred_order-1; j > 0; j--) {
                c = coeffs[j];
                asm volatile("l.mac %0, %1" : : "%r"(c ), "r"(d]));
                d = decoded[i-j];
                s1 += c*d;
            }
            c = coeffs[0];
            asm volatile("l.mac %0, %1" : : "%r"(c ), "r"(d));
            asm volatile("l.macrc %0, 0" : "=r"(s0) );            
            d = decoded[i] += s0 >> qlevel;
            s1 += c*d;
            decoded[i+1] += s1 >> qlevel;
        }
*/
        for (i = pred_order; i < s->blocksize-1; i += 2) {
            int c;
            int d = decoded[i-pred_order];
            int s0 = 0, s1 = 0;
            for (j = pred_order-1; j > 0; j--) {
                MLA(coeffs[j],d);
                d = decoded[i-j];
                s1 += coeffs[j]*d;
            }
            MLA(coeffs[j],d);
            asm volatile("l.macrc %0, 0" : "=r"(s0) );

            d = decoded[i] += s0 >> qlevel;
            s1 += coeffs[0]*d;
            decoded[i+1] += s1 >> qlevel;
        }
        
        if (i < s->blocksize) {
            int sum = 0;
            for (j = 0; j < pred_order; j++)
                MLA(coeffs[j], decoded[i-j-1]);

            asm volatile("l.macrc %0, 0" : "=r"(sum) );
            decoded[i] += sum >> qlevel;
        }

       
#endif        
    }

    return 0;
}

static __inline int decode_subframe(FLACContext *s, int channel)
{
    int type, wasted = 0;
    int i, tmp;

    s->curr_bps = s->bps;
    if (channel == 0) {
        if (s->ch_mode == FLAC_CHMODE_RIGHT_SIDE)
            s->curr_bps++;
    } else {
        if (s->ch_mode == FLAC_CHMODE_LEFT_SIDE || s->ch_mode == FLAC_CHMODE_MID_SIDE)
            s->curr_bps++;
    }

    if (get_bits1(&s->gb)) {
        printf("invalid subframe padding\n");
        return -161;
    }
    type = get_bits(&s->gb, 6);

    if (get_bits1(&s->gb)) {
        wasted = 1;
        while (!get_bits1(&s->gb))
            wasted++;
        s->curr_bps -= wasted;
    }
    if (s->curr_bps > 32) {
        printf( "decorrelated bit depth > 32");
        return -162;
    }

//FIXME use av_log2 for types
    if (type == 0) {
        tmp = get_sbits_long(&s->gb, s->curr_bps);
        for (i = 0; i < s->blocksize; i++)
            s->decoded[channel][i] = tmp;
    } else if (type == 1) {
        for (i = 0; i < s->blocksize; i++)
            s->decoded[channel][i] = get_sbits_long(&s->gb, s->curr_bps);
    } else if ((type >= 8) && (type <= 12)) {
        if (decode_subframe_fixed(s, channel, type & ~0x8) < 0)
            return -163;
    } else if (type >= 32) {
        if (decode_subframe_lpc(s, channel, (type & ~0x20)+1) < 0)
            return -164;
    } else {
        printf("invalid coding type\n");
        return -165;
    }

    if (wasted) {
        int i;
        for (i = 0; i < s->blocksize; i++)
            s->decoded[channel][i] <<= wasted;
    }

    return 0;
}

static int decode_frame(FLACContext *s)
{
    int i;
    GetBitContext *gb = &s->gb;
    FLACFrameInfo fi;
    int nResult;

    if (ff_flac_decode_frame_header(gb, &fi, 0)) {
        printf("invalid frame header\n");
        return -10;
    }

    if (s->channels && fi.channels != s->channels) {
        printf("switching channel layout mid-stream is not supported\n");
        return -11;
    }
    //s->channels = s->avctx->channels = fi.channels;
    s->channels = fi.channels;    
    s->ch_mode = fi.ch_mode;

    if (!s->bps && !fi.bps) {
        printf("bps not found in STREAMINFO or frame header\n");
        return -12;
    }
    if (!fi.bps) {
        fi.bps = s->bps;
    } else if (s->bps && fi.bps != s->bps) {
        printf("switching bps mid-stream is not supported\n");
        return -13;
    }
    //s->bps = s->avctx->bits_per_raw_sample = fi.bps;
    s->bps = fi.bps;

    if (s->bps > 16) {
        s->sample_fmt = AV_SAMPLE_FMT_S32;
        s->sample_shift = 32 - s->bps;
        s->is32 = 1;
    } else {
        s->sample_fmt = AV_SAMPLE_FMT_S16;
        s->sample_shift = 16 - s->bps;
        s->is32 = 0;
    }

    if (!s->max_blocksize)
        s->max_blocksize = FLAC_MAX_BLOCKSIZE;
    if (fi.blocksize > s->max_blocksize) {
        printf("blocksize %d > %d\n", fi.blocksize,s->max_blocksize);
        return -14;
    }
    s->blocksize = fi.blocksize;

    if (!s->samplerate && !fi.samplerate) {
        printf("sample rate not found in STREAMINFO or frame header\n");
        return -15;
    }
    if (fi.samplerate == 0) {
        fi.samplerate = s->samplerate;
    } else if (s->samplerate && fi.samplerate != s->samplerate) {
        printf("sample rate changed from %d to %d\n",s->samplerate, fi.samplerate);
    }
    //s->samplerate = s->avctx->sample_rate = fi.samplerate;
    s->samplerate = fi.samplerate;

    if (!s->got_streaminfo) {
        allocate_buffers(s);
        s->got_streaminfo = 1;
        dump_headers((FLACStreaminfo *)s);
    }

//    dump_headers(s->avctx, (FLACStreaminfo *)s);

    /* subframes */
    for (i = 0; i < s->channels; i++) {
        nResult = decode_subframe(s, i); 
        if (nResult < 0)
            return nResult;
    }

    align_get_bits(gb);

    /* frame footer */
    skip_bits(gb, 16); /* data crc */

    return 0;
}

//static int flac_decode_frame(FLACContext *fctx, void *data,
//                             int *got_frame_ptr, AVPacket *avpkt)
int flac_decode_frame(FLACContext *fctx, char *data,int size,
                             short* output)
                             
{
    uint8_t* buf = data;
    int buf_size = size;
    FLACContext *s = fctx;
    int i, j = 0, bytes_read = 0;
    int16_t *samples_16;
    int32_t *samples_32;
    int nResult;
//    int ret;

    //*got_frame_ptr = 0;

    if (s->max_framesize == 0) {
        s->max_framesize =
            ff_flac_get_max_frame_size(s->max_blocksize ? s->max_blocksize : FLAC_MAX_BLOCKSIZE,
                                       FLAC_MAX_CHANNELS, 32);
    }

    /* check that there is at least the smallest decodable amount of data.
       this amount corresponds to the smallest valid FLAC frame possible.
       FF F8 69 02 00 00 9A 00 00 34 46 */
    if (buf_size < FLAC_MIN_FRAME_SIZE)
        return buf_size;

    /* check for inline header */
    if (AV_RB32(buf) == MKBETAG('f','L','a','C')) {
        if (!s->got_streaminfo && parse_streaminfo(s, buf, buf_size)) {
            printf("invalid header\n");
            return -1;
        }
        return get_metadata_size(buf, buf_size);
    }

    /* decode frame */
    init_get_bits(&s->gb, buf, buf_size<<3);
    nResult = decode_frame(s);
    if (nResult < 0) {
        printf("decode_frame() failed\n");
        return nResult;
    }
    bytes_read = (get_bits_count(&s->gb)+7)>>3;

    /* get output buffer */
    //s->frame.nb_samples = s->blocksize;
    //if ((ret = avctx->get_buffer(avctx, &s->frame)) < 0) {
        //av_log(avctx, AV_LOG_ERROR, "get_buffer() failed\n");
        //return ret;
    //}
    //samples_16 = (int16_t *)s->frame.data[0];
    //samples_32 = (int32_t *)s->frame.data[0];

    samples_16 = output;
    samples_32 = output;

#define DECORRELATE(left, right)\
            assert(s->channels == 2);\
            for (i = 0; i < s->blocksize; i++) {\
                int a= s->decoded[0][i];\
                int b= s->decoded[1][i];\
                if (s->is32) {\
                    *samples_32++ = (left)  << s->sample_shift;\
                    *samples_32++ = (right) << s->sample_shift;\
                } else {\
                    *samples_16++ = (left)  << s->sample_shift;\
                    *samples_16++ = (right) << s->sample_shift;\
                }\
            }\
            break;

    switch (s->ch_mode) {
    case FLAC_CHMODE_INDEPENDENT:
        for (j = 0; j < s->blocksize; j++) {
            for (i = 0; i < s->channels; i++) {
                if (s->is32)
                    *samples_32++ = s->decoded[i][j] << s->sample_shift;
                else
                    *samples_16++ = s->decoded[i][j] << s->sample_shift;
            }
        }
        break;
    case FLAC_CHMODE_LEFT_SIDE:
        DECORRELATE(a,a-b)
    case FLAC_CHMODE_RIGHT_SIDE:
        DECORRELATE(a+b,b)
    case FLAC_CHMODE_MID_SIDE:
        DECORRELATE( (a-=b>>1) + b, a)
    }
    if (bytes_read > buf_size) {
        printf("overread: %d\n", bytes_read - buf_size);
        return -3;
    }
    if (bytes_read < buf_size) {
        printf("underread: %d orig size: %d\n",buf_size - bytes_read, buf_size);
    }
    fctx->read_byte = bytes_read;
    //*got_frame_ptr   = 1;
    //*(AVFrame *)data = s->frame;
    //return bytes_read;
    return (s->channels*s->blocksize)<<1;
}

//static av_cold int flac_decode_close(AVCodecContext *avctx)
//{
//    FLACContext *s = avctx->priv_data;
//    int i;

//    for (i = 0; i < s->channels; i++) {
//        av_freep(&s->decoded[i]);
//    }

//    return 0;
//}

/*
AVCodec ff_flac_decoder = {
    .name           = "flac",
    .type           = AVMEDIA_TYPE_AUDIO,
    .id             = CODEC_ID_FLAC,
    .priv_data_size = sizeof(FLACContext),
    .init           = flac_decode_init,
    .close          = flac_decode_close,
    .decode         = flac_decode_frame,
    .capabilities   = CODEC_CAP_DR1,
    .long_name= NULL_IF_CONFIG_SMALL("FLAC (Free Lossless Audio Codec)"),
};
*/
