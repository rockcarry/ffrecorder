#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "avimuxer.h"
#include "mp4muxer.h"
#include "recorder.h"
#include "utils.h"

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

#define RECTYPE_AVI  (('A' << 0) | ('V' << 8) | ('I' << 16))
#define RECTYPE_MP4  (('M' << 0) | ('P' << 8) | ('4' << 16))
#define AVI_ALAW_FRAME_SIZE   320
#define RECORDER_BUFFER_SIZE (1 * 1024 * 1024)

typedef struct {
    char      filename[256];
    int       duration;
    int       channels;
    int       samprate;
    int       width;
    int       height;
    int       fps;
    uint32_t  rectype;
    uint32_t  starttick;

    CODEC    *aenc;
    CODEC    *venc;

    #define FLAG_EXIT  (1 << 0)
    #define FLAG_START (1 << 1)
    #define FLAG_NEXT  (1 << 2)
    uint32_t  flags;
    pthread_t pthread;
    uint8_t   buffer[RECORDER_BUFFER_SIZE];
} RECORDER;

static void* record_thread_proc(void *argv)
{
    RECORDER *recorder = (RECORDER*)argv;
    char      filepath[256] = "";
    void     *muxer         = NULL;
    int       framesize, readsize, key;
    uint32_t  pts;

    while (1) {
        readsize = codec_read(recorder->venc, recorder->buffer, sizeof(recorder->buffer), &framesize, &key, &pts, 10);
        if ((recorder->flags & FLAG_START) == 0 || (readsize > 0 && (recorder->flags & FLAG_NEXT) && key)) { // if record stop or change to next record file
            if (muxer) {
                switch (recorder->rectype) {
                case RECTYPE_AVI: avimuxer_exit(muxer); break;
                case RECTYPE_MP4: mp4muxer_exit(muxer); break;
                }
                muxer = NULL;
                recorder->flags &= ~FLAG_NEXT;
            }
        }

        while ((recorder->flags & (FLAG_EXIT|FLAG_START)) == 0) usleep(100*1000); // if record stopped
        if (recorder->flags & FLAG_EXIT) break; // if recorder exited

        if ((recorder->flags & FLAG_START) && readsize > 0) { // if recorder started, and got video data
            if (!muxer && key) { // if muxer not created and this is video key frame
                int ish265 = !!strstr(recorder->venc->name, "h265");
                time_t     now= time(NULL);
                struct tm *tm = localtime(&now);
                snprintf(filepath, sizeof(filepath), "%s-%04d%02d%02d-%02d%02d%02d.%s", recorder->filename,
                        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
                        recorder->rectype == RECTYPE_AVI ? "avi" : "mp4");
                switch (recorder->rectype) {
                case RECTYPE_AVI:
                    muxer = avimuxer_init(filepath, recorder->duration, recorder->width, recorder->height, recorder->fps, AVI_ALAW_FRAME_SIZE, ish265);
                    break;
                case RECTYPE_MP4:
                    muxer = mp4muxer_init(filepath, recorder->duration, recorder->width, recorder->height, recorder->fps, recorder->fps * 2, recorder->channels, recorder->samprate, 16, 1024, recorder->aenc->aacinfo, ish265);
                    break;
                }
                recorder->starttick = get_tick_count();
                recorder->starttick = recorder->starttick ? recorder->starttick : 1;
            }

            if (muxer) { // if muxer created
                switch (recorder->rectype) {
                case RECTYPE_AVI: avimuxer_video(muxer, recorder->buffer, readsize, key, pts); break;
                case RECTYPE_MP4: mp4muxer_video(muxer, recorder->buffer, readsize, key, pts); break;
                }
            }
        }

        readsize = codec_read(recorder->aenc, recorder->buffer, recorder->rectype == RECTYPE_AVI ? AVI_ALAW_FRAME_SIZE : sizeof(recorder->buffer), &framesize, &key, &pts, 10);
        if ((recorder->flags & FLAG_START) != 0 && readsize > 0 && muxer) { // if recorder started and muxer created and got audio frame
            switch (recorder->rectype) {
            case RECTYPE_AVI: avimuxer_audio(muxer, recorder->buffer, readsize, key, pts); break;
            case RECTYPE_MP4: mp4muxer_audio(muxer, recorder->buffer, readsize, key, pts); break;
            }
        }

        if (recorder->starttick && (int32_t)get_tick_count() - (int32_t)recorder->starttick > recorder->duration) {
            recorder->starttick += recorder->duration;
            recorder->flags    |= FLAG_NEXT;
            codec_reset(recorder->venc, CODEC_REQUEST_IDR);
        }
    }
    return NULL;
}

void* ffrecorder_init(char *name, char *type, int duration, int channels, int samprate, int width, int height, int fps, CODEC *aenc, CODEC *venc)
{
    RECORDER *recorder = calloc(1, sizeof(RECORDER));
    if (!recorder) return NULL;

    strncpy(recorder->filename, name, sizeof(recorder->filename));
    recorder->duration = duration;
    recorder->channels = channels;
    recorder->samprate = samprate;
    recorder->width    = width;
    recorder->height   = height;
    recorder->fps      = fps;
    recorder->aenc     = aenc;
    recorder->venc     = venc;

    if (strcmp(type, "mp4") == 0) recorder->rectype = RECTYPE_MP4;
    if (strcmp(type, "avi") == 0) recorder->rectype = RECTYPE_AVI;

    // create server thread
    pthread_create(&recorder->pthread, NULL, record_thread_proc, recorder);
    return recorder;
}

void ffrecorder_exit(void *ctxt)
{
    RECORDER *recorder = (RECORDER*)ctxt;
    if (!ctxt) return;
    codec_start(recorder->aenc, 0);
    codec_start(recorder->venc, 0);
    recorder->flags = (recorder->flags & ~FLAG_START) | FLAG_EXIT;
    pthread_join(recorder->pthread, NULL);
    free(recorder);
}

void ffrecorder_start(void *ctxt, int start)
{
    RECORDER *recorder = (RECORDER*)ctxt;
    if (!ctxt) return;
    if (start) {
        recorder->flags |= FLAG_START;
        codec_reset(recorder->aenc, CODEC_CLEAR_INBUF|CODEC_CLEAR_OUTBUF|CODEC_REQUEST_IDR);
        codec_reset(recorder->venc, CODEC_CLEAR_INBUF|CODEC_CLEAR_OUTBUF|CODEC_REQUEST_IDR);
        codec_start(recorder->aenc, 1);
        codec_start(recorder->venc, 1);
    } else {
        recorder->flags &=~FLAG_START;
        codec_start(recorder->aenc, 0);
        codec_start(recorder->venc, 0);
    }
}

