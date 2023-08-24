#ifndef TIMER_H
#define TIMER_H

#define _POSIX_C_SOURCE 199309L
#include <time.h>

void getCurrentTime(struct timespec* t);
long getDeltaMs(struct timespec* t_start, struct timespec* t);
long getPassedTimeMs(struct timespec* t);

#endif
