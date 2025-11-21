#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

// Parse integer value (supports hex with 0x prefix)
static int parse_int(const char *str) {
    if (strncmp(str, "0x", 2) == 0 || strncmp(str, "0X", 2) == 0) {
        return (int)strtol(str, NULL, 16);
    }
    return atoi(str);
}

void config_load_defaults(void) {
    // Set default values
    strncpy(g_config.i2c_device, "/dev/i2c-1", sizeof(g_config.i2c_device) - 1);
    g_config.sensor1_address = 0x48;
    g_config.sensor1_interval = 1;
    g_config.sensor2_address = 0x49;
    g_config.sensor2_interval = 2;
    g_config.sensor_timeout = 10;
    g_config.network_port = 8080;
    g_config.network_backlog = 5;
    g_config.queue_max_size = 100;
    strncpy(g_config.log_file, "sensor_log.csv", sizeof(g_config.log_file) - 1);
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

    while (fgets(line, sizeof(line), file)) {
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
                section[sizeof(section) - 1] = '\0';
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
            } else if (strcmp(key, "sensor1_address") == 0) {
                g_config.sensor1_address = parse_int(value);
            } else if (strcmp(key, "sensor1_interval") == 0) {
                g_config.sensor1_interval = parse_int(value);
            } else if (strcmp(key, "sensor2_address") == 0) {
                g_config.sensor2_address = parse_int(value);
            } else if (strcmp(key, "sensor2_interval") == 0) {
                g_config.sensor2_interval = parse_int(value);
            } else if (strcmp(key, "sensor_timeout") == 0) {
                g_config.sensor_timeout = parse_int(value);
            }
        } else if (strcmp(section, "network") == 0) {
            if (strcmp(key, "port") == 0) {
                g_config.network_port = parse_int(value);
            } else if (strcmp(key, "backlog") == 0) {
                g_config.network_backlog = parse_int(value);
            }
        } else if (strcmp(section, "queue") == 0) {
            if (strcmp(key, "max_size") == 0) {
                g_config.queue_max_size = parse_int(value);
            }
        } else if (strcmp(section, "logging") == 0) {
            if (strcmp(key, "log_file") == 0) {
                strncpy(g_config.log_file, value, sizeof(g_config.log_file) - 1);
            }
        }
    }

    fclose(file);
    printf("[Config] Loaded configuration from '%s'\n", filename);
    printf("  Sensor1: address=0x%02x, interval=%ds\n",
           g_config.sensor1_address, g_config.sensor1_interval);
    printf("  Sensor2: address=0x%02x, interval=%ds\n",
           g_config.sensor2_address, g_config.sensor2_interval);
    printf("  Network: port=%d\n", g_config.network_port);
    printf("  Queue: max_size=%d\n", g_config.queue_max_size);

    return 0;
}
