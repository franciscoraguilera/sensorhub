#include "sensor.h"
#include "queue.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

// Helper function to read temperature from a TMP102 sensor at a given I2C address.
// Returns temperature in °C on success; on failure, returns -999.
static float read_temperature(int i2c_address) {
    const char *i2c_device = "/dev/i2c-1";
    int file = open(i2c_device, O_RDWR);
    if (file < 0) {
        perror("Opening I2C device");
        return -999;
    }

    if (ioctl(file, I2C_SLAVE, i2c_address) < 0) {
        perror("Setting I2C address");
        close(file);
        return -999;
    }

    // TMP102 temperature register is at 0x00.
    unsigned char reg = 0x00;
    if (write(file, &reg, 1) != 1) {
        perror("Writing register");
        close(file);
        return -999;
    }

    unsigned char data[2];
    if (read(file, data, 2) != 2) {
        perror("Reading temperature");
        close(file);
        return -999;
    }
    close(file);

    // Convert the data to temperature (TMP102: 12-bit resolution)
    int temp_raw = ((data[0] << 4) | (data[1] >> 4));
    if (temp_raw & 0x800) { // negative temperature
        temp_raw = temp_raw - 4096;
    }
    float temperature = temp_raw * 0.0625f;
    return temperature;
}

void *sensor1_thread(void *arg) {
    (void)arg;
    while (!should_exit()) {
        float temp = read_temperature(0x48);
        if (temp != -999) {
            sensor_reading_t reading;
            reading.sensor_id = 1;
            reading.temperature = temp;
            reading.timestamp = time(NULL);
            queue_push(&sensor_queue, reading);
            printf("[Sensor1] Temperature: %.2f°C\n", temp);
        } else {
            printf("[Sensor1] Error reading sensor.\n");
        }
        sleep(1);  // Read every 1 second
    }
    return NULL;
}

void *sensor2_thread(void *arg) {
    (void)arg;
    while (!should_exit()) {
        float temp = read_temperature(0x49);
        if (temp != -999) {
            sensor_reading_t reading;
            reading.sensor_id = 2;
            reading.temperature = temp;
            reading.timestamp = time(NULL);
            queue_push(&sensor_queue, reading);
            printf("[Sensor2] Temperature: %.2f°C\n", temp);
        } else {
            printf("[Sensor2] Error reading sensor.\n");
        }
        sleep(2);  // Read every 2 seconds
    }
    return NULL;
}