#include "network.h"
#include "utils.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>

#define BUFFER_SIZE 2048

// Helper function to HTML-escape a string to prevent XSS
static void html_escape(const char *src, char *dest, size_t dest_size) {
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j < dest_size - 6; i++) {
        // Reserve 6 chars for worst case entity like &quot;
        switch (src[i]) {
            case '<':
                if (j + 4 < dest_size) {
                    strcpy(dest + j, "&lt;");
                    j += 4;
                }
                break;
            case '>':
                if (j + 4 < dest_size) {
                    strcpy(dest + j, "&gt;");
                    j += 4;
                }
                break;
            case '&':
                if (j + 5 < dest_size) {
                    strcpy(dest + j, "&amp;");
                    j += 5;
                }
                break;
            case '"':
                if (j + 6 < dest_size) {
                    strcpy(dest + j, "&quot;");
                    j += 6;
                }
                break;
            case '\'':
                if (j + 6 < dest_size) {
                    strcpy(dest + j, "&#39;");
                    j += 5;
                }
                break;
            default:
                // Only allow printable ASCII characters
                if (isprint((unsigned char)src[i])) {
                    dest[j++] = src[i];
                } else {
                    // Replace non-printable with space
                    dest[j++] = ' ';
                }
                break;
        }
    }
    dest[j] = '\0';
}

// Helper function to parse HTTP request and extract method and path
// Prevents buffer overflow by limiting field widths
static void parse_http_request(const char *request, char *method, char *path) {
    // Limit method to 15 chars, path to 255 chars (leaving space for null terminator)
    sscanf(request, "%15s %255s", method, path);
}

// Helper function to send HTTP response
static void send_response(int socket, const char *status, const char *content_type,
                         const char *body) {
    char response[BUFFER_SIZE];
    int body_len = strlen(body);

    snprintf(response, sizeof(response),
             "HTTP/1.1 %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             status, content_type, body_len, body);

    ssize_t sent = write(socket, response, strlen(response));
    if (sent < 0) {
        perror("[Network] Failed to send response");
    }
}

// Generate HTML status page
static void generate_html_response(char *buffer, size_t size) {
    pthread_mutex_lock(&latest_mutex);

    // Check if we have valid data
    int has_data = (latest_reading.time_str[0] != '\0');

    if (has_data) {
        // Format sensor values (handle N/A cases)
        char sensor1_str[32], sensor2_str[32];

        if (latest_reading.sensor1 == -999) {
            snprintf(sensor1_str, sizeof(sensor1_str), "N/A");
        } else {
            snprintf(sensor1_str, sizeof(sensor1_str), "%.2f &deg;C", latest_reading.sensor1);
        }

        if (latest_reading.sensor2 == -999) {
            snprintf(sensor2_str, sizeof(sensor2_str), "N/A");
        } else {
            snprintf(sensor2_str, sizeof(sensor2_str), "%.2f &deg;C", latest_reading.sensor2);
        }

        snprintf(buffer, size,
                 "<!DOCTYPE html>"
                 "<html><head><title>SensorHub Status</title>"
                 "<style>body{font-family:Arial,sans-serif;margin:40px;}"
                 "h1{color:#333;}.status{background:#f0f0f0;padding:20px;border-radius:5px;}"
                 ".sensor{margin:10px 0;}</style></head>"
                 "<body>"
                 "<h1>SensorHub Status</h1>"
                 "<div class='status'>"
                 "<div class='sensor'><strong>Last Update:</strong> %s</div>"
                 "<div class='sensor'><strong>Sensor1:</strong> %s</div>"
                 "<div class='sensor'><strong>Sensor2:</strong> %s</div>"
                 "<div class='sensor'><strong>Average:</strong> %.2f &deg;C</div>"
                 "</div>"
                 "<p><a href='/json'>JSON API</a></p>"
                 "</body></html>",
                 latest_reading.time_str, sensor1_str, sensor2_str, latest_reading.average);
    } else {
        snprintf(buffer, size,
                 "<!DOCTYPE html>"
                 "<html><head><title>SensorHub Status</title></head>"
                 "<body>"
                 "<h1>SensorHub Status</h1>"
                 "<p>No sensor data available yet. Please wait...</p>"
                 "</body></html>");
    }

    pthread_mutex_unlock(&latest_mutex);
}

// Generate JSON status response
static void generate_json_response(char *buffer, size_t size) {
    pthread_mutex_lock(&latest_mutex);

    // Check if we have valid data
    int has_data = (latest_reading.time_str[0] != '\0');

    if (has_data) {
        // Format sensor values (handle N/A cases)
        char sensor1_str[32], sensor2_str[32];

        if (latest_reading.sensor1 == -999) {
            snprintf(sensor1_str, sizeof(sensor1_str), "null");
        } else {
            snprintf(sensor1_str, sizeof(sensor1_str), "%.2f", latest_reading.sensor1);
        }

        if (latest_reading.sensor2 == -999) {
            snprintf(sensor2_str, sizeof(sensor2_str), "null");
        } else {
            snprintf(sensor2_str, sizeof(sensor2_str), "%.2f", latest_reading.sensor2);
        }

        snprintf(buffer, size,
                 "{"
                 "\"timestamp\":\"%s\","
                 "\"sensor1\":%s,"
                 "\"sensor2\":%s,"
                 "\"average\":%.2f,"
                 "\"status\":\"ok\""
                 "}",
                 latest_reading.time_str, sensor1_str, sensor2_str, latest_reading.average);
    } else {
        snprintf(buffer, size,
                 "{"
                 "\"status\":\"no_data\","
                 "\"message\":\"No sensor data available yet\""
                 "}");
    }

    pthread_mutex_unlock(&latest_mutex);
}

// The network thread listens on a TCP port and serves HTTP responses
void *network_thread(void *arg) {
    (void)arg;
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("[Network] Socket creation failed");
        return NULL;
    }

    // Allow reuse of the address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("[Network] Setsockopt failed");
        close(server_fd);
        return NULL;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(g_config.network_port);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[Network] Bind failed");
        close(server_fd);
        return NULL;
    }

    if (listen(server_fd, g_config.network_backlog) < 0) {
        perror("[Network] Listen failed");
        close(server_fd);
        return NULL;
    }

    printf("[Network] Server listening on port %d\n", g_config.network_port);

    while (!should_exit()) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);

        if (new_socket < 0) {
            if (should_exit()) break;
            perror("[Network] Accept failed");
            continue;
        }

        // Read HTTP request
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_read = read(new_socket, buffer, BUFFER_SIZE - 1);

        if (bytes_read < 0) {
            perror("[Network] Read failed");
            close(new_socket);
            continue;
        }

        // Parse HTTP request
        char method[16] = {0};
        char path[256] = {0};
        parse_http_request(buffer, method, path);

        char response_body[BUFFER_SIZE];

        // Route based on method and path
        if (strcmp(method, "GET") == 0) {
            if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
                // HTML status page
                generate_html_response(response_body, sizeof(response_body));
                send_response(new_socket, "200 OK", "text/html; charset=utf-8", response_body);
            } else if (strcmp(path, "/json") == 0 || strcmp(path, "/api/status") == 0) {
                // JSON API
                generate_json_response(response_body, sizeof(response_body));
                send_response(new_socket, "200 OK", "application/json", response_body);
            } else {
                // 404 Not Found - HTML escape the path to prevent XSS
                char escaped_path[512];
                html_escape(path, escaped_path, sizeof(escaped_path));

                snprintf(response_body, sizeof(response_body),
                         "<!DOCTYPE html><html><head><title>404 Not Found</title></head>"
                         "<body><h1>404 Not Found</h1><p>The requested path '%s' was not found.</p>"
                         "<p><a href='/'>Go to homepage</a></p></body></html>",
                         escaped_path);
                send_response(new_socket, "404 Not Found", "text/html; charset=utf-8", response_body);
            }
        } else {
            // 405 Method Not Allowed
            snprintf(response_body, sizeof(response_body),
                     "<!DOCTYPE html><html><head><title>405 Method Not Allowed</title></head>"
                     "<body><h1>405 Method Not Allowed</h1><p>Only GET requests are supported.</p></body></html>");
            send_response(new_socket, "405 Method Not Allowed", "text/html; charset=utf-8", response_body);
        }

        close(new_socket);
    }

    close(server_fd);
    printf("[Network] Server shut down\n");
    return NULL;
}
