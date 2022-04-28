#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "avimuxer.h"
#include "mp4muxer.h"
#include "recorder.h"
#include "codec.h"
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

    #define MAX_CODEC_NUM 8
    CODEC    *codeclist[MAX_CODEC_NUM];
    int       codecnum;
    uint8_t   aacinfo[256];

    #define FLAG_EXIT  (1 << 0)
    #define FLAG_START (1 << 1)
    #define FLAG_NEXT  (1 << 2)
    uint32_t  flags;
    pthread_t pthread;
} RECORDER;

#define IS_AUDIO_KEYFRAME(type) ((char)(type) == 'A')
#define IS_VIDEO_KEYFRAME(type) ((char)(type) == 'V')
#define IS_VIDEO_H265_ENC(type) ((((type) >> 8) & 0xFF) == '5')
#define IS_VIDEO_FRAME(type)    ((char)(type) == 'V' || (char)(type) == 'v')

static void* record_thread_proc(void *argv)
{
    RECORDER *recorder = (RECORDER*)argv;
    char      filepath[273] = "";
    void    (*muxer_exit )(void*) = (recorder->rectype == RECTYPE_AVI) ? avimuxer_exit : mp4muxer_exit;
    void    (*muxer_video)(void*, unsigned char*, int, unsigned char*, int, int, unsigned) = (recorder->rectype == RECTYPE_AVI) ? avimuxer_video : mp4muxer_video;
    void    (*muxer_audio)(void*, unsigned char*, int, unsigned char*, int, int, unsigned) = (recorder->rectype == RECTYPE_AVI) ? avimuxer_audio : mp4muxer_audio;
    void     *muxer_ctxt = NULL;
    uint8_t  *buf1, *buf2;
    int       len1,  len2, ret, i;
    uint32_t  type, pts;

    while (!(recorder->flags & FLAG_EXIT)) {
        if (!(recorder->flags & FLAG_START)) {
            if (muxer_ctxt) { muxer_exit(muxer_ctxt); muxer_ctxt = NULL; }
            recorder->starttick = 0; usleep(100*1000); continue;
        }

        ret = codec_lockframe(recorder->codeclist[0], &buf1, &len1, &buf2, &len2, &type, &pts, 100);
        if (ret > 0 && (recorder->flags & FLAG_NEXT) && IS_VIDEO_KEYFRAME(type)) { // if record stop or change to next record file
            muxer_exit(muxer_ctxt); muxer_ctxt = NULL;
            recorder->flags &= ~FLAG_NEXT;
        }
        if ((recorder->flags & FLAG_START) && ret > 0) { // if recorder started, and got video data
            if (!muxer_ctxt && IS_VIDEO_KEYFRAME(type)) { // if muxer not created and this is video key frame
                time_t     now= time(NULL);
                struct tm *tm = localtime(&now);
                snprintf(filepath, sizeof(filepath), "%s-%04d%02d%02d-%02d%02d%02d.%s", recorder->filename,
                        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
                        recorder->rectype == RECTYPE_AVI ? "avi" : "mp4");
                if (recorder->rectype == RECTYPE_AVI) {
                    muxer_ctxt = avimuxer_init(filepath, recorder->duration, recorder->width, recorder->height, recorder->fps, recorder->fps * 2, IS_VIDEO_H265_ENC(type), 0);
                } else {
                    muxer_ctxt = mp4muxer_init(filepath, recorder->duration, recorder->width, recorder->height, recorder->fps, recorder->fps * 2, IS_VIDEO_H265_ENC(type), recorder->channels, recorder->samprate, 16, 1024, recorder->aacinfo);
                }
                if (recorder->starttick == 0 && muxer_ctxt) {
                    recorder->starttick = get_tick_count();
                    recorder->starttick = recorder->starttick ? recorder->starttick : 1;
                }
            }
            (IS_VIDEO_FRAME(type) ? muxer_video : muxer_audio)(muxer_ctxt, buf1, len1, buf2, len2, IS_VIDEO_KEYFRAME(type), pts);
        }
        codec_unlockframe(recorder->codeclist[0], ret);

        if (recorder->starttick && (int32_t)get_tick_count() - (int32_t)recorder->starttick >= recorder->duration) {
            recorder->starttick += recorder->duration;
            recorder->flags     |= FLAG_NEXT;
            for (i=0; i<recorder->codecnum; i++) codec_config(recorder->codeclist[i], CODEC_CONFIG_REQUEST_IDR, NULL, 0);
        }
    }
    muxer_exit(muxer_ctxt);
    return NULL;
}

void* ffrecorder_init(char *name, char *type, int duration, int channels, int samprate, int width, int height, int fps, void *codeclist, int codecnum)
{
    int       i;
    RECORDER *recorder = calloc(1, sizeof(RECORDER));
    if (!recorder) return NULL;

    strncpy(recorder->filename, name, sizeof(recorder->filename));
    recorder->duration = duration;
    recorder->channels = channels;
    recorder->samprate = samprate;
    recorder->width    = width;
    recorder->height   = height;
    recorder->fps      = fps;
    recorder->codecnum = MIN(codecnum, MAX_CODEC_NUM);
    memcpy(recorder->codeclist, codeclist, recorder->codecnum * sizeof(void*));
    if (strcmp(type, "mp4") == 0) recorder->rectype = RECTYPE_MP4;
    if (strcmp(type, "avi") == 0) recorder->rectype = RECTYPE_AVI;

    for (i=0; i<recorder->codecnum; i++) {
        if (strcmp(recorder->codeclist[i]->name, "aacenc") == 0) {
            memcpy(recorder->aacinfo, recorder->codeclist[i]->aacinfo, MIN(sizeof(recorder->aacinfo), sizeof(recorder->codeclist[i]->aacinfo)));
        }
    }

    // create server thread
    pthread_create(&recorder->pthread, NULL, record_thread_proc, recorder);
    return recorder;
}

void ffrecorder_exit(void *ctxt)
{
    RECORDER *recorder = (RECORDER*)ctxt;
    if (!ctxt) return;
    ffrecorder_start(ctxt, 0);
    recorder->flags |= FLAG_EXIT;
    pthread_join(recorder->pthread, NULL);
    free(recorder);
}

void ffrecorder_start(void *ctxt, int start)
{
    int       i;
    RECORDER *recorder = (RECORDER*)ctxt;
    if (!ctxt) return;
    if (start && !(recorder->flags & FLAG_START)) {
        recorder->flags |= FLAG_START;
        for (i=0; i<recorder->codecnum; i++) {
            codec_config(recorder->codeclist[i], CODEC_CONFIG_CLEAR_BUFF|CODEC_CONFIG_REQUEST_IDR, NULL, 0);
            codec_start (recorder->codeclist[i], 1);
        }
    } else if (!start && (recorder->flags & FLAG_START)) {
        recorder->flags &=~FLAG_START;
        for (i=0; i<recorder->codecnum; i++) {
            codec_start (recorder->codeclist[i], 0);
        }
    }
}

