#include "utils.h"
#include <signal.h>

// Global exit flag using C11 atomics for proper thread safety
static atomic_int exit_flag = ATOMIC_VAR_INIT(0);

void init_utils(void) {
    atomic_store(&exit_flag, 0);
}

int should_exit(void) {
    return atomic_load(&exit_flag);
}

void set_exit_flag(void) {
    atomic_store(&exit_flag, 1);
}

// Instantiate the global sensor queue
queue_t sensor_queue;

// Instantiate latest_reading and latest_mutex
latest_reading_t latest_reading = { "", 0.0f, 0.0f, 0.0f };
pthread_mutex_t latest_mutex = PTHREAD_MUTEX_INITIALIZER;