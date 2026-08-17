/**
 * GhostCore - boot_system.c
 * 
 * Boot sequence tanpa OLED.
 * Semua output ke Serial log (ESP_LOGI).
 * Web UI handle boot feedback via /api/sysinfo.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_random.h"
#include "globals.h"
#include "photo_data.h"

static const char *TAG = "GhostCore-Boot";

// ====================================================
// oled_draw_bitmap stub - dipertahankan agar fungsi
// lain yang masih referensi tidak error compile,
// tapi tidak melakukan apa-apa (no OLED).
// ====================================================
void oled_draw_bitmap(uint8_t id, int16_t x, int16_t y,
                      const uint8_t *bitmap, int16_t w, int16_t h,
                      int color) {
    // NO-OP: OLED dihapus, semua display via Web UI
    (void)id; (void)x; (void)y; (void)bitmap;
    (void)w; (void)h; (void)color;
}

// ====================================================
// Boot sequence via Serial log
// ====================================================
void tampilkanLogoDulu(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ██████╗ ██╗  ██╗ ██████╗ ███████╗████████╗");
    ESP_LOGI(TAG, " ██╔════╝ ██║  ██║██╔═══██╗██╔════╝╚══██╔══╝");
    ESP_LOGI(TAG, " ██║  ███╗███████║██║   ██║███████╗   ██║   ");
    ESP_LOGI(TAG, " ██║   ██║██╔══██║██║   ██║╚════██║   ██║   ");
    ESP_LOGI(TAG, " ╚██████╔╝██║  ██║╚██████╔╝███████║   ██║   ");
    ESP_LOGI(TAG, "  ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚══════╝   ╚═╝   ");
    ESP_LOGI(TAG, "  CORE  -  ESP32 Web Control Panel  v1.0");
    ESP_LOGI(TAG, "========================================");
    vTaskDelay(pdMS_TO_TICKS(200));
}

void tampilkanIntroAnime(void) {
    ESP_LOGI(TAG, "[BOOT] Initializing hardware...");
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "[BOOT] Checking Flash... OK");
    vTaskDelay(pdMS_TO_TICKS(80));
    ESP_LOGI(TAG, "[BOOT] Loading modules... OK");
    vTaskDelay(pdMS_TO_TICKS(80));
}

void tampilkanTeksSplash(void) {
    ESP_LOGI(TAG, ">> Initializing WiFi stack...");
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, ">> Starting hidden AP...");
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, ">> Launching web server...");
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, ">> GHOSTCORE READY!!");
    ESP_LOGI(TAG, ">> Connect ke WiFi: iPhone (hidden)");
    ESP_LOGI(TAG, ">> Password: andymbot");
    ESP_LOGI(TAG, ">> Buka browser: http://192.168.4.1");
}
