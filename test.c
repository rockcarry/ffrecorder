#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include "codec.h"
#include "font25x48.h"
#include "recorder.h"
#include "utils.h"

typedef struct {
    CODEC    *aacenc;
    CODEC    *h264enc;
    CODEC    *vbufenc;
    void     *recorder;
    #define FLAG_EXIT (1 << 0)
    uint32_t  flags;
    pthread_t thread;
} TESTCTXT;

static void gen_sin_wav(int16_t *pcm, int n, int samprate, int freq)
{
    int i; for (i=0; i<n; i++) pcm[i] = 32760 * sin(i * 2 * M_PI * freq / samprate);
}

static void* thread_proc(void *param)
{
    TESTCTXT *test = (TESTCTXT*)param;
    uint32_t  tick_next = get_tick_count() + 40;
    int32_t   tick_sleep= 0;
    uint16_t  counter   = 0;
    int16_t   abuf[16000 / 25] = {0};
    uint8_t   vbuf[640 * 480 * 3 / 2] = {0};
    char      str [256];
    int       key, ret = 0;

    gen_sin_wav(abuf, sizeof(abuf)/sizeof(int16_t)/2, 16000, 300);
    codec_start(test->aacenc , 1);
    codec_start(test->h264enc, 1);
    while (!(test->flags & FLAG_EXIT)) {
        time_t     now= time(NULL);
        struct tm *tm = localtime(&now);
        snprintf(str, sizeof(str), "%04d-%02d-%02d %02d:%02d:%02d %d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, counter++);
        memset(vbuf, 0, sizeof(vbuf));
        watermark_putstring(vbuf, 640, 10, 20, str);

        tick_sleep = (int32_t)tick_next - (int32_t)get_tick_count();
        tick_next += 40;

        codec_write(test->aacenc , abuf, sizeof(abuf));
        codec_write(test->h264enc, vbuf, sizeof(vbuf));

        ret = codec_read(test->h264enc, vbuf, sizeof(vbuf), NULL, &key, NULL, 0);
        if (ret > 0) codec_write(test->vbufenc, vbuf, (ret << 8) | (key ? 'V' : 'v'));
        if (tick_sleep > 0) usleep(tick_sleep * 1000);
    }
    codec_start(test->aacenc , 0);
    codec_start(test->h264enc, 0);
    return NULL;
}

int main(void)
{
    TESTCTXT test = {0};
    
    test.aacenc  = aacenc_init (1, 16000, 32000);
    test.h264enc = h264enc_init(25, 640, 480, 512000);
    test.vbufenc = bufenc_init ("h264enc", 256 * 1024);
    test.recorder= ffrecorder_init("test", "mp4", 60000, 1, 16000, 640, 480, 25, test.aacenc, test.vbufenc);
    pthread_create(&test.thread, NULL, thread_proc, &test);

    ffrecorder_start(test.recorder, 1);
    while (1) {
        char cmd[256]; scanf("%256s", cmd);
        if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) { test.flags |= FLAG_EXIT; break; }
    }
    ffrecorder_start(test.recorder, 0);

    pthread_join(test.thread, NULL);
    if (test.recorder) ffrecorder_exit(test.recorder);
    if (test.aacenc  ) codec_uninit(test.aacenc );
    if (test.h264enc ) codec_uninit(test.h264enc);
    if (test.vbufenc ) codec_uninit(test.vbufenc);
    return 0;
}
