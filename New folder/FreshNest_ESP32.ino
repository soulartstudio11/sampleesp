#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h>
#include <math.h>

// Replace these placeholders with your NEW credentials.
const char* WIFI_SSID     = "iPhone";
const char* WIFI_PASSWORD = "habeeb123";
const char* MQTT_HOST     = "f8b09f99.ala.eu-central-1.emqxsl.com";
const int   MQTT_PORT     = 8883;
const char* MQTT_USER     = "dilshad";
const char* MQTT_PASSWORD = "diludilshad95";
const char* MQTT_TOPIC    = "freshnest/basket01/telemetry";

const int TRIG_PIN = 5;
const int ECHO_PIN = 18;
const int GAS_PIN  = 34;

// Calibrate these to your actual basket.
const float EMPTY_DISTANCE_CM = 30.0f; // sensor -> fruit/bottom when empty
const float FULL_DISTANCE_CM  = 5.0f; // sensor -> fruit surface when full

const unsigned long PUBLISH_INTERVAL_MS = 2000;
const unsigned long WIFI_RETRY_MS = 5000;
const unsigned long MQTT_RETRY_MS = 5000;

WiFiClientSecure secureClient;
PubSubClient mqtt(secureClient);

float distanceCm = NAN;
float smoothedDistanceCm = NAN;
int gasRaw = 0;
int fillPercent = -1; // -1 means no valid sensor reading received yet
bool sensorValid = false;
unsigned long lastPublish = 0;
unsigned long lastWiFiAttempt = 0;
unsigned long lastMQTTAttempt = 0;

bool clockIsSet() {
  return time(nullptr) > 1700000000;
}

void startClockSync() {
  // UTC epoch is sent; the dashboard/browser formats it in local time.
  configTime(0, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
}

float readOneDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(3);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // pulseIn times out after 30 ms (about 5 m maximum theoretical range).
  const unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);
  if (duration == 0) return NAN;

  const float cm = duration * 0.0343f / 2.0f;
  // HC-SR04 practical minimum is around 2 cm. Reject implausible readings.
  if (cm < 2.0f || cm > 400.0f) return NAN;
  return cm;
}

// Median of up to five valid samples rejects isolated echo spikes.
float readFilteredDistance() {
  float samples[5];
  int n = 0;
  for (int i = 0; i < 5; ++i) {
    float v = readOneDistance();
    if (!isnan(v)) samples[n++] = v;
    delay(35);
  }
  if (n < 3) return NAN; // don't trust a reading with too few echoes

  for (int i = 0; i < n - 1; ++i) {
    for (int j = i + 1; j < n; ++j) {
      if (samples[j] < samples[i]) {
        float t = samples[i]; samples[i] = samples[j]; samples[j] = t;
      }
    }
  }
  return samples[n / 2];
}

int calculateFill(float distance) {
  if (isnan(distance) || EMPTY_DISTANCE_CM <= FULL_DISTANCE_CM) return -1;
  float p = (EMPTY_DISTANCE_CM - distance) * 100.0f /
            (EMPTY_DISTANCE_CM - FULL_DISTANCE_CM);
  p = constrain(p, 0.0f, 100.0f);
  return (int)roundf(p);
}

void maintainWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  const unsigned long now = millis();
  if (now - lastWiFiAttempt < WIFI_RETRY_MS) return;
  lastWiFiAttempt = now;
  Serial.println("Wi-Fi disconnected; starting reconnect...");
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void maintainMQTT() {
  if (WiFi.status() != WL_CONNECTED || mqtt.connected()) return;
  const unsigned long now = millis();
  if (now - lastMQTTAttempt < MQTT_RETRY_MS) return;
  lastMQTTAttempt = now;

  // Unique client ID avoids accidentally kicking off another ESP32 session.
  char clientId[40];
  snprintf(clientId, sizeof(clientId), "FreshNest-%llX",
           (unsigned long long)ESP.getEfuseMac());
  Serial.print("Connecting to MQTT... ");
  if (mqtt.connect(clientId, MQTT_USER, MQTT_PASSWORD)) {
    Serial.println("connected");
  } else {
    Serial.print("failed, state=");
    Serial.println(mqtt.state());
  }
}

void publishReadings() {
  char payload[256];
  char distanceText[20];
  char fillText[12];

  if (sensorValid && !isnan(distanceCm)) snprintf(distanceText, sizeof(distanceText), "%.2f", distanceCm);
  else snprintf(distanceText, sizeof(distanceText), "null");

  if (fillPercent >= 0) snprintf(fillText, sizeof(fillText), "%d", fillPercent);
  else snprintf(fillText, sizeof(fillText), "null");

  // Milliseconds since Unix epoch. Dashboard accepts timestampMs.
  long long timestampMs = clockIsSet() ? ((long long)time(nullptr) * 1000LL) : 0LL;

  snprintf(payload, sizeof(payload),
    "{\"deviceName\":\"FreshNest-Basket-01\","
    "\"distanceCm\":%s,\"fillPercent\":%s,\"gasRaw\":%d,"
    "\"sensorValid\":%s,\"timestampMs\":%lld}",
    distanceText, fillText, gasRaw, sensorValid ? "true" : "false", timestampMs);

  bool success = mqtt.connected() && mqtt.publish(MQTT_TOPIC, payload, false);
  Serial.println("\n----- FRESHNEST -----");
  Serial.printf("Distance: %s\n", sensorValid ? String(distanceCm, 2).c_str() : "invalid/no echo");
  Serial.printf("Fill: %s\n", fillPercent >= 0 ? (String(fillPercent) + "%").c_str() : "unknown");
  Serial.printf("Gas raw: %d\n", gasRaw);
  Serial.printf("Clock synced: %s\n", clockIsSet() ? "yes" : "no (timestampMs=0)");
  Serial.printf("MQTT publish: %s\n", success ? "SUCCESS" : "FAILED");
  Serial.print("JSON: "); Serial.println(payload);
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(GAS_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  analogReadResolution(12);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // NOTE: setInsecure() skips TLS certificate validation. Use the broker CA
  // certificate for production deployments; do not treat this as production TLS.
  secureClient.setInsecure();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setKeepAlive(30);
  mqtt.setSocketTimeout(5);
  mqtt.setBufferSize(512);

  startClockSync();
  Serial.println("FreshNest starting...");
}

void loop() {
  maintainWiFi();
  maintainMQTT();
  if (mqtt.connected()) mqtt.loop();

  const unsigned long now = millis();
  if (now - lastPublish >= PUBLISH_INTERVAL_MS) {
    lastPublish = now;

    float rawDistance = readFilteredDistance();
    if (!isnan(rawDistance)) {
      // Exponential smoothing reduces percentage flicker while still responding
      // to changes. First valid sample initializes the filter.
      if (isnan(smoothedDistanceCm)) smoothedDistanceCm = rawDistance;
      else smoothedDistanceCm = 0.70f * smoothedDistanceCm + 0.30f * rawDistance;
      distanceCm = smoothedDistanceCm;
      int calculated = calculateFill(distanceCm);
      if (calculated >= 0) fillPercent = calculated;
      sensorValid = true;
    } else {
      // Keep the last known value but explicitly mark the current sensor sample invalid.
      sensorValid = false;
      distanceCm = NAN;
    }

    gasRaw = analogRead(GAS_PIN);
    if (WiFi.status() == WL_CONNECTED && mqtt.connected()) publishReadings();
    else Serial.println("Skipped publish: Wi-Fi/MQTT not connected; will retry.");
  }
  delay(2); // yield to Wi-Fi tasks without blocking MQTT for long periods
}
