#!/bin/bash

# Mosquitto MQTT Broker Setup for Disco MQTT 2.0
# This script sets up a mosquitto broker accessible on the LAN

echo "Setting up Mosquitto MQTT Broker for Disco MQTT 2.0..."

# Get the local IP address (for macOS)
LOCAL_IP=$(ifconfig | grep "inet " | grep -v 127.0.0.1 | awk '{print $2}' | head -1)
echo "Local IP address: $LOCAL_IP"

# Create mosquitto configuration directory
mkdir -p mosquitto_config

# Create mosquitto configuration file
cat > mosquitto_config/mosquitto.conf << EOF
# Mosquitto configuration for Disco MQTT 2.0
listener 1883 0.0.0.0
allow_anonymous true
log_dest stdout
log_type all
connection_messages true
log_timestamp true
EOF

echo "Created mosquitto configuration file"

# Create Docker Compose file for easy management
cat > docker-compose.yml << EOF
version: '3.8'
services:
  mosquitto:
    image: eclipse-mosquitto:latest
    container_name: disco_mqtt_broker
    ports:
      - "1883:1883"
      - "9001:9001"
    volumes:
      - ./mosquitto_config/mosquitto.conf:/mosquitto/config/mosquitto.conf
    restart: unless-stopped
    networks:
      - mqtt_network

networks:
  mqtt_network:
    driver: bridge
EOF

echo "Created docker-compose.yml file"

# Start the mosquitto broker
echo "Starting Mosquitto broker..."
docker-compose up -d

# Wait a moment for the container to start
sleep 3

# Check if the container is running
if docker ps | grep -q disco_mqtt_broker; then
    echo "✅ Mosquitto broker is running successfully!"
    echo ""
    echo "MQTT Broker Information:"
    echo "  Host: $LOCAL_IP"
    echo "  Port: 1883"
    echo "  WebSocket Port: 9001"
    echo ""
    echo "Test Commands:"
    echo "  Subscribe to sensor data:"
    echo "    mosquitto_sub -h $LOCAL_IP -t 'disco/sensors'"
    echo ""
    echo "  Send LED command:"
    echo "    mosquitto_pub -h $LOCAL_IP -t 'disco/commands' -m 'led_on'"
    echo "    mosquitto_pub -h $LOCAL_IP -t 'disco/commands' -m 'led_off'"
    echo ""
    echo "  Get device status:"
    echo "    mosquitto_pub -h $LOCAL_IP -t 'disco/commands' -m 'status'"
    echo ""
    echo "Monitor broker logs:"
    echo "  docker-compose logs -f mosquitto"
    echo ""
    echo "Stop broker:"
    echo "  docker-compose down"
else
    echo "❌ Failed to start Mosquitto broker"
    echo "Check Docker logs: docker-compose logs mosquitto"
    exit 1
fi 