#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include "ringbuf.h"
#include "codec.h"
#include "x264.h"
#include "utils.h"

#define CODEC_FLAG_KEY_FRAME_DROPPED (1 << 3)

typedef struct {
    CODEC_COMMON_MEMBERS

    x264_param_t param;
    x264_t      *x264;
    int          vw, vh;
    pthread_t    thread;
} H264ENC;

static void* encode_thread_proc(void *param)
{
    H264ENC    *enc = (H264ENC*)param;
    x264_nal_t *nals= NULL;
    x264_picture_t pic_in, pic_out;
    int yuvsize = enc->vw * enc->vh * 3 / 2;
    int len, key, num, i;
    uint32_t size, type, pts;

    x264_picture_init(&pic_in );
    x264_picture_init(&pic_out);
    pic_in.img.i_csp       = X264_CSP_I420;
    pic_in.img.i_plane     = 3;
    pic_in.img.i_stride[0] = enc->vw;
    pic_in.img.i_stride[1] = enc->vw / 2;
    pic_in.img.i_stride[2] = enc->vw / 2;

    while (!(enc->flags & CODEC_FLAG_EXIT)) {
        if (!(enc->flags & CODEC_FLAG_START)) { usleep(100*1000); continue; }

        pthread_mutex_lock(&enc->mutex);
        while (enc->cursize == 0 && !(enc->flags & CODEC_FLAG_EXIT)) pthread_cond_wait(&enc->cond, &enc->mutex);
        if (enc->cursize >= yuvsize) {
            pic_in.img.plane[0] = enc->buff + enc->head;
            pic_in.img.plane[1] = enc->buff + enc->head + enc->vw * enc->vh * 4 / 4;
            pic_in.img.plane[2] = enc->buff + enc->head + enc->vw * enc->vh * 5 / 4;
            pic_in.i_type       =(enc->flags & CODEC_FLAG_REQIDR) ? X264_TYPE_IDR : 0;
            enc->flags         &=~CODEC_FLAG_REQIDR;
            enc->head          += yuvsize;
            enc->cursize       -= yuvsize;
            if (enc->head == enc->maxsize) enc->head = 0;
            pts = get_tick_count();
            len = x264_encoder_encode(enc->x264, &nals, &num, &pic_in, &pic_out);
        } else len = 0;
        pthread_mutex_unlock(&enc->mutex);

        if (len) { // get h264 data
            key = (nals[0].i_type == NAL_SPS);
            if ((enc->flags & CODEC_FLAG_KEY_FRAME_DROPPED) && !key) {
                printf("h264enc last key frame has dropped, and current frame is non-key frame, so drop it !\n");
            } else {
                CODEC *next = (CODEC*)enc->next;
                pthread_mutex_lock(&next->mutex);
                if (sizeof(uint32_t) * 3 + len <= next->maxsize - next->cursize) {
                    size = len; type = CODEC_FOURCC((key ? 'V' : 'v'), 0, 0, 0);
                    next->tail    = ringbuf_write(next->buff, next->maxsize, next->tail, (uint8_t*)&size, sizeof(uint32_t));
                    next->tail    = ringbuf_write(next->buff, next->maxsize, next->tail, (uint8_t*)&type, sizeof(uint32_t));
                    next->tail    = ringbuf_write(next->buff, next->maxsize, next->tail, (uint8_t*)&pts , sizeof(uint32_t));
                    next->cursize+= sizeof(uint32_t) * 3;
                    for (i=0; i<num; i++) next->tail = ringbuf_write(next->buff, next->maxsize, next->tail, nals[i].p_payload, nals[i].i_payload);
                    next->cursize+= len;
                    pthread_cond_signal(&next->cond);
                    if (key) enc->flags &= ~CODEC_FLAG_KEY_FRAME_DROPPED;
                } else {
                    printf("h264enc %s frame dropped !\n", key ? "key" : "non-key");
                    if (key) enc->flags |=  CODEC_FLAG_KEY_FRAME_DROPPED;
                }
                pthread_mutex_unlock(&next->mutex);
            }
        }
    }
    return NULL;
}

static void h264enc_free(void *ctxt)
{
    H264ENC *enc = (H264ENC*)ctxt;
    pthread_mutex_lock(&enc->mutex);
    enc->flags |= CODEC_FLAG_EXIT;
    pthread_cond_signal(&enc->cond);
    pthread_mutex_unlock(&enc->mutex);
    pthread_join(enc->thread, NULL);
    pthread_mutex_destroy(&enc->mutex);
    pthread_cond_destroy (&enc->cond );
    if (enc->x264) x264_encoder_close(enc->x264);
    free(enc);
}

static void h264enc_config(void *ctxt, int flags, void *param1, uint32_t param2)
{
    H264ENC *enc = (H264ENC*)ctxt;
    if (flags & CODEC_CONFIG_CLEAR_BUFF) {
        pthread_mutex_lock(&enc->mutex);
        enc->head = enc->tail = enc->cursize = 0;
        pthread_mutex_unlock(&enc->mutex);
    }
    if (flags & CODEC_CONFIG_REQUEST_IDR) {
        enc->flags |= CODEC_FLAG_REQIDR;
    }
    if (flags & CODEC_CONFIG_SET_BITRATE) {
        int bitrate = (int)param2, ret;
        enc->param.rc.i_bitrate         = bitrate / 1000;
        enc->param.rc.i_rc_method       = X264_RC_ABR;
        enc->param.rc.f_rate_tolerance  = 2;
        enc->param.rc.i_vbv_max_bitrate = 2 * bitrate / 1000;
        enc->param.rc.i_vbv_buffer_size = 2 * bitrate / 1000;
        ret = x264_encoder_reconfig(enc->x264, &enc->param);
        printf("x264_encoder_reconfig bitrate: %d, ret: %d\n", (int)param2, ret);
    }
}

void* h264enc_init(int bufsize, void *next, int bitrate, int frmrate , int w, int h)
{
    x264_nal_t *nals= NULL;
    H264ENC    *enc = NULL;
    int         n, i;

    if (bufsize < w * h * 3 / 2) bufsize = (w * h * 3 / 2) * 3;
    else bufsize = bufsize - bufsize % (w * h * 3 / 2);
    if (!(enc = codec_init("h264enc", sizeof(H264ENC), bufsize, next))) return NULL;
    enc->free   = h264enc_free;
    enc->config = h264enc_config;

    x264_param_default_preset(&enc->param, "ultrafast", "zerolatency");
    x264_param_apply_profile (&enc->param, "baseline");
    enc->param.b_repeat_headers = 1;
    enc->param.i_timebase_num   = 1;
    enc->param.i_timebase_den   = 1000;
    enc->param.i_csp            = X264_CSP_I420;
    enc->param.i_width          = w;
    enc->param.i_height         = h;
    enc->param.i_fps_num        = frmrate;
    enc->param.i_fps_den        = 1;
//  enc->param.i_slice_count_max= 1;
//  enc->param.i_threads        = 1;
    enc->param.i_keyint_min     = frmrate * 2;
    enc->param.i_keyint_max     = frmrate * 5;
    enc->param.rc.i_bitrate     = bitrate / 1000;
#if 0 // X264_RC_CQP
    enc->param.rc.i_rc_method       = X264_RC_CQP;
    enc->param.rc.i_qp_constant     = 35;
    enc->param.rc.i_qp_min          = 25;
    enc->param.rc.i_qp_max          = 50;
#endif
#if 0 // X264_RC_CRF
    enc->param.rc.i_rc_method       = X264_RC_CRF;
    enc->param.rc.f_rf_constant     = 25;
    enc->param.rc.f_rf_constant_max = 50;
#endif
#if 1 // X264_RC_ABR
    enc->param.rc.i_rc_method       = X264_RC_ABR;
    enc->param.rc.f_rate_tolerance  = 2;
    enc->param.rc.i_vbv_max_bitrate = 2 * bitrate / 1000;
    enc->param.rc.i_vbv_buffer_size = 2 * bitrate / 1000;
#endif

    enc->vw   = w;
    enc->vh   = h;
    enc->x264 = x264_encoder_open(&enc->param);

    x264_encoder_headers(enc->x264, &nals, &n);
    for (i=0; i<n; i++) {
        switch (nals[i].i_type) {
        case NAL_SPS:
            enc->spsinfo[0] = MIN(nals[i].i_payload, 255);
            memcpy(enc->spsinfo + 1, nals[i].p_payload, enc->spsinfo[0]);
            break;
        case NAL_PPS:
            enc->ppsinfo[0] = MIN(nals[i].i_payload, 255);
            memcpy(enc->ppsinfo + 1, nals[i].p_payload, enc->ppsinfo[0]);
            break;
        }
    }

    pthread_create(&enc->thread, NULL, encode_thread_proc, enc);
    return enc;
}
