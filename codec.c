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

static void base_codec_free(void *c)
{
    CODEC *codec = (CODEC*)c;
    pthread_mutex_destroy(&codec->mutex);
    pthread_cond_destroy (&codec->cond );
    free(codec);
}

static int base_codec_writebuf(void *c, uint8_t *buf, int len)
{
    CODEC *codec = (CODEC*)c;
    int    ret   = 0;
    pthread_mutex_lock(&codec->mutex);
    if (codec->cursize + len <= codec->maxsize) {
        codec->tail     = ringbuf_write(codec->buff, codec->maxsize, codec->tail, buf, len);
        codec->cursize += len;
        pthread_cond_signal(&codec->cond);
        ret = len;
    } else {
        printf("codec_write %s drop data %d, head: %d, tail: %d, cursize: %d, maxsize: %d\n", codec->name, len, codec->head, codec->tail, codec->cursize, codec->maxsize);
    }
    pthread_mutex_unlock(&codec->mutex);
    return ret;
}

static void base_codec_config(void *c, int flags, void *param1, uint32_t param2)
{
    CODEC *codec = (CODEC*)c;
    if (flags & CODEC_CONFIG_CLEAR_BUFF) {
        pthread_mutex_lock(&codec->mutex);
        codec->head = codec->tail = codec->cursize = 0;
        pthread_mutex_unlock(&codec->mutex);
    }
}

void* codec_init(char *name, int codecsize, int buffersize, void *next)
{
    CODEC *codec = calloc(1, codecsize + buffersize);
    if (!codec) return NULL;
    strncpy(codec->name, name, sizeof(codec->name));
    codec->next      = next;
    codec->buff      = (uint8_t*)codec + codecsize;
    codec->maxsize   = buffersize;
    codec->free      = base_codec_free;
    codec->writebuf  = base_codec_writebuf;
    codec->config    = base_codec_config;
    pthread_mutex_init(&codec->mutex, NULL);
    pthread_cond_init (&codec->cond , NULL);
    return codec;
}

int codec_readbuf(void *c, uint8_t *buf, int len)
{
    CODEC *codec = (CODEC*)c;
    int    ret   = 0;
    if (!codec) return -1;
    pthread_mutex_lock(&codec->mutex);
    if (codec->cursize > 0) {
        ret = MIN(len, codec->cursize);
        codec->head     = ringbuf_read(codec->buff, codec->maxsize, codec->head, buf, ret);
        codec->cursize += ret;
    }
    pthread_mutex_unlock(&codec->mutex);
    return ret;
}

int codec_writeframe(void *c, uint8_t *buf, int len, uint32_t type, uint32_t pts)
{
    CODEC  *codec = (CODEC*)c;
    int32_t size  = 0;
    if (!c) return -1;
    pthread_mutex_lock(&codec->mutex);
    if (sizeof(uint32_t) * 3 + len <= codec->maxsize - codec->cursize) {
        size = len;
        codec->tail    = ringbuf_write(codec->buff, codec->maxsize, codec->tail, (uint8_t*)&size, sizeof(uint32_t));
        codec->tail    = ringbuf_write(codec->buff, codec->maxsize, codec->tail, (uint8_t*)&type, sizeof(uint32_t));
        codec->tail    = ringbuf_write(codec->buff, codec->maxsize, codec->tail, (uint8_t*)&pts , sizeof(uint32_t));
        codec->tail    = ringbuf_write(codec->buff, codec->maxsize, codec->tail, (uint8_t*) buf , len);
        codec->cursize+= sizeof(uint32_t) * 3 + len;
        pthread_cond_signal(&codec->cond);
    }
    pthread_mutex_unlock(&codec->mutex);
    return size;
}

int codec_readframe(void *c, uint8_t *buf, int len, uint32_t *fsize, uint32_t *type, uint32_t *pts, int timeout)
{
    CODEC *codec = (CODEC*)c;
    struct timespec ts;
    int      readn = 0, ret;
    uint32_t size;
    if (!codec) return -1;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += timeout*1000*1000;
    ts.tv_sec  += ts.tv_nsec / 1000000000;
    ts.tv_nsec %= 1000000000;
    pthread_mutex_lock(&codec->mutex);
    while (timeout && codec->cursize == 0 && ret != ETIMEDOUT) ret = pthread_cond_timedwait(&codec->cond, &codec->mutex, &ts);
    if (codec->cursize > sizeof(uint32_t) * 3) {
        codec->head    = ringbuf_read(codec->buff, codec->maxsize, codec->head, (uint8_t*)&size, sizeof(uint32_t));
        codec->head    = ringbuf_read(codec->buff, codec->maxsize, codec->head, (uint8_t*) type, sizeof(uint32_t));
        codec->head    = ringbuf_read(codec->buff, codec->maxsize, codec->head, (uint8_t*) pts , sizeof(uint32_t));
        codec->cursize-= sizeof(uint32_t) * 3;
        readn = MIN(len, size);
        codec->head    = ringbuf_read(codec->buff, codec->maxsize, codec->head,  buf , readn);
        codec->head    = ringbuf_read(codec->buff, codec->maxsize, codec->head,  NULL, size - readn);
        if (fsize) *fsize = size;
    }
    pthread_mutex_unlock(&codec->mutex);
    return readn;
}

int codec_lockframe(void *c, uint8_t **ppbuf1, int *plen1, uint8_t **ppbuf2, int *plen2, uint32_t *type, uint32_t *pts, int timeout)
{
    CODEC  *codec = (CODEC*)c;
    uint8_t *buf1 = NULL, *buf2 = NULL;
    int      len1 = 0   ,  len2 = 0, ret;
    struct timespec ts;
    uint32_t size = 0;
    if (!codec) return -1;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += timeout*1000*1000;
    ts.tv_sec  += ts.tv_nsec / 1000000000;
    ts.tv_nsec %= 1000000000;
    pthread_mutex_lock(&codec->mutex);
    while (timeout && codec->cursize == 0 && ret != ETIMEDOUT) ret = pthread_cond_timedwait(&codec->cond, &codec->mutex, &ts);
    if (codec->cursize > sizeof(uint32_t) * 3) {
        codec->head    = ringbuf_read(codec->buff, codec->maxsize, codec->head, (uint8_t*)&size, sizeof(uint32_t));
        codec->head    = ringbuf_read(codec->buff, codec->maxsize, codec->head, (uint8_t*) type, sizeof(uint32_t));
        codec->head    = ringbuf_read(codec->buff, codec->maxsize, codec->head, (uint8_t*) pts , sizeof(uint32_t));
        codec->cursize-= sizeof(uint32_t) * 3;
        buf1= codec->buff + codec->head;
        len1= size;
        if (codec->head + len1 > codec->maxsize) {
            len1 = codec->maxsize - codec->head;
            buf2 = codec->buff;
            len2 = size - len1;
        }
        if (ppbuf1) *ppbuf1 = buf1;
        if (ppbuf2) *ppbuf2 = buf2;
        if (plen1 ) *plen1  = len1;
        if (plen2 ) *plen2  = len2;
    }
    return size;
}

void codec_unlockframe(void *c, int len)
{
    CODEC *codec = (CODEC*)c;
    if (!codec || len == -1) return;
    if (len > 0) {
        codec->head     = ringbuf_read(codec->buff, codec->maxsize, codec->head, NULL, len);
        codec->cursize -= len;
    }
    pthread_mutex_unlock(&codec->mutex);
}

void codec_start(void *c, int start)
{
    CODEC *codec = (CODEC*)c;
    if (!codec) return;
    pthread_mutex_lock(&codec->mutex);
    if (start) codec->flags |= CODEC_FLAG_START;
    else       codec->flags &=~CODEC_FLAG_START;
    pthread_mutex_unlock(&codec->mutex);
}

void codec_free(void *c)
{
    CODEC *codec = (CODEC*)c;
    if (codec && codec->free) codec->free(c);
}

int codec_writebuf(void *c, uint8_t *buf, int len)
{
    CODEC *codec = (CODEC*)c;
    if (codec && codec->config) return codec->writebuf(c, buf, len);
    return -1;
}

void codec_config(void *c, int flags, void *param1, uint32_t param2)
{
    CODEC *codec = (CODEC*)c;
    if (codec && codec->config) codec->config(c, flags, param1, param2);
}

