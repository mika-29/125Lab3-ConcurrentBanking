volatile int global_tick = 0;
pthread_mutex_t tick_lock;
pthread_cond_t tick_changed;