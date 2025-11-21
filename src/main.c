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
#include "config.h"

// Thread identifiers
pthread_t sensor1_tid, sensor2_tid, processor_tid, network_tid;

// Signal handler for SIGINT (Ctrl+C)
void sigint_handler(int signum) {
    (void)signum;
    printf("\nSIGINT received, shutting down...\n");
    set_exit_flag();
    // Wake up any threads waiting on the queue
    pthread_cond_broadcast(&sensor_queue.cond);
}

int main(int argc, char *argv[]) {
    // Load configuration
    const char *config_file = "config.ini";
    if (argc > 1) {
        config_file = argv[1];
    }

    if (config_load(config_file) != 0) {
        fprintf(stderr, "Warning: Failed to load config from '%s', using defaults\n", config_file);
    }

    printf("=== SensorHub Starting ===\n");

    // Initialize utilities
    init_utils();

    // Initialize the sensor queue with configured max size
    queue_init(&sensor_queue, g_config.queue_max_size);
    printf("[Main] Queue initialized with max_size=%d\n",
           g_config.queue_max_size == 0 ? -1 : g_config.queue_max_size);

    // Register signal handler
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

    printf("[Main] All threads started successfully\n");

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
