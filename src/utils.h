#ifndef UTILS_H
#define UTILS_H

#include <pthread.h>
#include "queue.h"

// Global shared sensor data queue
extern queue_t sensor_queue;

// Latest reading structure for network monitoring
typedef struct {
    char time_str[64];
    float sensor1;
    float sensor2;
    float average;
} latest_reading_t;

// Global variable for the latest processed sensor reading
extern latest_reading_t latest_reading;
// Mutex to protect latest_reading
extern pthread_mutex_t latest_mutex;

// Initialize utilities and global flags
void init_utils(void);

// Check if shutdown has been requested
int should_exit(void);

// Signal threads to exit
void set_exit_flag(void);

#endif // UTILS_H