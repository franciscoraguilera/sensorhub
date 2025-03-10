# Advanced Multithreaded Sensor Hub with Network Monitoring

A robust embedded application demonstrating real-time sensor data acquisition, multithreaded processing, and remote monitoring capabilities for POSIX-compliant systems.

## Features

- **Hardware Sensor Integration**: Reads temperature data from dual TMP102 sensors via I²C interface
- **Concurrent Processing**: Leverages POSIX threads with sophisticated synchronization primitives
- **Data Management**: Processes, logs, and exposes sensor readings through multiple interfaces
- **Remote Monitoring**: Embedded HTTP server provides real-time system status
- **Robust Architecture**: Thread-safe communication and graceful shutdown handling

## Requirements

- POSIX-compliant system (Linux, Raspberry Pi OS)
- Two TMP102 temperature sensors connected via I²C
- C compiler with C11 support
- CMake 3.10+

## Hardware Setup

| Component | Configuration |
|-----------|---------------|
| TMP102 Sensor 1 | I²C address: 0x48 on `/dev/i2c-1` |
| TMP102 Sensor 2 | I²C address: 0x49 on `/dev/i2c-1` |
| I²C Bus | Connected to SDA/SCL on `/dev/i2c-1` |

Connect both sensors to the system's I²C bus (SDA/SCL pins).

## Building and Running

```bash
# Clone repository
git clone https://github.com/franciscoraguilera/advanced-multithread_sensorhub.git
cd advanced-multithread_sensorhub

# Build
mkdir build && cd build
cmake ..
make

# Run (may require elevated privileges for I²C access)
sudo ./sensorhub
```

Exit the application with `Ctrl+C` for graceful shutdown.

## Monitoring Interface

The application serves a status page at `http://<device_ip>:8080/` displaying:
- Timestamp of the most recent sensor reading
- Current temperature from each sensor
- Calculated average temperature

Access via any web browser or command-line tools:
```bash
curl http://<device_ip>:8080/
```

## Project Structure

| Component | Description |
|-----------|-------------|
| `src/main.c` | Application entry point, thread initialization, signal handling |
| `src/sensor.c/h` | TMP102 sensor interface via Linux I²C-dev |
| `src/data_processor.c/h` | Data collection, averaging, logging to CSV |
| `src/queue.c/h` | Thread-safe blocking queue implementation |
| `src/network.c/h` | HTTP server for remote status monitoring |
| `src/utils.c/h` | Shared data structures and common utilities |

## Build Configuration

```cmake
cmake_minimum_required(VERSION 3.10)
project(advanced_multithread_sensorhub C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
add_executable(sensorhub
    src/main.c
    src/sensor.c
    src/data_processor.c
    src/queue.c
    src/network.c
    src/utils.c
)
```

## Implementation Notes

- The application logs processed sensor data to `sensor_log.csv` in the current directory
- I²C bus and sensor addresses can be configured in `sensor.c` if needed
- Network monitoring interface uses a lightweight HTTP implementation
- Thread communication occurs through a custom thread-safe queue implementation

## License

This project is licensed under the MIT License.


## Author

Fran (franciscoaguilera@ieee.org)
