# Disco MQTT 2.0

A modern MQTT sensor application for the STM32 Discovery L475 IoT board using Zephyr RTOS.

## Features

- **Modern Architecture**: Modular design with separate WiFi, MQTT, and device management components
- **JSON Sensor Data**: Temperature and humidity data published in JSON format
- **Command Support**: Remote control via MQTT commands
- **LED Status Indicators**: Visual feedback for WiFi, MQTT, and error states
- **Robust Error Handling**: Automatic reconnection and comprehensive error management
- **Work Queue Pattern**: Efficient periodic publishing using Zephyr work queues

## Hardware Requirements

- STM32 Discovery L475 IoT1 board (disco_l475_iot1)
- Onboard HTS221 temperature/humidity sensor
- Onboard WiFi module (ISM43362)
- Built-in LEDs for status indication

## Software Architecture

```
main.c              # Main application logic and connection management
├── device.c        # Sensor reading and LED control
├── wifi_manager.c  # WiFi connection management
└── mqtt_client.c   # MQTT client with JSON payload support
```

## Configuration

The application can be configured via Kconfig or by editing `prj.conf`:

### Network Settings
- `CONFIG_WIFI_SSID`: WiFi network name (default: "ss Wireless")
- `CONFIG_WIFI_PASSWORD`: WiFi password (default: "Vivian1123")

### MQTT Settings
- `CONFIG_NET_SAMPLE_MQTT_BROKER_HOSTNAME`: MQTT broker hostname (default: "test.mosquitto.org")
- `CONFIG_NET_SAMPLE_MQTT_BROKER_PORT`: MQTT broker port (default: "1883")
- `CONFIG_NET_SAMPLE_MQTT_PUB_TOPIC`: Sensor data topic (default: "disco/sensors")
- `CONFIG_NET_SAMPLE_MQTT_SUB_TOPIC_CMD`: Command topic (default: "disco/commands")
- `CONFIG_NET_SAMPLE_MQTT_PUBLISH_INTERVAL`: Publish interval in seconds (default: 5)

## Building and Flashing

### Prerequisites
1. Zephyr SDK installed and environment set up
2. West tool installed

### Build Steps
```bash
# Activate Zephyr environment
source $HOME/zephyr_install/env/bin/activate

# Navigate to repository root
cd /path/to/your/repo

# Build the application
west build -b disco_l475_iot1 Hermes-Azure/disco_mqtt_2 --pristine
```

### Flashing
```bash
# Flash the board
west flash
```

## MQTT Topics

### Published Topics
- `disco/sensors`: JSON-formatted sensor data
  ```json
  {
    "unit": "Celsius/Percent",
    "temperature": 23.45,
    "humidity": 65.2,
    "timestamp": 12345678
  }
  ```

### Subscribed Topics
- `disco/commands`: Remote control commands
  - `led_on`: Turn on status LED
  - `led_off`: Turn off status LED
  - `status`: Get device status

## LED Status Indicators

- **Status LED (LED0)**: General application status
  - ON: Network connected
  - OFF: Network disconnected
  
- **Network LED (LED2)**: MQTT connection status
  - ON: MQTT connected
  - OFF: MQTT disconnected
  
- **Error LED**: Error indication
  - ON: Critical error occurred

## Testing with Mosquitto

### Local MQTT Broker Setup

1. Run Mosquitto broker in Docker:
```bash
docker run -it -p 1883:1883 -p 9001:9001 eclipse-mosquitto:latest
```

2. Subscribe to sensor data:
```bash
mosquitto_sub -h localhost -t "disco/sensors"
```

3. Send commands:
```bash
mosquitto_pub -h localhost -t "disco/commands" -m "led_on"
mosquitto_pub -h localhost -t "disco/commands" -m "led_off"
mosquitto_pub -h localhost -t "disco/commands" -m "status"
```

### Using Public Broker

The default configuration uses `test.mosquitto.org` for easy testing:

```bash
# Subscribe to your device's data
mosquitto_sub -h test.mosquitto.org -t "disco/sensors"

# Send commands to your device
mosquitto_pub -h test.mosquitto.org -t "disco/commands" -m "led_on"
```

## Troubleshooting

### WiFi Connection Issues
- Verify SSID and password in configuration
- Check WiFi network availability
- Monitor logs for specific error codes

### MQTT Connection Issues
- Verify broker hostname and port
- Check network connectivity
- Ensure firewall allows MQTT traffic (port 1883)

### Build Issues
- Ensure Zephyr environment is properly activated
- Check that all dependencies are installed
- Try building with `--pristine` flag to clean build

## Logs and Debugging

Enable debug logging by modifying log levels in the source files:
```c
LOG_MODULE_REGISTER(module_name, LOG_LEVEL_DBG);
```

Monitor serial output at 115200 baud to see application logs.

## License

SPDX-License-Identifier: Apache-2.0 