#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include "ringbuf.h"
#include "codec.h"
#include "utils.h"

#define OUT_BUF_SIZE (1024 * 1 * 1)
typedef struct {
    CODEC_INTERFACE_FUNCS
} BUFENC;

static void bufenc_uninit(void *ctxt)
{
    BUFENC *enc = (BUFENC*)ctxt;
    if (!ctxt) return;
    pthread_mutex_destroy(&enc->omutex);
    pthread_cond_destroy (&enc->ocond );
    free(enc);
}

static void bufenc_write(void *ctxt, void *buf, int len)
{
    BUFENC *enc = (BUFENC*)ctxt;
    if (!ctxt) return;
    pthread_mutex_lock(&enc->omutex);
    if (sizeof(uint32_t) + sizeof(uint32_t) + ((uint32_t)len >> 8) <= enc->omaxsize - enc->ocursize) {
        uint32_t timestamp = get_tick_count();
        enc->otail     = ringbuf_write(enc->obuff, enc->omaxsize, enc->otail, (uint8_t*)&timestamp, sizeof(timestamp) );
        enc->otail     = ringbuf_write(enc->obuff, enc->omaxsize, enc->otail, (uint8_t*)&len      , sizeof(len      ) );
        enc->otail     = ringbuf_write(enc->obuff, enc->omaxsize, enc->otail, (uint8_t*) buf      , (uint32_t)len >> 8);
        enc->ocursize += sizeof(timestamp) + sizeof(uint32_t) + ((uint32_t)len >> 8);
        pthread_cond_signal(&enc->ocond);
    }
    pthread_mutex_unlock(&enc->omutex);
}

static void bufenc_start(void *ctxt, int start)
{
    BUFENC *enc = (BUFENC*)ctxt;
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

static void bufenc_reset(void *ctxt, int type)
{
    BUFENC *enc = (BUFENC*)ctxt;
    if (!ctxt) return;
    if (type & (CODEC_CLEAR_INBUF|CODEC_CLEAR_OUTBUF)) {
        pthread_mutex_lock(&enc->omutex);
        enc->ohead = enc->otail = enc->ocursize = 0;
        pthread_mutex_unlock(&enc->omutex);
    }
}

CODEC* bufenc_init(int obufsize, char *name)
{
    BUFENC *enc = NULL;
    if (obufsize < OUT_BUF_SIZE) obufsize = OUT_BUF_SIZE;
    if (!(enc = calloc(1, sizeof(BUFENC) + obufsize))) return NULL;

    strncpy(enc->name, name, sizeof(enc->name));
    enc->uninit     = bufenc_uninit;
    enc->write      = bufenc_write;
    enc->read       = codec_read_common;
    enc->obuflock   = codec_obuflock_common;
    enc->obufunlock = codec_obufunlock_common;
    enc->start      = bufenc_start;
    enc->reset      = bufenc_reset;
    enc->omaxsize   = obufsize;
    enc->obuff      = (uint8_t*)enc + sizeof(BUFENC);

    // init mutex & cond
    pthread_mutex_init(&enc->omutex, NULL);
    pthread_cond_init (&enc->ocond , NULL);
    return (CODEC*)enc;
}
