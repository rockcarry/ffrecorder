#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include "ringbuf.h"
#include "codec.h"
#include "faac.h"
#include "utils.h"

#define IN_BUF_SIZE  (1024 * 4 * 3)
#define OUT_BUF_SIZE (1024 * 8 * 1)
typedef struct {
    CODEC_INTERFACE_FUNCS

    uint8_t  ibuff[IN_BUF_SIZE];
    int      ihead;
    int      itail;
    int      isize;

    pthread_mutex_t imutex;
    pthread_cond_t  icond;
    pthread_t       thread;

    faacEncHandle faacenc;
    unsigned long insamples;
    unsigned long outbufsize;
    unsigned long aaccfgsize;
    uint8_t      *aaccfgptr;
} AACENC;

static void* aenc_encode_thread_proc(void *param)
{
    AACENC *enc = (AACENC*)param;
    uint8_t outbuf[8192];
    int32_t len = 0;

    while (!(enc->status & CODEC_FLAG_EXIT)) {
        if (!(enc->status & CODEC_FLAG_START)) { usleep(100*1000); continue; }

        pthread_mutex_lock(&enc->imutex);
        while (enc->isize < (int)(enc->insamples * sizeof(int16_t)) && !(enc->status & CODEC_FLAG_EXIT)) pthread_cond_wait(&enc->icond, &enc->imutex);
        if (!(enc->status & CODEC_FLAG_EXIT)) {
            len = faacEncEncode(enc->faacenc, (int32_t*)(enc->ibuff + enc->ihead), enc->insamples, outbuf, sizeof(outbuf));
            enc->ihead += enc->insamples * sizeof(int16_t);
            enc->isize -= enc->insamples * sizeof(int16_t);
            if (enc->isize < (int)(enc->insamples * sizeof(int16_t))) {
                memmove(enc->ibuff, enc->ibuff + enc->ihead, enc->isize);
                enc->ihead = 0; enc->itail = enc->isize;
            }
        }
        pthread_mutex_unlock(&enc->imutex);

        pthread_mutex_lock(&enc->omutex);
        if (len > 0 && sizeof(uint32_t) + sizeof(uint32_t) + len <= enc->omaxsize - enc->ocursize) {
            uint32_t timestamp = get_tick_count();
            uint32_t typelen   = 'A' | (len << 8);
            enc->otail     = ringbuf_write(enc->obuff, enc->omaxsize, enc->otail, (uint8_t*)&timestamp, sizeof(timestamp));
            enc->otail     = ringbuf_write(enc->obuff, enc->omaxsize, enc->otail, (uint8_t*)&typelen  , sizeof(typelen  ));
            enc->otail     = ringbuf_write(enc->obuff, enc->omaxsize, enc->otail, outbuf, len);
            enc->ocursize += sizeof(timestamp) + sizeof(typelen) + len;
            pthread_cond_signal(&enc->ocond);
        } else {
            if (len > 0) printf("aenc drop data %d !\n", len);
        }
        pthread_mutex_unlock(&enc->omutex);
    }
    return NULL;
}

static void aacenc_uninit(void *ctxt)
{
    AACENC *enc = (AACENC*)ctxt;
    if (!ctxt) return;

    pthread_mutex_lock(&enc->imutex);
    enc->status |= CODEC_FLAG_EXIT;
    pthread_cond_signal(&enc->icond);
    pthread_mutex_unlock(&enc->imutex);
    pthread_join(enc->thread, NULL);
    pthread_mutex_destroy(&enc->imutex);
    pthread_cond_destroy (&enc->icond );
    pthread_mutex_destroy(&enc->omutex);
    pthread_cond_destroy (&enc->ocond );
    if (enc->faacenc) faacEncClose(enc->faacenc);
    free(enc);
}

static void aacenc_write(void *ctxt, void *buf, int len)
{
    int nwrite;
    AACENC *enc = (AACENC*)ctxt;
    if (!ctxt) return;
    pthread_mutex_lock(&enc->imutex);
    nwrite = MIN(len, (int)sizeof(enc->ibuff) - enc->itail);
    if (nwrite > 0) {
        enc->itail = ringbuf_write(enc->ibuff, sizeof(enc->ibuff), enc->itail, buf, nwrite);
        enc->isize+= nwrite;
        pthread_cond_signal(&enc->icond);
    }
    pthread_mutex_unlock(&enc->imutex);
}

static void aacenc_start(void *ctxt, int start)
{
    AACENC *enc = (AACENC*)ctxt;
    if (!ctxt) return;
    if (start) {
        enc->status |= CODEC_FLAG_START;
    } else {
        pthread_mutex_lock(&enc->omutex);
        enc->status &=~CODEC_FLAG_START;
        pthread_cond_signal(&enc->ocond);
        pthread_mutex_unlock(&enc->omutex);
    }
}

static void aacenc_reset(void *ctxt, int type)
{
    AACENC *enc = (AACENC*)ctxt;
    if (!ctxt) return;
    if (type & CODEC_CLEAR_INBUF) {
        pthread_mutex_lock(&enc->imutex);
        enc->ihead = enc->itail = enc->isize = 0;
        pthread_mutex_unlock(&enc->imutex);
    }
    if (type & CODEC_CLEAR_OUTBUF) {
        pthread_mutex_lock(&enc->omutex);
        enc->ohead = enc->otail = enc->ocursize = 0;
        pthread_mutex_unlock(&enc->omutex);
    }
}

CODEC* aacenc_init(int obufsize, int channels, int samplerate, int bitrate)
{
    faacEncConfigurationPtr conf;
    AACENC *enc = NULL;
    if (obufsize < OUT_BUF_SIZE) obufsize = OUT_BUF_SIZE;
    if (!(enc = calloc(1, sizeof(AACENC) + obufsize))) return NULL;

    strncpy(enc->name, "aacenc", sizeof(enc->name));
    enc->uninit     = aacenc_uninit;
    enc->write      = aacenc_write;
    enc->read       = codec_read_common;
    enc->obuflock   = codec_obuflock_common;
    enc->obufunlock = codec_obufunlock_common;
    enc->start      = aacenc_start;
    enc->reset      = aacenc_reset;
    enc->omaxsize   = obufsize;
    enc->obuff      = (uint8_t*)enc + sizeof(AACENC);

    // init mutex & cond
    pthread_mutex_init(&enc->imutex, NULL);
    pthread_cond_init (&enc->icond , NULL);
    pthread_mutex_init(&enc->omutex, NULL);
    pthread_cond_init (&enc->ocond , NULL);

    enc->faacenc = faacEncOpen((unsigned long)samplerate, (unsigned int)channels, &enc->insamples, &enc->outbufsize);
    conf = faacEncGetCurrentConfiguration(enc->faacenc);
    conf->aacObjectType = LOW;
    conf->mpegVersion   = MPEG4;
    conf->useLfe        = 0;
    conf->useTns        = 0;
    conf->allowMidside  = 0;
    conf->bitRate       = bitrate;
    conf->outputFormat  = 0;
    conf->inputFormat   = FAAC_INPUT_16BIT;
    conf->shortctl      = SHORTCTL_NORMAL;
    conf->quantqual     = 88;
    faacEncSetConfiguration(enc->faacenc, conf);
    faacEncGetDecoderSpecificInfo(enc->faacenc, &enc->aaccfgptr, &enc->aaccfgsize);
    memcpy(enc->aacinfo, enc->aaccfgptr, MIN(sizeof(enc->aacinfo), enc->aaccfgsize));

    pthread_create(&enc->thread, NULL, aenc_encode_thread_proc, enc);
    return (CODEC*)enc;
}
