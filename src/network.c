#include "network.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// The network thread listens on a TCP port and serves a simple HTTP response
void *network_thread(void *arg) {
    (void)arg;
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        return NULL;
    }
    // Allow reuse of the address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_fd);
        return NULL;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        return NULL;
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        return NULL;
    }
    
    printf("[Network] Server listening on port %d\n", PORT);
    
    while (!should_exit()) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            if (should_exit()) break;
            perror("accept");
            continue;
        }
        
        // Prepare HTTP response with the latest sensor reading
        pthread_mutex_lock(&latest_mutex);
        char response_body[512];
        snprintf(response_body, sizeof(response_body),
                 "<html><body>"
                 "<h1>Sensor Hub Status</h1>"
                 "<p>Last Update: %s</p>"
                 "<p>Sensor1: %.2f &deg;C</p>"
                 "<p>Sensor2: %.2f &deg;C</p>"
                 "<p>Average: %.2f &deg;C</p>"
                 "</body></html>",
                 latest_reading.time_str, latest_reading.sensor1, latest_reading.sensor2, latest_reading.average);
        pthread_mutex_unlock(&latest_mutex);
        
        char http_response[BUFFER_SIZE];
        snprintf(http_response, sizeof(http_response),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html\r\n"
                 "Content-Length: %zu\r\n"
                 "\r\n"
                 "%s", strlen(response_body), response_body);
        
        write(new_socket, http_response, strlen(http_response));
        close(new_socket);
    }
    
    close(server_fd);
    return NULL;
}