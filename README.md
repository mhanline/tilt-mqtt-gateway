# tilt-mqtt-gateway
Scans for Tilt Hydrometers using Bluetooth, then pushes the results to
Google Cloud IoT Core over secured MQTT.

Google Cloud Functions can then monitor the Pub/Sub topic and write to a Google Sheet.
See: https://github.com/mhanline/tilt-pubsub-ingest for that project.

## Setup

1. Clone this repo
```
git clone https://github.com/mhanline/tilt-mqtt-gateway.git
```

2. Modify configurations specific to your environment

```
mv src/tilt_mqtt_gateway_config.h.sample src/tilt_mqtt_gateway_config.h
```
Edit src/tilt_mqtt_gateway_config.h and modify the lines starting with TO-TO:

3. Google Cloud IoT Core setup
For the remainder of setting up Cloud IoT Core, see https://github.com/mhanline/tilt-pubsub-ingest/README.md.
You will need a private key which can be followed using the related project.