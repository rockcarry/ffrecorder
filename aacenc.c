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

typedef struct {
    CODEC_COMMON_MEMBERS
    faacEncHandle faacenc;
    unsigned long insamples;
    unsigned long outbufsize;
    unsigned long aaccfgsize;
    uint8_t      *aaccfgptr;
    pthread_t     thread;
} AACENC;

static void* encode_thread_proc(void *param)
{
    AACENC  *enc = (AACENC*)param;
    uint8_t  buffer[8192];
    uint32_t size = 0, type = 0, pts = 0;

    while (!(enc->flags & CODEC_FLAG_EXIT)) {
        if (!(enc->flags & CODEC_FLAG_START)) { usleep(100*1000); continue; }

        pthread_mutex_lock(&enc->mutex);
        while (enc->cursize < (int)(enc->insamples * sizeof(int16_t)) && !(enc->flags & CODEC_FLAG_EXIT)) pthread_cond_wait(&enc->cond, &enc->mutex);
        if (!(enc->flags & CODEC_FLAG_EXIT)) {
            size = faacEncEncode(enc->faacenc, (int32_t*)(enc->buff + enc->head), enc->insamples, buffer, sizeof(buffer));
            type = CODEC_FOURCC('A', 0, 0, 0);
            pts  = get_tick_count();
            enc->head   += enc->insamples * sizeof(int16_t);
            enc->cursize-= enc->insamples * sizeof(int16_t);
            if (enc->cursize < (int)(enc->insamples * sizeof(int16_t))) {
                memmove(enc->buff, enc->buff + enc->head, enc->cursize);
                enc->head = 0; enc->tail = enc->cursize;
            }
        }
        pthread_mutex_unlock(&enc->mutex);
        if ((int32_t)size > 0) codec_writeframe(enc->next, buffer, size, type, pts);
    }
    return NULL;
}

static void aacenc_free(void *ctxt)
{
    AACENC *enc = (AACENC*)ctxt;
    pthread_mutex_lock(&enc->mutex);
    enc->flags |= CODEC_FLAG_EXIT;
    pthread_cond_signal(&enc->cond);
    pthread_mutex_unlock(&enc->mutex);
    pthread_join(enc->thread, NULL);
    pthread_mutex_destroy(&enc->mutex);
    pthread_cond_destroy (&enc->cond );
    if (enc->faacenc) faacEncClose(enc->faacenc);
    free(enc);
}

void* aacenc_init(int bufsize, void *next, int bitrate, int samprate, int channels)
{
    faacEncConfigurationPtr conf;
    AACENC *enc = codec_init("aacenc", sizeof(AACENC), MAX(4096, bufsize), next);
    if (!enc) return NULL;

    enc->free    = aacenc_free;
    enc->faacenc = faacEncOpen((unsigned long)samprate, (unsigned int)channels, &enc->insamples, &enc->outbufsize);
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

    pthread_create(&enc->thread, NULL, encode_thread_proc, enc);
    return enc;
}
