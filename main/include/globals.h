#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdbool.h>
#include <stdint.h>

// --- PINOUT (Joystick fisik opsional) ---
#define PIN_LEFT  40
#define PIN_RIGHT 39
#define PIN_OK    38

#define BTN_NONE  0
#define BTN_LEFT  1
#define BTN_RIGHT 2
#define BTN_OK    3

// --- STRUKTUR DATA WIFI ---
typedef struct {
    int id;
    char ssid[33];
    int rssi;
    int channel;
    char encrypt[20];
    bool is_open;
    char mac[18];
} WiFiData;

typedef struct {
    int id;
    uint8_t mac[6];
    int rssi;
    int paket_count;
} StationInfo;

// --- EVIL TWIN ---
extern bool isEvilTwin;
extern int evilTwinState;
extern char stolenPassword[64];
extern bool triggerEvilTwin;

// --- WIFI STATE ---
extern bool isWiFiConnected;
extern char connSSID[33];
extern int connRSSI;
extern int connCH;
extern bool triggerDisconnect;
extern int statusKoneksi;
extern char inputPassword[64];
extern int cursorPass;
extern bool triggerConnect;
extern bool triggerTrack;
extern int deauthProgress;
extern bool inSubMenu;
extern int currentMenu;
extern int currentSub;
extern int topMenu;
extern WiFiData listWiFi[30];
extern StationInfo listStation[30];
extern StationInfo targetSta;
extern WiFiData targetTerkunci;
extern int brightnessValue;
extern int spamState;
extern bool isSpamming;
extern int aktifModeSpam;
extern bool spamUdahSetup;
extern bool deauthUdahSetup;
extern int scannerState;
extern int scannerStateSta;
extern uint32_t popUpTimer;
extern bool triggerScan;
extern bool triggerScanSta;
extern bool scanDone;
extern bool scanStaDone;
extern int totalWiFi;
extern int totalStation;
extern int cursorInScanner;
extern int cursorInScanSta;
extern int scrollPosScanner;
extern int targetLockedIdx;
extern int contextCursor;
extern bool adaTarget;
extern bool adaTargetSta;
extern int deauthState;
extern bool isDeauthing;
extern bool isDeauthSta;
extern bool sedang_scan;
extern int appMode;

// --- STARS / CAROUSEL ---
extern int starX[5];
extern int starY[5];
extern int carouselCurrentIdx;
extern int carouselDirection;
extern bool carouselAnimating;
extern uint32_t carouselAnimStart;

// --- WEB CONFIG ---
#define GHOSTCORE_AP_SSID    "iPhone"
#define GHOSTCORE_AP_PASS    "andymbot"
#define GHOSTCORE_AP_HIDDEN   1
#define GHOSTCORE_AP_CHANNEL  6

// --- FUNCTION PROTOTYPES ---
void start_dns_server(void);
void start_web_server(void);
void ghostcore_web_init(void);

#endif
