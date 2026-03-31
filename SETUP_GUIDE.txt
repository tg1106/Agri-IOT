╔════════════════════════════════════════════════════════════════════════════════╗
║                                                                                ║
║           PS2 PRECISION FARMING NODE - SETUP & INTEGRATION GUIDE              ║
║                                                                                ║
║                          Autonomous Plant Watering System                      ║
║                                                                                ║
╚════════════════════════════════════════════════════════════════════════════════╝

📋 TABLE OF CONTENTS
═══════════════════════════════════════════════════════════════════════════════════

1. SYSTEM OVERVIEW
2. HARDWARE CHECKLIST
3. ESP32 CODE UPLOAD
4. PYTHON BRIDGE SETUP
5. WEBSITE LAUNCH
6. TESTING PROCEDURE
7. TROUBLESHOOTING

═══════════════════════════════════════════════════════════════════════════════════
1. SYSTEM OVERVIEW
═══════════════════════════════════════════════════════════════════════════════════

ARCHITECTURE:
┌──────────────┐        USB Cable         ┌──────────────┐       Flask Server
│              │◄─────────────────────────►│   Python     │  ◄──►  localhost:5000
│    ESP32     │    Serial (115200 baud)  │   Bridge     │
│              │                          │  (localhost) │
└──────────────┘                          └──────────────┘
      ▲                                           ▲
      │                                           │
  4 Sensors                                  Web Dashboard
  2 Relays                             (HTML + JavaScript)
      │                                           │
      └───────────────────────────────────────────┘
             Real-time Monitoring & Control

COMPONENTS:
──────────
• esp32_firmware.ino    → Arduino code for ESP32 (uploads to microcontroller)
• dashboard.html        → Web interface (open in browser)
• serial_bridge.py      → Python bridge (runs on laptop)

WORKFLOW:
────────
1. Upload esp32_firmware.ino to ESP32 using Arduino IDE
2. Connect ESP32 to laptop via USB cable
3. Run serial_bridge.py on laptop
4. Open dashboard.html in web browser at http://localhost:5000
5. Monitor and control plant watering in real-time

═══════════════════════════════════════════════════════════════════════════════════
2. HARDWARE CHECKLIST
═══════════════════════════════════════════════════════════════════════════════════

WIRING CONFIRMATION (Before powering on):
─────────────────────────────────────────

[ ] TCS3200 Color Sensor
    • VCC → 3.3V
    • GND → GND
    • OUT → GPIO A1 (Analog pin)

[ ] BME280 Temperature/Humidity/Pressure Sensor (I2C)
    • VCC → 3.3V
    • GND → GND
    • SDA → GPIO 21 (I2C Data)
    • SCL → GPIO 22 (I2C Clock)

[ ] Soil Moisture Sensor
    • VCC → 3.3V
    • GND → GND
    • OUT → GPIO A0 (Analog pin)

[ ] NPK Sensor (Serial UART)
    • VCC → 5V
    • GND → GND
    • RX → GPIO 16 (ESP32 TX → Sensor RX)
    • TX → GPIO 17 (ESP32 RX → Sensor TX)

[ ] Water Pump Relay
    • Signal → GPIO 12 (Water pump control)
    • VCC → 5V
    • GND → GND
    • Relay Output → Water pump

[ ] Fertilizer LED
    • Positive → GPIO 13 (through 220Ω resistor)
    • Negative → GND

═══════════════════════════════════════════════════════════════════════════════════
3. ESP32 CODE UPLOAD
═══════════════════════════════════════════════════════════════════════════════════

REQUIRED LIBRARIES (Install via Arduino IDE):
──────────────────────────────────────────────

Tools → Manage Libraries → Search & Install:
  ✓ Adafruit BME280 Library (by Adafruit)
  ✓ ArduinoJson (by Benoit Blanchon)

UPLOAD STEPS:
─────────────

1. Open Arduino IDE
2. Sketch → Include Library → Add .ZIP Library
3. Select: esp32_firmware.ino

4. Tools → Board → Select "ESP32 Dev Module"
5. Tools → Port → Select your COM port (check Device Manager)
   - Windows: COM3, COM4, COM5, etc.
   - Mac: /dev/cu.usbserial-*
   - Linux: /dev/ttyUSB0, /dev/ttyACM0, etc.

6. Tools → Upload Speed → 921600

7. Click Upload (or Sketch → Upload)

8. Wait for "Uploading..." → "Done Uploading"
   Expected output in Serial Monitor:
   ╔════════════════════════════════════════════╗
   ║  PS2 - Precision Farming Node (ESP32)     ║
   ║  Initializing Sensors...                  ║
   ╚════════════════════════════════════════════╝
   ✓ BME280 initialized (Temp, Humidity, Pressure)
   ✓ NPK sensor serial port ready...
   ✓ All systems initialized. Starting autonomous mode...

═══════════════════════════════════════════════════════════════════════════════════
4. PYTHON BRIDGE SETUP
═══════════════════════════════════════════════════════════════════════════════════

REQUIREMENTS:
──────────────
• Python 3.7+ (Download from python.org)
• pip (Usually comes with Python)

INSTALLATION:
──────────────

1. Open Terminal/Command Prompt

2. Install required packages:
   pip install pyserial flask flask-cors

3. Navigate to the folder with serial_bridge.py:
   cd C:\path\to\files  (Windows)
   cd /path/to/files    (Mac/Linux)

RUNNING THE BRIDGE:
───────────────────

1. Keep ESP32 connected via USB cable
2. Run: python serial_bridge.py

3. Expected output:
   ==================================================
     PS2 Serial Bridge v1.0
     Local Web Server for ESP32 Dashboard
   ==================================================
   
   🔍 Scanning for ESP32 on available COM ports...
   ✓ Found ESP32 on port: COM3
   ✓ Connected to ESP32 on COM3 at 115200 baud
   ✓ Serial reader thread started
   
   ==================================================
     🌐 Web Server Starting...
   ==================================================
   
   ✓ Dashboard available at: http://localhost:5000
   ✓ API endpoint: http://localhost:5000/api/sensor
   
   Press Ctrl+C to stop the server.

4. Keep this terminal window open (it must stay running)

MANUAL PORT SELECTION:
──────────────────────
If auto-detection fails:
1. Open Device Manager (Windows) or System Report (Mac)
2. Note the COM port your ESP32 is connected to
3. Edit serial_bridge.py, line ~23:
   PORTS_TO_TRY = ['COM3', ...]  ← Add your port to the list

═══════════════════════════════════════════════════════════════════════════════════
5. WEBSITE LAUNCH
═══════════════════════════════════════════════════════════════════════════════════

OPENING THE DASHBOARD:
──────────────────────

Option A (Recommended): Open from File System
  1. Right-click dashboard.html
  2. Select "Open with" → Choose your browser (Chrome, Firefox, Edge)
  3. Bookmark: http://localhost:5000 (if manually navigating)

Option B: Direct URL in Browser
  1. Keep terminal running (serial_bridge.py)
  2. Open any web browser
  3. Navigate to: http://localhost:5000

Option C: Run local server
  If you have Python http.server:
  python -m http.server 8000 (in the folder with dashboard.html)
  Then visit: http://localhost:8000/dashboard.html

EXPECTED INTERFACE:
───────────────────
✓ Header: "PS2 Precision Farming Node"
✓ Decision section: Shows current decision (WATER_NOW, MONITOR, FERTILIZE, WAIT)
✓ Soil Health table: N, P, K values
✓ Plant Health table: Temp, Humidity, Pressure, Light, Moisture
✓ Manual Control buttons: "Force Water" and "Force Fertilize"
✓ Performance Metrics: Uptime, Last Watering, Last Fertilize, Decision Count
✓ Real-time updates: All values refresh every 3 seconds

═══════════════════════════════════════════════════════════════════════════════════
6. TESTING PROCEDURE
═══════════════════════════════════════════════════════════════════════════════════

PHASE 1: HARDWARE CHECK
────────────────────────

[ ] ESP32 Serial Monitor shows sensor data
    Tools → Serial Monitor (set to 115200 baud)
    Should see: ✓ BME280 initialized, ✓ NPK sensor ready, etc.

[ ] All 4 sensors reading values
    Monitor should show JSON output every 3 minutes
    Example: {"sensors": {"moisture": 42.5, "temperature": 24.1, ...}}

[ ] Pump relay responds to GPIO 12
    - Monitor shows "Pump triggered"
    - Relay clicks audibly

[ ] LED turns on for GPIO 13
    - Monitor shows "Fertilizer LED activated"
    - LED lights up

PHASE 2: SERIAL BRIDGE
───────────────────────

[ ] Python bridge auto-detects ESP32 port
    Terminal shows: "✓ Found ESP32 on port: COM3"

[ ] Flask server starts successfully
    Terminal shows: "✓ Dashboard available at: http://localhost:5000"

[ ] Browser can access API
    Open: http://localhost:5000/api/sensor
    Should see JSON with current sensor values

PHASE 3: WEBSITE FUNCTIONALITY
────────────────────────────────

[ ] Dashboard loads without errors
    Browser console (F12) should show no red errors

[ ] Sensor tables show real values
    Soil Health: N, P, K ppm (not dashes)
    Plant Health: Temp, Humidity, etc. (not dashes)

[ ] Decision section shows current action
    "WATER_NOW", "MONITOR", "FERTILIZE", or "WAIT"
    PSI percentage displayed (0-100%)

[ ] Real-time updates working
    Refresh values every 3 seconds without manual refresh

[ ] Master override buttons work
    Click "Force Water" → Pump triggers for 20 seconds
    Click "Force Fertilize" → LED turns on for 5 seconds
    Confirmation message appears on dashboard

[ ] Performance metrics update
    "Last Watering" changes after clicking button
    Uptime increases every second

═══════════════════════════════════════════════════════════════════════════════════
7. TROUBLESHOOTING
═══════════════════════════════════════════════════════════════════════════════════

PROBLEM: Serial Monitor shows nothing
──────────────────────────────────────
Solution:
  1. Check USB cable is properly connected
  2. Verify Board: Tools → Board → "ESP32 Dev Module"
  3. Verify Port: Tools → Port → Select correct COM port
  4. Check Device Manager: ESP32 appears as "USB Serial Device"
  5. Try different USB port on computer
  6. Reinstall CH340 driver (ESP32 USB driver)

PROBLEM: "BME280 not found on I2C"
────────────────────────────────────
Solution:
  1. Check BME280 wiring: SDA=GPIO21, SCL=GPIO22, VCC=3.3V, GND=GND
  2. Verify pull-up resistors (4.7kΩ) on SDA/SCL if needed
  3. Check I2C address (default 0x76, can also be 0x77)
  4. Restart ESP32 (press reset button)

PROBLEM: Python bridge says "Could not find ESP32"
──────────────────────────────────────────────────
Solution:
  1. Open Device Manager (Windows) → Ports (COM & LPT)
  2. Note the COM port your ESP32 uses (e.g., COM3)
  3. Edit serial_bridge.py line 9:
     PORTS_TO_TRY = ['COM3', 'COM4', ...]  ← Add your port
  4. Run again: python serial_bridge.py

PROBLEM: Dashboard shows "Disconnected"
──────────────────────────────────────
Solution:
  1. Ensure serial_bridge.py is still running
  2. Check terminal for errors (red text)
  3. Verify USB cable is connected
  4. Refresh browser (Ctrl+R or Cmd+R)
  5. Restart serial_bridge.py

PROBLEM: Buttons don't trigger pump/LED
────────────────────────────────────────
Solution:
  1. Check GPIO 12 (pump) and GPIO 13 (LED) are properly wired
  2. Verify relay/LED has power
  3. Check Serial Monitor for relay clicks
  4. Confirm button click shows no console errors (F12)
  5. Restart ESP32 and bridge

PROBLEM: Sensor values not updating
────────────────────────────────────
Solution:
  1. Check sensor wiring (especially power and ground)
  2. Verify BME280 I2C connection (wires not loose)
  3. Check moisture sensor on ADC pin A0
  4. Check light sensor on ADC pin A1
  5. Restart ESP32

═══════════════════════════════════════════════════════════════════════════════════
DECISION LOGIC EXPLANATION
═══════════════════════════════════════════════════════════════════════════════════

The system uses a "Plant Stress Index" (PSI) that combines 5 sensors:

PSI = (0.35 × Moisture) + (0.20 × Temperature) + (0.15 × Humidity) 
    + (0.15 × Light) + (0.15 × NPK)

Where each sensor is normalized to 0–1 scale (1 = perfect, 0 = critical)

DECISION RULES:
──────────────
✓ PSI < 0.3               → WATER_NOW (critical stress)
✓ PSI < 0.45 + dry soil   → WATER_NOW (moderate stress)
✓ PSI < 0.45 + dry air    → WAIT (water will evaporate)
✓ PSI < 0.5 + low NPK     → FERTILIZE (nutrient deficiency)
✓ PSI > 0.7 + wet soil    → WAIT (over-saturation)
✓ PSI > 0.75              → MONITOR (excellent health)
✓ Otherwise               → MONITOR

This "harmonic" approach means the system considers all factors together.
Example: High humidity reduces watering urgency even if soil is dry,
because water will evaporate slowly.

═══════════════════════════════════════════════════════════════════════════════════
PERFORMANCE SPECIFICATIONS
═══════════════════════════════════════════════════════════════════════════════════

Update Rates:
  • Sensor reading: Every 5 seconds (internally)
  • Decision logic: Every 3 minutes (180 seconds)
  • Dashboard updates: Every 3 seconds (via AJAX)
  • Serial JSON output: After each decision

Sensor Ranges (Normalized):
  • Moisture:  20% (dry) → 70% (saturated)
  • Temperature: 10°C (cold) → 30°C (hot), optimal 20-25°C
  • Humidity: 30% (dry) → 80% (humid), optimal 50-70%
  • Light: 0 Lux (dark) → 10000 Lux (very bright)
  • NPK: N(100-150), P(30-50), K(150-200) ppm

Actuation:
  • Pump: 20 seconds duration
  • Fertilizer LED: 5 seconds duration
  • Cooldown: Configurable in code

═══════════════════════════════════════════════════════════════════════════════════

Questions? Check the serial monitor output or browser console (F12) for error messages.

Last Updated: 2024-03-31
Version: 1.0 (ESP32 + Python Bridge + Web Dashboard)
