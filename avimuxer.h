#ifndef __AVIMUXER_H__
#define __AVIMUXER_H__

void* avimuxer_init (char *file, int duration, int w, int h, int frate, int gop, int h265, int sampnum);
void  avimuxer_exit (void *ctx);
void  avimuxer_video(void *ctx, unsigned char *buf1, int len1, unsigned char *buf2, int len2, int key, unsigned pts);
void  avimuxer_audio(void *ctx, unsigned char *buf1, int len1, unsigned char *buf2, int len2, int key, unsigned pts);

#endif



