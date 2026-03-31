#include <Wire.h>
#include <Adafruit_BME280.h>
#include <ArduinoJson.h>
#include <time.h>

// ==================== SENSOR PINS ====================
#define MOISTURE_PIN A0      // Analog pin for soil moisture
#define LIGHT_PIN A1         // Analog pin for light sensor (TCS3200)
#define TEMP_ONE_WIRE 4      // OneWire for temperature (DS18B20) - optional
#define NPK_RX_PIN 16        // Serial RX for NPK sensor
#define NPK_TX_PIN 17        // Serial TX for NPK sensor

// ==================== ACTUATION PINS ====================
#define PUMP_RELAY_PIN 12    // GPIO 12 for water pump relay
#define FERTILIZER_LED_PIN 13 // GPIO 13 for fertilizer LED

// ==================== TIMING ====================
#define DECISION_INTERVAL 180000  // 3 minutes in milliseconds (180 sec)
#define SERIAL_BAUD_RATE 115200
#define SENSOR_READ_INTERVAL 5000 // Read sensors every 5 seconds

// ==================== GLOBALS ====================
unsigned long lastDecisionTime = 0;
unsigned long lastSensorReadTime = 0;
unsigned long systemStartTime = 0;
unsigned long waterEventTime = 0;
unsigned long fertilizeEventTime = 0;
int decisionCount = 0;

// Sensor data structure
struct SensorData {
  float moisture;      // 0-100%
  float temperature;   // °C
  float humidity;      // %
  float pressure;      // hPa
  float light;         // Lux
  struct {
    uint16_t n, p, k;  // NPK in ppm
  } npk;
  unsigned long timestamp;
};

SensorData currentSensors;

// BME280 sensor
Adafruit_BME280 bme;

// ==================== SETUP ====================
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(2000); // Wait for serial monitor to connect
  
  Serial.println("\n\n╔════════════════════════════════════════════╗");
  Serial.println("║  PS2 - Precision Farming Node (ESP32)     ║");
  Serial.println("║  Initializing Sensors...                  ║");
  Serial.println("╚════════════════════════════════════════════╝\n");

  // GPIO Setup
  pinMode(MOISTURE_PIN, INPUT);
  pinMode(LIGHT_PIN, INPUT);
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(FERTILIZER_LED_PIN, OUTPUT);
  
  // Default state: pumps and LED off
  digitalWrite(PUMP_RELAY_PIN, LOW);
  digitalWrite(FERTILIZER_LED_PIN, LOW);

  // Initialize BME280 on I2C (pins 21=SDA, 22=SCL)
  Wire.begin(21, 22);
  if (!bme.begin(0x76)) {  // 0x76 is default I2C address for BME280
    Serial.println("❌ ERROR: BME280 not found on I2C!");
    Serial.println("   Check wiring: SDA=GPIO21, SCL=GPIO22");
  } else {
    Serial.println("✓ BME280 initialized (Temp, Humidity, Pressure)");
  }

  // Initialize Serial2 for NPK sensor (RX=16, TX=17)
  Serial2.begin(9600, SERIAL_8N1, NPK_RX_PIN, NPK_TX_PIN);
  Serial.println("✓ NPK sensor serial port ready (RX=GPIO16, TX=GPIO17)");

  Serial.println("✓ Moisture sensor ready (ADC pin A0)");
  Serial.println("✓ Light sensor ready (ADC pin A1)");
  Serial.println("✓ Pump relay ready (GPIO 12)");
  Serial.println("✓ Fertilizer LED ready (GPIO 13)");

  // Set system start time
  systemStartTime = millis();
  
  Serial.println("\n✅ All systems initialized. Starting autonomous mode...\n");
}

// ==================== MAIN LOOP ====================
void loop() {
  unsigned long now = millis();

  // Read sensors at regular interval
  if (now - lastSensorReadTime >= SENSOR_READ_INTERVAL) {
    readAllSensors();
    lastSensorReadTime = now;
  }

  // Run decision logic every 3 minutes
  if (now - lastDecisionTime >= DECISION_INTERVAL) {
    runDecisionEngine();
    lastDecisionTime = now;
    decisionCount++;
  }

  // Check if pump/LED pulse timeout is over
  checkActuationTimeout();

  delay(100); // Small delay to prevent CPU thrashing
}

// ==================== SENSOR READING ====================
void readAllSensors() {
  // Read Moisture (ADC 0-1023 → 0-100%)
  int rawMoisture = analogRead(MOISTURE_PIN);
  currentSensors.moisture = map(rawMoisture, 0, 1023, 0, 100) + random(-2, 3); // Add slight noise for realism
  currentSensors.moisture = constrain(currentSensors.moisture, 0, 100);

  // Read Light Intensity (ADC 0-1023 → 0-1000 Lux approximation)
  int rawLight = analogRead(LIGHT_PIN);
  currentSensors.light = (rawLight / 1023.0) * 1000 + random(-20, 20);
  currentSensors.light = constrain(currentSensors.light, 0, 1000);

  // Read BME280 (Temperature, Humidity, Pressure)
  currentSensors.temperature = bme.readTemperature();
  currentSensors.humidity = bme.readHumidity();
  currentSensors.pressure = bme.readPressure() / 100.0F; // Convert Pa to hPa

  // Read NPK sensor (mock values - replace with actual serial parsing)
  readNPKSensor();

  currentSensors.timestamp = millis();
}

void readNPKSensor() {
  // For simulation: generate realistic NPK values
  // In production, parse Serial2 data from actual NPK sensor
  
  // Check if data is available on Serial2
  if (Serial2.available() > 0) {
    String npkData = Serial2.readStringUntil('\n');
    // Expected format: "N:120,P:45,K:180"
    // For now, use mock values
  }
  
  // Mock NPK values (in real implementation, parse serial data)
  currentSensors.npk.n = 100 + random(-10, 20);
  currentSensors.npk.p = 45 + random(-5, 10);
  currentSensors.npk.k = 170 + random(-10, 20);
}

// ==================== DECISION ENGINE ====================
void runDecisionEngine() {
  // Normalize all sensors to 0-1 scale
  float moistureScore = normalizeMoisture(currentSensors.moisture);
  float tempScore = normalizeTemperature(currentSensors.temperature);
  float humidityScore = normalizeHumidity(currentSensors.humidity);
  float lightScore = normalizeLight(currentSensors.light);
  float npkScore = normalizeNPK(currentSensors.npk.n, currentSensors.npk.p, currentSensors.npk.k);

  // Calculate Plant Stress Index (PSI) - weighted harmonic sum
  // PSI = 0.35*moisture + 0.20*temp + 0.15*humidity + 0.15*light + 0.15*npk
  float psi = (0.35 * moistureScore) + 
              (0.20 * tempScore) + 
              (0.15 * humidityScore) + 
              (0.15 * lightScore) + 
              (0.15 * npkScore);

  psi = constrain(psi, 0, 1.0);

  // Determine action based on multi-constraint logic
  String action = "MONITOR";
  float confidence = 0.5;
  String reasoning = "Plant health is stable";

  // Critical stress across all parameters
  if (psi < 0.3) {
    action = "WATER_NOW";
    confidence = 0.95;
    reasoning = "Critical plant stress detected. Moisture and multiple factors indicate urgent watering need.";
    triggerPump(20);
  }
  // Moderate stress with dry soil
  else if (psi < 0.45 && moistureScore < 0.4) {
    action = "WATER_NOW";
    confidence = 0.85;
    reasoning = "Soil is dry and plant stress is building. Immediate watering recommended.";
    triggerPump(20);
  }
  // Moderate stress but dry air prevents water absorption
  else if (psi < 0.45 && moistureScore < 0.55 && humidityScore < 0.3) {
    action = "WAIT";
    confidence = 0.80;
    reasoning = "Moderate stress but low humidity means water will evaporate quickly. Wait for better conditions.";
  }
  // Nutrient deficiency detected
  else if (psi < 0.5 && npkScore < 0.4) {
    action = "FERTILIZE";
    confidence = 0.75;
    reasoning = "NPK levels are low. Plant needs nutrients to utilize water effectively. Fertilizing recommended.";
    triggerFertilizer();
  }
  // Over-saturation risk
  else if (psi > 0.7 && moistureScore > 0.65 && humidityScore > 0.6) {
    action = "WAIT";
    confidence = 0.90;
    reasoning = "Soil is saturated and humidity is high. Risk of root rot. Do not water at this time.";
  }
  // Excellent conditions
  else if (psi > 0.75) {
    action = "MONITOR";
    confidence = 0.85;
    reasoning = "Plant health is excellent. All parameters within optimal range. Continue monitoring.";
  }
  // Default: marginal conditions
  else {
    action = "MONITOR";
    confidence = 0.65;
    reasoning = "Conditions are marginal. Continue monitoring. Next decision in 3 minutes.";
  }

  // Send JSON output to serial
  sendJSONReport(action, psi, confidence, reasoning, 
                 moistureScore, tempScore, humidityScore, lightScore, npkScore);
}

// ==================== NORMALIZATION FUNCTIONS ====================
float normalizeMoisture(float moisture) {
  // Target range: 20% (dry) to 70% (saturated)
  // Score = 1.0 means perfect, 0.0 means critically dry
  if (moisture < 20) return 0.0;
  if (moisture > 70) return 1.0;
  return (moisture - 20) / 50.0;
}

float normalizeTemperature(float temp) {
  // Optimal: 20-25°C, Range: 10-30°C
  if (temp < 10 || temp > 30) return 0.0;
  
  if (temp >= 20 && temp <= 25) return 1.0;
  
  // Linear falloff outside optimal range
  if (temp < 20) {
    return (temp - 10) / 10.0; // 10-20°C maps to 0-1
  } else {
    return 1.0 - ((temp - 25) / 5.0); // 25-30°C maps to 1-0
  }
}

float normalizeHumidity(float humidity) {
  // Optimal: 50-70%, Range: 30-80%
  if (humidity < 30 || humidity > 80) return 0.0;
  
  if (humidity >= 50 && humidity <= 70) return 1.0;
  
  if (humidity < 50) {
    return (humidity - 30) / 20.0; // 30-50% maps to 0-1
  } else {
    return 1.0 - ((humidity - 70) / 10.0); // 70-80% maps to 1-0
  }
}

float normalizeLight(float light) {
  // Optimal: 1000-5000 Lux, Range: 0-10000 Lux
  if (light < 0) return 0.0;
  if (light > 10000) return 0.5; // Diminishing returns for excess light
  
  if (light >= 1000 && light <= 5000) return 1.0;
  
  if (light < 1000) {
    return (light / 1000.0) * 0.5; // 0-1000 Lux maps to 0-0.5
  } else {
    return 1.0 - ((light - 5000) / 5000.0) * 0.5; // 5000-10000 maps to 1-0.5
  }
}

float normalizeNPK(uint16_t n, uint16_t p, uint16_t k) {
  // Optimal ranges: N(100-150), P(30-50), K(150-200)
  float nScore = (n >= 100 && n <= 150) ? 1.0 : (n < 80 ? 0.0 : 0.5);
  float pScore = (p >= 30 && p <= 50) ? 1.0 : (p < 25 ? 0.0 : 0.5);
  float kScore = (k >= 150 && k <= 200) ? 1.0 : (k < 130 ? 0.0 : 0.5);
  
  return (nScore + pScore + kScore) / 3.0;
}

// ==================== ACTUATION ====================
void triggerPump(int durationSeconds) {
  digitalWrite(PUMP_RELAY_PIN, HIGH);
  waterEventTime = millis();
  Serial.println("💧 [ACTION] Pump triggered! Running for " + String(durationSeconds) + " seconds");
  // Pump will turn off in checkActuationTimeout()
}

void triggerFertilizer() {
  digitalWrite(FERTILIZER_LED_PIN, HIGH);
  fertilizeEventTime = millis();
  Serial.println("🌱 [ACTION] Fertilizer LED activated for 5 seconds");
  // LED will turn off in checkActuationTimeout()
}

void checkActuationTimeout() {
  unsigned long now = millis();

  // Pump: run for 20 seconds
  if (waterEventTime > 0 && (now - waterEventTime) >= 20000) {
    digitalWrite(PUMP_RELAY_PIN, LOW);
    waterEventTime = 0;
  }

  // Fertilizer LED: run for 5 seconds
  if (fertilizeEventTime > 0 && (now - fertilizeEventTime) >= 5000) {
    digitalWrite(FERTILIZER_LED_PIN, LOW);
    fertilizeEventTime = 0;
  }
}

// ==================== JSON OUTPUT ====================
void sendJSONReport(String action, float psi, float confidence, String reasoning,
                    float moistScore, float tempScore, float humScore, float lightScore, float npkScore) {
  
  StaticJsonDocument<512> doc;
  
  // Timestamp
  unsigned long uptime = (millis() - systemStartTime) / 1000;
  doc["timestamp"] = (millis() / 1000);
  doc["uptime_seconds"] = uptime;
  
  // Sensor readings
  JsonObject sensors = doc.createNestedObject("sensors");
  sensors["moisture"] = round(currentSensors.moisture * 10) / 10.0;
  sensors["temperature"] = round(currentSensors.temperature * 10) / 10.0;
  sensors["humidity"] = round(currentSensors.humidity * 10) / 10.0;
  sensors["pressure"] = round(currentSensors.pressure * 10) / 10.0;
  sensors["light"] = round(currentSensors.light);
  
  JsonObject npk = sensors.createNestedObject("npk");
  npk["n"] = currentSensors.npk.n;
  npk["p"] = currentSensors.npk.p;
  npk["k"] = currentSensors.npk.k;

  // Decision data
  JsonObject decision = doc.createNestedObject("decision");
  decision["action"] = action;
  decision["psi"] = round(psi * 1000) / 1000.0;
  decision["confidence"] = round(confidence * 100) / 100.0;
  decision["reasoning"] = reasoning;
  
  // Score breakdown
  JsonObject scores = decision.createNestedObject("score_breakdown");
  scores["moisture"] = round(moistScore * 100) / 100.0;
  scores["temperature"] = round(tempScore * 100) / 100.0;
  scores["humidity"] = round(humScore * 100) / 100.0;
  scores["light"] = round(lightScore * 100) / 100.0;
  scores["npk"] = round(npkScore * 100) / 100.0;

  // Metrics
  doc["decision_count"] = decisionCount;

  // Serialize and send
  Serial.println("\n╔════════════════════════════════════════════╗");
  Serial.println("║  DECISION CYCLE #" + String(decisionCount) + "");
  Serial.println("╚════════════════════════════════════════════╝");
  serializeJson(doc, Serial);
  Serial.println("\n");
}

// ==================== UTILITY ====================
int round(float value, int decimals = 0) {
  return (int)(value * pow(10, decimals) + 0.5);
}
