/*
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>

#include <libavcodec/avcodec.h>
#include <libavutil/audioconvert.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>

#include "talloc.h"

#include "config.h"
#include "mp_msg.h"
#include "options.h"

#include "ad_internal.h"
#include "libaf/reorder_ch.h"

#include "mpbswap.h"

#ifdef CONFIG_LIBAVRESAMPLE
#include <libavresample/avresample.h>
#endif

static const ad_info_t info =
{
    "libavcodec audio decoders",
    "ffmpeg",
    "",
    "",
    "",
    .print_name = "libavcodec",
};

LIBAD_EXTERN(ffmpeg)

struct priv {
    AVCodecContext *avctx;
    AVFrame *avframe;
    char *output;
    int output_left;
    int unitsize;
    int previous_data_left;  // input demuxer packet data

#ifdef CONFIG_LIBAVRESAMPLE
    AVAudioResampleContext *avr;
    enum AVSampleFormat resample_fmt;
    enum AVSampleFormat out_fmt;
    int resample_channels;
    uint8_t *resample_buf;
    uint64_t resample_buf_size;
#endif
};

static int preinit(sh_audio_t *sh)
{
    return 1;
}

static const int sample_fmt_map[][2] = {
    { AV_SAMPLE_FMT_U8,  AF_FORMAT_U8 },
    { AV_SAMPLE_FMT_S16, AF_FORMAT_S16_NE },
    { AV_SAMPLE_FMT_S32, AF_FORMAT_S32_NE },
    { AV_SAMPLE_FMT_FLT, AF_FORMAT_FLOAT_NE },
};

static int sample_fmt_lavc2native(enum AVSampleFormat sample_fmt)
{
    for (int i = 0; i < FF_ARRAY_ELEMS(sample_fmt_map); i++)
        if (sample_fmt_map[i][0] == sample_fmt)
            return sample_fmt_map[i][1];
    return AF_FORMAT_UNKNOWN;
}

/* Prefer playing audio with the samplerate given in container data
 * if available, but take number the number of channels and sample format
 * from the codec, since if the codec isn't using the correct values for
 * those everything breaks anyway.
 */
static int setup_format(sh_audio_t *sh_audio)
{
    struct priv *priv = sh_audio->context;
    AVCodecContext *codec = priv->avctx;

    int sample_format = sample_fmt_lavc2native(codec->sample_fmt);
    if (sample_format == AF_FORMAT_UNKNOWN) {
#ifndef CONFIG_LIBAVRESAMPLE
        if (av_sample_fmt_is_planar(codec->sample_fmt))
            mp_msg(MSGT_DECAUDIO, MSGL_ERR,
                   "The player has been compiled without libavresample "
                   "support,\nwhich is needed with this libavcodec decoder "
                   "version.\nCompile with libavresample enabled to make "
                   "audio decoding work!\n");
        else
            mp_msg(MSGT_DECAUDIO, MSGL_ERR, "Unsupported sample format\n");
        goto error;
#else
        if (priv->avr && (priv->resample_fmt      != codec->sample_fmt ||
                          priv->resample_channels != codec->channels))
            avresample_free(&priv->avr);

        if (!priv->avr) {
            int ret;
            uint8_t error[128];
            enum AVSampleFormat out_fmt =
                av_get_packed_sample_fmt(codec->sample_fmt);
            uint64_t ch_layout = codec->channel_layout;

            mp_msg(MSGT_DECAUDIO, MSGL_V,
                   "(Re)initializing libavresample format conversion...\n");

            if (!ch_layout)
                ch_layout = av_get_default_channel_layout(codec->channels);

            /* if lavc format is planar, try just getting packed equivalent */
            sample_format = sample_fmt_lavc2native(out_fmt);
            if (sample_format == AF_FORMAT_UNKNOWN) {
                /* fallback to s16 */
                out_fmt = AV_SAMPLE_FMT_S16;
                sample_format = AF_FORMAT_S16_NE;
            }

            priv->avr = avresample_alloc_context();
            if (!priv->avr) {
                mp_msg(MSGT_DECAUDIO, MSGL_FATAL, "Out of memory.\n");
                abort();
            }
            av_opt_set_int(priv->avr, "in_channel_layout",  ch_layout, 0);
            av_opt_set_int(priv->avr, "out_channel_layout", ch_layout, 0);
            av_opt_set_int(priv->avr, "in_sample_rate",  codec->sample_rate, 0);
            av_opt_set_int(priv->avr, "out_sample_rate", codec->sample_rate, 0);
            av_opt_set_int(priv->avr, "in_sample_fmt",   codec->sample_fmt, 0);
            av_opt_set_int(priv->avr, "out_sample_fmt",  out_fmt, 0);

            if ((ret = avresample_open(priv->avr)) < 0) {
                av_strerror(ret, error, sizeof(error));
                mp_msg(MSGT_DECAUDIO, MSGL_ERR,
                       "Error opening libavresample: %s.\n", error);
                goto error;
            }
            priv->resample_fmt      = codec->sample_fmt;
            priv->resample_channels = codec->channels;
            priv->out_fmt           = out_fmt;
            priv->unitsize          = av_get_bytes_per_sample(out_fmt) *
                                      codec->channels;
        } else
            sample_format = sh_audio->sample_format;
    } else if (priv->avr) {
        avresample_free(&priv->avr);
#endif
    }

    bool broken_srate        = false;
    int samplerate           = codec->sample_rate;
    int container_samplerate = sh_audio->container_out_samplerate;
    if (!container_samplerate && sh_audio->wf)
        container_samplerate = sh_audio->wf->nSamplesPerSec;
    if (codec->codec_id == CODEC_ID_AAC
        && samplerate == 2 * container_samplerate)
        broken_srate = true;
    else if (container_samplerate)
        samplerate = container_samplerate;

    if (codec->channels != sh_audio->channels ||
        samplerate != sh_audio->samplerate ||
        sample_format != sh_audio->sample_format) {
        sh_audio->channels = codec->channels;
        sh_audio->samplerate = samplerate;
        sh_audio->sample_format = sample_format;
        sh_audio->samplesize = af_fmt2bits(sh_audio->sample_format) / 8;
        if (broken_srate)
            mp_msg(MSGT_DECAUDIO, MSGL_WARN,
                   "Ignoring broken container sample rate for AAC with SBR\n");
        return 1;
    }
    return 0;
error:
#ifdef CONFIG_LIBAVRESAMPLE
    avresample_free(&priv->avr);
#endif
    return -1;
}

static int init(sh_audio_t *sh_audio)
{
    struct MPOpts *opts = sh_audio->opts;
    AVCodecContext *lavc_context;
    AVCodec *lavc_codec;

    if (sh_audio->codec->dll) {
        lavc_codec = avcodec_find_decoder_by_name(sh_audio->codec->dll);
        if (!lavc_codec) {
            mp_tmsg(MSGT_DECAUDIO, MSGL_ERR,
                    "Cannot find codec '%s' in libavcodec...\n",
                    sh_audio->codec->dll);
            return 0;
        }
    } else if (!sh_audio->libav_codec_id) {
        mp_tmsg(MSGT_DECAUDIO, MSGL_INFO, "No Libav codec ID known. "
                "Generic lavc decoder is not applicable.\n");
        return 0;
    } else {
        lavc_codec = avcodec_find_decoder(sh_audio->libav_codec_id);
        if (!lavc_codec) {
            mp_tmsg(MSGT_DECAUDIO, MSGL_INFO, "Libavcodec has no decoder "
                   "for this codec\n");
            return 0;
        }
    }

    sh_audio->codecname = lavc_codec->long_name;
    if (!sh_audio->codecname)
        sh_audio->codecname = lavc_codec->name;

    struct priv *ctx = talloc_zero(NULL, struct priv);
    sh_audio->context = ctx;
    lavc_context = avcodec_alloc_context3(lavc_codec);
    ctx->avctx = lavc_context;
    ctx->avframe = avcodec_alloc_frame();

    // Always try to set - option only exists for AC3 at the moment
    av_opt_set_double(lavc_context, "drc_scale", opts->drc_level,
                      AV_OPT_SEARCH_CHILDREN);
    lavc_context->sample_rate = sh_audio->samplerate;
    lavc_context->bit_rate = sh_audio->i_bps * 8;
    if (sh_audio->wf) {
        lavc_context->channels = sh_audio->wf->nChannels;
        lavc_context->sample_rate = sh_audio->wf->nSamplesPerSec;
        lavc_context->bit_rate = sh_audio->wf->nAvgBytesPerSec * 8;
        lavc_context->block_align = sh_audio->wf->nBlockAlign;
        lavc_context->bits_per_coded_sample = sh_audio->wf->wBitsPerSample;
    }
    lavc_context->request_channels = opts->audio_output_channels;
    lavc_context->codec_tag = sh_audio->format; //FOURCC
    lavc_context->codec_type = AVMEDIA_TYPE_AUDIO;
    lavc_context->codec_id = lavc_codec->id; // not sure if required, imho not --A'rpi

    /* alloc extra data */
    if (sh_audio->wf && sh_audio->wf->cbSize > 0) {
        lavc_context->extradata = av_mallocz(sh_audio->wf->cbSize + FF_INPUT_BUFFER_PADDING_SIZE);
        lavc_context->extradata_size = sh_audio->wf->cbSize;
        memcpy(lavc_context->extradata, sh_audio->wf + 1,
               lavc_context->extradata_size);
    }

    // for QDM2
    if (sh_audio->codecdata_len && sh_audio->codecdata &&
            !lavc_context->extradata) {
        lavc_context->extradata = av_malloc(sh_audio->codecdata_len +
                                            FF_INPUT_BUFFER_PADDING_SIZE);
        lavc_context->extradata_size = sh_audio->codecdata_len;
        memcpy(lavc_context->extradata, (char *)sh_audio->codecdata,
               lavc_context->extradata_size);
    }

    /* open it */
    if (avcodec_open2(lavc_context, lavc_codec, NULL) < 0) {
        mp_tmsg(MSGT_DECAUDIO, MSGL_ERR, "Could not open codec.\n");
        uninit(sh_audio);
        return 0;
    }
    mp_msg(MSGT_DECAUDIO, MSGL_V, "INFO: libavcodec \"%s\" init OK!\n",
           lavc_codec->name);

    if (sh_audio->format == 0x3343414D) {
        // MACE 3:1
        sh_audio->ds->ss_div = 2 * 3; // 1 samples/packet
        sh_audio->ds->ss_mul = 2 * sh_audio->wf->nChannels; // 1 byte*ch/packet
    } else if (sh_audio->format == 0x3643414D) {
        // MACE 6:1
        sh_audio->ds->ss_div = 2 * 6; // 1 samples/packet
        sh_audio->ds->ss_mul = 2 * sh_audio->wf->nChannels; // 1 byte*ch/packet
    }

    // Decode at least 1 byte:  (to get header filled)
    for (int tries = 0;;) {
        int x = decode_audio(sh_audio, sh_audio->a_buffer, 1,
                             sh_audio->a_buffer_size);
        if (x > 0) {
            sh_audio->a_buffer_len = x;
            break;
        }
        if (++tries >= 5) {
            mp_msg(MSGT_DECAUDIO, MSGL_ERR,
                   "ad_ffmpeg: initial decode failed\n");
            uninit(sh_audio);
            return 0;
        }
    }

    sh_audio->i_bps = lavc_context->bit_rate / 8;
    if (sh_audio->wf && sh_audio->wf->nAvgBytesPerSec)
        sh_audio->i_bps = sh_audio->wf->nAvgBytesPerSec;

    return 1;
}

static void uninit(sh_audio_t *sh)
{
    sh->codecname = NULL;
    struct priv *ctx = sh->context;
    if (!ctx)
        return;
    AVCodecContext *lavc_context = ctx->avctx;

    if (lavc_context) {
        if (avcodec_close(lavc_context) < 0)
            mp_tmsg(MSGT_DECVIDEO, MSGL_ERR, "Could not close codec.\n");
        av_freep(&lavc_context->extradata);
        av_freep(&lavc_context);
    }
#ifdef CONFIG_LIBAVRESAMPLE
    avresample_free(&ctx->avr);
#endif
#if LIBAVCODEC_VERSION_INT >= (54 << 16 | 28 << 8)
    avcodec_free_frame(&ctx->avframe);
#else
    av_free(ctx->avframe);
#endif
    talloc_free(ctx);
    sh->context = NULL;
}

static int control(sh_audio_t *sh, int cmd, void *arg, ...)
{
    struct priv *ctx = sh->context;
    switch (cmd) {
    case ADCTRL_RESYNC_STREAM:
        avcodec_flush_buffers(ctx->avctx);
        ds_clear_parser(sh->ds);
        ctx->previous_data_left = 0;
        ctx->output_left = 0;
        return CONTROL_TRUE;
    }
    return CONTROL_UNKNOWN;
}

static int decode_new_packet(struct sh_audio *sh)
{
    struct priv *priv = sh->context;
    AVCodecContext *avctx = priv->avctx;
    double pts = MP_NOPTS_VALUE;
    int insize;
    bool packet_already_used = priv->previous_data_left;
    struct demux_packet *mpkt = ds_get_packet2(sh->ds,
                                               priv->previous_data_left);
    unsigned char *start;
    if (!mpkt) {
        assert(!priv->previous_data_left);
        start = NULL;
        insize = 0;
        ds_parse(sh->ds, &start, &insize, pts, 0);
        if (insize <= 0)
            return -1;  // error or EOF
    } else {
        assert(mpkt->len >= priv->previous_data_left);
        if (!priv->previous_data_left) {
            priv->previous_data_left = mpkt->len;
            pts = mpkt->pts;
        }
        insize = priv->previous_data_left;
        start = mpkt->buffer + mpkt->len - priv->previous_data_left;
        int consumed = ds_parse(sh->ds, &start, &insize, pts, 0);
        priv->previous_data_left -= consumed;
        priv->previous_data_left = FFMAX(priv->previous_data_left, 0);
    }

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = start;
    pkt.size = insize;
    if (mpkt && mpkt->avpacket) {
        pkt.side_data = mpkt->avpacket->side_data;
        pkt.side_data_elems = mpkt->avpacket->side_data_elems;
    }
    if (pts != MP_NOPTS_VALUE && !packet_already_used) {
        sh->pts = pts;
        sh->pts_bytes = 0;
    }
    int got_frame = 0;
    int ret = avcodec_decode_audio4(avctx, priv->avframe, &got_frame, &pkt);
    // LATM may need many packets to find mux info
    if (ret == AVERROR(EAGAIN))
        return 0;
    if (ret < 0) {
        mp_msg(MSGT_DECAUDIO, MSGL_V, "lavc_audio: error\n");
        return -1;
    }
    // The "insize >= ret" test is sanity check against decoder overreads
    if (!sh->parser && insize >= ret)
        priv->previous_data_left = insize - ret;
    if (!got_frame)
        return 0;

    int format_result = setup_format(sh);
    if (format_result < 0)
        return format_result;

#ifdef CONFIG_LIBAVRESAMPLE
    if (priv->avr) {
        int ret;
        uint64_t needed_size = av_samples_get_buffer_size(
                NULL, priv->resample_channels, priv->avframe->nb_samples,
                priv->resample_fmt, 0);
        if (needed_size > priv->resample_buf_size) {
            priv->resample_buf = talloc_realloc(priv, priv->resample_buf,
                                                uint8_t, needed_size);
            priv->resample_buf_size = needed_size;
        }

        ret = avresample_convert(priv->avr, &priv->resample_buf,
                priv->resample_buf_size, priv->avframe->nb_samples,
                priv->avframe->extended_data, priv->avframe->linesize[0],
                priv->avframe->nb_samples);
        if (ret < 0) {
            uint8_t error[128];
            av_strerror(ret, error, sizeof(error));
            mp_msg(MSGT_DECAUDIO, MSGL_ERR,
                   "Error during sample format conversion: %s.\n", error);
            return -1;
        }

        assert(ret == priv->avframe->nb_samples);

        priv->output = priv->resample_buf;
        priv->output_left = priv->unitsize * ret;
    } else
#endif
    {
        uint64_t unitsize = av_get_bytes_per_sample(avctx->sample_fmt) *
                            (uint64_t)avctx->channels;
        if (unitsize > 100000)
            abort();
        priv->unitsize = unitsize;
        uint64_t output_left = unitsize * priv->avframe->nb_samples;
        if (output_left > 500000000)
            abort();
        priv->output_left = output_left;
        priv->output = priv->avframe->data[0];
    }

    mp_dbg(MSGT_DECAUDIO, MSGL_DBG2, "Decoded %d -> %d  \n", insize,
           priv->output_left);
    return format_result;
}


static int decode_audio(sh_audio_t *sh_audio, unsigned char *buf, int minlen,
                        int maxlen)
{
    struct priv *priv = sh_audio->context;
    AVCodecContext *avctx = priv->avctx;

    int len = -1;
    while (len < minlen) {
        if (!priv->output_left) {
            if (decode_new_packet(sh_audio) != 0)
                break;
            continue;
        }
        int size = (minlen - len + priv->unitsize - 1);
        size -= size % priv->unitsize;
        size = FFMIN(size, priv->output_left);
        if (size > maxlen)
            abort();
        memcpy(buf, priv->output, size);
        priv->output += size;
        priv->output_left -= size;
        if (avctx->channels >= 5) {
            int samplesize = av_get_bytes_per_sample(avctx->sample_fmt);
            reorder_channel_nch(buf, AF_CHANNEL_LAYOUT_LAVC_DEFAULT,
                                AF_CHANNEL_LAYOUT_MPLAYER_DEFAULT,
                                avctx->channels,
                                size / samplesize, samplesize);
        }
        if (len < 0)
            len = size;
        else
            len += size;
        buf += size;
        maxlen -= size;
        sh_audio->pts_bytes += size;
    }
    return len;
}
