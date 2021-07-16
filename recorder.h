#ifndef __RECORDER_H__
#define __RECORDER_H__

void* ffrecorder_init (char *name, char *type, int duration, int channels, int samprate, int width, int height, int fps, void *codeclist, int codecnum);
void  ffrecorder_exit (void *ctxt);
void  ffrecorder_start(void *ctxt, int start);

#endif
