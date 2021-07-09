#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "codec.h"
#include "recorder.h"
#include "utils.h"

typedef struct {
    CODEC    *aacenc;
    CODEC    *h264enc;
    void     *recorder;
    #define FLAG_EXIT (1 << 0)
    uint32_t  flags;
    pthread_t thread;
} TESTCTXT;

static void* thread_proc(void *param)
{
    TESTCTXT *test = (TESTCTXT*)param;
    uint32_t  tick_next = get_tick_count() + 40;
     int32_t  tick_sleep;
    uint8_t   vbuf[640 * 480 * 3 / 2];
    int16_t   abuf[16000 / 25];
    while (!(test->flags & FLAG_EXIT)) {
        tick_sleep = (int32_t)tick_next - (int32_t)get_tick_count();
        tick_next += 40;
        codec_write(test->aacenc , abuf, sizeof(abuf));
        codec_write(test->h264enc, vbuf, sizeof(vbuf));
        if (tick_sleep) usleep(tick_sleep);
    }
    return NULL;
}

int main(void)
{
    TESTCTXT test = {0};
    
    test.aacenc  = aacenc_init (1, 16000, 32000);
    test.h264enc = h264enc_init(25, 640, 480, 512000);
    test.recorder= ffrecorder_init("test", "mp4", 60000, 1, 16000, 640, 480, 25, test.aacenc, test.h264enc);
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
    return 0;
}
