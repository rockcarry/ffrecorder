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
} ALAWENC;

static void alawenc_uninit(void *ctxt)
{
    ALAWENC *enc = (ALAWENC*)ctxt;
    if (!ctxt) return;
    pthread_mutex_destroy(&enc->omutex);
    pthread_cond_destroy (&enc->ocond );
    free(enc);
}

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

static void alawenc_write(void *ctxt, void *buf, int len)
{
    uint32_t timestamp, typelen, i;
    ALAWENC *enc = (ALAWENC*)ctxt;
    if (!ctxt) return;
    pthread_mutex_lock(&enc->omutex);
    typelen = len / sizeof(int16_t);
    if (sizeof(uint32_t) + sizeof(uint32_t) + typelen <= enc->omaxsize - enc->ocursize) {
        timestamp = get_tick_count();
        typelen   = 'A' | (typelen << 8);
        enc->otail    = ringbuf_write(enc->obuff, enc->omaxsize, enc->otail, (uint8_t*)&timestamp, sizeof(timestamp));
        enc->otail    = ringbuf_write(enc->obuff, enc->omaxsize, enc->otail, (uint8_t*)&typelen  , sizeof(typelen  ));
        enc->ocursize+= sizeof(timestamp) + sizeof(typelen);
        for (i=0; i<len/sizeof(int16_t); i++,enc->ocursize++) {
            if (enc->otail == enc->omaxsize) enc->otail = 0;
            enc->obuff[enc->otail++] = pcm2alaw(((int16_t*)buf)[i]);
        }
        pthread_cond_signal(&enc->ocond);
    } else {
        printf("aenc drop data %d !\n", len);
    }
    pthread_mutex_unlock(&enc->omutex);
}


static void alawenc_start(void *ctxt, int start)
{
    ALAWENC *enc = (ALAWENC*)ctxt;
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

static void alawenc_reset(void *ctxt, int type)
{
    ALAWENC *enc = (ALAWENC*)ctxt;
    if (!ctxt) return;
    if (type & (CODEC_CLEAR_INBUF|CODEC_CLEAR_OUTBUF)) {
        pthread_mutex_lock(&enc->omutex);
        enc->ohead = enc->otail = enc->ocursize = 0;
        pthread_mutex_unlock(&enc->omutex);
    }
}

CODEC* alawenc_init(int obufsize)
{
    ALAWENC *enc = NULL;
    if (obufsize < OUT_BUF_SIZE) obufsize = OUT_BUF_SIZE;
    if (!(enc = calloc(1, sizeof(ALAWENC) + obufsize))) return NULL;

    strncpy(enc->name, "alawenc", sizeof(enc->name));
    enc->uninit     = alawenc_uninit;
    enc->write      = alawenc_write;
    enc->read       = codec_read_common;
    enc->obuflock   = codec_obuflock_common;
    enc->obufunlock = codec_obufunlock_common;
    enc->start      = alawenc_start;
    enc->reset      = alawenc_reset;
    enc->omaxsize   = obufsize;
    enc->obuff      = (uint8_t*)enc + sizeof(ALAWENC);

    // init mutex & cond
    pthread_mutex_init(&enc->omutex, NULL);
    pthread_cond_init (&enc->ocond , NULL);
    return (CODEC*)enc;
}
