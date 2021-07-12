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

typedef struct {
    CODEC_INTERFACE_FUNCS

    int      head;
    int      tail;
    int      size_cur;
    int      size_max;
    uint8_t *buffer;

    #define TS_EXIT  (1 << 0)
    #define TS_START (1 << 1)
    int      status;

    pthread_mutex_t mutex;
    pthread_cond_t  cond ;
    pthread_t       thread;
} BUFENC;

static void bufenc_uninit(void *ctxt)
{
    BUFENC *enc = (BUFENC*)ctxt;
    if (!ctxt) return;
    pthread_mutex_destroy(&enc->mutex);
    pthread_cond_destroy (&enc->cond );
    free(enc);
}

static void bufenc_write(void *ctxt, void *buf, int len)
{
    BUFENC  *enc     = (BUFENC*)ctxt;
    uint32_t typelen = len;
    if (!ctxt) return;
    pthread_mutex_lock(&enc->mutex);
    if (sizeof(uint32_t) + sizeof(uint32_t) + (typelen >> 8) <= enc->size_max - enc->size_cur) {
        uint32_t timestamp = get_tick_count();
        enc->tail      = ringbuf_write(enc->buffer, enc->size_max, enc->tail, (uint8_t*)&timestamp, sizeof(timestamp));
        enc->tail      = ringbuf_write(enc->buffer, enc->size_max, enc->tail, (uint8_t*)&typelen  , sizeof(typelen  ));
        enc->tail      = ringbuf_write(enc->buffer, enc->size_max, enc->tail, (uint8_t*) buf      , typelen >> 8     );
        enc->size_cur += sizeof(timestamp) + sizeof(uint32_t) + (typelen >> 8);
        pthread_cond_signal(&enc->cond);
    }
    pthread_mutex_unlock(&enc->mutex);
}

static int bufenc_read(void *ctxt, void *buf, int len, int *fsize, int *key, uint32_t *pts, int timeout)
{
    BUFENC *enc = (BUFENC*)ctxt;
    uint32_t timestamp = 0;
    int32_t  framesize = 0, readsize = 0, ret = 0;
    struct   timespec ts;
    if (!ctxt) return 0;

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += timeout*1000*1000;
    ts.tv_sec  += ts.tv_nsec / 1000000000;
    ts.tv_nsec %= 1000000000;

    pthread_mutex_lock(&enc->mutex);
    while (timeout && enc->size_cur <= 0 && (enc->status & TS_START) && ret != ETIMEDOUT) ret = pthread_cond_timedwait(&enc->cond, &enc->mutex, &ts);
    if (enc->size_cur > 0) {
        enc->head      = ringbuf_read(enc->buffer, enc->size_max, enc->head, (uint8_t*)&timestamp, sizeof(timestamp));
        enc->head      = ringbuf_read(enc->buffer, enc->size_max, enc->head, (uint8_t*)&framesize, sizeof(framesize));
        enc->size_cur -= sizeof(timestamp) + sizeof(framesize);
        framesize      = ((uint32_t)framesize >> 8);
        readsize       = MIN(len, framesize);
        enc->head      = ringbuf_read(enc->buffer, enc->size_max, enc->head,  buf , readsize);
        enc->head      = ringbuf_read(enc->buffer, enc->size_max, enc->head,  NULL, framesize - readsize);
        enc->size_cur -= framesize;
    }
    if (pts  ) *pts   = timestamp;
    if (fsize) *fsize = framesize;
    if (key  ) *key   = 1;
    pthread_mutex_unlock(&enc->mutex);
    return readsize;
}

static void bufenc_start(void *ctxt, int start)
{
    BUFENC *enc = (BUFENC*)ctxt;
    if (!ctxt) return;
    if (start) {
        enc->status |= TS_START;
    } else {
        pthread_mutex_lock(&enc->mutex);
        enc->status &= ~TS_START;
        pthread_cond_signal(&enc->cond);
        pthread_mutex_unlock(&enc->mutex);
    }
}

static void bufenc_reset(void *ctxt, int type)
{
    BUFENC *enc = (BUFENC*)ctxt;
    if (!ctxt) return;
    if (type & (CODEC_CLEAR_INBUF|CODEC_CLEAR_OUTBUF)) {
        pthread_mutex_lock(&enc->mutex);
        enc->head = enc->tail = enc->size_cur = 0;
        pthread_mutex_unlock(&enc->mutex);
    }
}

CODEC* bufenc_init(char *name, int bufsize)
{
    BUFENC *enc = calloc(1, sizeof(BUFENC) + bufsize);
    if (!enc) return NULL;

    strncpy(enc->name, name, sizeof(enc->name));
    enc->uninit   = bufenc_uninit;
    enc->write    = bufenc_write;
    enc->read     = bufenc_read;
    enc->start    = bufenc_start;
    enc->reset    = bufenc_reset;
    enc->buffer   = (uint8_t*)enc + sizeof(BUFENC);
    enc->size_max = bufsize;

    // init mutex & cond
    pthread_mutex_init(&enc->mutex, NULL);
    pthread_cond_init (&enc->cond , NULL);
    return (CODEC*)enc;
}
