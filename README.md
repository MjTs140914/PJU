# 💡 ESP32 RTC Relay Scheduler

![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![Framework](https://img.shields.io/badge/framework-Arduino-green)
![RTC](https://img.shields.io/badge/RTC-DS3231-orange)
![License](https://img.shields.io/badge/license-MIT-lightgrey)
![Status](https://img.shields.io/badge/status-Stable-brightgreen)

An **automatic schedule-based relay controller** using **ESP32 + DS3231
RTC** with a modern Web UI.

The relay operates automatically based on a defined schedule or can be
controlled manually via a responsive web interface supporting:

-   🌙 Light / Dark theme
-   🔄 RTC synchronization
-   📶 WiFi configuration
-   💾 Persistent storage (NVS)
-   🔗 mDNS hostname access

ESP32 runs in **Access Point (AP) mode**, allowing direct access from
smartphone or PC without an external router.

------------------------------------------------------------------------

# 📌 Features

-   ⏰ DS3231 Real-Time Clock (battery-backed)
-   🕒 Automatic ON/OFF scheduling
-   🔘 Manual override (30-second temporary control)
-   🌐 Async Web Server (non-blocking)
-   📡 WiFi AP configuration page
-   🎨 Light / Dark UI theme
-   🔗 mDNS support (`http://your_address.local`)
-   📜 Relay activity logging (last ON/OFF time)
-   💾 NVS storage (settings survive reboot)

------------------------------------------------------------------------

# ⚡ Hardware Requirements

-   ESP32
-   DS3231 RTC Module
-   1-Channel Relay Module (Active LOW)
-   Jumper wires
-   5V power supply

------------------------------------------------------------------------

# 🔌 ESP32 Pinout

  ESP32 Pin   Function       Description
  ----------- -------------- ---------------------------------------
  GPIO 16     Relay Output   Active LOW → `LOW = ON`, `HIGH = OFF`
  GPIO 21     SDA (I²C)      RTC Data line
  GPIO 22     SCL (I²C)      RTC Clock line
  GND         Ground         Shared ground
  3.3V / 5V   VCC            Power for RTC & relay

------------------------------------------------------------------------

# 🖼 Wiring Diagram

    ESP32        DS3231
    ------       --------
    3.3V/5V  ->  VCC
    GND      ->  GND
    GPIO 21  ->  SDA
    GPIO 22  ->  SCL

    ESP32        Relay Module
    ------       ------------
    GPIO 16  ->  IN
    5V       ->  VCC
    GND      ->  GND

⚠ Relay must be **Active LOW type**.

------------------------------------------------------------------------

# 🌐 Web Access

### Main Page

http://192.168.4.1/ http://your_address.local/

### WiFi Configuration

http://192.168.4.1/config

### Default Credentials

SSID : Your_SSID Password : Your_Password (minimum 8 characters)

------------------------------------------------------------------------

# 📑 API Endpoints

  Endpoint                                    Description
  ------------------------------------------- --------------------------
  `/`                                         Main dashboard
  `/config`                                   AP configuration page
  `/status`                                   JSON status data
  `/on`                                       Manual relay ON
  `/off`                                      Manual relay OFF
  `/reset`                                    Return to schedule mode
  `/set?on=HH:MM&off=HH:MM`                   Set schedule
  `/toggle?state=1/0`                         Enable/disable scheduler
  `/sync?y=YYYY&m=MM&d=DD&h=HH&min=MM&s=SS`   Sync RTC
  `/theme?dark=1/0`                           Change theme

------------------------------------------------------------------------

# 📦 Dependencies

### Required Libraries

-   RTClib
-   AsyncTCP
-   ESPAsyncWebServer

### Included in ESP32 Core

-   Wire.h
-   WiFi.h
-   Preferences.h
-   ESPmDNS.h

------------------------------------------------------------------------

# 🛠 Installation

## Arduino IDE

1.  Install **RTClib** from Library Manager.
2.  Download:
    -   ESPAsyncWebServer
    -   AsyncTCP
3.  Sketch → Include Library → Add .ZIP Library

## PlatformIO

Add to `platformio.ini`:

``` ini
lib_deps =
    adafruit/RTClib
    me-no-dev/AsyncTCP
    me-no-dev/ESPAsyncWebServer
```

------------------------------------------------------------------------

# 📌 Notes

-   Relay defaults to **OFF (HIGH)** on reboot.
-   All settings are stored in ESP32 NVS.
-   DS3231 battery ensures time retention.

------------------------------------------------------------------------

# 📄 License

MIT License

© 2025 MjTs-140914™
