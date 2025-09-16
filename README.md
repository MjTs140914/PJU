# 💡 ESP32 RTC Relay Scheduler

Proyek ini adalah **kontrol relay berbasis jadwal otomatis dengan RTC DS3231** menggunakan **ESP32**.  
Relay dapat dikontrol otomatis sesuai jadwal, atau manual melalui **Web UI interaktif** dengan dukungan **tema gelap/terang**, **sinkronisasi RTC**, dan **konfigurasi WiFi**.  

ESP32 bekerja dalam mode **Access Point (AP)** sehingga bisa diakses langsung dari smartphone/PC tanpa router tambahan.

---

## ⚡ Pinout ESP32

| Pin ESP32 | Fungsi       | Keterangan |
|-----------|--------------|------------|
| **GPIO 16** | Relay Output | Relay aktif **LOW** → `LOW = ON`, `HIGH = OFF`. |
| **GPIO 21** | SDA (I²C)   | Jalur data ke RTC DS3231. |
| **GPIO 22** | SCL (I²C)   | Jalur clock ke RTC DS3231. |
| **GND**    | Ground       | Terhubung ke GND RTC & relay module. |
| **3.3V/5V** | VCC         | Daya untuk RTC & relay (tergantung modul relay). |

---

## 🧩 Fitur Utama

- **RTC DS3231** → menyimpan waktu walau listrik mati.  
- **Jadwal otomatis** → atur jam ON dan OFF (contoh: ON jam 18:00, OFF jam 06:00).  
- **Kontrol manual override** → tombol ON/OFF di web, aktif 30 detik lalu kembali ke mode jadwal.  
- **Web Server (ESPAsyncWebServer)**:  
  - Halaman utama → status relay, RTC, jadwal, kontrol manual, tema.  
  - Halaman konfigurasi WiFi → ubah SSID & password AP.  
- **Penyimpanan NVS (Preferences)** → setting tersimpan walau restart.  
- **Tema UI (light/dark)**.  
- **mDNS** → akses via `http://alamat_anda.local`.  
- **Log aktivitas** → simpan waktu terakhir relay ON dan OFF.  

---

## 🌐 Akses Web

- **Halaman utama:**  
  `http://192.168.4.1/` atau `http://alamat_anda.local/`  

- **Halaman konfigurasi WiFi:**  
  `http://192.168.4.1/config`  

Default:
- **SSID** = `SSID_Anda`  
- **Password** = `Sandi_Anda` (minimal 8 karakter)  

---

## 📑 Endpoint API

| Endpoint | Fungsi |
|----------|--------|
| `/` | Halaman utama (status & kontrol). |
| `/config` | Halaman konfigurasi SSID & password AP. |
| `/status` | JSON status relay, RTC, jadwal, dan riwayat ON/OFF. |
| `/on` | Menyalakan relay (manual override). |
| `/off` | Mematikan relay (manual override). |
| `/reset` | Reset ke mode jadwal. |
| `/set?on=HH:MM&off=HH:MM` | Atur jadwal ON/OFF. |
| `/toggle?state=1/0` | Aktifkan/nonaktifkan jadwal otomatis. |
| `/sync?y=YYYY&m=MM&d=DD&h=HH&min=MM&s=SS` | Sinkronisasi RTC dari waktu HP/PC. |
| `/theme?dark=1/0` | Ganti tema (1 = Gelap, 0 = Terang). |

---

## 📦 Dependencies

Agar kode bisa dikompilasi di **Arduino IDE / PlatformIO**, pastikan library berikut sudah terpasang:  

### ✅ Wajib Install
- [RTClib](https://github.com/adafruit/RTClib) → komunikasi RTC DS3231.  
- [AsyncTCP](https://github.com/me-no-dev/AsyncTCP) → backend TCP untuk ESP32.  
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) → web server non-blocking.  

### 📂 Sudah Bawaan Core ESP32
- **Wire.h** → komunikasi I²C.  
- **WiFi.h** → pengaturan WiFi (AP/STA).  
- **Preferences.h** → penyimpanan NVS.  
- **ESPmDNS.h** → akses hostname via mDNS.  

---

## 🛠️ Cara Install Library

### 1. Via Arduino IDE Library Manager
- Buka **Tools → Manage Libraries...**  
- Cari dan install:  
  - `RTClib` (by Adafruit).  

### 2. Manual dari GitHub
- Download ZIP:  
  - [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)  
  - [AsyncTCP](https://github.com/me-no-dev/AsyncTCP)  
- Arduino IDE → **Sketch → Include Library → Add .ZIP Library...** → pilih file ZIP.  

---

## 📷 Tampilan Web UI
- Status relay (ON/OFF).  
- RTC (waktu real-time).  
- Jadwal ON/OFF (form input jam).  
- Tombol manual ON/OFF.  
- Sinkronisasi RTC.  
- Reset ke mode jadwal.  
- Switch tema (☀️ Terang / 🌙 Gelap).  

---

## 📌 Catatan

- Relay harus tipe **aktif LOW**.  
- RTC DS3231 wajib terhubung ke pin SDA = 21, SCL = 22.  
- Semua konfigurasi (jadwal, WiFi, tema) tersimpan otomatis ke memori NVS ESP32.  
- Saat ESP32 restart → relay default **OFF** (HIGH).  

---

© 2025 MjTs-140914™
