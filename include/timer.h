#ifndef TIMER_H
#define TIMER_H

#include <pthread.h>

extern volatile int global_tick = 0;
extern pthread_mutex_t tick_lock;
extern pthread_cond_t tick_changed;

extern int tick_interval_ms;
extern volatile int simul_running;

void *timer_thread(void*arg);
void wait_for_tick(int target_tick);
void timer_init(void);
void timer_cleanup(void);

#endif 