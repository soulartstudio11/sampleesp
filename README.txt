FRESHNEST SMART BASKET DASHBOARD

Files
- index.html: single-page dashboard with Dashboard, Auto-order control, Budget & history, Alerts, Settings.

Run
1. Keep internet enabled for the MQTT.js and Chart.js libraries (loaded from CDNs).
2. Open index.html in a browser, preferably through a local static server or HTTPS hosting.
3. Open Settings and enter your MQTT broker host, secure WebSocket port, path, username/password, and telemetry topic.
4. Press Connect. Your ESP32 should publish JSON telemetry to the configured topic.

Expected telemetry JSON examples
{"fillPercent":42,"distanceCm":23.2,"gas":850}
{"capacityPercent":5,"gasRaw":1950,"distance_cm":38.0}

Accepted fill fields: fillPercent, capacityPercent, capacity, fill_percentage, percent.
Accepted distance fields: distanceCm, distance_cm, distance, distCm.
Accepted gas fields: gas, gasValue, gasRaw, mq3, mqValue, gas_adc.
If fill percentage is not included, the website estimates it as (1 - distanceCm / bowlHeightCm) * 100. This simple estimate must be calibrated for your actual basket geometry. Prefer publishing a calibrated fillPercent from the ESP32.

Order safety
- When capacity goes below the low-stock threshold, one order request is created and the order latch is locked.
- It will not trigger again until capacity rises to the configured re-arm percentage.
- Semi-auto mode requires owner approval. Email delivery requires a backend notification endpoint; without one the browser opens a mail draft (manual send required).
- Full-auto mode sends a POST request to the configured shop API endpoint. If no endpoint is configured, no purchase is made. You must build/use an authorized shop integration and verify its API contract.
- The browser dashboard is a prototype: its settings/history are saved in localStorage on that browser. For reliable automation, use a secured backend/database, authentication, server-side order locking/idempotency, and server-side budget checks. Do not expose shop or SMTP secrets in HTML.

MQTT security
- Use secure WSS (for example, EMQX Cloud's WSS endpoint/port as configured in your deployment).
- Use a broker user limited to the required telemetry subscribe and reorder publish permissions.
- Never put administrator credentials in a public website.

Hardware note
If the HC-SR04 ECHO pin outputs 5 V, use a resistor voltage divider/level shifter before connecting it to an ESP32 GPIO. A raw MQ sensor reading is not a scientifically validated fruit-spoilage measurement; calibrate a threshold and treat it as a warning only.
