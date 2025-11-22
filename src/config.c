#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

// Global configuration instance
config_t g_config;

// Trim whitespace from string
static char* trim(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

// Parse integer value with error handling (supports hex with 0x prefix)
// Returns 1 on success, 0 on error
static int parse_int(const char *str, int *result) {
    if (!str || !result) return 0;

    char *endptr;
    long val;

    errno = 0;
    if (strncmp(str, "0x", 2) == 0 || strncmp(str, "0X", 2) == 0) {
        val = strtol(str, &endptr, 16);
    } else {
        val = strtol(str, &endptr, 10);
    }

    // Check for conversion errors
    if (errno == ERANGE || val > INT_MAX || val < INT_MIN) {
        fprintf(stderr, "[Config] Error: Integer overflow for value '%s'\n", str);
        return 0;
    }

    if (endptr == str || *endptr != '\0') {
        fprintf(stderr, "[Config] Error: Invalid integer value '%s'\n", str);
        return 0;
    }

    *result = (int)val;
    return 1;
}

// Validate configuration values
static int validate_config(void) {
    int valid = 1;

    // Validate sensor intervals (must be positive)
    if (g_config.sensor1_interval <= 0) {
        fprintf(stderr, "[Config] Error: sensor1_interval must be > 0 (got %d)\n",
                g_config.sensor1_interval);
        valid = 0;
    }
    if (g_config.sensor2_interval <= 0) {
        fprintf(stderr, "[Config] Error: sensor2_interval must be > 0 (got %d)\n",
                g_config.sensor2_interval);
        valid = 0;
    }

    // Validate sensor timeout (must be positive)
    if (g_config.sensor_timeout <= 0) {
        fprintf(stderr, "[Config] Error: sensor_timeout must be > 0 (got %d)\n",
                g_config.sensor_timeout);
        valid = 0;
    }

    // Validate network port (1-65535)
    if (g_config.network_port < 1 || g_config.network_port > 65535) {
        fprintf(stderr, "[Config] Error: network_port must be in range 1-65535 (got %d)\n",
                g_config.network_port);
        valid = 0;
    }

    // Validate network backlog (must be positive)
    if (g_config.network_backlog <= 0) {
        fprintf(stderr, "[Config] Error: network_backlog must be > 0 (got %d)\n",
                g_config.network_backlog);
        valid = 0;
    }

    // Validate queue max size (must be non-negative)
    if (g_config.queue_max_size < 0) {
        fprintf(stderr, "[Config] Error: queue_max_size must be >= 0 (got %d)\n",
                g_config.queue_max_size);
        valid = 0;
    }

    // Validate I2C addresses (0x03-0x77 for 7-bit addressing)
    if (g_config.sensor1_address < 0x03 || g_config.sensor1_address > 0x77) {
        fprintf(stderr, "[Config] Warning: sensor1_address 0x%02x outside typical I2C range (0x03-0x77)\n",
                g_config.sensor1_address);
    }
    if (g_config.sensor2_address < 0x03 || g_config.sensor2_address > 0x77) {
        fprintf(stderr, "[Config] Warning: sensor2_address 0x%02x outside typical I2C range (0x03-0x77)\n",
                g_config.sensor2_address);
    }

    return valid;
}

void config_load_defaults(void) {
    // Set default values
    strncpy(g_config.i2c_device, "/dev/i2c-1", sizeof(g_config.i2c_device) - 1);
    g_config.i2c_device[sizeof(g_config.i2c_device) - 1] = '\0';  // Ensure null termination

    g_config.sensor1_address = 0x48;
    g_config.sensor1_interval = 1;
    g_config.sensor2_address = 0x49;
    g_config.sensor2_interval = 2;
    g_config.sensor_timeout = 10;
    g_config.network_port = 8080;
    g_config.network_backlog = 5;
    g_config.queue_max_size = 100;

    strncpy(g_config.log_file, "sensor_log.csv", sizeof(g_config.log_file) - 1);
    g_config.log_file[sizeof(g_config.log_file) - 1] = '\0';  // Ensure null termination
}

int config_load(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Warning: Could not open config file '%s', using defaults\n", filename);
        config_load_defaults();
        return -1;
    }

    // Start with defaults
    config_load_defaults();

    char line[512];
    char section[64] = "";
    int line_num = 0;

    while (fgets(line, sizeof(line), file)) {
        line_num++;
        char *trimmed = trim(line);

        // Skip empty lines and comments
        if (trimmed[0] == '\0' || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }

        // Check for section header
        if (trimmed[0] == '[') {
            char *end = strchr(trimmed, ']');
            if (end) {
                *end = '\0';
                strncpy(section, trimmed + 1, sizeof(section) - 1);
                section[sizeof(section) - 1] = '\0';  // Ensure null termination
            }
            continue;
        }

        // Parse key-value pair
        char *equals = strchr(trimmed, '=');
        if (!equals) continue;

        *equals = '\0';
        char *key = trim(trimmed);
        char *value = trim(equals + 1);

        // Parse based on section and key
        if (strcmp(section, "sensors") == 0) {
            if (strcmp(key, "i2c_device") == 0) {
                strncpy(g_config.i2c_device, value, sizeof(g_config.i2c_device) - 1);
                g_config.i2c_device[sizeof(g_config.i2c_device) - 1] = '\0';  // Ensure null termination
            } else if (strcmp(key, "sensor1_address") == 0) {
                int val;
                if (parse_int(value, &val)) {
                    g_config.sensor1_address = val;
                } else {
                    fprintf(stderr, "[Config] Line %d: Invalid sensor1_address, using default\n", line_num);
                }
            } else if (strcmp(key, "sensor1_interval") == 0) {
                int val;
                if (parse_int(value, &val)) {
                    g_config.sensor1_interval = val;
                } else {
                    fprintf(stderr, "[Config] Line %d: Invalid sensor1_interval, using default\n", line_num);
                }
            } else if (strcmp(key, "sensor2_address") == 0) {
                int val;
                if (parse_int(value, &val)) {
                    g_config.sensor2_address = val;
                } else {
                    fprintf(stderr, "[Config] Line %d: Invalid sensor2_address, using default\n", line_num);
                }
            } else if (strcmp(key, "sensor2_interval") == 0) {
                int val;
                if (parse_int(value, &val)) {
                    g_config.sensor2_interval = val;
                } else {
                    fprintf(stderr, "[Config] Line %d: Invalid sensor2_interval, using default\n", line_num);
                }
            } else if (strcmp(key, "sensor_timeout") == 0) {
                int val;
                if (parse_int(value, &val)) {
                    g_config.sensor_timeout = val;
                } else {
                    fprintf(stderr, "[Config] Line %d: Invalid sensor_timeout, using default\n", line_num);
                }
            }
        } else if (strcmp(section, "network") == 0) {
            if (strcmp(key, "port") == 0) {
                int val;
                if (parse_int(value, &val)) {
                    g_config.network_port = val;
                } else {
                    fprintf(stderr, "[Config] Line %d: Invalid port, using default\n", line_num);
                }
            } else if (strcmp(key, "backlog") == 0) {
                int val;
                if (parse_int(value, &val)) {
                    g_config.network_backlog = val;
                } else {
                    fprintf(stderr, "[Config] Line %d: Invalid backlog, using default\n", line_num);
                }
            }
        } else if (strcmp(section, "queue") == 0) {
            if (strcmp(key, "max_size") == 0) {
                int val;
                if (parse_int(value, &val)) {
                    g_config.queue_max_size = val;
                } else {
                    fprintf(stderr, "[Config] Line %d: Invalid max_size, using default\n", line_num);
                }
            }
        } else if (strcmp(section, "logging") == 0) {
            if (strcmp(key, "log_file") == 0) {
                strncpy(g_config.log_file, value, sizeof(g_config.log_file) - 1);
                g_config.log_file[sizeof(g_config.log_file) - 1] = '\0';  // Ensure null termination
            }
        }
    }

    fclose(file);

    // Validate all configuration values
    if (!validate_config()) {
        fprintf(stderr, "[Config] Error: Configuration validation failed. Falling back to defaults and continuing.\n");
        config_load_defaults();
        return -1;
    }

    printf("[Config] Loaded configuration from '%s'\n", filename);
    printf("  Sensor1: address=0x%02x, interval=%ds\n",
           g_config.sensor1_address, g_config.sensor1_interval);
    printf("  Sensor2: address=0x%02x, interval=%ds\n",
           g_config.sensor2_address, g_config.sensor2_interval);
    printf("  Network: port=%d\n", g_config.network_port);
    printf("  Queue: max_size=%d\n", g_config.queue_max_size);

    return 0;
}
