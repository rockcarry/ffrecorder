#ifndef __FFRECORDER_UTILS_H__
#define __FFRECORDER_UTILS_H__

#ifdef WIN32
#include <windows.h>
#define usleep(t)      Sleep((t) / 1000)
#define get_tick_count GetTickCount
#define snprintf      _snprintf
#else
#include <stdint.h>
#include <unistd.h>
uint32_t get_tick_count(void);
#endif

#define ARRAY_SIZE(a)  (sizeof(a) / sizeof(a[0]))
#define ALIGN(x, y)    ((x + y - 1) & ~(y - 1))
#define MIN(a, b)      ((a) < (b) ? (a) : (b))
#define MAX(a, b)      ((a) < (b) ? (a) : (b))

#endif
