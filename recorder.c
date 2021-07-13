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
} RECORDER;

static void* record_thread_proc(void *argv)
{
    RECORDER *recorder = (RECORDER*)argv;
    char      filepath[273] = "";
    void    (*muxer_exit )(void*) = (recorder->rectype == RECTYPE_AVI) ? avimuxer_exit : mp4muxer_exit;
    void    (*muxer_video)(void*, unsigned char*, int, unsigned char*, int, int, unsigned) = (recorder->rectype == RECTYPE_AVI) ? avimuxer_video : mp4muxer_video;
    void    (*muxer_audio)(void*, unsigned char*, int, unsigned char*, int, int, unsigned) = (recorder->rectype == RECTYPE_AVI) ? avimuxer_audio : mp4muxer_audio;
    void     *muxer_ctxt = NULL;
    uint8_t  *buf1, *buf2;
    int       len1,  len2;
    int       key, ret;
    uint32_t  pts;

    while (!(recorder->flags & FLAG_EXIT)) {
        if (!(recorder->flags & FLAG_START)) { recorder->starttick = 0; usleep(100*1000); continue; }

        ret = codec_obuflock(recorder->venc, &buf1, &len1, &buf2, &len2, &key, &pts, 10);
        if ((recorder->flags & FLAG_START) == 0 || (ret > 0 && (recorder->flags & FLAG_NEXT) && key)) { // if record stop or change to next record file
            muxer_exit(muxer_ctxt); muxer_ctxt = NULL;
            recorder->flags &= ~FLAG_NEXT;
        }
        if (ret > 0) { // if recorder started, and got video data
            if (!muxer_ctxt && key) { // if muxer not created and this is video key frame
                int ish265 = !!strstr(recorder->venc->name, "h265");
                time_t     now= time(NULL);
                struct tm *tm = localtime(&now);
                snprintf(filepath, sizeof(filepath), "%s-%04d%02d%02d-%02d%02d%02d.%s", recorder->filename,
                        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
                        recorder->rectype == RECTYPE_AVI ? "avi" : "mp4");
                if (recorder->rectype == RECTYPE_AVI) {
                    muxer_ctxt = avimuxer_init(filepath, recorder->duration, recorder->width, recorder->height, recorder->fps, ret, ish265);
                } else {
                    muxer_ctxt = mp4muxer_init(filepath, recorder->duration, recorder->width, recorder->height, recorder->fps, recorder->fps * 2, recorder->channels, recorder->samprate, 16, 1024, recorder->aenc->aacinfo, ish265);
                }
                if (recorder->starttick == 0 && muxer_ctxt) {
                    recorder->starttick = get_tick_count();
                    recorder->starttick = recorder->starttick ? recorder->starttick : 1;
                }
            }
            muxer_video(muxer_ctxt, buf1, len1, buf2, len2, key, pts);
        }
        if (ret >= 0) codec_obufunlock(recorder->venc, ret);

        ret = codec_obuflock(recorder->aenc, &buf1, &len1, &buf2, &len2, &key, &pts, 10);
        if (ret >  0) muxer_audio(muxer_ctxt, buf1, len1, buf2, len2, key, pts); // if recorder started and muxer created and got audio frame
        if (ret >= 0) codec_obufunlock(recorder->aenc, ret);

        if (recorder->starttick && (int32_t)get_tick_count() - (int32_t)recorder->starttick >= recorder->duration) {
            recorder->starttick += recorder->duration;
            recorder->flags     |= FLAG_NEXT;
            codec_reset(recorder->venc, CODEC_REQUEST_IDR);
        }
    }
    muxer_exit(muxer_ctxt);
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
    if (start && !(recorder->flags & FLAG_START)) {
        recorder->flags |= FLAG_START;
        codec_reset(recorder->aenc, CODEC_CLEAR_INBUF|CODEC_CLEAR_OUTBUF|CODEC_REQUEST_IDR);
        codec_reset(recorder->venc, CODEC_CLEAR_INBUF|CODEC_CLEAR_OUTBUF|CODEC_REQUEST_IDR);
        codec_start(recorder->aenc, 1);
        codec_start(recorder->venc, 1);
    } else if (!start && (recorder->flags & FLAG_START)) {
        recorder->flags &=~FLAG_START;
        codec_start(recorder->aenc, 0);
        codec_start(recorder->venc, 0);
    }
}

