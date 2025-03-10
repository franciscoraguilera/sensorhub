#include "data_processor.h"
#include "queue.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

// Data processor: collects readings from both sensors, computes the average temperature,
// prints the result, logs it to a CSV file, and updates the latest reading for remote monitoring.
void *data_processor_thread(void *arg) {
    (void)arg;
    float latest_temp1 = -999, latest_temp2 = -999;
    int got_sensor1 = 0, got_sensor2 = 0;

    FILE *log_file = fopen("sensor_log.csv", "a");
    if (!log_file) {
        perror("Opening log file");
        return NULL;
    }
    // If file is empty, write CSV header
    fseek(log_file, 0, SEEK_END);
    if (ftell(log_file) == 0) {
        fprintf(log_file, "timestamp,sensor1,sensor2,average\n");
        fflush(log_file);
    }

    while (!should_exit()) {
        sensor_reading_t reading;
        if (queue_pop(&sensor_queue, &reading) != 0) {
            break;
        }
        // Update latest value based on sensor ID
        if (reading.sensor_id == 1) {
            latest_temp1 = reading.temperature;
            got_sensor1 = 1;
        } else if (reading.sensor_id == 2) {
            latest_temp2 = reading.temperature;
            got_sensor2 = 1;
        }

        // When both sensor readings are available, process them
        if (got_sensor1 && got_sensor2) {
            float average = (latest_temp1 + latest_temp2) / 2.0f;
            time_t now = time(NULL);
            char time_str[64];
            struct tm *tm_info = localtime(&now);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
            printf("[Processor] %s | Sensor1: %.2f°C, Sensor2: %.2f°C, Average: %.2f°C\n",
                   time_str, latest_temp1, latest_temp2, average);
            fprintf(log_file, "%s,%.2f,%.2f,%.2f\n", time_str, latest_temp1, latest_temp2, average);
            fflush(log_file);
            
            // Update the latest reading for network monitoring
            pthread_mutex_lock(&latest_mutex);
            snprintf(latest_reading.time_str, sizeof(latest_reading.time_str), "%s", time_str);
            latest_reading.sensor1 = latest_temp1;
            latest_reading.sensor2 = latest_temp2;
            latest_reading.average = average;
            pthread_mutex_unlock(&latest_mutex);
            
            // Reset flags so that new readings are required for the next average
            got_sensor1 = got_sensor2 = 0;
        }
        usleep(100000); // Simulate processing time (100 ms)
    }
    fclose(log_file);
    return NULL;
}