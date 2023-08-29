#define _POSIX_C_SOURCE 199309L
#include <time.h>

#include <timer.h>
#include <udp_common.h>

void getCurrentTime(struct timespec* t) {
    if (clock_gettime(CLOCK_MONOTONIC_RAW, t) == -1)
    {
        Die("Error getting current time.");
    }
}

long getDeltaMs(struct timespec* t_start, struct timespec* t) {
    long dt_s, dt_ns;
    dt_s = (t->tv_sec - t_start->tv_sec) * 1000;
    dt_ns = (t->tv_nsec - t_start->tv_nsec) / 1e6;
    return dt_s + dt_ns;
}

long getPassedTimeMs(struct timespec* t) {
    struct timespec t2;
    getCurrentTime(&t2);
    return getDeltaMs(t, &t2);
}

void sleepMs(long t) {
    struct timespec ts;
    ts.tv_sec = t / 1000;
    ts.tv_nsec = (t % 1000) * 1000000;
    nanosleep(&ts, NULL);
}
