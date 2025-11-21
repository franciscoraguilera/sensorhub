# Advanced Multithreaded Sensor Hub with Network Monitoring

A production-grade embedded application demonstrating real-time sensor data acquisition, multithreaded processing, and remote monitoring capabilities for POSIX-compliant systems.

## Features

- **Hardware Sensor Integration**: Reads temperature data from dual TMP102 sensors via I²C interface
- **Concurrent Processing**: Leverages POSIX threads with C11 atomics for proper thread safety
- **Configurable System**: INI-based configuration file for all runtime parameters
- **Error Recovery**: Automatic handling of single sensor failures with timeout-based fallback
- **Queue Management**: Bounded queue with configurable size limits to prevent OOM conditions
- **Data Management**: Processes, logs, and exposes sensor readings through multiple interfaces
- **Remote Monitoring**: Enhanced HTTP server with proper routing, JSON API, and error handling
- **Robust Architecture**: Thread-safe communication and graceful shutdown handling
- **Unit Tests**: Comprehensive test suite for core functionality

## Requirements

- POSIX-compliant system (Linux, Raspberry Pi OS)
- Two TMP102 temperature sensors connected via I²C
- C compiler with C11 support (for atomics)
- CMake 3.10+
- pthread library

## Hardware Setup

| Component | Configuration |
|-----------|---------------|
| TMP102 Sensor 1 | I²C address: 0x48 on `/dev/i2c-1` (configurable) |
| TMP102 Sensor 2 | I²C address: 0x49 on `/dev/i2c-1` (configurable) |
| I²C Bus | Connected to SDA/SCL on `/dev/i2c-1` |

Connect both sensors to the system's I²C bus (SDA/SCL pins).

## Configuration

The application uses a `config.ini` file for all runtime parameters. Create one in the same directory as the executable:

```ini
# SensorHub Configuration File

[sensors]
# I2C device path
i2c_device = /dev/i2c-1

# Sensor 1 configuration
sensor1_address = 0x48
sensor1_interval = 1

# Sensor 2 configuration
sensor2_address = 0x49
sensor2_interval = 2

# Timeout for single sensor failure (seconds)
# If one sensor fails for this long, process readings from working sensor only
sensor_timeout = 10

[network]
# HTTP server port
port = 8080
# Listen backlog
backlog = 5

[queue]
# Maximum queue size (0 = unbounded)
max_size = 100

[logging]
# CSV log file path
log_file = sensor_log.csv
```

### Configuration Options

#### Sensors Section
- **i2c_device**: Path to I²C device (default: `/dev/i2c-1`)
- **sensor1_address**: I²C address for sensor 1 in hex (default: `0x48`)
- **sensor1_interval**: Reading interval for sensor 1 in seconds (default: `1`)
- **sensor2_address**: I²C address for sensor 2 in hex (default: `0x49`)
- **sensor2_interval**: Reading interval for sensor 2 in seconds (default: `2`)
- **sensor_timeout**: Seconds to wait before processing single sensor (default: `10`)

#### Network Section
- **port**: HTTP server port (default: `8080`)
- **backlog**: TCP listen backlog (default: `5`)

#### Queue Section
- **max_size**: Maximum queue size, 0 for unbounded (default: `100`)

#### Logging Section
- **log_file**: Path to CSV log file (default: `sensor_log.csv`)

## Building and Running

```bash
# Clone repository
git clone https://github.com/franciscoraguilera/sensorhub.git
cd sensorhub

# Build
mkdir build && cd build
cmake ..
make

# Run tests
make check
# OR
ctest --output-on-failure

# Run application (may require elevated privileges for I²C access)
sudo ./sensorhub

# Run with custom config
sudo ./sensorhub /path/to/config.ini
```

Exit the application with `Ctrl+C` for graceful shutdown.

## Monitoring Interface

The application provides multiple HTTP endpoints:

### HTML Status Page
Access at `http://<device_ip>:8080/` or `http://<device_ip>:8080/index.html`

Displays:
- Timestamp of the most recent sensor reading
- Current temperature from each sensor (or N/A if unavailable)
- Calculated average temperature
- Link to JSON API

### JSON API
Access at `http://<device_ip>:8080/json` or `http://<device_ip>:8080/api/status`

Returns JSON format:
```json
{
  "timestamp": "2025-11-21 14:32:45",
  "sensor1": 23.50,
  "sensor2": 24.62,
  "average": 24.06,
  "status": "ok"
}
```

If a sensor is unavailable, its value will be `null`.

### Error Handling
- **404 Not Found**: Invalid paths return proper 404 page
- **405 Method Not Allowed**: Non-GET requests return 405 error

### Command-Line Access
```bash
# HTML page
curl http://<device_ip>:8080/

# JSON API
curl http://<device_ip>:8080/json
```

## Project Structure

| Component | Description |
|-----------|-------------|
| `src/main.c` | Application entry point, thread initialization, signal handling |
| `src/sensor.c/h` | TMP102 sensor interface via Linux I²C-dev |
| `src/data_processor.c/h` | Data collection, averaging, timeout handling, CSV logging |
| `src/queue.c/h` | Thread-safe bounded queue with size limits |
| `src/network.c/h` | HTTP server with routing, JSON API, proper error codes |
| `src/utils.c/h` | Shared data structures with C11 atomic operations |
| `src/config.c/h` | INI configuration file parser |
| `tests/` | Unit tests for queue, config, and utilities |

## Architecture Improvements

### Thread Safety
- **C11 Atomics**: Exit flag now uses `atomic_int` for proper thread safety
- **Mutex Protection**: All shared state properly protected with pthread mutexes
- **Condition Variables**: Efficient thread wakeup on shutdown

### Error Recovery
- **Single Sensor Timeout**: If one sensor fails for `sensor_timeout` seconds, system continues with the working sensor
- **Graceful Degradation**: CSV logs show "N/A" for failed sensors
- **Recovery Detection**: System automatically detects when failed sensor recovers

### Queue Management
- **Bounded Queue**: Configurable max size prevents out-of-memory conditions
- **Backpressure**: When queue is full, new readings are dropped with warning
- **Size Tracking**: Thread-safe queue size tracking

### HTTP Server
- **Request Parsing**: Proper HTTP method and path parsing
- **Routing**: Multiple endpoints with different content types
- **Status Codes**: Proper 200 OK, 404 Not Found, 405 Method Not Allowed
- **JSON Support**: RESTful JSON API for programmatic access
- **Styled HTML**: Clean, readable HTML with CSS styling

## Testing

Run the test suite:

```bash
cd build
make check
```

Or run individual tests:
```bash
./test_utils
./test_config
./test_queue
```

Tests cover:
- Queue operations (push, pop, bounds, thread safety)
- Configuration loading and defaults
- Atomic exit flag operations
- Shared state management

## Implementation Notes

- **CSV Logging**: Logs to configured file path (default: `sensor_log.csv`) in current directory
- **Sensor Timeout**: After configured timeout, system processes single sensor readings
- **Thread-Safe**: All shared state protected with appropriate synchronization primitives
- **Configuration**: All hardcoded values moved to `config.ini` for easy customization
- **Standards Compliance**: Uses C11 standard for modern features (atomics, inline)

## Performance Characteristics

- **Memory**: Minimal footprint, bounded queue prevents unbounded growth
- **CPU**: Low overhead, sensors poll at configurable intervals (1-2s default)
- **Network**: Lightweight HTTP server, no external dependencies
- **Reliability**: Handles sensor failures gracefully, continues operation with degraded capability

## Development

### Adding New Sensors
1. Update `config.ini` with new sensor configuration
2. Create new thread function in `sensor.c`
3. Update `main.c` to spawn new thread
4. Modify `data_processor.c` to handle additional sensor IDs

### Customizing Intervals
Edit `config.ini`:
```ini
sensor1_interval = 5  # Read every 5 seconds
sensor2_interval = 10 # Read every 10 seconds
```

### Changing Network Port
Edit `config.ini`:
```ini
[network]
port = 9000  # Use port 9000 instead
```

## Troubleshooting

### I²C Permission Denied
```bash
sudo usermod -a -G i2c $USER
# Log out and back in
```

### Config File Not Found
Application will use built-in defaults and display warning. Create `config.ini` in the same directory as the executable.

### Queue Full Messages
Increase queue size in `config.ini`:
```ini
[queue]
max_size = 200  # Or 0 for unbounded
```

### Sensor Timeout
Check sensor connections and I²C bus. Adjust timeout in `config.ini`:
```ini
sensor_timeout = 30  # Wait 30 seconds before fallback
```

## License

This project is licensed under the MIT License.

## Author

Fran (franciscoaguilera@ieee.org)

## Changelog

### v2.0 - Major Improvements
- Added INI-based configuration system
- Implemented C11 atomics for proper thread safety
- Added bounded queue with configurable size limits
- Implemented single sensor failure recovery with timeouts
- Enhanced HTTP server with routing, JSON API, and proper error codes
- Added comprehensive unit test suite
- Improved error handling throughout
- Better logging and diagnostics

### v1.0 - Initial Release
- Basic multithreaded sensor reading
- CSV logging
- Simple HTTP status page
