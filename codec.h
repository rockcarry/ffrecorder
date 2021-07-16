#ifndef __CODEC_H__
#define __CODEC_H__

#include <stdint.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    CODEC_CONFIG_CLEAR_BUFF  = (1 << 0),
    CODEC_CONFIG_REQUEST_IDR = (1 << 1),
    CODEC_CONFIG_SET_BITRATE = (1 << 2),
};

enum {
    CODEC_FLAG_EXIT   = (1 << 0),
    CODEC_FLAG_START  = (1 << 1),
    CODEC_FLAG_REQIDR = (1 << 2),
};

#define CODEC_FOURCC(a, b, c, d) (((a) << 0) | ((b) << 8) | ((c) << 16) | ((d) << 24))

#define CODEC_COMMON_MEMBERS \
    void    *next;         \
    char     name   [8];   \
    uint8_t  aacinfo[8];   \
    uint8_t  vpsinfo[256]; \
    uint8_t  spsinfo[256]; \
    uint8_t  ppsinfo[256]; \
    uint8_t *buff;    \
    int      head;    \
    int      tail;    \
    int      maxsize; \
    int      cursize; \
    uint32_t flags  ; \
    pthread_mutex_t mutex; \
    pthread_cond_t  cond;  \
    void (*free    )(void *c); \
    int  (*writebuf)(void *c, uint8_t *buf, int len); \
    void (*config  )(void *c, int flags, void *param1, uint32_t param2);

typedef struct {
    CODEC_COMMON_MEMBERS
} CODEC;

void* codec_init       (char *name, int codecsize, int buffersize, void *next);
void  codec_free       (void *c);
int   codec_writebuf   (void *c, uint8_t *buf, int len);
int   codec_readbuf    (void *c, uint8_t *buf, int len);
int   codec_writeframe (void *c, uint8_t *buf, int len, uint32_t type, uint32_t pts);
int   codec_readframe  (void *c, uint8_t *buf, int len, uint32_t *fsize, uint32_t *type, uint32_t *pts, int timeout);
int   codec_lockframe  (void *c, uint8_t **ppbuf1, int *plen1, uint8_t **ppbuf2, int *plen2, uint32_t *type, uint32_t *pts, int timeout);
void  codec_unlockframe(void *c, int len);
void  codec_start      (void *c, int start);
void  codec_config     (void *c, int flags, void *param1, uint32_t param2);

void* alawenc_init(int bufsize, void *next);
void* aacenc_init (int bufsize, void *next, int bitrate, int samprate, int channels);
void* h264enc_init(int bufsize, void *next, int bitrate, int frmrate , int w, int h);

#ifdef __cplusplus
}
#endif

#endif
