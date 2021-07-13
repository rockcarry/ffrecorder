#ifndef __CODEC_H__
#define __CODEC_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    CODEC_CLEAR_INBUF  = (1 << 2),
    CODEC_CLEAR_OUTBUF = (1 << 3),
    CODEC_REQUEST_IDR  = (1 << 4),
};

enum {
    CODEC_FLAG_EXIT              = (1 << 0),
    CODEC_FLAG_START             = (1 << 1),
    CODEC_FLAG_REQUEST_IDR_FRAME = (1 << 2),
    CODEC_FLAG_KEY_FRAME_DROPPED = (1 << 3),
};

typedef void (*PFN_CODEC_CALLBACK)(void *ctxt, void *buf[8], int len[8]);

#define CODEC_INTERFACE_FUNCS \
    char     name   [8];   \
    uint8_t  aacinfo[8];   \
    uint8_t  vpsinfo[256]; \
    uint8_t  spsinfo[256]; \
    uint8_t  ppsinfo[256]; \
    uint8_t *obuff;    \
    int      ohead;    \
    int      otail;    \
    int      omaxsize; \
    int      ocursize; \
    uint32_t status  ; \
    pthread_mutex_t omutex; \
    pthread_cond_t  ocond;  \
    void (*uninit    )(void *ctxt); \
    void (*write     )(void *ctxt, void *buf, int len); \
    int  (*read      )(void *ctxt, void *buf, int len, int *fsize, int *key, uint32_t *pts, int timeout); \
    int  (*obuflock  )(void *ctxt, uint8_t **ppbuf1, int *plen1, uint8_t **ppbuf2, int *plen2, int *pkey, uint32_t *ppts, int timeout); \
    void (*obufunlock)(void *ctxt, int len  ); \
    void (*start     )(void *ctxt, int start); \
    void (*reset     )(void *ctxt, int type ); \
    void (*reconfig  )(void *ctxt, int bitrate);

typedef struct {
    CODEC_INTERFACE_FUNCS
} CODEC;

CODEC* alawenc_init(int obufsize);
CODEC* aacenc_init (int obufsize, int channels, int samplerate, int bitrate);
CODEC* h264enc_init(int obufsize, int frate, int w, int h, int bitrate);
CODEC* bufenc_init (int obufsize, char *name);

int  codec_read_common(void *ctxt, void *buf, int len, int *fsize, int *key, uint32_t *pts, int timeout);
int  codec_obuflock_common(void *ctxt, uint8_t **ppbuf1, int *plen1, uint8_t **ppbuf2, int *plen2, int *pkey, uint32_t *ppts, int timeout);
void codec_obufunlock_common(void *ctxt, int len);

#define codec_uninit(codec)                             (codec)->uninit(codec)
#define codec_write(codec, buf, len)                    (codec)->write(codec, buf, len)
#define codec_read(codec, buf, len, fsize, key, pts, t) (codec)->read(codec, buf, len, fsize, key, pts, t)
#define codec_obuflock(codec, a, b, c, d, e, f, g)      (codec)->obuflock(codec, a, b, c, d, e, f, g)
#define codec_obufunlock(codec, len)                    (codec)->obufunlock(codec, len)
#define codec_start(codec, s)                           (codec)->start(codec, s)
#define codec_reset(codec, t)                           (codec)->reset(codec, t)
#define codec_reconfig(codec, b)                        (codec)->reconfig(codec, b)

#ifdef __cplusplus
}
#endif

#endif
