#!/usr/bin/env python3
"""
PS2 Serial Bridge
Reads data from ESP32 via USB serial and serves a local HTTP API
for the web dashboard to poll.
"""

import serial
import json
import time
import threading
from flask import Flask, jsonify, request
from flask_cors import CORS
from datetime import datetime
import sys

# ==================== CONFIGURATION ====================
BAUD_RATE = 115200
PORTS_TO_TRY = ['COM3', 'COM4', 'COM5', 'COM6', '/dev/ttyUSB0', '/dev/ttyUSB1', '/dev/ttyACM0', '/dev/ttyACM1']
TIMEOUT = 2

# ==================== GLOBALS ====================
ser = None
current_data = {
    "timestamp": None,
    "sensors": {
        "moisture": 0,
        "temperature": 0,
        "humidity": 0,
        "pressure": 0,
        "light": 0,
        "npk": {"n": 0, "p": 0, "k": 0}
    },
    "decision": {
        "action": "MONITOR",
        "psi": 0,
        "confidence": 0,
        "reasoning": "Initializing..."
    },
    "uptime_seconds": 0
}

pump_triggered = False
fertilizer_triggered = False

# ==================== SERIAL DETECTION ====================
def find_esp32_port():
    """Auto-detect ESP32 serial port"""
    print("🔍 Scanning for ESP32 on available COM ports...")
    
    for port in PORTS_TO_TRY:
        try:
            test_ser = serial.Serial(port, BAUD_RATE, timeout=1)
            time.sleep(0.5)
            test_ser.close()
            print(f"✓ Found ESP32 on port: {port}")
            return port
        except (serial.SerialException, OSError):
            continue
    
    print("❌ Could not find ESP32 on any port!")
    print(f"   Tried ports: {', '.join(PORTS_TO_TRY)}")
    print("   Please check Device Manager and specify port manually in code.")
    return None

# ==================== SERIAL CONNECTION ====================
def connect_serial(port):
    """Connect to ESP32 via serial"""
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=TIMEOUT)
        print(f"✓ Connected to ESP32 on {port} at {BAUD_RATE} baud")
        return ser
    except serial.SerialException as e:
        print(f"❌ Error connecting to {port}: {e}")
        return None

def read_serial_data():
    """Read JSON data from ESP32 continuously"""
    global ser, current_data
    
    buffer = ""
    while True:
        try:
            if ser and ser.in_waiting > 0:
                data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                buffer += data
                
                # Look for complete JSON objects (ending with '}\n')
                while '\n' in buffer:
                    line = buffer[:buffer.index('\n')].strip()
                    buffer = buffer[buffer.index('\n') + 1:]
                    
                    # Try to parse JSON
                    if line.startswith('{'):
                        try:
                            parsed = json.loads(line)
                            current_data = parsed
                            print(f"✓ Data updated - Decision: {parsed['decision']['action']} (PSI: {parsed['decision']['psi']:.2f})")
                        except json.JSONDecodeError:
                            pass
        except Exception as e:
            print(f"⚠️  Serial read error: {e}")
        
        time.sleep(0.1)

# ==================== FLASK WEB SERVER ====================
app = Flask(__name__)
CORS(app)  # Enable CORS for all routes

@app.route('/api/sensor', methods=['GET'])
def get_sensor_data():
    """
    API endpoint to get current sensor data and decision
    Called by web dashboard every 3 seconds
    """
    return jsonify(current_data)

@app.route('/water', methods=['POST'])
def trigger_water():
    """
    Trigger water pump
    Duration: ?duration=20 (seconds)
    """
    global pump_triggered
    
    duration = request.args.get('duration', 20, type=int)
    
    if ser and ser.is_open:
        # Send command to ESP32 to trigger pump
        command = f"PUMP:{duration}\n"
        ser.write(command.encode())
        pump_triggered = True
        print(f"💧 Water pump triggered for {duration} seconds")
        return jsonify({"status": "ok", "message": f"Pump activated for {duration} seconds"})
    else:
        return jsonify({"status": "error", "message": "Serial connection lost"}), 500

@app.route('/fertilize', methods=['POST'])
def trigger_fertilize():
    """
    Trigger fertilizer LED (5 seconds)
    """
    global fertilizer_triggered
    
    if ser and ser.is_open:
        # Send command to ESP32 to trigger fertilizer
        command = "FERTILIZE:5\n"
        ser.write(command.encode())
        fertilizer_triggered = True
        print("🌱 Fertilizer LED triggered for 5 seconds")
        return jsonify({"status": "ok", "message": "Fertilizer activated for 5 seconds"})
    else:
        return jsonify({"status": "error", "message": "Serial connection lost"}), 500

@app.route('/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    return jsonify({
        "status": "ok",
        "serial_connected": ser is not None and ser.is_open,
        "timestamp": datetime.now().isoformat()
    })

# ==================== STARTUP ====================
def main():
    global ser
    
    print("\n" + "="*50)
    print("  PS2 Serial Bridge v1.0")
    print("  Local Web Server for ESP32 Dashboard")
    print("="*50 + "\n")
    
    # Find and connect to ESP32
    port = find_esp32_port()
    if not port:
        print("\n⚠️  Manual port entry:")
        port = input("Enter COM port (e.g., COM3 or /dev/ttyUSB0): ").strip()
    
    ser = connect_serial(port)
    if not ser:
        print("\n❌ Failed to connect. Exiting.")
        sys.exit(1)
    
    # Start serial reader thread
    serial_thread = threading.Thread(target=read_serial_data, daemon=True)
    serial_thread.start()
    print("✓ Serial reader thread started\n")
    
    # Wait a moment for first data
    print("⏳ Waiting for first data from ESP32...")
    time.sleep(2)
    
    # Start Flask web server
    print("\n" + "="*50)
    print("  🌐 Web Server Starting...")
    print("="*50)
    print("\n✓ Dashboard available at: http://localhost:5000")
    print("✓ API endpoint: http://localhost:5000/api/sensor")
    print("\nOpen dashboard.html in your browser (File → Open)")
    print("Make sure to update the API_BASE in dashboard.html if using different port.\n")
    print("Press Ctrl+C to stop the server.\n")
    
    try:
        app.run(host='localhost', port=5000, debug=False, use_reloader=False)
    except KeyboardInterrupt:
        print("\n\n👋 Shutting down...")
        if ser:
            ser.close()
        print("✓ Serial connection closed")
        sys.exit(0)

if __name__ == '__main__':
    main()
