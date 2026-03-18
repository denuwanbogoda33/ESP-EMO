# 🤖 emo robot v10

An ESP32-powered desktop robot with an OLED face that reacts to moods, shows live F1 timing, Spotify now-playing, weather, and a full web dashboard — all controllable over WiFi.

![Platform](https://img.shields.io/badge/platform-ESP32-blue?style=flat-square)
![OLED](https://img.shields.io/badge/display-SSD1306%200.96%22-green?style=flat-square)
![License](https://img.shields.io/badge/license-MIT-orange?style=flat-square)

---

## ✨ Features

- **Animated OLED eyes** — 11 moods (happy, sad, angry, love, excited, sleepy, surprised, curious, proud, scared, idle) with smooth animations and auto blink
- **Live Spotify** — shows currently playing track, artist and progress bar, authorized directly on the ESP32 via OAuth
- **F1 Live timing** — real-time race positions, gaps, lap counter and safety car alerts via OpenF1 API
- **F1 Standings** — top 3 drivers with points, constructor, and next race countdown via Jolpica/Ergast
- **Weather** — temperature, description and wind speed from Open-Meteo (no API key needed)
- **NTP clock** — synced time with day display and blinking indicator
- **Snake game** — playable on the OLED via touch pads
- **10 built-in songs** — Tetris, Mario, Imperial March, Nokia, Birthday, Pacman, Pink Panther, Godfather, Doom, Game of Thrones
- **Touch controls** — 6 capacitive touch pads for page switching, mood cycling, volume, sleep and more
- **WebSocket dashboard** — full browser dashboard (PC + mobile) with live data, mood buttons, message sending, alarms and raw command input
- **OTA updates** — Arduino IDE OTA + web browser OTA upload at `http://IP/update`
- **Burn protection** — periodic display invert to protect the OLED
- **Night mode** — auto sleep between 22:00 and 06:00
- **EEPROM persistence** — volume, brightness and Spotify refresh token survive reboots

---

## 🧰 Hardware

| Component | Details |
|-----------|---------|
| Microcontroller | ESP32 DevKit V1 |
| Display | SSD1306 0.96" OLED (128×64, I2C) |
| Speaker | Passive buzzer / speaker on GPIO 25 (DAC) |
| Touch pads | 6× capacitive touch (T4–T9) |

### Wiring

| OLED | ESP32 |
|------|-------|
| SDA  | GPIO 21 |
| SCL  | GPIO 22 |
| VCC  | 3.3V |
| GND  | GND |

| Component | GPIO |
|-----------|------|
| Speaker (DAC) | 25 |
| Touch T4 | 13 |
| Touch T5 | 12 |
| Touch T6 | 14 |
| Touch T7 | 27 |
| Touch T8 | 33 |
| Touch T9 | 32 |

---

## 📦 Libraries

Install via Arduino Library Manager:

- `Adafruit SSD1306`
- `Adafruit GFX Library`
- `ArduinoJson` (v7)
- `WebSockets` by Markus Sattler (arduinoWebSockets)
- ESP32 Arduino Core (includes WiFi, HTTPClient, ArduinoOTA, WebServer, EEPROM)

---

## ⚙️ Configuration

Edit these defines at the top of `emo_robot_v10.ino`:

```cpp
#define WIFI_SSID        "your_wifi_name"
#define WIFI_PASS        "your_wifi_password"
#define OTA_PASSWORD     "emo"

// Location for weather (default: Matara, Sri Lanka)
#define WX_LAT           "5.9496"
#define WX_LON           "80.5469"
#define TZ_OFFSET_SEC    19800   // UTC+5:30, change to your timezone

// Spotify (see setup below)
#define SPOTIFY_CLIENT_ID     "your_client_id"
#define SPOTIFY_CLIENT_SECRET "your_client_secret"
#define SPOTIFY_REDIRECT_URI  "your_redirect_uri"
```

---

## 🎵 Spotify Setup

1. Go to [developer.spotify.com/dashboard](https://developer.spotify.com/dashboard)
2. Create a free app
3. Add your redirect URI in app settings (e.g. `http://ESP32_IP/callback`)
4. Copy Client ID and Client Secret into the sketch
5. Flash, then open `http://ESP32_IP/spotify` in your browser
6. Click **Login with Spotify** — authorize once
7. Token is saved to EEPROM and auto-refreshes forever

> If your redirect URI is an external page (like a relay page), see `emo_oauth.html` which handles the OAuth code forwarding back to the ESP32.

---

## 🕹️ Touch Controls

| Pad | Tap | Hold |
|-----|-----|------|
| T4 | Next page | Prev page |
| T5 | Cycle mood | Random mood |
| T6 | Play / Pause song | Next song |
| T7 | Volume up | Volume down |
| T8 | Toggle F1 Live page | Sleep / Wake |
| T9 | Random scroll message | Show event |

**In Snake game:** T4=Up, T5=Down, T6=Left, T7=Right

---

## 📺 Pages

| # | Page | Description |
|---|------|-------------|
| 0 | Eyes | Default animated face |
| 1 | Clock | NTP synced time + day |
| 2 | Weather | Temp, description, wind |
| 3 | F1 Standings | Top 3 drivers + next race |
| 4 | F1 Live | Real-time positions + lap |
| 5 | Music | Spotify now playing |
| 6 | Stats | Session counters + uptime |
| 7 | WiFi | IP, RSSI, OTA info |

---

## 🌐 WebSocket Commands

Connect to `ws://ESP32_IP:81` and send JSON:

```json
{"t":"mood","v":"happy"}
{"t":"mood","v":"sad","pin":true}
{"t":"page","v":1}
{"t":"msg","text":"hello world"}
{"t":"notify","text":"alert!","icon":"f1"}
{"t":"song","name":"tetris"}
{"t":"volume","v":7}
{"t":"brightness","v":2}
{"t":"alarm","h":7,"m":30,"label":"Wake up"}
{"t":"event","text":"Meeting 3pm"}
{"t":"game","v":"snake"}
{"t":"stopwatch","action":"start"}
{"t":"countdown","seconds":60}
{"t":"sleep"}
{"t":"wake"}
{"t":"ping"}
```

Available moods: `idle happy excited sad surprised sleepy angry love curious proud scared`

Available songs: `tetris mario imperial nokia birthday pacman pinkpanther godfather doom got`

Notify icons: `f1 music alarm event info`

---

## 🖥️ Web Dashboard

Open `emo_dashboard.html` in any browser (no server needed, just open the file).

- Enter your ESP32's IP and click **CONNECT**
- Controls mood, pages, volume, brightness
- Sends messages and notifications
- Sets alarms and countdowns
- Shows live F1 timing from `wss://f1dash.net/ws`
- Shows F1 standings from Jolpica API (2026)
- Shows weather from Open-Meteo
- Raw JSON command input with presets
- Live WebSocket message log

---

## 🔄 OTA Updates

**Arduino IDE OTA:**
1. After first USB flash, the ESP32 appears as a network port in Tools → Port
2. Select the network port (named `emo`) and upload normally

**Web OTA:**
1. In Arduino IDE: Sketch → Export Compiled Binary
2. Open `http://ESP32_IP/update`
3. Upload the `.bin` file

Password: `emo` (change `OTA_PASSWORD` before flashing)

---

## 📡 Data Sources

| Data | Source | Key needed |
|------|--------|-----------|
| Time | NTP (`pool.ntp.org`) | ❌ |
| Weather | [Open-Meteo](https://open-meteo.com) | ❌ |
| F1 Standings | [Jolpica/Ergast](https://jolpi.ca) | ❌ |
| F1 Live | [OpenF1](https://openf1.org) + [f1dash.net](https://f1dash.net) | ❌ |
| Music | Spotify Web API | ✅ Free dev account |

---

## 📁 Files

```
emo-robot/
├── emo_robot_v10.ino     # Main ESP32 sketch
├── emo_dashboard.html    # Web dashboard (open in browser)
├── emo_oauth.html        # Spotify OAuth relay page
└── README.md
```

---

## 📝 License

MIT — do whatever you want with it, just don't blame me if your robot gets feelings 🤖
