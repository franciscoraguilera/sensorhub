#include "utils.h"
#include <signal.h>

// Global exit flag
static volatile int exit_flag = 0;

void init_utils(void) {
    exit_flag = 0;
}

int should_exit(void) {
    return exit_flag;
}

void set_exit_flag(void) {
    exit_flag = 1;
}

// Instantiate the global sensor queue
queue_t sensor_queue;

// Instantiate latest_reading and latest_mutex
latest_reading_t latest_reading = { "", 0.0f, 0.0f, 0.0f };
pthread_mutex_t latest_mutex = PTHREAD_MUTEX_INITIALIZER;