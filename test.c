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
    CODEC    *codeclist[4];
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
    int16_t   abuf[8000 / 25] = {0};
    uint8_t   vbuf[640 * 480 * 3 / 2] = {0};
    char      str [256];

    gen_sin_wav(abuf, sizeof(abuf)/sizeof(int16_t)/2, 8000, 500);
    while (!(test->flags & FLAG_EXIT)) {
        time_t     now= time(NULL);
        struct tm *tm = localtime(&now);
        snprintf(str, sizeof(str), "%04d-%02d-%02d %02d:%02d:%02d %d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, counter++);
        memset(vbuf, 0, sizeof(vbuf));
        watermark_putstring(vbuf, 640, 10, 20, str);

        tick_sleep = (int32_t)tick_next - (int32_t)get_tick_count();
        tick_next += 40;

        codec_writebuf(test->codeclist[2], (uint8_t*)abuf, sizeof(abuf));
        codec_writebuf(test->codeclist[3], (uint8_t*)vbuf, sizeof(vbuf));

        if (tick_sleep > 0) usleep(tick_sleep * 1000);
    }
    return NULL;
}

int main(void)
{
    TESTCTXT test = {0};
    int      i;
    test.codeclist[0] = codec_init  ("buffer", sizeof(CODEC), 512 * 1024, NULL);
    test.codeclist[1] = alawenc_init(0, test.codeclist[0]);
    test.codeclist[2] = aacenc_init (0, test.codeclist[0], 32000 , 8000, 1);
    test.codeclist[3] = h264enc_init(0, test.codeclist[0], 512000, 25, 640, 480);
    test.recorder= ffrecorder_init("test", "mp4", 60000, 1, 8000, 640, 480, 25, test.codeclist, 4);
    ffrecorder_start(test.recorder, 1);

    pthread_create(&test.thread, NULL, thread_proc, &test);
    while (1) {
        char cmd[256]; scanf("%256s", cmd);
        if (strcmp(cmd, "start") == 0) {
            ffrecorder_start(test.recorder, 1);
        } else if (strcmp(cmd, "stop") == 0) {
            ffrecorder_start(test.recorder, 0);
        } else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            test.flags |= FLAG_EXIT; break;
        }
    }
    pthread_join(test.thread, NULL);

    ffrecorder_start(test.recorder, 0);
    ffrecorder_exit(test.recorder);
    for (i=3; i>=0; i--) codec_free(test.codeclist[i]);
    return 0;
}
