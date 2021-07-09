#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "utils.h"

#ifndef WIN32
uint32_t get_tick_count(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}
#endif

