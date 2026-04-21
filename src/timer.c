#include <stdio.h>
#include <unistd.h>
#include "timer.h"

volatile int global_tick = 0;
pthread_mutex_t tick_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t tick_changed = PTHREAD_COND_INITIALIZER;
int tick_interval_ms = 1000; 
volatile int simul_running = 1;

void timer_init(void) {
    global_tick = 0;
    simul_running = 1;
}

void timer_cleanup(void) {
    pthread_mutex_destroy(&tick_lock);
    pthread_cond_destroy(&tick_changed);
}

void *timer_thread(void *arg) {
    (void)arg;

     printf("Timer thread started [Tick interval: %d ms]\n", tick_interval_ms);

     while(simul_running) {
        usleep((useconds_t)(tick_interval_ms * 1000));
        pthread_mutex_lock(&tick_lock);
        global_tick++;
        printf("Tick %d:\n", global_tick);
        pthread_cond_broadcast(&tick_changed);
        pthread_mutex_unlock(&tick_lock);
     }

    printf("Timer thread exiting...\n");
    return NULL;

    void wait_for_tick(int target_tick) {
        pthread_mutex_lock(&tick_lock);

        while(global_tick < target_tick) {
            pthread_cond_wait(&tick_changed, &tick_Lock);
        }
        pthread_mutex_unlock(&tick_lock);
    }
}
