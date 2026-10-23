FreshNest reliability update

Files:
- FreshNest_ESP32.ino: improved ESP32 firmware with median ultrasonic filtering, exponential smoothing, reconnect retries, sensorValid, and timestampMs.
- index.html: dashboard updated to use device timestampMs/timestamp for live-update and chart labels, and to report malformed MQTT payloads more specifically.

Before uploading:
1. Change your Wi-Fi password and EMQX MQTT password because the old credentials were exposed in chat.
2. Put the new credentials into the placeholder fields at the top of FreshNest_ESP32.ino.
3. Confirm the topic is exactly freshnest/basket01/telemetry and use MQTT TLS port 8883 on ESP32.
4. The browser dashboard uses WSS port 8084, path /mqtt, and the same telemetry topic.
5. Keep EMPTY_DISTANCE_CM and FULL_DISTANCE_CM calibrated for the actual sensor mounting. The 5 cm full distance must match your real setup; if the sensor's practical near limit is about 2 cm, measure and calibrate accordingly.
6. Open Serial Monitor at 115200 baud. Check "Clock synced: yes", "MQTT publish: SUCCESS", and the JSON payload.

If Invalid telemetry continues, inspect all clients publishing to freshnest/basket01/telemetry. One non-JSON publisher or an old retained payload can cause the warning. This firmware publishes non-retained messages.

Security note: secureClient.setInsecure() disables TLS certificate verification. It is useful for a quick test but is not recommended for a production device; install the appropriate CA certificate for certificate verification.
