#include "data_processor.h"
#include "queue.h"
#include "utils.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

// Data processor: collects readings from both sensors, computes the average temperature,
// prints the result, logs it to a CSV file, and updates the latest reading for remote monitoring.
// Now with timeout-based error recovery for single sensor failures.
void *data_processor_thread(void *arg) {
    (void)arg;
    float latest_temp1 = -999, latest_temp2 = -999;
    int got_sensor1 = 0, got_sensor2 = 0;
    time_t last_sensor1_time = 0, last_sensor2_time = 0;
    int sensor1_timeout_warned = 0, sensor2_timeout_warned = 0;

    FILE *log_file = fopen(g_config.log_file, "a");
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

        time_t now = time(NULL);

        // Update latest value based on sensor ID
        if (reading.sensor_id == 1) {
            latest_temp1 = reading.temperature;
            got_sensor1 = 1;
            last_sensor1_time = now;
            if (sensor1_timeout_warned) {
                printf("[Processor] Sensor1 recovered\n");
                sensor1_timeout_warned = 0;
            }
        } else if (reading.sensor_id == 2) {
            latest_temp2 = reading.temperature;
            got_sensor2 = 1;
            last_sensor2_time = now;
            if (sensor2_timeout_warned) {
                printf("[Processor] Sensor2 recovered\n");
                sensor2_timeout_warned = 0;
            }
        }

        // Check for sensor timeouts on every reading
        // Only check timeout for sensors that have sent at least one reading
        int sensor1_timed_out = 0;
        int sensor2_timed_out = 0;

        if (got_sensor1 && last_sensor1_time > 0 &&
            (now - last_sensor1_time) > g_config.sensor_timeout) {
            sensor1_timed_out = 1;
            if (!sensor1_timeout_warned) {
                printf("[Processor] Warning: Sensor1 timeout (no data for %lds)\n",
                       (long)(now - last_sensor1_time));
                sensor1_timeout_warned = 1;
            }
        }

        if (got_sensor2 && last_sensor2_time > 0 &&
            (now - last_sensor2_time) > g_config.sensor_timeout) {
            sensor2_timed_out = 1;
            if (!sensor2_timeout_warned) {
                printf("[Processor] Warning: Sensor2 timeout (no data for %lds)\n",
                       (long)(now - last_sensor2_time));
                sensor2_timeout_warned = 1;
            }
        }

        // Process readings based on availability
        int should_process = 0;
        float average = 0;
        char time_str[64];
        struct tm tm_info;
        localtime_r(&now, &tm_info);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_info);

        if (got_sensor1 && got_sensor2 && !sensor1_timed_out && !sensor2_timed_out) {
            // Both sensors working - process normally
            average = (latest_temp1 + latest_temp2) / 2.0f;
            printf("[Processor] %s | Sensor1: %.2f°C, Sensor2: %.2f°C, Average: %.2f°C\n",
                   time_str, latest_temp1, latest_temp2, average);
            fprintf(log_file, "%s,%.2f,%.2f,%.2f\n", time_str, latest_temp1, latest_temp2, average);
            should_process = 1;
            got_sensor1 = got_sensor2 = 0;
        } else if (got_sensor1 && (sensor2_timed_out || !got_sensor2)) {
            // Only sensor1 available or sensor2 timed out
            average = latest_temp1;  // Use sensor1 only
            printf("[Processor] %s | Sensor1: %.2f°C, Sensor2: N/A, Average: %.2f°C (sensor2 unavailable)\n",
                   time_str, latest_temp1, average);
            fprintf(log_file, "%s,%.2f,N/A,%.2f\n", time_str, latest_temp1, average);
            should_process = 1;
            got_sensor1 = 0;
        } else if (got_sensor2 && (sensor1_timed_out || !got_sensor1)) {
            // Only sensor2 available or sensor1 timed out
            average = latest_temp2;  // Use sensor2 only
            printf("[Processor] %s | Sensor1: N/A, Sensor2: %.2f°C, Average: %.2f°C (sensor1 unavailable)\n",
                   time_str, latest_temp2, average);
            fprintf(log_file, "%s,N/A,%.2f,%.2f\n", time_str, latest_temp2, average);
            should_process = 1;
            got_sensor2 = 0;
        }

        if (should_process) {
            fflush(log_file);

            // Update the latest reading for network monitoring
            // Use timeout flags instead of got_sensorX (which are already reset)
            pthread_mutex_lock(&latest_mutex);
            snprintf(latest_reading.time_str, sizeof(latest_reading.time_str), "%s", time_str);
            latest_reading.sensor1 = sensor1_timed_out ? -999 : latest_temp1;
            latest_reading.sensor2 = sensor2_timed_out ? -999 : latest_temp2;
            latest_reading.average = average;
            pthread_mutex_unlock(&latest_mutex);
        }

        usleep(100000); // Simulate processing time (100 ms)
    }
    fclose(log_file);
    return NULL;
}
