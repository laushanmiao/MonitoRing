# 🔔 MonitoRing: Environment Monitoring System

**MonitoRing** is an innovative, ESP32-powered IoT solution designed to provide comprehensive safety for neglected corners of your home, such as attics and storerooms. It balances environmental monitoring with a humane approach to wildlife management.

---

## 🚀 Overview

Neglected areas are prone to environmental hazards and wildlife intrusion. **MonitoRing** addresses this by monitoring air quality and climate while using microwave radar technology to detect and humanely deter animal intruders using light and sound.

### ✨ Key Features

* **4-in-1 Monitoring**: Real-time tracking of Temperature, Humidity, Gas Levels (PPM), and Motion.
* **Humane Wildlife Deterrence**: Uses a **RCWL-0516** microwave radar sensor to detect living beings and triggers a piezo buzzer and white LED to safely chase them away.
* **Dual Dashboards**:
**Local Web Server**: A sleek, responsive dashboard hosted directly on the ESP32.
**Google Sheets Integration**: Automatically logs sensor data every 5 seconds for long-term history and analysis.
* **Connectivity**: Powered by the **ESP32 Dev Kit V1** using WiFi for remote access.

---

## 🛠️ Hardware Requirements

* 
**Microcontroller**: ESP32 Dev Kit V1 


* 
**Climate Sensor**: DHT11 (Temperature & Humidity) 


* 
**Gas Sensor**: MQ135 (Air Quality/PPM) 


* 
**Motion Sensor**: RCWL-0516 (Microwave Radar) 


* **Indicators**: Piezo Buzzer & White LED

---

## 💻 Software & Setup

### 1. Google Sheets Integration 📊

To log data, you must deploy a Google Apps Script as a web app:

* Create a new Google Sheet and open the **Apps Script** editor.
* Paste the provided `google_apps_script.js` code.
* Deploy as a **Web App** with access set to **"Anyone"**.
* Copy the Deployment URL into the Arduino code.

### 2. Arduino Configuration 🔧

Update the following constants in `arduinocoding.ino`:

* `ssid`: Your WiFi name.


* `password`: Your WiFi password.


* `googleScriptURL`: Your deployed Google Script URL.



---

## 📊 Data Scales

### Gas Quality Index (MQ135)

| Level | PPM Range | Condition |
| --- | --- | --- |
| 1 | 0 - 100 | Excellent |
| 2 | 101 - 200 | Good |
| 3 | 201 - 400 | Moderate |
| 4 | 401 - 600 | Poor |
| 5 | > 600 | Hazardous |

---

## 🌟 Why MonitoRing?

Unlike standard monitors, MonitoRing is specifically designed for **coexistence**. It protects your property from disasters like gas leaks or mold growth while ensuring wildlife is deterred humanely without the use of lethal traps.

**"MonitoRing, For Safe Spaces, and Smiling Faces"**
