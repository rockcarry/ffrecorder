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
    CODEC_COMMON_MEMBERS
} ALAWENC;

static uint8_t pcm2alaw(int16_t pcm)
{
    uint8_t sign = (pcm >> 8) & (1 << 7);
    int  mask, eee, wxyz, alaw;
    if (sign) pcm = -pcm;
    for (mask=0x4000,eee=7; (pcm&mask)==0&&eee>0; eee--,mask>>=1);
    wxyz  = (pcm >> ((eee == 0) ? 4 : (eee + 3))) & 0xf;
    alaw  = sign | (eee << 4) | wxyz;
    return (alaw ^ 0xd5);
}

static int alawenc_writebuf(void *ctxt, uint8_t *buf, int len)
{
    ALAWENC  *enc = (ALAWENC*)ctxt;
    int   samples = len / sizeof(int16_t), n, i;
    uint8_t *pdst = enc->buff + enc->tail;
    int16_t *psrc = (int16_t*)buf;
    while (samples > 0) {
        n = MIN(samples, enc->maxsize - enc->tail);
        for (i=0; i<n; i++) *pdst++ = pcm2alaw(*psrc++);
        samples -= n; enc->tail += n;
        if (enc->tail == enc->maxsize) {
            enc->tail = 0; pdst = enc->buff;
            codec_writeframe(enc->next, enc->buff, enc->maxsize, CODEC_FOURCC('A', 0, 0, 0), get_tick_count());
        }
    }
    return (uint8_t*)psrc - buf;
}

void* alawenc_init(int bufsize, void *next)
{
    CODEC *codec = codec_init("alawenc", sizeof(ALAWENC), MAX(320, bufsize), next);
    if (!codec) return NULL;
    codec->writebuf = alawenc_writebuf;
    return codec;
}
