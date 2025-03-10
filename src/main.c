#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include "sensor.h"
#include "data_processor.h"
#include "network.h"
#include "utils.h"
#include "queue.h"

// Thread identifiers
pthread_t sensor1_tid, sensor2_tid, processor_tid, network_tid;

// Signal handler for SIGINT (Ctrl+C)
void sigint_handler(int signum) {
    (void)signum;
    printf("\nSIGINT received, shutting down...\n");
    set_exit_flag();
    // Wake up any threads waiting on the queue
    pthread_cond_broadcast(&sensor_queue.cond);
    // Also wake up the network thread (if blocked in accept)
    // (Accept will return error when should_exit() is true.)
}

int main(void) {
    init_utils();
    // Initialize the sensor queue
    queue_init(&sensor_queue);
    
    signal(SIGINT, sigint_handler);

    // Create sensor threads
    if (pthread_create(&sensor1_tid, NULL, sensor1_thread, NULL) != 0) {
        perror("Failed to create sensor1 thread");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&sensor2_tid, NULL, sensor2_thread, NULL) != 0) {
        perror("Failed to create sensor2 thread");
        exit(EXIT_FAILURE);
    }
    // Create data processing thread
    if (pthread_create(&processor_tid, NULL, data_processor_thread, NULL) != 0) {
        perror("Failed to create data processor thread");
        exit(EXIT_FAILURE);
    }
    // Create network interface thread
    if (pthread_create(&network_tid, NULL, network_thread, NULL) != 0) {
        perror("Failed to create network thread");
        exit(EXIT_FAILURE);
    }

    // Wait for all threads to complete
    pthread_join(sensor1_tid, NULL);
    pthread_join(sensor2_tid, NULL);
    pthread_join(processor_tid, NULL);
    pthread_join(network_tid, NULL);

    // Clean up the sensor queue
    queue_destroy(&sensor_queue);

    printf("All threads terminated. Exiting program.\n");
    return 0;
}