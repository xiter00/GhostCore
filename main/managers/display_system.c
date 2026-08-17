/**
 * GhostCore - display_system.c
 * 
 * Semua logika tampilan sekarang dihandle via Web UI lokal.
 * File ini hanya menyimpan state mesin (appMode, menu, dll)
 * yang dibaca oleh web server untuk dikirim ke browser sebagai JSON.
 * 
 * TIDAK ADA OLED - semua output via hotspot ESP32 -> browser.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "globals.h"

static const char *TAG = "GhostCore-Display";

// ====================================================
// INISIALISASI JOYSTICK (tetap ada untuk kontrol fisik)
// ====================================================
void init_joystick() {
    int pins[] = {PIN_LEFT, PIN_RIGHT, PIN_OK};
    for (int i = 0; i < 3; i++) {
        gpio_set_direction(pins[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(pins[i], GPIO_PULLUP_ONLY);
    }
    ESP_LOGI(TAG, "Joystick initialized (3 buttons: LEFT, RIGHT, OK)");
}

// ====================================================
// UTILITY
// ====================================================
uint32_t display_millis() {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

// ====================================================
// DISPLAY TASK - sekarang hanya jaga state & input fisik
// Web server yang handle render ke browser
// ====================================================
extern void handleJoystick(void);
extern void ghostcore_web_init(void);

bool introDone = false;

void task_display(void *pvParameters) {
    init_joystick();
    
    ESP_LOGI(TAG, "GhostCore Web UI booting...");

    // Boot delay singkat
    vTaskDelay(pdMS_TO_TICKS(500));

    // Inisialisasi Web Server (hotspot hidden + HTTP server)
    ghostcore_web_init();

    introDone = true;
    ESP_LOGI(TAG, "Web UI Ready. Connect ke hotspot GhostCore lalu buka http://192.168.4.1");

    for (;;) {
        // Handle input fisik joystick (opsional, tetap bisa kontrol dari tombol)
        handleJoystick();

        // Delay ~30fps untuk polling input
        vTaskDelay(pdMS_TO_TICKS(33));
    }
}
