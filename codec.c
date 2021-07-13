#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include "ringbuf.h"
#include "codec.h"
#include "utils.h"

int codec_read_common(void *ctxt, void *buf, int len, int *fsize, int *key, uint32_t *pts, int timeout)
{
    CODEC *enc = (CODEC*)ctxt;
    uint32_t timestamp = 0, framesize = 0, frametype;
    int32_t  readsize = 0, ret = 0;
    struct   timespec ts;
    if (!ctxt) return 0;

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += timeout*1000*1000;
    ts.tv_sec  += ts.tv_nsec / 1000000000;
    ts.tv_nsec %= 1000000000;

    pthread_mutex_lock(&enc->omutex);
    while (timeout && enc->ocursize <= 0 && (enc->status & CODEC_FLAG_START) && ret != ETIMEDOUT) ret = pthread_cond_timedwait(&enc->ocond, &enc->omutex, &ts);
    if (enc->ocursize >= sizeof(uint32_t) * 2) {
        enc->ohead    = ringbuf_read(enc->obuff, enc->omaxsize, enc->ohead, (uint8_t*)&timestamp, sizeof(timestamp));
        enc->ohead    = ringbuf_read(enc->obuff, enc->omaxsize, enc->ohead, (uint8_t*)&framesize, sizeof(framesize));
        enc->ocursize-= sizeof(timestamp) + sizeof(framesize);
        frametype     = (framesize & 0xFF );
        framesize     = (framesize >> 8   );
        readsize      = MIN(len, framesize);
        enc->ohead    = ringbuf_read(enc->obuff, enc->omaxsize, enc->ohead,  buf , readsize);
        enc->ohead    = ringbuf_read(enc->obuff, enc->omaxsize, enc->ohead,  NULL, framesize - readsize);
        enc->ocursize-= framesize;
    }
    if (pts  ) *pts   = timestamp;
    if (fsize) *fsize = framesize;
    if (key  ) *key   = frametype >= 'A' && frametype <= 'Z';
    pthread_mutex_unlock(&enc->omutex);
    return readsize;
}

int codec_obuflock_common(void *ctxt, uint8_t **ppbuf1, int *plen1, uint8_t **ppbuf2, int *plen2, int *pkey, uint32_t *ppts, int timeout)
{
    CODEC *enc = (CODEC*)ctxt;
    struct   timespec ts;
    uint8_t *buf1 = NULL, *buf2 = NULL;
    int      len1 = 0, len2 = 0, key = 0, ret;
    uint32_t pts  = 0, tlen = 0;
    if (!ctxt) return -1;

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += timeout*1000*1000;
    ts.tv_sec  += ts.tv_nsec / 1000000000;
    ts.tv_nsec %= 1000000000;
    pthread_mutex_lock(&enc->omutex);
    while (timeout && enc->ocursize <= 0 && (enc->status & CODEC_FLAG_START) && ret != ETIMEDOUT) ret = pthread_cond_timedwait(&enc->ocond, &enc->omutex, &ts);

    if (enc->ocursize >= sizeof(uint32_t) * 2) {
        enc->ohead    = ringbuf_read(enc->obuff, enc->omaxsize, enc->ohead, (uint8_t*)&pts , sizeof(pts ));
        enc->ohead    = ringbuf_read(enc->obuff, enc->omaxsize, enc->ohead, (uint8_t*)&tlen, sizeof(tlen));
        enc->ocursize-= sizeof(pts) + sizeof(tlen);
        key = (tlen & 0xFF) >= 'A' && (tlen & 0xFF) <= 'Z';
        buf1= enc->obuff + enc->ohead;
        len1= (tlen >> 8);
        if (enc->ohead + len1 > enc->omaxsize) {
            len1 = enc->omaxsize - enc->ohead;
            buf2 = enc->obuff;
            len2 = (tlen >> 8) - len1;
        }
    }

    if (ppbuf1) *ppbuf1 = buf1;
    if (ppbuf2) *ppbuf2 = buf2;
    if (plen1 ) *plen1  = len1;
    if (plen2 ) *plen2  = len2;
    if (pkey  ) *pkey   = key ;
    if (ppts  ) *ppts   = pts ;
    return (tlen >> 8);
}

void codec_obufunlock_common(void *ctxt, int len)
{
    CODEC *enc = (CODEC*)ctxt;
    if (!ctxt) return;
    if (len > 0) {
        enc->ohead     = ringbuf_read(enc->obuff, enc->omaxsize, enc->ohead, NULL, len);
        enc->ocursize -= len;
    }
    pthread_mutex_unlock(&enc->omutex);
}
