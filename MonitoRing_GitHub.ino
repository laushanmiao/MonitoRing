/*
 * MonitoRing - Environment Monitoring System
 * 
 * Hardware:
 * - ESP32 Dev Kit V1
 * - DHT11 (Pin D4/GPIO4) - Temperature & Humidity
 * - MQ135 (Pin D34/GPIO34) - Gas Sensor
 * - RCWL-0516 (Pin D32/GPIO32) - Motion Sensor
 * - LED (Pin D5/GPIO5) - Motion Alert
 * 
 * Features:
 * - Logs data to Google Sheets every 5 seconds
 * - Hosts local web dashboard
 * - Blinks LED on motion detection
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include "DHT.h"

// ==================== USER CONFIGURATION ====================
// TODO: Replace with your WiFi credentials
const char* ssid = "Your Wifi Name";
const char* password = "Your Wifi Password";

// TODO: Replace with your Google Apps Script deployment URL
const char* googleScriptURL = "Your Google App Script Deployment URL";
// ============================================================

// Pin Definitions
#define DHT_PIN 4       // D4
#define MQ135_PIN 34    // D34
#define MOTION_PIN 32   // D32
#define LED_PIN 5       // D5

// DHT Sensor Configuration
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// Web Server
WebServer server(80);

// Timing Variables
unsigned long lastSensorRead = 0;
unsigned long lastLEDBlink = 0;
const unsigned long SENSOR_INTERVAL = 5000;  // 5 seconds
const unsigned long LED_BLINK_INTERVAL = 200; // 200ms blink

// LED State
bool ledState = false;

// Motion debouncing
unsigned long lastMotionTime = 0;
const unsigned long MOTION_HOLD_TIME = 2000; // Keep motion "detected" for 2 seconds after last trigger
bool rawMotionState = false;

// Current Sensor Values
float temperature = 0;
float humidity = 0;
int gasPPM = 0;
int gasCondition = 1;
bool motionDetected = false;
bool wifiConnected = false;

// History Buffer (last 30 readings)
#define HISTORY_SIZE 30
struct SensorReading {
  String timestamp;
  float temperature;
  float humidity;
  int gasPPM;
  int gasCondition;
  bool motion;
};
SensorReading history[HISTORY_SIZE];
int historyIndex = 0;
int historyCount = 0;

// Function Prototypes
void connectWiFi();
void readSensors();
void printToSerial();
void sendToGoogleSheets();
void handleRoot();
void handleData();
void handleLEDBlink();
int calculateGasCondition(int ppm);
String getTimestamp();
void addToHistory();
String getHistoryJSON();

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  Serial.println("\n\n=== MonitoRing Starting ===\n");
  
  // Initialize Pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(MOTION_PIN, INPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Test LED - blink 3 times on startup
  Serial.println("Testing LED...");
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
  Serial.println("LED test complete!");
  
  // Enable internal pull-up for DHT11 data pin
  pinMode(DHT_PIN, INPUT_PULLUP);
  
  // Initialize DHT Sensor
  dht.begin();
  
  // Connect to WiFi
  connectWiFi();
  
  // Setup Web Server Routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("Web server started on port 80");
  
  // Initial sensor read
  delay(2000);  // Give DHT11 time to stabilize
  readSensors();
  printToSerial();
}

void loop() {
  // Handle web server requests
  server.handleClient();
  
  // Read motion sensor with debouncing
  unsigned long currentMillis = millis();
  rawMotionState = digitalRead(MOTION_PIN) == HIGH;
  
  // If motion detected, update the last motion time
  if (rawMotionState) {
    lastMotionTime = currentMillis;
  }
  
  // Motion stays "detected" for MOTION_HOLD_TIME after last trigger
  bool newMotionState = (currentMillis - lastMotionTime) < MOTION_HOLD_TIME;
  
  // Debug: Print when motion state changes
  if (newMotionState != motionDetected) {
    Serial.print("[MOTION] State changed: ");
    Serial.println(newMotionState ? "DETECTED!" : "No motion");
  }
  
  motionDetected = newMotionState;
  
  // Handle LED blinking when motion detected
  handleLEDBlink();
  
  // Read sensors and update every 5 seconds
  if (currentMillis - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = currentMillis;
    
    readSensors();
    printToSerial();
    addToHistory();
    sendToGoogleSheets();
  }
}

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\nWiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    wifiConnected = false;
    Serial.println("\nWiFi connection failed!");
  }
}

void readSensors() {
  // Check WiFi status
  wifiConnected = (WiFi.status() == WL_CONNECTED);
  
  // Read DHT11
  float newTemp = dht.readTemperature();
  float newHum = dht.readHumidity();
  
  // Debug: Print raw DHT readings
  Serial.print("[DEBUG] DHT Raw - Temp: ");
  Serial.print(newTemp);
  Serial.print(", Hum: ");
  Serial.println(newHum);
  
  if (!isnan(newTemp)) {
    temperature = newTemp;
  } else {
    Serial.println("[DEBUG] DHT11 read failed - check wiring!");
  }
  if (!isnan(newHum)) {
    humidity = newHum;
  }
  
  // Read MQ135 (analog value to PPM approximation)
  int analogValue = analogRead(MQ135_PIN);
  
  // Debug: Print raw analog value
  Serial.print("[DEBUG] MQ135 Raw ADC: ");
  Serial.println(analogValue);
  
  // Convert analog reading to approximate PPM
  // ESP32 ADC is 12-bit (0-4095), MQ135 typical range mapping
  gasPPM = map(analogValue, 0, 4095, 0, 1000);
  
  // Calculate gas condition (1-5 scale)
  gasCondition = calculateGasCondition(gasPPM);
  
  // Motion is read in loop() for real-time response
}

int calculateGasCondition(int ppm) {
  // Gas Quality Scale for indoor storage/neglected rooms
  // 1 = Excellent (0-100 PPM)
  // 2 = Good (101-200 PPM)
  // 3 = Moderate (201-400 PPM)
  // 4 = Poor (401-600 PPM)
  // 5 = Hazardous (>600 PPM)
  
  if (ppm <= 100) return 1;
  else if (ppm <= 200) return 2;
  else if (ppm <= 400) return 3;
  else if (ppm <= 600) return 4;
  else return 5;
}

void printToSerial() {
  Serial.println("\n--- Sensor Readings ---");
  Serial.print("WiFi: ");
  Serial.println(wifiConnected ? "connected" : "not connected");
  Serial.print("Temperature: ");
  Serial.println(temperature, 1);
  Serial.print("Humidity: ");
  Serial.println(humidity, 1);
  Serial.print("Gas pollution: ");
  Serial.println(gasPPM);
  Serial.print("Gas condition: ");
  Serial.println(gasCondition);
  Serial.print("Motion: ");
  Serial.println(motionDetected ? "detected" : "no motion detected");
  Serial.println("-----------------------");
}

void handleLEDBlink() {
  unsigned long currentMillis = millis();
  
  if (motionDetected) {
    // Blink LED at set interval
    if (currentMillis - lastLEDBlink >= LED_BLINK_INTERVAL) {
      lastLEDBlink = currentMillis;
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
    }
  } else {
    // Immediately turn off LED when no motion
    digitalWrite(LED_PIN, LOW);
    ledState = false;
  }
}

String getTimestamp() {
  // Simple timestamp using millis (since we don't have RTC)
  unsigned long seconds = millis() / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  
  seconds = seconds % 60;
  minutes = minutes % 60;
  hours = hours % 24;
  
  char timestamp[10];
  sprintf(timestamp, "%02lu:%02lu:%02lu", hours, minutes, seconds);
  return String(timestamp);
}

void addToHistory() {
  history[historyIndex].timestamp = getTimestamp();
  history[historyIndex].temperature = temperature;
  history[historyIndex].humidity = humidity;
  history[historyIndex].gasPPM = gasPPM;
  history[historyIndex].gasCondition = gasCondition;
  history[historyIndex].motion = motionDetected;
  
  historyIndex = (historyIndex + 1) % HISTORY_SIZE;
  if (historyCount < HISTORY_SIZE) {
    historyCount++;
  }
}

void sendToGoogleSheets() {
  if (!wifiConnected) {
    Serial.println("WiFi not connected, skipping Google Sheets update");
    return;
  }
  
  // Check if URL is configured
  if (String(googleScriptURL) == "YOUR_GOOGLE_SCRIPT_URL") {
    Serial.println("Google Script URL not configured, skipping");
    return;
  }
  
  HTTPClient http;
  
  // Build URL with parameters
  String url = String(googleScriptURL) + 
    "?temperature=" + String(temperature, 1) +
    "&humidity=" + String(humidity, 1) +
    "&gasPPM=" + String(gasPPM) +
    "&gasCondition=" + String(gasCondition) +
    "&motion=" + (motionDetected ? "detected" : "no motion");
  
  http.begin(url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    Serial.print("Google Sheets update: ");
    Serial.println(httpCode == 200 || httpCode == 302 ? "Success" : String(httpCode));
  } else {
    Serial.print("Google Sheets error: ");
    Serial.println(http.errorToString(httpCode));
  }
  
  http.end();
}

String getHistoryJSON() {
  String json = "[";
  
  // Start from oldest reading
  int start = (historyCount < HISTORY_SIZE) ? 0 : historyIndex;
  
  for (int i = 0; i < historyCount; i++) {
    int idx = (start + i) % HISTORY_SIZE;
    
    if (i > 0) json += ",";
    
    json += "{";
    json += "\"time\":\"" + history[idx].timestamp + "\",";
    json += "\"temp\":" + String(history[idx].temperature, 1) + ",";
    json += "\"hum\":" + String(history[idx].humidity, 1) + ",";
    json += "\"gas\":" + String(history[idx].gasPPM) + ",";
    json += "\"cond\":" + String(history[idx].gasCondition) + ",";
    json += "\"motion\":" + String(history[idx].motion ? "true" : "false");
    json += "}";
  }
  
  json += "]";
  return json;
}

void handleData() {
  // Add CORS headers to allow external dashboard to access
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  
  // JSON endpoint for current data
  String json = "{";
  json += "\"wifi\":" + String(wifiConnected ? "true" : "false") + ",";
  json += "\"temperature\":" + String(temperature, 1) + ",";
  json += "\"humidity\":" + String(humidity, 1) + ",";
  json += "\"gasPPM\":" + String(gasPPM) + ",";
  json += "\"gasCondition\":" + String(gasCondition) + ",";
  json += "\"motion\":" + String(motionDetected ? "true" : "false") + ",";
  json += "\"history\":" + getHistoryJSON();
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>MonitoRing Dashboard</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
      min-height: 100vh;
      color: #fff;
      padding: 20px;
    }
    .container { max-width: 900px; margin: 0 auto; }
    h1 {
      text-align: center;
      font-size: 2.5em;
      margin-bottom: 10px;
      background: linear-gradient(90deg, #00d9ff, #00ff88);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
    }
    .subtitle { text-align: center; color: #888; margin-bottom: 30px; }
    .status-bar {
      display: flex;
      justify-content: center;
      gap: 20px;
      margin-bottom: 30px;
    }
    .status {
      padding: 8px 20px;
      border-radius: 20px;
      font-size: 0.9em;
    }
    .status.online { background: rgba(0, 255, 136, 0.2); color: #00ff88; }
    .status.offline { background: rgba(255, 0, 0, 0.2); color: #ff4444; }
    .status.motion { background: rgba(255, 200, 0, 0.2); color: #ffc800; }
    .cards {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
      gap: 20px;
      margin-bottom: 30px;
    }
    .card {
      background: rgba(255, 255, 255, 0.05);
      border-radius: 15px;
      padding: 25px;
      text-align: center;
      border: 1px solid rgba(255, 255, 255, 0.1);
      backdrop-filter: blur(10px);
    }
    .card-icon { font-size: 2.5em; margin-bottom: 10px; }
    .card-value { font-size: 2.2em; font-weight: bold; margin-bottom: 5px; }
    .card-label { color: #888; font-size: 0.9em; }
    .card.temp .card-value { color: #ff6b6b; }
    .card.hum .card-value { color: #4ecdc4; }
    .card.gas .card-value { color: #ffe66d; }
    .gas-scale { margin-top: 10px; font-size: 0.85em; }
    .gas-scale span { padding: 3px 8px; border-radius: 10px; }
    .gas-1 { background: #00ff88; color: #000; }
    .gas-2 { background: #88ff00; color: #000; }
    .gas-3 { background: #ffcc00; color: #000; }
    .gas-4 { background: #ff8800; color: #000; }
    .gas-5 { background: #ff0000; color: #fff; }
    .history-section {
      background: rgba(255, 255, 255, 0.05);
      border-radius: 15px;
      padding: 25px;
      border: 1px solid rgba(255, 255, 255, 0.1);
    }
    .history-section h2 {
      margin-bottom: 20px;
      color: #00d9ff;
    }
    table { width: 100%; border-collapse: collapse; }
    th, td {
      padding: 12px 8px;
      text-align: center;
      border-bottom: 1px solid rgba(255, 255, 255, 0.1);
    }
    th { color: #00d9ff; font-weight: 600; }
    tbody tr:hover { background: rgba(255, 255, 255, 0.05); }
    .motion-yes { color: #ffc800; }
    .motion-no { color: #666; }
    .update-time { text-align: center; color: #666; margin-top: 20px; font-size: 0.85em; }
  </style>
</head>
<body>
  <div class="container">
    <h1>🔔 MonitoRing</h1>
    <p class="subtitle">Environment Monitoring Dashboard</p>
    
    <div class="status-bar">
      <span id="wifiStatus" class="status offline">WiFi: Checking...</span>
      <span id="motionStatus" class="status">Motion: Checking...</span>
    </div>
    
    <div class="cards">
      <div class="card temp">
        <div class="card-icon">🌡️</div>
        <div class="card-value" id="temp">--</div>
        <div class="card-label">Temperature (°C)</div>
      </div>
      <div class="card hum">
        <div class="card-icon">💧</div>
        <div class="card-value" id="hum">--</div>
        <div class="card-label">Humidity (%)</div>
      </div>
      <div class="card gas">
        <div class="card-icon">💨</div>
        <div class="card-value" id="gas">--</div>
        <div class="card-label">Gas (PPM)</div>
        <div class="gas-scale">
          Quality: <span id="gasCond" class="gas-1">--</span>
        </div>
      </div>
    </div>
    
    <div class="history-section">
      <h2>📊 History (Last 30 Readings)</h2>
      <table>
        <thead>
          <tr>
            <th>Time</th>
            <th>Temp</th>
            <th>Humidity</th>
            <th>Gas</th>
            <th>Quality</th>
            <th>Motion</th>
          </tr>
        </thead>
        <tbody id="historyTable">
          <tr><td colspan="6">Loading...</td></tr>
        </tbody>
      </table>
    </div>
    
    <p class="update-time">Auto-refreshes every 5 seconds</p>
  </div>
  
  <script>
    const condLabels = ['', 'Excellent', 'Good', 'Moderate', 'Poor', 'Hazardous'];
    
    function updateData() {
      fetch('/data')
        .then(res => res.json())
        .then(data => {
          // Update status bar
          document.getElementById('wifiStatus').className = 
            'status ' + (data.wifi ? 'online' : 'offline');
          document.getElementById('wifiStatus').textContent = 
            'WiFi: ' + (data.wifi ? 'Connected' : 'Disconnected');
          
          document.getElementById('motionStatus').className = 
            'status ' + (data.motion ? 'motion' : '');
          document.getElementById('motionStatus').textContent = 
            'Motion: ' + (data.motion ? 'DETECTED!' : 'None');
          
          // Update cards
          document.getElementById('temp').textContent = data.temperature.toFixed(1);
          document.getElementById('hum').textContent = data.humidity.toFixed(1);
          document.getElementById('gas').textContent = data.gasPPM;
          
          const condEl = document.getElementById('gasCond');
          condEl.textContent = condLabels[data.gasCondition];
          condEl.className = 'gas-' + data.gasCondition;
          
          // Update history table
          let tableHTML = '';
          const history = data.history.slice().reverse(); // Show newest first
          
          if (history.length === 0) {
            tableHTML = '<tr><td colspan="6">No data yet</td></tr>';
          } else {
            history.forEach(row => {
              tableHTML += `<tr>
                <td>${row.time}</td>
                <td>${row.temp.toFixed(1)}°C</td>
                <td>${row.hum.toFixed(1)}%</td>
                <td>${row.gas}</td>
                <td><span class="gas-${row.cond}">${condLabels[row.cond]}</span></td>
                <td class="${row.motion ? 'motion-yes' : 'motion-no'}">
                  ${row.motion ? '⚠️ Yes' : 'No'}
                </td>
              </tr>`;
            });
          }
          document.getElementById('historyTable').innerHTML = tableHTML;
        })
        .catch(err => console.error('Error fetching data:', err));
    }
    
    // Initial load and refresh every 5 seconds
    updateData();
    setInterval(updateData, 5000);
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}
