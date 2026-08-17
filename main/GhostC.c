/**
 * GhostCore - GhostC.c
 * Main entry point.
 * 
 * Arsitektur:
 *   - task_display : polling joystick fisik + memicu ghostcore_web_init()
 *   - loopWiFi     : semua operasi WiFi (scan, deauth, spam, evil twin, track)
 * 
 * Tidak ada OLED. Semua UI via hotspot hidden ESP32 -> Web Browser.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "globals.h"

static const char *TAG = "GhostCore";

extern void loopWiFi(void *pvParameters);
extern void task_display(void *pvParameters);

// ========================================================
// IEEE80211 raw frame bypass (untuk deauth & beacon spam)
// ========================================================
int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    return 0;
}

// ========================================================
// DEFINISI VARIABEL GLOBAL
// ========================================================
bool triggerTrack     = false;
bool isEvilTwin       = false;
int  evilTwinState    = 0;
char stolenPassword[64] = "";
bool triggerEvilTwin  = false;

char inputPassword[64] = {0};
int  cursorPass        = 0;
int  statusKoneksi     = 2; // 2 = idle
bool isWiFiConnected   = false;
char connSSID[33]      = {0};
int  connCH            = 0;
int  connRSSI          = 0;
bool triggerConnect    = false;
bool triggerDisconnect = false;

int  deauthProgress    = 0;
bool adaTargetSta      = false;
bool isDeauthSta       = false;
bool inSubMenu         = false;
int  currentMenu       = 0;
int  currentSub        = 0;
int  topMenu           = 0;

WiFiData    listWiFi[30];
StationInfo listStation[30];
int  brightnessValue   = 150;  // tidak dipakai (no OLED), retained for compat
int  spamState         = 0;
bool isSpamming        = false;
int  aktifModeSpam     = 0;
bool spamUdahSetup     = false;
bool deauthUdahSetup   = false;
int  scannerState      = 0;
int  scannerStateSta   = 0;
uint32_t popUpTimer    = 0;
bool triggerScan       = false;
bool triggerScanSta    = false;
bool scanDone          = false;
bool scanStaDone       = false;
int  totalWiFi         = 0;
int  totalStation      = 0;
int  cursorInScanner   = 0;
int  cursorInScanSta   = 0;
int  scrollPosScanner  = 0;
int  targetLockedIdx   = -1;
int  contextCursor     = 0;

StationInfo targetSta;
WiFiData    targetTerkunci;
bool adaTarget         = false;
int  deauthState       = 0;
bool isDeauthing       = false;
bool sedang_scan       = false;
int  appMode           = 0;

TaskHandle_t TaskWiFi;

// Carousel & star state (retained for input_system compat)
int  carouselCurrentIdx  = 0;
int  carouselDirection   = 0;
bool carouselAnimating   = false;
uint32_t carouselAnimStart = 0;
int  starX[5]            = {0};
int  starY[5]            = {0};

// ========================================================
// APP MAIN
// ========================================================
void app_main(void) {
    ESP_LOGI(TAG, "GhostCore booting...");

    // NVS init (dibutuhkan WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS dirty, erasing...");
        nvs_flash_erase();
        nvs_flash_init();
    }

    // Netif & event loop (ghostcore_web_init juga init ini,
    // tapi lebih aman init di sini biar tidak double-init)
    esp_netif_init();
    esp_event_loop_create_default();

    // task_display: inisialisasi web UI + polling joystick fisik
    xTaskCreate(task_display, "DisplayTask", 8192, NULL, 1, NULL);

    // loopWiFi: semua operasi WiFi background
    xTaskCreate(loopWiFi, "TaskWiFi", 16384, NULL, 1, &TaskWiFi);

    ESP_LOGI(TAG, "Tasks launched. GhostCore running.");
}
