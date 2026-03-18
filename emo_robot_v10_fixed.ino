/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║      EMO ROBOT v10  ·  ESP32 DevKit V1 + SSD1306 0.96"      ║
 * ╠══════════════════════════════════════════════════════════════╣
 * ║  WIRING:                                                     ║
 * ║   OLED SDA  → GPIO 21    OLED SCL  → GPIO 22                ║
 * ║   Speaker   → GPIO 25 (DAC)                                  ║
 * ║   Touch T4  → GPIO 13   Touch T5  → GPIO 12                 ║
 * ║   Touch T6  → GPIO 14   Touch T7  → GPIO 27                 ║
 * ║   Touch T8  → GPIO 33   Touch T9  → GPIO 32                 ║
 * ╠══════════════════════════════════════════════════════════════╣
 * ║  LIBRARIES (install via Arduino Library Manager):           ║
 * ║   • Adafruit SSD1306                                        ║
 * ║   • Adafruit GFX Library                                    ║
 * ║   • ArduinoJson  (v7)                                       ║
 * ║   • WebSockets by Markus Sattler (arduinoWebSockets)        ║
 * ║   • ESP32 Arduino Core (built-in WiFi, HTTPClient,          ║
 * ║     ArduinoOTA, WebServer, EEPROM, touch)                   ║
 * ╠══════════════════════════════════════════════════════════════╣
 * ║  TOUCH CONTROLS:                                            ║
 * ║   T4  tap = next page     hold = prev page                  ║
 * ║   T5  tap = cycle mood    hold = random mood                ║
 * ║   T6  tap = play/pause    hold = next song                  ║
 * ║   T7  tap = vol up        hold = vol down                   ║
 * ║   T8  tap = F1 mode       hold = sleep/wake                 ║
 * ║   T9  tap = scroll msg    hold = notify                     ║
 * ║   (In Snake game: T4=up T5=down T6=left T7=right)           ║
 * ╠══════════════════════════════════════════════════════════════╣
 * ║  PAGES (cycle with T4):                                     ║
 * ║   0=Eyes  1=Clock  2=Weather  3=F1 Standings                ║
 * ║   4=F1 Live  5=Music  6=Stats  7=WiFi/OTA                   ║
 * ╠══════════════════════════════════════════════════════════════╣
 * ║  REAL DATA SOURCES:                                         ║
 * ║   • Time    : NTP (pool.ntp.org)                            ║
 * ║   • Weather : Open-Meteo API (free, no key)                 ║
 * ║   • F1 Standings: Jolpica/Ergast API (free, no key)         ║
 * ║   • F1 Live : OpenF1 API (free, no key)                     ║
 * ║   • Music   : Spotify (OAuth direct on ESP32)               ║
 * ╠══════════════════════════════════════════════════════════════╣
 * ║  OTA UPDATE:                                                ║
 * ║   Arduino IDE OTA  → hostname "emo-robot"                   ║
 * ║   Web OTA          → http://ESP32_IP/update                 ║
 * ║   Spotify login    → http://ESP32_IP/spotify                ║
 * ╠══════════════════════════════════════════════════════════════╣
 * ║  WS COMMANDS (send JSON to ws://ESP32_IP:81):               ║
 * ║   {"t":"mood","v":"happy"}                                  ║
 * ║   {"t":"page","v":1}                                        ║
 * ║   {"t":"msg","text":"hello"}                                ║
 * ║   {"t":"notify","text":"alert!","icon":"f1"}                ║
 * ║   {"t":"song","name":"tetris"}                              ║
 * ║   {"t":"music","title":"...","artist":"...","playing":1}    ║
 * ║   {"t":"alarm","h":7,"m":30,"label":"Wake up"}              ║
 * ║   {"t":"event","text":"Meeting 3pm"}                        ║
 * ║   {"t":"volume","v":7}                                      ║
 * ║   {"t":"brightness","v":2}                                  ║
 * ║   {"t":"game","v":"snake"}                                  ║
 * ║   {"t":"stopwatch","action":"start"}                        ║
 * ║   {"t":"countdown","seconds":60}                            ║
 * ║   {"t":"ping"}                                              ║
 * ╚══════════════════════════════════════════════════════════════╝
 */

// ══════════════════════════════════════════════════════════════
//  INCLUDES
// ══════════════════════════════════════════════════════════════
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebSocketsServer.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <EEPROM.h>
#include <time.h>
#include <math.h>

// ══════════════════════════════════════════════════════════════
//  !! EDIT THESE !!
// ══════════════════════════════════════════════════════════════
#define WIFI_SSID        "your wifi ssid"
#define WIFI_PASS        "pass wifi password"
#define OTA_PASSWORD     "emo"
#define OTA_HOSTNAME     "emo"

// NTP servers  ← FIX #1: were undefined, caused configTime() error
#define NTP1  "pool.ntp.org"
#define NTP2  "time.nist.gov"

// Location (Matara, Sri Lanka — change to yours)
#define WX_LAT           "5.9496"
#define WX_LON           "80.5469"
#define TZ_OFFSET_SEC    19800             // UTC+5:30 Sri Lanka

// Spotify — create a free app at https://developer.spotify.com/dashboard
#define SPOTIFY_CLIENT_ID     ""
#define SPOTIFY_CLIENT_SECRET ""
#define SPOTIFY_REDIRECT_URI  ""
// ══════════════════════════════════════════════════════════════

// ── HARDWARE PINS ─────────────────────────────────────────────
#define I2C_SDA       21
#define I2C_SCL       22
#define PIN_DAC       25
#define TOUCH_PIN_T4  13
#define TOUCH_PIN_T5  12
#define TOUCH_PIN_T6  14
#define TOUCH_PIN_T7  27
#define TOUCH_PIN_T8  33
#define TOUCH_PIN_T9  32
#define TOUCH_THRESH  40
#define TOUCH_HOLD_MS 800
#define TOUCH_DEBOUNCE_MS 600
#define NUM_TOUCH_PINS 6

// ── OLED ──────────────────────────────────────────────────────
#define SCREEN_W   128
#define SCREEN_H    64
#define OLED_ADDR  0x3C
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// ── SERVERS ───────────────────────────────────────────────────
WebSocketsServer wsServer(81);
WebServer        otaServer(80);
uint8_t          wsClient = 255;

// ── EEPROM ────────────────────────────────────────────────────
#define EEPROM_SIZE  512
#define EE_VOL         0
#define EE_BRIGHT      1
#define EE_SPOTIFY_TOK 2        // 256 bytes — refresh token string

// ── PAGES ─────────────────────────────────────────────────────
#define PAGE_EYES    0
#define PAGE_CLOCK   1
#define PAGE_WEATHER 2
#define PAGE_F1      3
#define PAGE_F1LIVE  4
#define PAGE_MUSIC   5
#define PAGE_STATS   6
#define PAGE_WIFI    7
#define NUM_PAGES    8
int  currentPage  = PAGE_EYES;
bool pageUserSet  = false;

// ── MOODS ─────────────────────────────────────────────────────
enum Mood {
  MOOD_IDLE=0, MOOD_HAPPY, MOOD_EXCITED, MOOD_SAD,
  MOOD_SURPRISED, MOOD_SLEEPY, MOOD_ANGRY, MOOD_LOVE,
  MOOD_CURIOUS, MOOD_PROUD, MOOD_SCARED
};
Mood          currentMood  = MOOD_IDLE;
bool          moodActive   = false;
bool          moodPinned   = false;
unsigned long moodEndMs    = 0;

// ── EYE GEOMETRY ──────────────────────────────────────────────
#define EYE_W  40
#define EYE_H  40
#define EYE_R  10
#define EYE_GAP 10
int lx, ly, rx, ry;
int lookDx = 0, lookDy = 0;

// blink
bool          blinkActive = false;
int           blinkH      = EYE_H;
int           blinkDir    = -1;
unsigned long blinkMs     = 0;
#define BLINK_STEP_MS  18
#define BLINK_IV_MS    4500

// ── GLOBAL TIMER ──────────────────────────────────────────────
unsigned long nowMs = 0;

// ── SLEEP ─────────────────────────────────────────────────────
bool sleepMode = false;

// ── VOLUME ────────────────────────────────────────────────────
uint8_t volume = 7;  // 0-10

// ── BEEP QUEUE ────────────────────────────────────────────────
struct BeepNote { int freq; int dur; };
BeepNote bq[16];
int bqLen=0, bqIdx=0;
unsigned long bqNextMs=0;

// ── SONG ENGINE ───────────────────────────────────────────────
#define REST 0
const int*    songPtr    = nullptr;
int           songLen    = 0;
int           songBpm    = 0;
int           songIdx    = 0;
bool          songActive = false;
unsigned long songNextMs = 0;
int           songCurrent = 0;
#define NUM_SONGS 10

// ── NOTE DEFINES ──────────────────────────────────────────────
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_A6  1760
#define NOTE_C7  2093
#define NOTE_E7  2637
#define NOTE_G7  3136

// ── SONG DATA (PROGMEM) ───────────────────────────────────────
static const int PROGMEM sng_tetris[] = {
  NOTE_E5,4,NOTE_B4,8,NOTE_C5,8,NOTE_D5,4,NOTE_C5,8,NOTE_B4,8,
  NOTE_A4,4,NOTE_A4,8,NOTE_C5,8,NOTE_E5,4,NOTE_D5,8,NOTE_C5,8,
  NOTE_B4,-4,NOTE_C5,8,NOTE_D5,4,NOTE_E5,4,NOTE_C5,4,NOTE_A4,4,NOTE_A4,8,REST,8,
  REST,4,NOTE_D5,-4,NOTE_F5,8,NOTE_A5,4,NOTE_G5,8,NOTE_F5,8,
  NOTE_E5,-4,NOTE_C5,8,NOTE_E5,4,NOTE_D5,8,NOTE_C5,8,NOTE_B4,4,NOTE_B4,8,NOTE_C5,8,
  NOTE_D5,4,NOTE_E5,4,NOTE_C5,4,NOTE_A4,4,NOTE_A4,4,REST,4
};
#define TETRIS_LEN (sizeof(sng_tetris)/sizeof(int)/2)

static const int PROGMEM sng_mario[] = {
  NOTE_E5,8,NOTE_E5,8,REST,8,NOTE_E5,8,REST,8,NOTE_C5,8,NOTE_E5,8,REST,8,
  NOTE_G5,4,REST,4,NOTE_G4,4,REST,4,
  NOTE_C5,-4,NOTE_G4,8,REST,4,NOTE_E4,-4,
  NOTE_A4,4,NOTE_B4,4,NOTE_AS4,8,NOTE_A4,4,
  NOTE_G4,-8,NOTE_E5,-8,NOTE_G5,-8,NOTE_A5,4,NOTE_F5,8,NOTE_G5,8,
  REST,8,NOTE_E5,4,NOTE_C5,8,NOTE_D5,8,NOTE_B4,-4
};
#define MARIO_LEN (sizeof(sng_mario)/sizeof(int)/2)

static const int PROGMEM sng_imperial[] = {
  NOTE_A4,4,NOTE_A4,4,NOTE_A4,4,NOTE_F4,-8,NOTE_C5,16,
  NOTE_A4,4,NOTE_F4,-8,NOTE_C5,16,NOTE_A4,2,
  NOTE_E5,4,NOTE_E5,4,NOTE_E5,4,NOTE_F5,-8,NOTE_C5,16,
  NOTE_A4,4,NOTE_F4,-8,NOTE_C5,16,NOTE_A4,2
};
#define IMPERIAL_LEN (sizeof(sng_imperial)/sizeof(int)/2)

static const int PROGMEM sng_nokia[] = {
  NOTE_E5,8,NOTE_D5,8,NOTE_FS4,4,NOTE_GS4,4,
  NOTE_CS5,8,NOTE_B4,8,NOTE_D4,4,NOTE_E4,4,
  NOTE_B4,8,NOTE_A4,8,NOTE_CS4,4,NOTE_E4,4,NOTE_A4,2
};
#define NOKIA_LEN (sizeof(sng_nokia)/sizeof(int)/2)

static const int PROGMEM sng_birthday[] = {
  NOTE_C4,4,NOTE_C4,8,NOTE_D4,4,NOTE_C4,4,NOTE_F4,4,NOTE_E4,2,
  NOTE_C4,4,NOTE_C4,8,NOTE_D4,4,NOTE_C4,4,NOTE_G4,4,NOTE_F4,2,
  NOTE_C4,4,NOTE_C4,8,NOTE_C5,4,NOTE_A4,4,NOTE_F4,4,NOTE_E4,4,NOTE_D4,2,
  NOTE_AS4,4,NOTE_AS4,8,NOTE_A4,4,NOTE_F4,4,NOTE_G4,4,NOTE_F4,2
};
#define BIRTHDAY_LEN (sizeof(sng_birthday)/sizeof(int)/2)

static const int PROGMEM sng_pacman[] = {
  NOTE_B4,16,NOTE_B5,16,NOTE_FS5,16,NOTE_DS5,16,
  NOTE_B5,32,NOTE_FS5,-16,NOTE_DS5,8,NOTE_C5,16,
  NOTE_C6,16,NOTE_G6,16,NOTE_E6,16,NOTE_C6,32,NOTE_G6,-16,NOTE_E6,8
};
#define PACMAN_LEN (sizeof(sng_pacman)/sizeof(int)/2)

static const int PROGMEM sng_pink[] = {
  REST,4,NOTE_DS4,8,NOTE_E4,-4,REST,8,NOTE_FS4,8,NOTE_G4,-4,
  REST,8,NOTE_DS4,8,NOTE_E4,-8,NOTE_FS4,8,
  NOTE_B4,8,NOTE_D5,8,NOTE_C5,-4,NOTE_AS4,8,NOTE_A4,4,REST,4
};
#define PINK_LEN (sizeof(sng_pink)/sizeof(int)/2)

static const int PROGMEM sng_godfather[] = {
  NOTE_A4,8,NOTE_A4,4,NOTE_A4,8,NOTE_F4,8,NOTE_A4,4,
  NOTE_A4,8,NOTE_A4,8,NOTE_AS4,8,NOTE_A4,8,NOTE_G4,2
};
#define GODFATHER_LEN (sizeof(sng_godfather)/sizeof(int)/2)

static const int PROGMEM sng_doom[] = {
  NOTE_E3,8,NOTE_E3,8,NOTE_E4,8,NOTE_E3,8,NOTE_E3,8,NOTE_D4,8,
  NOTE_E3,8,NOTE_E3,8,NOTE_C4,8,NOTE_E3,8,NOTE_E3,8,NOTE_AS3,8
};
#define DOOM_LEN (sizeof(sng_doom)/sizeof(int)/2)

static const int PROGMEM sng_got[] = {
  NOTE_G4,8,NOTE_C4,4,NOTE_DS4,8,NOTE_F4,8,NOTE_G4,8,NOTE_C4,4,
  NOTE_DS4,8,NOTE_F4,4,NOTE_D4,8,NOTE_F4,4,NOTE_AS3,8,NOTE_C4,4
};
#define GOT_LEN (sizeof(sng_got)/sizeof(int)/2)

struct SongEntry { const int* data; int len; int bpm; const char* name; };
const SongEntry SONGS[NUM_SONGS] = {
  {sng_tetris,   TETRIS_LEN,   144, "tetris"},
  {sng_mario,    MARIO_LEN,    200, "mario"},
  {sng_imperial, IMPERIAL_LEN, 120, "imperial"},
  {sng_nokia,    NOKIA_LEN,    225, "nokia"},
  {sng_birthday, BIRTHDAY_LEN, 140, "birthday"},
  {sng_pacman,   PACMAN_LEN,   105, "pacman"},
  {sng_pink,     PINK_LEN,     120, "pinkpanther"},
  {sng_godfather,GODFATHER_LEN, 80, "godfather"},
  {sng_doom,     DOOM_LEN,     160, "doom"},
  {sng_got,      GOT_LEN,       85, "got"}
};

// ── SCROLL / NOTIFY ───────────────────────────────────────────
char  scrollBuf[64] = "";
bool  scrollActive  = false;
int   scrollX       = SCREEN_W;
unsigned long scrollMs = 0;
#define SCROLL_MS 28

char  notifyBuf[32] = "";
char  notifyIcon[8] = "";
bool  notifyActive  = false;
unsigned long notifyEndMs = 0;

// ── TOUCH ─────────────────────────────────────────────────────
const uint8_t TOUCH_GPIOS[NUM_TOUCH_PINS] = {
  TOUCH_PIN_T4, TOUCH_PIN_T5, TOUCH_PIN_T6,
  TOUCH_PIN_T7, TOUCH_PIN_T8, TOUCH_PIN_T9
};
struct TPState {
  bool     active;
  bool     holdFired;
  unsigned long pressMs;
  unsigned long lastMs;
};
TPState tp[NUM_TOUCH_PINS] = {};

// ── IDLE ANIMATION ────────────────────────────────────────────
unsigned long lastIdleMs  = 0;
unsigned long lastBlinkMs = 0;
unsigned long lastPageMs  = 0;
#define IDLE_IV   15000
#define PAGE_IV   10000

// ── BURN PROTECTION ───────────────────────────────────────────
unsigned long lastBurnMs   = 0;
bool          burnInvertOn = false;
unsigned long burnInvertMs = 0;
#define BURN_IV (3UL*60*1000)

// ── RANDOM MESSAGES ───────────────────────────────────────────
const char* const RAND_MSGS[] = {
  "BOX BOX BOX!","Beep boop :)","I love circuits","Low on snacks",
  "Are we human?","404: feelings","Recharging...","[happy noises]",
  "Vroom vroom!","Debugging life","*robot noises*","Stay curious!",
  "I see you :)","Calculating...","Emoji loading","Need coffee!"
};
#define NUM_RAND_MSGS 16
unsigned long lastRandMsgMs = 0;
#define RAND_MSG_IV (5UL*60*1000)

// ── SESSION STATS ─────────────────────────────────────────────
int statMoods=0, statMsgs=0, statNotifs=0;

// ── SNAKE GAME ────────────────────────────────────────────────
enum GameState { GAME_OFF, GAME_RUNNING, GAME_OVER };
GameState gameState = GAME_OFF;
#define SN_COLS 16
#define SN_ROWS  8
#define SN_CW   (SCREEN_W/SN_COLS)
#define SN_CH   (SCREEN_H/SN_ROWS)
struct SCell { int8_t x,y; };
SCell snBody[64];
int8_t snLen=4, snDx=1, snDy=0;
int8_t snFx=8, snFy=4;
int snScore=0;
unsigned long snMoveMs=0;
#define SN_SPEED 200

// ── ALARM / EVENT ─────────────────────────────────────────────
struct Alarm { int h,m; bool armed; char label[20]; };
Alarm alarms[4];
int   alarmCount = 0;

struct Event { char text[32]; bool active; };
Event events[4];
int   eventCount = 0;

// ── STOPWATCH / COUNTDOWN ─────────────────────────────────────
bool          swRunning   = false;
unsigned long swStartMs   = 0;
unsigned long swElapsed   = 0;
bool          cdActive    = false;
unsigned long cdEndMs     = 0;

// ── FETCH TIMERS ──────────────────────────────────────────────
unsigned long lastTimeTick    = 0;
unsigned long lastWxFetch     = 0;
unsigned long lastF1Fetch     = 0;
unsigned long lastMusicFetch  = 0;
unsigned long lastF1LiveFetch = 0;
#define TIME_IV      1000
#define WX_IV        (10UL*60*1000)
#define F1_IV        (30UL*60*1000)
#define MUSIC_IV     (15UL*1000)
#define F1LIVE_IV    (8UL*1000)

// ── DATA STRUCTS ──────────────────────────────────────────────
struct TimeData {
  int h=12,m=0,s=0; char day[4]="---"; bool valid=false;
} tdata;

struct WeatherData {
  float temp=0,wind=0; int code=0; char desc[20]="---"; bool valid=false;
} wdata;

struct F1StandData {
  char p1[8]="---",p1pts[12]="---",p1team[20]="---";
  char p2[8]="---",p2pts[12]="---";
  char p3[8]="---",p3pts[12]="---";
  int  nextDays=0,nextHrs=0; char nextName[28]="---";
  bool valid=false, nextValid=false;
} f1stand;

struct F1LiveData {
  char p1[8]="---",p2[8]="---",gap[16]="---";
  int  lap=0,total=0; bool sc=false; bool valid=false;
  char session[20]="---"; char circuit[16]="---";
} f1live;

struct MusicData {
  char title[32]="---"; char artist[24]="---";
  bool playing=false; bool valid=false;
  int  progress=0;
} mdata;

// ── SPOTIFY STATE ─────────────────────────────────────────────
char spAccessToken[256]  = "";
char spRefreshToken[256] = "";
unsigned long spTokenExpiresMs = 0;
bool spAuthorized = false;

// ── SERIAL BUFFER ─────────────────────────────────────────────
char sbuf[512]; int sbufLen=0;

// ── OPENF1 SESSION ────────────────────────────────────────────
int  of1SessionKey = 0;
bool of1HasSession = false;

// ══════════════════════════════════════════════════════════════
//  FORWARD DECLARATIONS
// ══════════════════════════════════════════════════════════════
void wsSend(const char* msg);
void parseJSON(char* buf);
void resetEyes();
void drawBase();
void showEyes();
void applyMood(Mood m, bool snd=true, bool pin=false);
void startScroll(const char* txt);
void showNotify(const char* txt, const char* icon);
void showPage();
void setupOTA();
void fetchWeather();
void fetchF1Standings();
void fetchOpenF1Session();
void fetchOpenF1Live();
void fetchSpotify();
bool spExchangeToken(const char* codeOrRefresh, int mode);
void spSaveRefreshToken();
void spLoadRefreshToken();
void setupSpotifyRoutes();
void pageClock();
void pageWeather();
void pageF1();
void pageF1Live();
void pageMusic();
void pageStats();
void pageWifi();

// ══════════════════════════════════════════════════════════════
//  BASE64 HELPER (for Spotify auth header)
// ══════════════════════════════════════════════════════════════
static const char b64chars[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

String b64Encode(const uint8_t* data, size_t len) {
  String out;
  for(size_t i=0; i<len; i+=3){
    uint8_t a=data[i];
    uint8_t b=(i+1<len)?data[i+1]:0;
    uint8_t c=(i+2<len)?data[i+2]:0;
    out+=(char)b64chars[a>>2];
    out+=(char)b64chars[((a&3)<<4)|(b>>4)];
    out+=(i+1<len)?(char)b64chars[((b&0xf)<<2)|(c>>6)]:'=';
    out+=(i+2<len)?(char)b64chars[c&0x3f]:'=';
  }
  return out;
}

String spCredB64() {
  char creds[128];
  snprintf(creds,sizeof(creds),"%s:%s",SPOTIFY_CLIENT_ID,SPOTIFY_CLIENT_SECRET);
  return b64Encode((const uint8_t*)creds, strlen(creds));
}

// ══════════════════════════════════════════════════════════════
//  EEPROM
// ══════════════════════════════════════════════════════════════
void eepromLoad() {
  EEPROM.begin(EEPROM_SIZE);
  uint8_t v = EEPROM.read(EE_VOL);
  uint8_t b = EEPROM.read(EE_BRIGHT);
  volume = (v<=10) ? v : 7;
  if(b>=1&&b<=4) display.dim(b<=2);
  spLoadRefreshToken();   // FIX #2: was wrongly written as spRefreshToken()
}

void eepromSave() {
  EEPROM.write(EE_VOL, volume);
  EEPROM.commit();
}

// ══════════════════════════════════════════════════════════════
//  WEBSOCKET
// ══════════════════════════════════════════════════════════════
void wsSend(const char* msg) {
  if(wsClient!=255) wsServer.sendTXT(wsClient, msg);
}

// FIX #3: closing brace was missing from wsEvent — added below
void wsEvent(uint8_t n, WStype_t t, uint8_t* p, size_t l) {
  if(t==WStype_CONNECTED)    { wsClient=n; wsSend("{\"s\":\"ok\",\"v\":10}"); }
  if(t==WStype_DISCONNECTED) { if(n==wsClient) wsClient=255; }
  if(t==WStype_TEXT && l>0 && l<511) {
    char buf[512]; memcpy(buf,p,l); buf[l]='\0'; parseJSON(buf);
  }
}   // ← FIX #3: this brace was missing in the original

// ══════════════════════════════════════════════════════════════
//  BEEP QUEUE
// ══════════════════════════════════════════════════════════════
void qBeeps(BeepNote* n, int cnt) {
  if(volume==0) return;
  bqLen=min(cnt,16); bqIdx=0;
  for(int i=0;i<bqLen;i++) bq[i]=n[i];
  bqNextMs=0;
}
void tickBeeps() {
  if(bqIdx>=bqLen || nowMs<bqNextMs) return;
  if(bq[bqIdx].freq>0 && volume>0) tone(PIN_DAC,bq[bqIdx].freq,bq[bqIdx].dur);
  bqNextMs = nowMs + bq[bqIdx].dur + 28;
  bqIdx++;
}

// ── SFX ───────────────────────────────────────────────────────
void sndBoot()      { BeepNote n[]={{440,70},{554,70},{659,70},{880,70},{1108,150}}; qBeeps(n,5); }
void sndWifiOk()    { BeepNote n[]={{800,60},{1000,60},{1200,60},{1600,150}}; qBeeps(n,4); }
void sndWifiFail()  { BeepNote n[]={{800,100},{600,100},{400,200}}; qBeeps(n,3); }
void sndHappy()     { BeepNote n[]={{1200,65},{1600,65},{2000,100}}; qBeeps(n,3); }
void sndExcited()   { BeepNote n[]={{1800,50},{2100,50},{2400,50},{2800,100}}; qBeeps(n,4); }
void sndSad()       { BeepNote n[]={{800,150},{600,150},{400,250}}; qBeeps(n,3); }
void sndSurprised() { BeepNote n[]={{600,40},{2600,150}}; qBeeps(n,2); }
void sndLove()      { BeepNote n[]={{1046,80},{1318,80},{1568,80},{1318,80},{1046,180}}; qBeeps(n,5); }
void sndAngry()     { BeepNote n[]={{250,120},{200,120},{160,120},{130,200}}; qBeeps(n,4); }
void sndSleepy()    { BeepNote n[]={{800,100},{600,150},{400,200},{300,300}}; qBeeps(n,4); }
void sndWake()      { BeepNote n[]={{400,80},{600,80},{900,80},{1200,120}}; qBeeps(n,4); }
void sndCurious()   { BeepNote n[]={{800,60},{900,60},{1050,60},{1200,150}}; qBeeps(n,4); }
void sndProud()     { BeepNote n[]={{784,80},{988,80},{1175,80},{1568,200},{0,60},{1568,100}}; qBeeps(n,6); }
void sndScared()    { BeepNote n[]={{2800,30},{0,20},{2800,30},{0,20},{2800,30},{0,20},{2200,80},{1600,150}}; qBeeps(n,8); }
void sndNotify()    { BeepNote n[]={{1318,80},{1568,80},{1760,80},{2093,150}}; qBeeps(n,4); }
void sndMsg()       { BeepNote n[]={{880,60},{1100,90}}; qBeeps(n,2); }
void sndAlarm()     { BeepNote n[]={{1000,100},{0,50},{1000,100},{0,50},{1000,100},{0,50},{1500,300}}; qBeeps(n,7); }
void sndF1()        { BeepNote n[]={{1000,80},{0,200},{1000,80},{0,200},{1000,80},{0,200},{1000,80},{0,200},{1000,400}}; qBeeps(n,9); }
void sndPage()      { BeepNote n[]={{1760,35},{2200,55}}; qBeeps(n,2); }
void sndClick()     { if(volume>0) tone(PIN_DAC,2800,20); }
void sndVolUp()     { BeepNote n[]={{1500,40},{2000,60}}; qBeeps(n,2); }
void sndVolDown()   { BeepNote n[]={{2000,40},{1500,60}}; qBeeps(n,2); }
void sndGameOver()  { BeepNote n[]={{400,200},{300,200},{200,400}}; qBeeps(n,3); }
void sndGameEat()   { BeepNote n[]={{1800,30},{2200,40}}; qBeeps(n,2); }

// ══════════════════════════════════════════════════════════════
//  SONG ENGINE
// ══════════════════════════════════════════════════════════════
void songPlay(int idx) {
  if(volume==0||idx<0||idx>=NUM_SONGS) return;
  noTone(PIN_DAC);
  songPtr=SONGS[idx].data; songLen=SONGS[idx].len;
  songBpm=SONGS[idx].bpm; songIdx=0; songNextMs=0;
  songActive=true; songCurrent=idx;
}
void songStop() { songActive=false; noTone(PIN_DAC); }
void tickSong() {
  if(!songActive||!songPtr||nowMs<songNextMs) return;
  if(songIdx>=songLen){ songStop(); return; }
  int freq = (int)(int16_t)pgm_read_word(&songPtr[songIdx*2]);
  int div  = (int)(int16_t)pgm_read_word(&songPtr[songIdx*2+1]);
  if(div==0) div=4;
  int whole = (60000/songBpm)*4;
  int dur   = (div>0) ? whole/div : (int)(whole/(-div)*1.5f);  // FIX #4: abs() on negative div was wrong
  if(dur<=0) dur=100;
  if(freq!=REST&&volume>0) tone(PIN_DAC,freq,(int)(dur*0.9f));
  else noTone(PIN_DAC);
  songNextMs = nowMs + (unsigned long)(dur*1.3f);
  songIdx++;
}
void songByName(const char* name) {
  for(int i=0;i<NUM_SONGS;i++)
    if(strcmp(name,SONGS[i].name)==0){ songPlay(i); return; }
  if(strcmp(name,"stop")==0) songStop();
}

// ══════════════════════════════════════════════════════════════
//  EYE HELPERS
// ══════════════════════════════════════════════════════════════
void resetEyes() {
  lx=SCREEN_W/2-EYE_W/2-EYE_GAP/2;  ly=SCREEN_H/2;
  rx=SCREEN_W/2+EYE_W/2+EYE_GAP/2;  ry=SCREEN_H/2;
  lookDx=0; lookDy=0;
}
void drawBase() {
  display.clearDisplay();
  display.fillRoundRect(lx-EYE_W/2+lookDx, ly-EYE_H/2+lookDy, EYE_W, EYE_H, EYE_R, SSD1306_WHITE);
  display.fillRoundRect(rx-EYE_W/2+lookDx, ry-EYE_H/2+lookDy, EYE_W, EYE_H, EYE_R, SSD1306_WHITE);
}
void showEyes() { drawBase(); display.display(); }

// ── ALL MOOD EYE FUNCTIONS ────────────────────────────────────
void eyeHappy() {
  drawBase();
  int off=EYE_H/2;
  display.fillTriangle(lx-EYE_W/2-1,ly+off, lx+EYE_W/2+1,ly+5+off, lx-EYE_W/2-1,ly+EYE_H+off, SSD1306_BLACK);
  display.fillTriangle(rx+EYE_W/2+1,ry+off, rx-EYE_W/2-1,ry+5+off, rx+EYE_W/2+1,ry+EYE_H+off, SSD1306_BLACK);
  display.display();
}
void eyeSad() {
  display.clearDisplay();
  display.fillRoundRect(lx-EYE_W/2,ly-EYE_H/2+8,EYE_W,EYE_H-8,EYE_R,SSD1306_WHITE);
  display.fillRoundRect(rx-EYE_W/2,ry-EYE_H/2+8,EYE_W,EYE_H-8,EYE_R,SSD1306_WHITE);
  display.fillTriangle(lx-EYE_W/2,ly-EYE_H/2+8,lx,ly-EYE_H/2+8,lx-EYE_W/2,ly-EYE_H/2+20,SSD1306_BLACK);
  display.fillTriangle(rx+EYE_W/2,ry-EYE_H/2+8,rx,ry-EYE_H/2+8,rx+EYE_W/2,ry-EYE_H/2+20,SSD1306_BLACK);
  display.display();
}
void eyeAngry() {
  drawBase();
  display.fillTriangle(lx-EYE_W/2,ly-EYE_H/2,lx+EYE_W/2,ly-EYE_H/2,lx-EYE_W/2,ly-EYE_H/2+EYE_H/3,SSD1306_BLACK);
  display.fillTriangle(rx+EYE_W/2,ry-EYE_H/2,rx-EYE_W/2,ry-EYE_H/2,rx+EYE_W/2,ry-EYE_H/2+EYE_H/3,SSD1306_BLACK);
  display.display();
}
void eyeSurprised() {
  display.clearDisplay();
  display.drawRoundRect(lx-EYE_W/2-5,ly-EYE_H/2-5,EYE_W+10,EYE_H+10,EYE_R,SSD1306_WHITE);
  display.fillRoundRect(lx-EYE_W/2,ly-EYE_H/2,EYE_W,EYE_H,EYE_R,SSD1306_WHITE);
  display.drawRoundRect(rx-EYE_W/2-5,ry-EYE_H/2-5,EYE_W+10,EYE_H+10,EYE_R,SSD1306_WHITE);
  display.fillRoundRect(rx-EYE_W/2,ry-EYE_H/2,EYE_W,EYE_H,EYE_R,SSD1306_WHITE);
  display.display();
}
void eyeSleepy() {
  display.clearDisplay();
  display.fillRoundRect(lx-EYE_W/2,ly-EYE_H/4,EYE_W,EYE_H/2,EYE_R,SSD1306_WHITE);
  display.fillRoundRect(rx-EYE_W/2,ry-EYE_H/4,EYE_W,EYE_H/2,EYE_R,SSD1306_WHITE);
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(rx+EYE_W/2+3,ry-8);  display.print("z");
  display.setCursor(rx+EYE_W/2+9,ry-18); display.print("Z");
  display.display();
}
void eyeExcited() {
  display.clearDisplay();
  display.fillTriangle(lx,ly-19,lx-8,ly,lx+8,ly,SSD1306_WHITE);
  display.fillTriangle(lx,ly+19,lx-8,ly,lx+8,ly,SSD1306_WHITE);
  display.fillTriangle(lx-19,ly,lx,ly-8,lx,ly+8,SSD1306_WHITE);
  display.fillTriangle(lx+19,ly,lx,ly-8,lx,ly+8,SSD1306_WHITE);
  display.fillTriangle(rx,ry-19,rx-8,ry,rx+8,ry,SSD1306_WHITE);
  display.fillTriangle(rx,ry+19,rx-8,ry,rx+8,ry,SSD1306_WHITE);
  display.fillTriangle(rx-19,ry,rx,ry-8,rx,ry+8,SSD1306_WHITE);
  display.fillTriangle(rx+19,ry,rx,ry-8,rx,ry+8,SSD1306_WHITE);
  display.display();
}
void eyeLove() {
  display.clearDisplay();
  display.fillCircle(lx-6,ly-8,9,SSD1306_WHITE);
  display.fillCircle(lx+6,ly-8,9,SSD1306_WHITE);
  display.fillTriangle(lx-14,ly-3,lx+14,ly-3,lx,ly+13,SSD1306_WHITE);
  display.fillCircle(rx-6,ry-8,9,SSD1306_WHITE);
  display.fillCircle(rx+6,ry-8,9,SSD1306_WHITE);
  display.fillTriangle(rx-14,ry-3,rx+14,ry-3,rx,ry+13,SSD1306_WHITE);
  display.display();
}
void eyeCurious() {
  drawBase();
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(2); display.setCursor(rx-6,ry-11); display.print("?");
  display.drawFastHLine(lx-EYE_W/2,ly-EYE_H/2-4,EYE_W/2,SSD1306_WHITE);
  display.display();
}
void eyeProud() {
  drawBase();
  display.fillRect(lx-EYE_W/2,ly-EYE_H/2,EYE_W,EYE_H/3,SSD1306_BLACK);
  display.fillRect(rx-EYE_W/2,ry-EYE_H/2,EYE_W,EYE_H/3,SSD1306_BLACK);
  for(int i=0;i<3;i++){
    display.drawFastHLine(lx-EYE_W/2+i,ly-EYE_H/2-3-i,EYE_W-i*2,SSD1306_WHITE);
    display.drawFastHLine(rx-EYE_W/2+i,ry-EYE_H/2-3-i,EYE_W-i*2,SSD1306_WHITE);
  }
  display.display();
}
void eyeScared() {
  display.clearDisplay();
  display.fillRoundRect(lx-EYE_W/2-5,ly-EYE_H/2-5,EYE_W+10,EYE_H+10,EYE_R,SSD1306_WHITE);
  display.fillRoundRect(rx-EYE_W/2-5,ry-EYE_H/2-5,EYE_W+10,EYE_H+10,EYE_R,SSD1306_WHITE);
  display.fillCircle(lx+8,ly+8,6,SSD1306_BLACK);
  display.fillCircle(rx+8,ry+8,6,SSD1306_BLACK);
  display.display();
}
void eyeWink() {
  display.clearDisplay();
  display.fillRoundRect(lx-EYE_W/2,ly-EYE_H/2,EYE_W,EYE_H,EYE_R,SSD1306_WHITE);
  display.drawFastHLine(rx-EYE_W/2,ry,EYE_W,SSD1306_WHITE);
  display.display();
}
void eyeThinking() {
  display.clearDisplay();
  display.fillRoundRect(lx-EYE_W/2,ly,EYE_W,EYE_H/2,EYE_R,SSD1306_WHITE);
  display.fillRoundRect(rx-EYE_W/2,ry-EYE_H/2,EYE_W,EYE_H,EYE_R,SSD1306_WHITE);
  display.fillRect(rx-EYE_W/2,ry-EYE_H/2,EYE_W,EYE_H/3,SSD1306_BLACK);
  display.fillCircle(rx+EYE_W/2+4,ry-EYE_H/2+2,2,SSD1306_WHITE);
  display.fillCircle(rx+EYE_W/2+10,ry-EYE_H/2-4,3,SSD1306_WHITE);
  display.fillCircle(rx+EYE_W/2+18,ry-EYE_H/2-10,4,SSD1306_WHITE);
  display.display();
}

// ── LOOK AROUND ───────────────────────────────────────────────
void eyeLook(int dx, int dy) {
  lookDx=dx; lookDy=dy; drawBase(); display.display();
  delay(350);
  lookDx=0; lookDy=0;
}

// ── BLINK ─────────────────────────────────────────────────────
void startBlink() {
  if(blinkActive||moodActive||gameState!=GAME_OFF) return;
  blinkActive=true; blinkH=EYE_H; blinkDir=-1; blinkMs=nowMs;
}
void tickBlink() {
  if(!blinkActive) return;
  if(nowMs-blinkMs < BLINK_STEP_MS) return;
  blinkMs=nowMs;
  display.clearDisplay();
  display.fillRoundRect(lx-EYE_W/2,ly-blinkH/2,EYE_W,blinkH,EYE_R,SSD1306_WHITE);
  display.fillRoundRect(rx-EYE_W/2,ry-blinkH/2,EYE_W,blinkH,EYE_R,SSD1306_WHITE);
  display.display();
  blinkH+=blinkDir*8;
  if(blinkH<=2)    { blinkH=2; blinkDir=1; }
  if(blinkH>=EYE_H){ blinkActive=false; resetEyes(); showEyes(); }
}

// ══════════════════════════════════════════════════════════════
//  MOOD SYSTEM
// ══════════════════════════════════════════════════════════════
Mood moodFromStr(const char* s) {
  if(!s) return MOOD_IDLE;
  if(strcmp(s,"happy")==0)     return MOOD_HAPPY;
  if(strcmp(s,"excited")==0)   return MOOD_EXCITED;
  if(strcmp(s,"sad")==0)       return MOOD_SAD;
  if(strcmp(s,"surprised")==0) return MOOD_SURPRISED;
  if(strcmp(s,"sleepy")==0)    return MOOD_SLEEPY;
  if(strcmp(s,"angry")==0)     return MOOD_ANGRY;
  if(strcmp(s,"love")==0)      return MOOD_LOVE;
  if(strcmp(s,"curious")==0)   return MOOD_CURIOUS;
  if(strcmp(s,"proud")==0)     return MOOD_PROUD;
  if(strcmp(s,"scared")==0)    return MOOD_SCARED;
  return MOOD_IDLE;
}
void applyMood(Mood m, bool snd, bool pin) {
  currentMood=m; moodActive=(m!=MOOD_IDLE);
  moodPinned=pin; blinkActive=false;
  resetEyes();
  unsigned long hold=2500;
  switch(m){
    case MOOD_HAPPY:     eyeHappy();     if(snd)sndHappy();     hold=2500; break;
    case MOOD_EXCITED:   eyeExcited();   if(snd)sndExcited();   hold=3000; break;
    case MOOD_SAD:       eyeSad();       if(snd)sndSad();       hold=3000; break;
    case MOOD_SURPRISED: eyeSurprised(); if(snd)sndSurprised(); hold=2000; break;
    case MOOD_SLEEPY:    eyeSleepy();    if(snd)sndSleepy();    hold=5000; break;
    case MOOD_ANGRY:     eyeAngry();     if(snd)sndAngry();     hold=2500; break;
    case MOOD_LOVE:      eyeLove();      if(snd)sndLove();      hold=4000; break;
    case MOOD_CURIOUS:   eyeCurious();   if(snd)sndCurious();   hold=3000; break;
    case MOOD_PROUD:     eyeProud();     if(snd)sndProud();     hold=2500; break;
    case MOOD_SCARED:    eyeScared();    if(snd)sndScared();    hold=2000; break;
    default: showEyes(); moodActive=false; currentMood=MOOD_IDLE; return;
  }
  statMoods++;
  moodEndMs = pin ? 0xFFFFFFFFUL : nowMs+hold;
  char buf[64];
  snprintf(buf,sizeof(buf),"{\"s\":\"mood\",\"m\":%d}",(int)m);
  wsSend(buf);
}
void tickMood() {
  if(!moodActive||moodPinned) return;
  if(nowMs>=moodEndMs){
    moodActive=false; currentMood=MOOD_IDLE;
    resetEyes(); showEyes();
    if(pageUserSet && currentPage!=PAGE_EYES) showPage();
  }
}

// ══════════════════════════════════════════════════════════════
//  SLEEP
// ══════════════════════════════════════════════════════════════
void enterSleep() {
  if(sleepMode) return;
  sleepMode=true; sndSleepy();
  applyMood(MOOD_SLEEPY,false,true);
  wsSend("{\"s\":\"sleep\"}");
}
void exitSleep() {
  if(!sleepMode) return;
  sleepMode=false; moodActive=false; moodPinned=false;
  currentMood=MOOD_IDLE; sndWake();
  resetEyes(); showEyes();
  wsSend("{\"s\":\"wake\"}");
}

// ══════════════════════════════════════════════════════════════
//  IDLE ANIMATIONS
// ══════════════════════════════════════════════════════════════
void doIdle() {
  switch(random(0,8)){
    case 0: eyeLook(-8,0);  break;
    case 1: eyeLook(8,0);   break;
    case 2: eyeLook(0,-6);  break;
    case 3: eyeLook(0,6);   break;
    case 4: eyeWink();  delay(500); resetEyes(); showEyes(); break;
    case 5: eyeThinking(); delay(1200); resetEyes(); showEyes(); break;
    case 6: startBlink(); break;
    case 7:
      if(nowMs-lastRandMsgMs > RAND_MSG_IV){
        lastRandMsgMs=nowMs;
        startScroll(RAND_MSGS[random(0,NUM_RAND_MSGS)]);
      }
      break;
  }
}

// ══════════════════════════════════════════════════════════════
//  SCROLL + NOTIFY
// ══════════════════════════════════════════════════════════════
void startScroll(const char* txt) {
  strncpy(scrollBuf,txt,63); scrollBuf[63]='\0';
  scrollX=SCREEN_W; scrollActive=true; scrollMs=nowMs;
  sndMsg(); statMsgs++;
}
void tickScroll() {
  if(!scrollActive) return;
  if(nowMs-scrollMs < SCROLL_MS) return;
  scrollMs=nowMs;
  display.clearDisplay();
  display.fillRoundRect(38,2,22,16,4,SSD1306_WHITE);
  display.fillRoundRect(68,2,22,16,4,SSD1306_WHITE);
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(2,22); display.print("> MSG:");
  display.setCursor(scrollX,38); display.print(scrollBuf);
  display.display();
  scrollX-=2;
  if(scrollX < -(int)(strlen(scrollBuf)*6)){
    scrollActive=false; resetEyes(); showEyes();
    if(pageUserSet&&currentPage!=PAGE_EYES) showPage();
  }
}

void showNotify(const char* txt, const char* icon) {
  strncpy(notifyBuf,txt,31); notifyBuf[31]='\0';
  strncpy(notifyIcon,icon,7); notifyIcon[7]='\0';
  notifyActive=true; notifyEndMs=nowMs+4500;
  sndNotify(); statNotifs++;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.drawRect(0,0,SCREEN_W,SCREEN_H,SSD1306_WHITE);
  display.drawRect(2,2,SCREEN_W-4,SCREEN_H-4,SSD1306_WHITE);
  display.setTextSize(1);
  if(strcmp(icon,"f1")==0)       { display.setCursor(4,5); display.print("[ F1 UPDATE ]"); }
  else if(strcmp(icon,"music")==0){ display.setCursor(4,5); display.print("[ NOW PLAYING ]"); }
  else if(strcmp(icon,"alarm")==0){ display.setCursor(4,5); display.print("!! ALARM !!"); }
  else if(strcmp(icon,"event")==0){ display.setCursor(4,5); display.print("[ EVENT ]"); }
  else                            { display.setCursor(4,5); display.print("[ INFO ]"); }
  display.drawFastHLine(4,15,SCREEN_W-8,SSD1306_WHITE);
  int len=strlen(notifyBuf);
  if(len<=16) { display.setCursor((SCREEN_W-len*6)/2,26); display.print(notifyBuf); }
  else        { display.setCursor(4,22); display.print(notifyBuf); }
  display.display();
}
void tickNotify() {
  if(!notifyActive) return;
  if(nowMs>=notifyEndMs){
    notifyActive=false; resetEyes(); showEyes();
    if(pageUserSet&&currentPage!=PAGE_EYES) showPage();
  }
}

// ══════════════════════════════════════════════════════════════
//  DISPLAY PAGES
// ══════════════════════════════════════════════════════════════
void pageClock() {
  if(!tdata.valid){ resetEyes(); showEyes(); return; }
  display.clearDisplay();
  display.fillRoundRect(SCREEN_W/2-29,10,24,16,4,SSD1306_WHITE);
  display.fillRoundRect(SCREEN_W/2+5, 10,24,16,4,SSD1306_WHITE);
  char buf[9]; snprintf(buf,sizeof(buf),"%02d:%02d",tdata.h,tdata.m);
  display.setTextSize(2); display.setTextColor(SSD1306_WHITE);
  display.setCursor((SCREEN_W-(int)strlen(buf)*12)/2,30); display.print(buf);
  display.setTextSize(1);
  display.setCursor(2,SCREEN_H-9); display.print(tdata.day);
  if(tdata.s%2==0) display.fillCircle(SCREEN_W-5,SCREEN_H-4,2,SSD1306_WHITE);
  if(alarmCount>0){
    char ab[8]; snprintf(ab,sizeof(ab),"[A%d]",alarmCount);
    display.setCursor(SCREEN_W-30,SCREEN_H-9); display.print(ab);
  }
  display.display();
}

void pageWeather() {
  if(!wdata.valid){ resetEyes(); showEyes(); return; }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1); display.setCursor(0,0); display.print("WEATHER");
  char buf[24];
  snprintf(buf,sizeof(buf),"%.1f C",wdata.temp);
  display.setTextSize(2); display.setCursor(0,12); display.print(buf);
  display.setTextSize(1);
  display.setCursor(0,36); display.print(wdata.desc);
  snprintf(buf,sizeof(buf),"Wind:%.0fkm/h",wdata.wind);
  display.setCursor(0,48); display.print(buf);
  display.drawRect(68,48,56,6,SSD1306_WHITE);
  display.fillRect(69,49,constrain((int)(wdata.wind/60.0f*54),0,54),4,SSD1306_WHITE);
  display.display();
}

void pageF1() {
  if(!f1stand.valid){ resetEyes(); showEyes(); return; }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1); display.setCursor(0,0); display.print("F1 STANDINGS 2025");
  display.drawFastHLine(0,9,SCREEN_W,SSD1306_WHITE);
  char buf[28];
  snprintf(buf,sizeof(buf),"P1 %-3s %s pts",f1stand.p1,f1stand.p1pts);
  display.setCursor(0,12); display.print(buf);
  snprintf(buf,sizeof(buf),"P2 %-3s %s",f1stand.p2,f1stand.p2pts);
  display.setCursor(0,23); display.print(buf);
  snprintf(buf,sizeof(buf),"P3 %-3s %s",f1stand.p3,f1stand.p3pts);
  display.setCursor(0,33); display.print(buf);
  display.setCursor(0,43); display.print(f1stand.p1team);
  if(f1stand.nextValid){
    snprintf(buf,sizeof(buf),"NXT:%dd %dh",f1stand.nextDays,f1stand.nextHrs);
    display.setCursor(0,54); display.print(buf);
    char short_name[16]; strncpy(short_name,f1stand.nextName,15); short_name[15]='\0';
    char* gp=strstr(short_name," Grand"); if(gp)*gp='\0';
    display.setCursor(64,54); display.print(short_name);
  }
  display.display();
}

void pageF1Live() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  if(!f1live.valid){
    display.setTextSize(1); display.setCursor(0,0); display.print("F1 LIVE");
    display.drawFastHLine(0,9,SCREEN_W,SSD1306_WHITE);
    display.setCursor(0,18); display.print(f1live.session);
    display.setCursor(0,30); display.print("No active session");
    display.setCursor(0,42); display.print("or race weekend.");
    display.display(); return;
  }
  display.setTextSize(1); display.setCursor(0,0);
  if(f1live.sc) {
    display.print("!SC! F1 LIVE");
  } else {
    char hdr[20];
    if(f1live.total>0) snprintf(hdr,sizeof(hdr),"LAP %d/%d  %s",f1live.lap,f1live.total,f1live.circuit);
    else               snprintf(hdr,sizeof(hdr),"F1 LIVE  %s",f1live.circuit);
    display.print(hdr);
  }
  display.drawFastHLine(0,9,SCREEN_W,SSD1306_WHITE);
  display.setTextSize(2); display.setCursor(0,13);
  display.print("P1 "); display.print(f1live.p1);
  display.setTextSize(1); display.setCursor(0,34);
  char g[22]; snprintf(g,sizeof(g),"P2 %-3s  %s",f1live.p2,f1live.gap);
  display.print(g);
  if(f1live.sc){
    display.fillRoundRect(0,44,SCREEN_W,17,2,SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(8,49); display.print("** SAFETY CAR **");
    display.setTextColor(SSD1306_WHITE);
  } else if(f1live.total>0){
    int bw=constrain((int)(100.0f*f1live.lap/f1live.total),0,100);
    display.setCursor(0,46); display.print("PROGRESS:");
    display.drawRect(0,55,102,7,SSD1306_WHITE);
    display.fillRect(1,56,bw,5,SSD1306_WHITE);
    char pct[6]; snprintf(pct,sizeof(pct),"%d%%",bw);
    display.setCursor(104,55); display.print(pct);
  }
  display.display();
}

void pageMusic() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1); display.setCursor(0,0);
  if(!mdata.valid)        display.print("MUSIC");
  else if(mdata.playing)  display.print("> NOW PLAYING");
  else                    display.print("|| PAUSED");
  display.drawFastHLine(0,9,SCREEN_W,SSD1306_WHITE);
  display.fillRect(SCREEN_W-12,1,2,8,SSD1306_WHITE);
  display.fillCircle(SCREEN_W-12,9,3,SSD1306_WHITE);
  if(mdata.valid){
    display.setTextSize(1);
    char title[22]; strncpy(title,mdata.title,21); title[21]='\0';
    display.setCursor(0,13); display.print(title);
    char artist[22]; strncpy(artist,mdata.artist,21); artist[21]='\0';
    display.setCursor(0,24); display.print(artist);
    if(mdata.playing){
      display.drawRect(0,35,SCREEN_W,5,SSD1306_WHITE);
      display.fillRect(1,36,(int)(mdata.progress*126/100),3,SSD1306_WHITE);
    }
  } else {
    display.setCursor(0,18); display.print("No track data.");
    display.setCursor(0,30); display.print("Login at /spotify");
    display.setCursor(0,40); display.print("to enable Spotify");
  }
  for(int i=0;i<16;i++){
    float ph = (float)(nowMs/80+i*18)*0.1f;
    int h = 4+(int)(8.0f*fabsf(sinf(ph)));   // FIX #5: fabs → fabsf for float
    display.fillRect(i*8, SCREEN_H-h, 6, h, SSD1306_WHITE);
  }
  display.display();
}

void pageStats() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1); display.setCursor(0,0); display.print("SESSION STATS");
  display.drawFastHLine(0,9,SCREEN_W,SSD1306_WHITE);
  char buf[24];
  snprintf(buf,sizeof(buf),"MOODS:  %d",statMoods);  display.setCursor(0,12); display.print(buf);
  snprintf(buf,sizeof(buf),"MSGS:   %d",statMsgs);   display.setCursor(0,23); display.print(buf);
  snprintf(buf,sizeof(buf),"NOTIFS: %d",statNotifs); display.setCursor(0,34); display.print(buf);
  snprintf(buf,sizeof(buf),"ALARMS: %d",alarmCount); display.setCursor(0,45); display.print(buf);
  unsigned long up=nowMs/1000;
  snprintf(buf,sizeof(buf),"UP: %luh %lum",up/3600,(up%3600)/60);
  display.setCursor(0,56); display.print(buf);
  display.display();
}

void pageWifi() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1); display.setCursor(0,0); display.print("WIFI + OTA");
  display.drawFastHLine(0,9,SCREEN_W,SSD1306_WHITE);
  if(WiFi.status()==WL_CONNECTED){
    display.setCursor(0,12); display.print(WiFi.localIP().toString().c_str());
    display.setCursor(0,23); display.print("WS :81  HTTP :80");
    char rb[22]; snprintf(rb,sizeof(rb),"RSSI: %d dBm",WiFi.RSSI());
    display.setCursor(0,34); display.print(rb);
    display.setCursor(0,45); display.print("OTA: http://IP/update");
    char cli[22]; snprintf(cli,sizeof(cli),"WS CLIENT: %s",wsClient==255?"NONE":"OK");
    display.setCursor(0,56); display.print(cli);
  } else {
    display.setCursor(0,18); display.print("DISCONNECTED");
    display.setCursor(0,32); display.print(WIFI_SSID);
  }
  display.display();
}

void showPage() {
  if(notifyActive||scrollActive||moodActive) return;
  sndPage();
  switch(currentPage){
    case PAGE_EYES:    resetEyes(); showEyes(); break;
    case PAGE_CLOCK:   pageClock();   break;
    case PAGE_WEATHER: pageWeather(); break;
    case PAGE_F1:      pageF1();      break;
    case PAGE_F1LIVE:  pageF1Live();  break;
    case PAGE_MUSIC:   pageMusic();   break;
    case PAGE_STATS:   pageStats();   break;
    case PAGE_WIFI:    pageWifi();    break;
    default: currentPage=0; resetEyes(); showEyes(); break;
  }
}

// ══════════════════════════════════════════════════════════════
//  SNAKE GAME
// ══════════════════════════════════════════════════════════════
void snakeInit() {
  snLen=4; snDx=1; snDy=0; snScore=0;
  for(int i=0;i<snLen;i++){ snBody[i].x=4-i; snBody[i].y=4; }
  snFx=random(1,SN_COLS-1); snFy=random(1,SN_ROWS-1);
  gameState=GAME_RUNNING;
}
void snakeDraw() {
  display.clearDisplay();
  display.drawRect(0,0,SCREEN_W,SCREEN_H,SSD1306_WHITE);
  if((nowMs/300)%2==0) display.fillRect(snFx*SN_CW,snFy*SN_CH,SN_CW,SN_CH,SSD1306_WHITE);
  for(int i=0;i<snLen;i++){
    int sx=snBody[i].x*SN_CW, sy=snBody[i].y*SN_CH;
    if(i==0) display.fillRect(sx,sy,SN_CW,SN_CH,SSD1306_WHITE);
    else     display.drawRect(sx,sy,SN_CW,SN_CH,SSD1306_WHITE);
  }
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(2,1); display.print(snScore);
  display.display();
}
void tickSnake() {
  if(gameState!=GAME_RUNNING) return;
  if(nowMs-snMoveMs < SN_SPEED) return;
  snMoveMs=nowMs;
  int nx=snBody[0].x+snDx, ny=snBody[0].y+snDy;
  bool dead=false;
  if(nx<0||nx>=SN_COLS||ny<0||ny>=SN_ROWS) dead=true;
  for(int i=1;i<snLen;i++) if(snBody[i].x==nx&&snBody[i].y==ny) dead=true;
  if(dead){
    sndGameOver(); applyMood(MOOD_SAD,false,false);
    gameState=GAME_OVER;
    display.clearDisplay();
    display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
    display.setCursor(28,8); display.print("GAME OVER");
    display.setTextSize(2); display.setCursor(40,24);
    char sc[8]; snprintf(sc,sizeof(sc),"%d",snScore);
    display.print(sc);
    display.setTextSize(1); display.setCursor(18,50); display.print("Tap any key to play");
    display.display();
    return;
  }
  for(int i=snLen-1;i>0;i--) snBody[i]=snBody[i-1];
  snBody[0].x=nx; snBody[0].y=ny;
  if(nx==snFx&&ny==snFy){
    sndGameEat(); snScore+=10;
    snLen=min(snLen+1,63);
    snFx=random(1,SN_COLS-1); snFy=random(1,SN_ROWS-1);
  }
  snakeDraw();
}

// ══════════════════════════════════════════════════════════════
//  ALARMS + EVENTS
// ══════════════════════════════════════════════════════════════
void checkAlarms() {
  if(!tdata.valid||tdata.s!=0) return;
  for(int i=0;i<alarmCount;i++){
    if(alarms[i].armed && alarms[i].h==tdata.h && alarms[i].m==tdata.m){
      sndAlarm();
      showNotify(alarms[i].label[0]?alarms[i].label:"ALARM!","alarm");
      applyMood(MOOD_EXCITED,false,false);
      alarms[i].armed=false;
    }
  }
}

// ══════════════════════════════════════════════════════════════
//  NIGHT MODE
// ══════════════════════════════════════════════════════════════
void checkNightMode() {
  if(!tdata.valid) return;
  bool isNight=(tdata.h>=22||tdata.h<6);
  if(isNight && !sleepMode) enterSleep();
  if(!isNight && sleepMode) exitSleep();
}

// ══════════════════════════════════════════════════════════════
//  BURN PROTECTION
// ══════════════════════════════════════════════════════════════
void tickBurn() {
  if(!burnInvertOn && nowMs-lastBurnMs>=BURN_IV){
    lastBurnMs=nowMs;
    if(!moodActive&&!scrollActive&&!notifyActive&&gameState==GAME_OFF){
      display.invertDisplay(true); burnInvertOn=true; burnInvertMs=nowMs;
    }
  }
  if(burnInvertOn && nowMs-burnInvertMs>=120){
    display.invertDisplay(false); burnInvertOn=false;
  }
}

// ══════════════════════════════════════════════════════════════
//  TOUCH
// ══════════════════════════════════════════════════════════════
void handleTouch() {
  for(int i=0;i<NUM_TOUCH_PINS;i++){
    bool touched = (touchRead(TOUCH_GPIOS[i]) < TOUCH_THRESH);
    unsigned long now = nowMs;

    if(touched && !tp[i].active){
      if(now-tp[i].lastMs > TOUCH_DEBOUNCE_MS){
        tp[i].active=true; tp[i].pressMs=now; tp[i].holdFired=false;
      }
    }

    if(tp[i].active && !tp[i].holdFired && (now-tp[i].pressMs)>TOUCH_HOLD_MS){
      tp[i].holdFired=true;
      sndClick();
      if(gameState==GAME_RUNNING){
        if(i==0&&snDy!=1)  {snDx=0;snDy=-1;}
        if(i==1&&snDy!=-1) {snDx=0;snDy=1;}
        if(i==2&&snDx!=1)  {snDx=-1;snDy=0;}
        if(i==3&&snDx!=-1) {snDx=1;snDy=0;}
      } else {
        switch(i){
          case 0:
            currentPage=(currentPage+NUM_PAGES-1)%NUM_PAGES;
            pageUserSet=true; showPage(); break;
          case 1:
            applyMood((Mood)random(1,11)); break;
          case 2:
            songStop(); songCurrent=(songCurrent+1)%NUM_SONGS;
            songPlay(songCurrent); break;
          case 3:
            volume=(uint8_t)max(0,(int)volume-1); eepromSave(); sndVolDown(); break;
          case 4:
            sleepMode ? exitSleep() : enterSleep(); break;
          case 5:
            if(eventCount>0) showNotify(events[0].text,"event");
            else showNotify("No events set","event"); break;
        }
      }
    }

    if(!touched && tp[i].active){
      if(!tp[i].holdFired){
        sndClick();
        if(gameState==GAME_OVER){ snakeInit(); tp[i].active=false; tp[i].lastMs=now; continue; }
        if(gameState==GAME_RUNNING){
          if(i==0&&snDy!=1)  {snDx=0;snDy=-1;}
          if(i==1&&snDy!=-1) {snDx=0;snDy=1;}
          if(i==2&&snDx!=1)  {snDx=-1;snDy=0;}
          if(i==3&&snDx!=-1) {snDx=1;snDy=0;}
        } else {
          switch(i){
            case 0:
              currentPage=(currentPage+1)%NUM_PAGES;
              pageUserSet=true; showPage(); break;
            case 1:
              { int m=(int)currentMood+1; if(m>10)m=1; applyMood((Mood)m); } break;
            case 2:
              if(songActive){ songStop(); }
              else { songPlay(songCurrent); } break;
            case 3:
              volume=(uint8_t)min(10,(int)volume+1); eepromSave(); sndVolUp(); break;
            case 4:
              if(currentPage==PAGE_F1LIVE){ currentPage=PAGE_EYES; pageUserSet=false; showEyes(); }
              else { currentPage=PAGE_F1LIVE; pageUserSet=true; showPage(); applyMood(MOOD_EXCITED,true,false); }
              break;
            case 5:
              startScroll(RAND_MSGS[random(0,NUM_RAND_MSGS)]); break;
          }
        }
      }
      tp[i].active=false; tp[i].lastMs=now;
    }
  }
}

// ══════════════════════════════════════════════════════════════
//  SERIAL PARSER
// ══════════════════════════════════════════════════════════════
void tickSerial() {
  while(Serial.available()){
    char c=(char)Serial.read();
    if(c=='\n'||c=='\r'){
      if(sbufLen>0){ sbuf[sbufLen]='\0'; parseJSON(sbuf); sbufLen=0; }
    } else if(sbufLen<511){ sbuf[sbufLen++]=c; }
  }
}

// ══════════════════════════════════════════════════════════════
//  JSON COMMAND PARSER
// ══════════════════════════════════════════════════════════════
void parseJSON(char* buf) {
  JsonDocument doc;
  if(deserializeJson(doc,buf)!=DeserializationError::Ok) return;
  const char* t=doc["t"]; if(!t) return;

  if(strcmp(t,"ping")==0){ wsSend("{\"s\":\"pong\"}"); Serial.println(F("{\"s\":\"pong\"}")); return; }

  if(strcmp(t,"mood")==0){
    const char* v=doc["v"];
    bool pin=doc["pin"]|false;
    if(v) applyMood(moodFromStr(v),true,pin);
    return;
  }

  if(strcmp(t,"page")==0){
    int v=doc["v"]|0;
    if(v>=0&&v<NUM_PAGES){ currentPage=v; pageUserSet=true; showPage(); }
    return;
  }

  if(strcmp(t,"msg")==0){
    const char* txt=doc["text"]; if(txt) startScroll(txt); return;
  }

  if(strcmp(t,"notify")==0){
    const char* txt=doc["text"];
    const char* ico=doc["icon"]|"info";
    if(txt) showNotify(txt,ico);
    return;
  }

  if(strcmp(t,"song")==0){
    const char* nm=doc["name"]; if(nm) songByName(nm); return;
  }

  if(strcmp(t,"music")==0){
    const char* ti=doc["title"];  if(ti) strlcpy(mdata.title,ti,32);
    const char* ar=doc["artist"]; if(ar) strlcpy(mdata.artist,ar,24);
    mdata.playing = doc["playing"]|false;
    mdata.valid   = true;
    if(ti){
      char nb[32]; snprintf(nb,sizeof(nb),"%.28s",mdata.title);
      showNotify(nb,"music");
    }
    if(currentPage==PAGE_MUSIC) pageMusic();
    return;
  }

  if(strcmp(t,"f1live")==0){
    bool prevSC = f1live.sc;
    const char* d=doc["driver"]; if(d) strlcpy(f1live.p1,d,8);
    const char* p=doc["p2"];     if(p) strlcpy(f1live.p2,p,8);
    const char* g=doc["gap"];    if(g) strlcpy(f1live.gap,g,16);
    f1live.lap   = doc["lap"]|0;
    f1live.total = doc["total"]|0;
    f1live.sc    = doc["sc"]|false;
    f1live.valid = true;
    if(f1live.sc&&!prevSC) applyMood(MOOD_SURPRISED,true,false);
    if(currentPage==PAGE_F1LIVE) pageF1Live();
    return;
  }

  if(strcmp(t,"alarm")==0){
    if(alarmCount<4){
      alarms[alarmCount].h = doc["h"]|7;
      alarms[alarmCount].m = doc["m"]|0;
      alarms[alarmCount].armed = true;
      const char* lbl=doc["label"]|"ALARM";
      strlcpy(alarms[alarmCount].label,lbl,20);
      alarmCount++;
      char nb[24]; snprintf(nb,sizeof(nb),"Alarm set %02d:%02d",alarms[alarmCount-1].h,alarms[alarmCount-1].m);
      showNotify(nb,"alarm");
    }
    return;
  }

  if(strcmp(t,"event")==0){
    if(eventCount<4){
      const char* txt=doc["text"];
      if(txt){
        strlcpy(events[eventCount].text,txt,32);
        events[eventCount].active=true;
        eventCount++;
        showNotify(txt,"event");
      }
    }
    return;
  }

  if(strcmp(t,"volume")==0){
    volume=(uint8_t)constrain((int)(doc["v"]|7),0,10);
    eepromSave(); return;
  }

  if(strcmp(t,"brightness")==0){
    int v=constrain((int)(doc["v"]|2),1,4);
    display.dim(v<=2);
    EEPROM.write(EE_BRIGHT,(uint8_t)v); EEPROM.commit();
    return;
  }

  if(strcmp(t,"sleep")==0){ enterSleep(); return; }
  if(strcmp(t,"wake")==0) { exitSleep();  return; }

  if(strcmp(t,"game")==0){
    const char* v=doc["v"]|"snake";
    if(strcmp(v,"snake")==0){ currentPage=PAGE_EYES; pageUserSet=false; snakeInit(); }
    else if(strcmp(v,"stop")==0){ gameState=GAME_OFF; resetEyes(); showEyes(); }
    return;
  }

  if(strcmp(t,"stopwatch")==0){
    const char* act=doc["action"]|"start";
    if(strcmp(act,"start")==0&&!swRunning){ swStartMs=nowMs; swRunning=true; }
    else if(strcmp(act,"stop")==0&&swRunning){ swElapsed+=nowMs-swStartMs; swRunning=false; }
    else if(strcmp(act,"reset")==0){ swElapsed=0; swRunning=false; }
    return;
  }

  if(strcmp(t,"countdown")==0){
    unsigned long secs=(unsigned long)(doc["seconds"]|60);
    cdEndMs=nowMs+secs*1000UL; cdActive=true; return;
  }
}

// ══════════════════════════════════════════════════════════════
//  NTP TIME
// ══════════════════════════════════════════════════════════════
void updateTime() {
  time_t now=time(nullptr);
  if(now<100000) return;
  struct tm* ti=localtime(&now);
  tdata.h=ti->tm_hour; tdata.m=ti->tm_min; tdata.s=ti->tm_sec;
  tdata.valid=true;
  const char* days[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
  strlcpy(tdata.day,days[ti->tm_wday],4);
  if(tdata.s==0){
    checkAlarms();
    if(tdata.m==0) checkNightMode();
    char msg[64];
    snprintf(msg,sizeof(msg),"{\"s\":\"time\",\"h\":%d,\"m\":%d,\"s\":%d,\"day\":\"%s\"}",
      tdata.h,tdata.m,tdata.s,tdata.day);
    wsSend(msg);
  }
}

// ══════════════════════════════════════════════════════════════
//  WEATHER  (Open-Meteo)
// ══════════════════════════════════════════════════════════════
void fetchWeather() {
  if(WiFi.status()!=WL_CONNECTED) return;
  HTTPClient http;
  char url[180];
  snprintf(url,sizeof(url),
    "https://api.open-meteo.com/v1/forecast"
    "?latitude=%s&longitude=%s"
    "&current_weather=true&wind_speed_unit=kmh",
    WX_LAT, WX_LON);
  http.begin(url); http.setTimeout(8000);
  if(http.GET()==200){
    String body=http.getString();
    JsonDocument doc;
    if(deserializeJson(doc,body)==DeserializationError::Ok){
      JsonObject cw=doc["current_weather"];
      wdata.temp = cw["temperature"]|0.0f;
      wdata.wind = cw["windspeed"]|0.0f;
      wdata.code = cw["weathercode"]|0;
      wdata.valid= true;
      int c=wdata.code;
      if(c==0)       strlcpy(wdata.desc,"Clear sky",20);
      else if(c<=2)  strlcpy(wdata.desc,"Partly cloudy",20);
      else if(c<=3)  strlcpy(wdata.desc,"Overcast",20);
      else if(c<=9)  strlcpy(wdata.desc,"Fog",20);
      else if(c<=29) strlcpy(wdata.desc,"Drizzle/Rain",20);
      else if(c<=67) strlcpy(wdata.desc,"Rain",20);
      else if(c<=77) strlcpy(wdata.desc,"Snow",20);
      else if(c<=82) strlcpy(wdata.desc,"Showers",20);
      else if(c<=99) strlcpy(wdata.desc,"Thunderstorm",20);
      else           strlcpy(wdata.desc,"Unknown",20);
      Mood wm=MOOD_IDLE;
      if(c==0) wm=MOOD_HAPPY;
      else if(c<=3)  wm=MOOD_IDLE;
      else if(c<=67) wm=MOOD_SAD;
      else if(c<=77) wm=MOOD_SURPRISED;
      else if(c>=95) wm=MOOD_ANGRY;
      if(wm!=MOOD_IDLE) applyMood(wm,false,false);
      char msg[100];
      snprintf(msg,sizeof(msg),"{\"s\":\"wx\",\"t\":%.1f,\"w\":%.1f,\"c\":%d,\"d\":\"%s\"}",
        wdata.temp,wdata.wind,wdata.code,wdata.desc);
      wsSend(msg);
    }
  }
  http.end();
}

// ══════════════════════════════════════════════════════════════
//  F1 STANDINGS  (Jolpica/Ergast)
// ══════════════════════════════════════════════════════════════
void fetchF1Standings() {
  if(WiFi.status()!=WL_CONNECTED) return;
  HTTPClient http;
  http.begin("http://api.jolpi.ca/ergast/f1/current/driverStandings.json");
  http.setTimeout(10000);
  if(http.GET()==200){
    String body=http.getString();
    JsonDocument doc;
    if(deserializeJson(doc,body)==DeserializationError::Ok){
      JsonArray list=doc["MRData"]["StandingsTable"]["StandingsLists"][0]["DriverStandings"];
      if(list.size()>=3){
        const char* d; const char* p;
        d=list[0]["Driver"]["code"];           if(d) strlcpy(f1stand.p1,d,8);
        p=list[0]["points"];                   if(p) strlcpy(f1stand.p1pts,p,12);
        d=list[0]["Constructors"][0]["name"];  if(d) strlcpy(f1stand.p1team,d,20);
        d=list[1]["Driver"]["code"];           if(d) strlcpy(f1stand.p2,d,8);
        p=list[1]["points"];                   if(p) strlcpy(f1stand.p2pts,p,12);
        d=list[2]["Driver"]["code"];           if(d) strlcpy(f1stand.p3,d,8);
        p=list[2]["points"];                   if(p) strlcpy(f1stand.p3pts,p,12);
        f1stand.valid=true;
      }
    }
  }
  http.end();
  delay(200);

  http.begin("http://api.jolpi.ca/ergast/f1/current/next.json");
  http.setTimeout(10000);
  if(http.GET()==200){
    String body=http.getString();
    JsonDocument doc;
    if(deserializeJson(doc,body)==DeserializationError::Ok){
      JsonObject race=doc["MRData"]["RaceTable"]["Races"][0];
      const char* rn=race["raceName"]; if(rn) strlcpy(f1stand.nextName,rn,28);
      const char* ds=race["date"];
      if(ds){
        struct tm rt={};
        int yr,mo,dy,hh=0,mm=0;
        sscanf(ds,"%d-%d-%d",&yr,&mo,&dy);
        const char* ts=race["time"]; if(ts) sscanf(ts,"%d:%d",&hh,&mm);
        rt.tm_year=yr-1900; rt.tm_mon=mo-1; rt.tm_mday=dy;
        rt.tm_hour=hh; rt.tm_min=mm; rt.tm_isdst=-1;
        time_t raceEpoch=mktime(&rt);
        time_t nowEpoch=time(nullptr);
        long diff=(long)(raceEpoch-nowEpoch);
        if(diff>0){
          f1stand.nextDays=(int)(diff/86400);
          f1stand.nextHrs=(int)((diff%86400)/3600);
          f1stand.nextValid=true;
        }
      }
    }
  }
  http.end();
  sndF1(); applyMood(MOOD_EXCITED,false,false);
  if(f1stand.valid){
    char msg[180];
    snprintf(msg,sizeof(msg),
      "{\"s\":\"f1stand\",\"p1\":\"%s\",\"p1pts\":\"%s\",\"team\":\"%s\","
      "\"p2\":\"%s\",\"p2pts\":\"%s\",\"p3\":\"%s\",\"p3pts\":\"%s\","
      "\"nextName\":\"%s\",\"nextDays\":%d,\"nextHrs\":%d}",
      f1stand.p1,f1stand.p1pts,f1stand.p1team,
      f1stand.p2,f1stand.p2pts,f1stand.p3,f1stand.p3pts,
      f1stand.nextName,f1stand.nextDays,f1stand.nextHrs);
    wsSend(msg);
  }
}

// ══════════════════════════════════════════════════════════════
//  F1 LIVE  (OpenF1 API)
// ══════════════════════════════════════════════════════════════
void fetchOpenF1Session() {
  if(WiFi.status()!=WL_CONNECTED) return;
  char url[120];
  time_t now=time(nullptr);
  struct tm* ti=localtime(&now);
  snprintf(url,sizeof(url),
    "https://api.openf1.org/v1/sessions?year=%d&session_status=Started",
    ti->tm_year+1900);
  HTTPClient http;
  http.begin(url); http.setTimeout(8000);
  if(http.GET()==200){
    String body=http.getString();
    JsonDocument doc;
    if(deserializeJson(doc,body)==DeserializationError::Ok && doc.is<JsonArray>()){
      JsonArray arr=doc.as<JsonArray>();
      int lastKey=0;
      const char* lastSession="---";
      const char* lastCircuit="---";
      for(JsonObject s : arr){
        int k=s["session_key"]|0;
        if(k>lastKey){
          lastKey=k;
          lastSession=s["session_name"]|"---";
          lastCircuit=s["circuit_short_name"]|"---";
        }
      }
      if(lastKey>0){
        of1SessionKey=lastKey; of1HasSession=true;
        strlcpy(f1live.session,lastSession,20);
        strlcpy(f1live.circuit,lastCircuit,16);
      } else {
        of1HasSession=false;
        strlcpy(f1live.session,"No session today",20);
        f1live.valid=false;
      }
    }
  } else {
    of1HasSession=false; f1live.valid=false;
  }
  http.end();
}

void fetchOpenF1Live() {
  if(WiFi.status()!=WL_CONNECTED||!of1HasSession||of1SessionKey==0) return;

  HTTPClient http;
  char url[100];

  // Get latest lap number
  snprintf(url,sizeof(url),
    "https://api.openf1.org/v1/laps?session_key=%d&driver_number=1",
    of1SessionKey);
  http.begin(url); http.setTimeout(8000);
  int totalLaps=0;
  if(http.GET()==200){
    String body=http.getString();
    JsonDocument doc;
    if(deserializeJson(doc,body)==DeserializationError::Ok && doc.is<JsonArray>()){
      JsonArray arr=doc.as<JsonArray>();
      int sz=(int)arr.size();
      if(sz>0) totalLaps=arr[sz-1]["lap_number"]|0;
    }
  }
  http.end(); delay(100);

  // Get top positions
  snprintf(url,sizeof(url),
    "https://api.openf1.org/v1/position?session_key=%d&position%%3C%%3D3",
    of1SessionKey);
  http.begin(url); http.setTimeout(8000);
  if(http.GET()==200){
    String body=http.getString();
    JsonDocument doc;
    if(deserializeJson(doc,body)==DeserializationError::Ok && doc.is<JsonArray>()){
      JsonArray arr=doc.as<JsonArray>();
      // Walk array; later entries overwrite earlier ones for same position
      char p1d[8]="---", p2d[8]="---";
      for(JsonObject pos : arr){
        int position=pos["position"]|0;
        int dnum=pos["driver_number"]|0;
        if(position==1) snprintf(p1d,8,"%d",dnum);
        if(position==2) snprintf(p2d,8,"%d",dnum);
      }
      strlcpy(f1live.p1,p1d,8);
      strlcpy(f1live.p2,p2d,8);
    }
  }
  http.end(); delay(100);

  // Get gap
  snprintf(url,sizeof(url),
    "https://api.openf1.org/v1/intervals?session_key=%d",
    of1SessionKey);
  http.begin(url); http.setTimeout(8000);
  if(http.GET()==200){
    String body=http.getString();
    JsonDocument doc;
    if(deserializeJson(doc,body)==DeserializationError::Ok && doc.is<JsonArray>()){
      JsonArray arr=doc.as<JsonArray>();
      int sz=(int)arr.size();
      if(sz>0){
        float gap=arr[sz-1]["gap_to_leader"]|0.0f;
        snprintf(f1live.gap,16,"+%.3fs",gap);
      }
    }
  }
  http.end();

  f1live.lap=totalLaps;
  f1live.valid=true;

  // Safety car check
  snprintf(url,sizeof(url),
    "https://api.openf1.org/v1/race_control?session_key=%d&flag=SAFETY_CAR",
    of1SessionKey);
  http.begin(url); http.setTimeout(6000);
  bool prevSC=f1live.sc; f1live.sc=false;
  if(http.GET()==200){
    String body=http.getString();
    JsonDocument doc;
    if(deserializeJson(doc,body)==DeserializationError::Ok && doc.is<JsonArray>()){
      JsonArray arr=doc.as<JsonArray>();
      int sz=(int)arr.size();
      if(sz>0){
        const char* msg2=arr[sz-1]["message"]|"";
        if(strstr(msg2,"DEPLOYED")!=nullptr) f1live.sc=true;
      }
    }
  }
  http.end();
  if(f1live.sc&&!prevSC) applyMood(MOOD_SURPRISED,true,false);

  if(currentPage==PAGE_F1LIVE) pageF1Live();
  char dmsg[120];
  snprintf(dmsg,sizeof(dmsg),
    "{\"s\":\"f1live\",\"p1\":\"%s\",\"p2\":\"%s\",\"gap\":\"%s\","
    "\"lap\":%d,\"sc\":%s,\"session\":\"%s\",\"circuit\":\"%s\"}",
    f1live.p1,f1live.p2,f1live.gap,f1live.lap,
    f1live.sc?"true":"false",f1live.session,f1live.circuit);
  wsSend(dmsg);
}

// ══════════════════════════════════════════════════════════════
//  SPOTIFY — DIRECT ON ESP32
// ══════════════════════════════════════════════════════════════

// FIX #6: '\0' used correctly everywhere (original had garbled bytes)
void spSaveRefreshToken() {
  for(int i=0;i<256;i++) EEPROM.write(EE_SPOTIFY_TOK+i, (uint8_t)spRefreshToken[i]);
  EEPROM.commit();
}

void spLoadRefreshToken() {
  for(int i=0;i<255;i++) spRefreshToken[i]=(char)EEPROM.read(EE_SPOTIFY_TOK+i);
  spRefreshToken[255]='\0';   // FIX #6a: proper null terminator
  bool ok=(strlen(spRefreshToken)>20);
  for(int i=0; ok && spRefreshToken[i]; i++)
    if(!isAlphaNumeric((unsigned char)spRefreshToken[i]) &&
       spRefreshToken[i]!='_' && spRefreshToken[i]!='-')
      ok=false;
  if(!ok) spRefreshToken[0]='\0';  // FIX #6b: clear invalid token
  else    spAuthorized=true;
}

bool spExchangeToken(const char* codeOrRefresh, int mode) {
  if(WiFi.status()!=WL_CONNECTED) return false;
  HTTPClient http;
  http.begin("https://accounts.spotify.com/api/token");
  http.setTimeout(10000);
  http.addHeader("Content-Type","application/x-www-form-urlencoded");
  String auth = "Basic " + spCredB64();
  http.addHeader("Authorization", auth);

  String body;
  if(mode==0){
    body = "grant_type=authorization_code&code=";
    body += codeOrRefresh;
    body += "&redirect_uri=";
    body += SPOTIFY_REDIRECT_URI;
  } else {
    body = "grant_type=refresh_token&refresh_token=";
    body += codeOrRefresh;
  }

  int code = http.POST(body);
  bool ok = false;
  if(code==200){
    String resp = http.getString();
    JsonDocument doc;
    if(deserializeJson(doc,resp)==DeserializationError::Ok){
      const char* at = doc["access_token"];
      if(at){
        strlcpy(spAccessToken, at, 256);
        int expires_in = doc["expires_in"]|3600;
        spTokenExpiresMs = millis() + (unsigned long)(expires_in-60)*1000UL;
        ok=true; spAuthorized=true;
      }
      const char* rt = doc["refresh_token"];
      if(rt && strlen(rt)>5){
        strlcpy(spRefreshToken, rt, 256);
        spSaveRefreshToken();
      }
    }
  }
  http.end();
  return ok;
}

void fetchSpotify() {
  if(WiFi.status()!=WL_CONNECTED) return;
  if(!spAuthorized || strlen(spAccessToken)==0) return;

  if(millis() >= spTokenExpiresMs){
    if(strlen(spRefreshToken)>0){
      spExchangeToken(spRefreshToken, 1);
    }
    return;
  }

  HTTPClient http;
  http.begin("https://api.spotify.com/v1/me/player/currently-playing"
             "?market=LK&additional_types=track");
  http.setTimeout(8000);
  String authH = "Bearer "; authH += spAccessToken;
  http.addHeader("Authorization", authH);

  int code = http.GET();
  if(code==200){
    String resp = http.getString();
    JsonDocument doc;
    if(deserializeJson(doc,resp)==DeserializationError::Ok){
      bool isPlaying = doc["is_playing"]|false;
      const char* title  = doc["item"]["name"]|"---";
      const char* artist = doc["item"]["artists"][0]["name"]|"---";
      int  progressMs    = doc["progress_ms"]|0;
      int  durationMs    = doc["item"]["duration_ms"]|1;
      if(durationMs==0) durationMs=1;  // FIX #7: avoid div-by-zero
      int  progressPct   = (int)(100.0f*progressMs/durationMs);

      bool changed = (strcmp(title, mdata.title)!=0);
      strlcpy(mdata.title,  title,  32);
      strlcpy(mdata.artist, artist, 24);
      mdata.progress = progressPct;
      mdata.playing  = isPlaying;
      mdata.valid    = true;

      if(isPlaying && changed){
        char nb[32]; snprintf(nb,sizeof(nb),"%.28s",mdata.title);
        showNotify(nb,"music");
        applyMood(MOOD_HAPPY,false,false);
      }

      if(currentPage==PAGE_MUSIC) pageMusic();

      char msg[140];
      snprintf(msg,sizeof(msg),
        "{\"s\":\"music\",\"title\":\"%s\",\"artist\":\"%s\","
        "\"playing\":%s,\"pct\":%d}",
        mdata.title,mdata.artist,
        mdata.playing?"true":"false", mdata.progress);
      wsSend(msg);
    }
  } else if(code==204){
    mdata.playing=false;
    if(currentPage==PAGE_MUSIC) pageMusic();
  } else if(code==401){
    spTokenExpiresMs=0;
  }
  http.end();
}

void setupSpotifyRoutes() {
  otaServer.on("/spotify", HTTP_GET, [](){
    if(spAuthorized){
      String page = "<!DOCTYPE html><html><head><meta charset='utf-8'>"
        "<style>body{background:#06060f;color:#dde1ff;font-family:monospace;"
        "padding:30px;max-width:480px;margin:auto}"
        "h2{color:#1DB954}.ok{color:#1DB954}.btn{display:inline-block;"
        "background:#c0392b;color:#fff;padding:10px 20px;border-radius:6px;"
        "text-decoration:none;margin-top:14px}</style></head><body>"
        "<h2>&#9654; Spotify Connected</h2>"
        "<p class='ok'>&#10003; Authorised &mdash; polling every 15s</p>"
        "<p>Currently playing: <b>";
      page += mdata.valid ? mdata.title : "---";
      page += "</b> by <b>";
      page += mdata.valid ? mdata.artist : "---";
      page += "</b></p><a class='btn' href='/spotify/logout'>Disconnect Spotify</a>"
              "</body></html>";
      otaServer.send(200,"text/html",page);
      return;
    }
    char url[512];
    snprintf(url,sizeof(url),
      "https://accounts.spotify.com/authorize"
      "?response_type=code"
      "&client_id=%s"
      "&scope=user-read-currently-playing%%20user-read-playback-state"
      "&redirect_uri=%s"
      "&show_dialog=true",
      SPOTIFY_CLIENT_ID, SPOTIFY_REDIRECT_URI);
    String page = "<!DOCTYPE html><html><head><meta charset='utf-8'>"
      "<style>body{background:#06060f;color:#dde1ff;font-family:monospace;"
      "padding:30px;max-width:480px;margin:auto}"
      "h2{color:#1DB954}p{color:#888}"
      ".btn{display:inline-block;background:#1DB954;color:#000;"
      "padding:12px 28px;border-radius:24px;text-decoration:none;"
      "font-weight:bold;font-size:1.1em;margin-top:16px}"
      ".btn:hover{background:#1ed760}</style></head><body>"
      "<h2>&#129302; Emo Robot &mdash; Spotify</h2>"
      "<p>Connect your Spotify account to show now-playing on the OLED.</p>"
      "<p>You only need to do this <b>once</b> &mdash; the token is saved.</p>"
      "<a class='btn' href='";
    page += url;
    page += "'>&#9654; Login with Spotify</a>"
            "</body></html>";
    otaServer.send(200,"text/html",page);
  });

  otaServer.on("/callback", HTTP_GET, [](){
    String code = otaServer.arg("code");
    String error= otaServer.arg("error");
    if(error.length()>0 || code.length()==0){
      otaServer.send(200,"text/html",
        "<h2 style='color:red'>Spotify auth failed: "+error+"</h2>");
      return;
    }
    otaServer.send(200,"text/html",
      "<!DOCTYPE html><html><head><meta charset='utf-8'>"
      "<meta http-equiv='refresh' content='4;url=/spotify'>"
      "<style>body{background:#06060f;color:#dde1ff;font-family:monospace;"
      "padding:30px;max-width:480px;margin:auto}h2{color:#1DB954}</style></head><body>"
      "<h2>&#9654; Authorising...</h2><p>Exchanging token, please wait...</p>"
      "</body></html>");
    bool ok = spExchangeToken(code.c_str(), 0);
    if(ok){
      display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1); display.setCursor(10,20);
      display.print("Spotify connected!"); display.display();
      sndHappy(); delay(1500); showEyes();
    }
  });

  otaServer.on("/spotify/logout", HTTP_GET, [](){
    spAccessToken[0]='\0'; spRefreshToken[0]='\0';  // FIX #6c
    spAuthorized=false; spTokenExpiresMs=0;
    spSaveRefreshToken();
    mdata.valid=false; mdata.playing=false;
    otaServer.send(200,"text/html",
      "<meta http-equiv='refresh' content='2;url=/spotify'>"
      "<p style='color:#888;font-family:monospace;padding:20px'>"
      "Disconnected. Redirecting...</p>");
  });
}

// ══════════════════════════════════════════════════════════════
//  BOOT ANIMATIONS
// ══════════════════════════════════════════════════════════════
void wakeupAnim() {
  resetEyes();
  for(int h=0;h<=EYE_H;h+=4){
    display.clearDisplay();
    display.fillRoundRect(lx-EYE_W/2,ly-h/2,EYE_W,h,EYE_R,SSD1306_WHITE);
    display.fillRoundRect(rx-EYE_W/2,ry-h/2,EYE_W,h,EYE_R,SSD1306_WHITE);
    display.display(); delay(40);
  }
}
void bootScreen() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
  display.setCursor(16,0);  display.print("EMO ROBOT  v10");
  display.drawFastHLine(0,10,SCREEN_W,SSD1306_WHITE);
  display.setCursor(0,14);  display.print("ESP32 + SSD1306");
  display.setCursor(0,24);  display.print("Open-Meteo weather");
  display.setCursor(0,34);  display.print("F1: Ergast+OpenF1");
  display.setCursor(0,44);  display.print("Spotify integration");
  display.setCursor(0,54);  display.print("OTA: http://IP/update");
  display.display();
}

// ══════════════════════════════════════════════════════════════
//  OTA SETUP
// ══════════════════════════════════════════════════════════════
void setupOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([](){
    display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
    display.setCursor(22,12); display.print("OTA UPDATE");
    display.setCursor(8,28);  display.print("Do not power off!");
    display.display();
  });
  ArduinoOTA.onProgress([](unsigned int done, unsigned int total){
    int pct=(int)(done*100UL/total);
    display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
    display.setCursor(22,8);  display.print("OTA UPDATE");
    display.setTextSize(2);
    char buf[8]; snprintf(buf,sizeof(buf),"%d%%",pct);
    display.setCursor(40,24); display.print(buf);
    display.drawRect(4,46,120,10,SSD1306_WHITE);
    display.fillRect(5,47,(int)(pct*118/100),8,SSD1306_WHITE);
    display.display();
  });
  ArduinoOTA.onEnd([](){
    display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
    display.setCursor(20,20); display.print("Done! Rebooting...");
    display.display(); delay(1000);
  });
  ArduinoOTA.onError([](ota_error_t e){
    display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
    display.setCursor(10,20); display.print("OTA ERROR!");
    char eb[24]; snprintf(eb,sizeof(eb),"Code: %u",(unsigned)e);
    display.setCursor(10,36); display.print(eb);
    display.display(); delay(2000); showEyes();
  });
  ArduinoOTA.begin();

  // Web OTA
  otaServer.on("/", HTTP_GET, [](){
    otaServer.send(200,"text/html",
      "<!DOCTYPE html><html><head><meta charset='utf-8'>"
      "<title>Emo Robot OTA</title>"
      "<style>*{box-sizing:border-box}body{background:#06060f;color:#dde1ff;"
      "font-family:monospace;padding:32px;max-width:500px;margin:auto}"
      "h2{color:#7b4fff;letter-spacing:2px}p{color:#888}"
      "input[type=file]{background:#111;color:#dde1ff;border:1px solid #2a2a4a;"
      "padding:10px;border-radius:6px;width:100%;margin:12px 0;display:block}"
      "button{background:#7b4fff;color:#fff;border:none;padding:10px 24px;"
      "border-radius:6px;cursor:pointer;font-size:1em;width:100%}"
      "button:hover{background:#9b6fff}</style></head><body>"
      "<h2>&#129302; Emo Robot v10</h2>"
      "<p>Upload a <code>.bin</code> from Arduino IDE:<br>"
      "Sketch &rarr; Export Compiled Binary, then upload here.</p>"
      "<form method='POST' action='/update' enctype='multipart/form-data'>"
      "<input type='file' name='firmware' accept='.bin'>"
      "<button type='submit'>&#9889; Flash Firmware</button></form>"
      "<br><a href='/reboot' style='color:#ff4fa3'>Reboot</a>"
      "</body></html>");
  });

  otaServer.on("/update", HTTP_POST,
    [](){
      otaServer.sendHeader("Connection","close");
      bool ok=!Update.hasError();
      otaServer.send(200,"text/html",
        ok ? "<h2 style='color:green'>Flash OK! Rebooting...</h2>"
           : "<h2 style='color:red'>Flash FAILED.</h2>");
      delay(800); if(ok) ESP.restart();
    },
    [](){
      HTTPUpload& up=otaServer.upload();
      if(up.status==UPLOAD_FILE_START){
        Update.begin(UPDATE_SIZE_UNKNOWN);
        display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1); display.setCursor(10,20);
        display.print("Web OTA uploading..."); display.display();
      } else if(up.status==UPLOAD_FILE_WRITE){
        Update.write(up.buf,up.currentSize);
      } else if(up.status==UPLOAD_FILE_END){
        Update.end(true);
        display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1); display.setCursor(10,20);
        display.print("Done! Rebooting."); display.display(); delay(500);
      }
    }
  );

  otaServer.on("/reboot",HTTP_GET,[](){
    otaServer.send(200,"text/plain","Rebooting..."); delay(500); ESP.restart();
  });

  setupSpotifyRoutes();
  otaServer.begin();
}

// ══════════════════════════════════════════════════════════════
//  SETUP
// ══════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200); Serial.setTimeout(10);
  randomSeed(esp_random());

  Wire.begin(I2C_SDA, I2C_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)){
    pinMode(2,OUTPUT);
    while(true){ digitalWrite(2,HIGH);delay(200);digitalWrite(2,LOW);delay(200); }
  }
  display.clearDisplay(); display.display();
  resetEyes();

  eepromLoad();
  sndBoot(); delay(300);
  bootScreen(); delay(1400);
  wakeupAnim(); delay(200);
  eyeHappy(); delay(700);
  resetEyes(); showEyes();

  display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);  display.print("CONNECTING WiFi");
  display.drawFastHLine(0,10,SCREEN_W,SSD1306_WHITE);
  display.setCursor(0,14); display.print(WIFI_SSID);
  display.setCursor(0,26); display.print("Please wait...");
  display.display();

  WiFi.mode(WIFI_STA); WiFi.begin(WIFI_SSID, WIFI_PASS);
  int tries=0;
  while(WiFi.status()!=WL_CONNECTED && tries<40){ delay(500); tries++; }

  if(WiFi.status()==WL_CONNECTED){
    sndWifiOk();
    display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
    display.setCursor(0,0);  display.print("WiFi CONNECTED!");
    display.drawFastHLine(0,10,SCREEN_W,SSD1306_WHITE);
    display.setCursor(0,14); display.print(WiFi.localIP().toString().c_str());
    display.setCursor(0,26); display.print("WS :81  HTTP :80");
    display.setCursor(0,38); display.print("OTA: http://");
    display.setCursor(0,48); display.print(WiFi.localIP().toString().c_str());
    display.setCursor(0,58); display.print("/update  /spotify");
    display.display();
    Serial.printf("{\"s\":\"wifi_ok\",\"ip\":\"%s\"}\n",WiFi.localIP().toString().c_str());

    // NTP — FIX #1 applied: NTP1/NTP2 now defined
    configTime(TZ_OFFSET_SEC, 0, NTP1, NTP2);
    delay(1500);
    updateTime();

    wsServer.begin();
    wsServer.onEvent(wsEvent);

    setupOTA();  // FIX #8: setupOTA must come AFTER wsServer so otaServer routes work

    fetchWeather();
    fetchF1Standings();
    fetchOpenF1Session();
    fetchSpotify();

  } else {
    sndWifiFail();
    display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
    display.setCursor(0,0);  display.print("WiFi FAILED");
    display.drawFastHLine(0,10,SCREEN_W,SSD1306_WHITE);
    display.setCursor(0,16); display.print("Check credentials.");
    display.setCursor(0,28); display.print("Serial at 115200.");
    display.display();
    Serial.println(F("{\"s\":\"wifi_fail\"}"));
  }

  delay(2500);
  resetEyes(); showEyes();

  nowMs = millis();
  lastBlinkMs = lastIdleMs = lastPageMs = nowMs;
  lastWxFetch = lastF1Fetch = lastTimeTick = nowMs;
  lastMusicFetch = lastF1LiveFetch = lastBurnMs = nowMs;

  Serial.printf("{\"s\":\"ready\",\"v\":10,\"ip\":\"%s\"}\n",
    WiFi.localIP().toString().c_str());
}

// ══════════════════════════════════════════════════════════════
//  MAIN LOOP
// ══════════════════════════════════════════════════════════════
void loop() {
  nowMs = millis();

  wsServer.loop();
  ArduinoOTA.handle();
  otaServer.handleClient();
  tickSerial();
  tickBeeps();
  tickSong();
  tickBlink();
  tickMood();
  handleTouch();
  tickScroll();
  tickNotify();
  tickSnake();
  tickBurn();

  if(nowMs-lastTimeTick >= TIME_IV){
    lastTimeTick=nowMs;
    updateTime();
    if(currentPage==PAGE_CLOCK && !moodActive && !scrollActive && !notifyActive)
      pageClock();
  }

  if(cdActive && nowMs>=cdEndMs){
    cdActive=false; sndAlarm();
    showNotify("TIME'S UP!","alarm");
    applyMood(MOOD_EXCITED,false,false);
  }

  if(sleepMode) return;

  if(!moodActive && nowMs-lastBlinkMs>BLINK_IV_MS){
    lastBlinkMs=nowMs;
    if(currentMood==MOOD_IDLE && gameState==GAME_OFF) startBlink();
  }

  if(!moodActive && nowMs-lastIdleMs>IDLE_IV && gameState==GAME_OFF){
    lastIdleMs=nowMs; doIdle();
  }

  if(!moodActive&&!blinkActive&&!scrollActive&&!notifyActive&&gameState==GAME_OFF){
    if(nowMs-lastPageMs>PAGE_IV){
      lastPageMs=nowMs;
      if(currentPage!=PAGE_EYES) showPage();
    }
  }

  if(WiFi.status()==WL_CONNECTED){
    if(nowMs-lastWxFetch >= WX_IV){
      lastWxFetch=nowMs; fetchWeather();
    }
    // FIX #9: F1 standings and session check had duplicate timer condition;
    //         session check now has its own separate timer
    if(nowMs-lastF1Fetch >= F1_IV){
      lastF1Fetch=nowMs;
      fetchF1Standings();
      fetchOpenF1Session();   // re-check session at same interval
    }
    if(of1HasSession && nowMs-lastF1LiveFetch >= F1LIVE_IV){
      lastF1LiveFetch=nowMs;
      fetchOpenF1Live();
    }
    if(nowMs-lastMusicFetch >= MUSIC_IV){
      lastMusicFetch=nowMs; fetchSpotify();
    }
  }
}
