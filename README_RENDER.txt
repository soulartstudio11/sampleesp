FreshNest — Render Static Site Deployment

Files
- index.html: dashboard layout and styles
- script.js: dashboard logic, MQTT-over-WebSocket connection, telemetry handling, charts and controls
- FreshNest_ESP32.ino: ESP32 firmware (upload separately using Arduino IDE)

Deploy to Render
1. Put index.html and script.js in the same folder at the root of the GitHub repository. Keep the MQTT and Chart.js CDN script tags in index.html.
2. Push/commit the files to GitHub.
3. In Render, choose New + > Static Site and connect that repository.
4. Set Root Directory to the folder containing index.html (leave blank if at repository root). Build Command can be blank; Publish Directory should be . (or that folder).
5. Deploy, then open the published HTTPS site.
6. Open Settings in FreshNest. Configure the EMQX host, secure WebSocket port 8084, path /mqtt, telemetry topic freshnest/basket01/telemetry, and your MQTT username/password. Save and connect.

Important
- Browser MQTT uses secure WebSockets (wss://host:8084/mqtt), not raw MQTT TLS port 8883.
- Do not put ESP32 Wi-Fi credentials in website files. If MQTT credentials were exposed in chat or a public repository, rotate them and use a restricted account.
- A Render Static Site only hosts the website; it does not run the ESP32 firmware or act as a private backend. Anything entered in browser settings is visible to that browser user. For production, use a backend to protect broker/shop secrets.
- Keep index.html and script.js together in the same published directory.
- This site depends on internet access for MQTT.js and Chart.js CDN resources.
