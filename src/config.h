#ifndef CONFIG_H
#define CONFIG_H

// Configuration structure
typedef struct {
    // Sensor configuration
    char i2c_device[256];
    int sensor1_address;
    int sensor1_interval;
    int sensor2_address;
    int sensor2_interval;
    int sensor_timeout;

    // Network configuration
    int network_port;
    int network_backlog;

    // Queue configuration
    int queue_max_size;

    // Logging configuration
    char log_file[256];
} config_t;

// Global configuration instance
extern config_t g_config;

// Load configuration from file
// Returns 0 on success, -1 on failure
int config_load(const char *filename);

// Load default configuration
void config_load_defaults(void);

#endif // CONFIG_H
