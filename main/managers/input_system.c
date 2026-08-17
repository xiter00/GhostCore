#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "globals.h"
#include "esp_wifi.h"
#include <string.h>





// Pengganti millis()
uint32_t input_millis() {
    return (uint32_t)(esp_timer_get_time() / 1000);
}







// Deklarasi fungsi di bawah
void handleNavigasiScanner(int btn);
void handleNavigasiDeauth(int btn);
void handleNavigasiSpam(int btn);
void handleNavigasiScanSta(int btn);
void handleEvilTwinInput(int btn);








void handleJoystick() {
    static uint32_t lastPress = 0;
    uint32_t debounceLimit = (appMode == 8) ? 120 : 250; 
    if (input_millis() - lastPress < debounceLimit) return; 

    // --- 1. TENTUIN DULU BTN NYA (Cuma 3 tombol fisik: LEFT, RIGHT, OK) ---
    int btn = BTN_NONE;
    if (gpio_get_level(PIN_LEFT) == 0)       btn = BTN_LEFT;
    else if (gpio_get_level(PIN_RIGHT) == 0) btn = BTN_RIGHT;
    else if (gpio_get_level(PIN_OK) == 0)    btn = BTN_OK;

    if (btn == BTN_NONE) return; 



    // --- HELPER LAINNYA (UDAH AMAN KARENA ADA RETURN) ---
    if (appMode == 1) { handleNavigasiScanner(btn); lastPress = input_millis(); return; }
    if (appMode == 2) { handleNavigasiDeauth(btn);  lastPress = input_millis(); return; }
    if (appMode == 4) { handleNavigasiSpam(btn);    lastPress = input_millis(); return; } 
    if (appMode == 5) { handleNavigasiScanSta(btn); lastPress = input_millis(); return; }
    if (appMode == 8) { handleEvilTwinInput(btn);   lastPress = input_millis(); return; }
    
    if (appMode == 6) {
        if (btn == BTN_LEFT) { scannerState = 4;  appMode = 1; triggerTrack = false; }
        lastPress = input_millis();
        return;
    }
    
    if (appMode == 7) {
        if (btn == BTN_LEFT) { 
            esp_wifi_stop(); 
            isDeauthSta = false;
            appMode = 5; 
        }
        lastPress = input_millis();
        return;
    }
    
    if (appMode == 3) {
     if (btn == BTN_LEFT) { 
            appMode = 1;
            currentMenu = 2;
            currentSub = 0;
        }
        lastPress = input_millis();
        return;
    }
    
    if (appMode == 14) {
     if (btn == BTN_LEFT) { 
            appMode = 1;
            currentMenu = 2;
            currentSub = 1;
        }
        lastPress = input_millis();
        return;

    }
    
    if (appMode == 15) {
     if (btn == BTN_LEFT) { 
            appMode = 1;
            currentMenu = 2;
            currentSub = 1;
        
        lastPress = input_millis();
        return;
        }
        
        else if (btn == BTN_OK) {
            esp_restart();
        lastPress = input_millis();
        return;
        }
    }




    // ==========================================
    // 3. LOGIKA MENU UTAMA (APPMODE == 0)
    // ==========================================
    // DIBUNGKUS IF(0) BIAR MENU LAIN GAK NGERUSAK KURSOR DI SINI!

    if (appMode == 0) {
        if (!inSubMenu) {
          if (btn == BTN_RIGHT) {
    carouselCurrentIdx = (carouselCurrentIdx + 1) % 3;
    currentMenu = (currentMenu + 1) % 3;
    carouselDirection = 1;
    carouselAnimating = true;
    carouselAnimStart = input_millis();
    lastPress = input_millis();
    return;
}
else if (btn == BTN_LEFT) {
    carouselCurrentIdx = (carouselCurrentIdx - 1 + 3) % 3;
    currentMenu = (currentMenu -1 + 3) % 3;
    carouselDirection = -1;
    carouselAnimating = true;
    carouselAnimStart = input_millis();
    lastPress = input_millis();
    return;
}
    else if (btn == BTN_OK) {
        inSubMenu = true;
        currentMenu = carouselCurrentIdx;
        currentSub = 0;
        lastPress = input_millis();
        return;
    }
        }
        else { // DI DALAM LIST SUBMENU
                        if (btn == BTN_RIGHT) {
                // Cuma satu arah (RIGHT). Kalo udah nyampe item paling bawah, balik ke atas lagi (wrap).
                int limitMenu = 0; 
                if(currentMenu == 0)      limitMenu = 4; 
                else if(currentMenu == 1) limitMenu = 3;
                else if(currentMenu == 2) limitMenu = 3; // Settings: Brightness, About, Reboot
                else limitMenu = 3;


                if (currentSub < (limitMenu - 1)) { 
                    currentSub++;
                    if (currentSub >= topMenu + 5) topMenu++;
                } else {
                    // Nyampe batas bawah -> balik ke atas
                    currentSub = 0;
                    topMenu = 0;
                }
            }
            else if (btn == BTN_LEFT) {
                inSubMenu = false; // BALIK KE LOGO RootX
            }
            else if (btn == BTN_OK) {
                if (currentMenu == 0 && currentSub == 0) {
                    appMode = 1;      
                    scannerState = 0; 
                } else if (currentMenu == 0 && currentSub == 1) {
                    appMode = 1;
                    scannerState = 2;     
                    cursorInScanner = 0;  
                    scrollPosScanner = 0; 
                } else if (currentMenu == 0 && currentSub == 2) {
                    aktifModeSpam = 1; 
                    appMode = 4;       
                    spamState = 0;
                } else if (currentMenu == 0 && currentSub == 3) {
                    aktifModeSpam = 2; 
                    appMode = 4;
                    spamState = 0;
                } else if (currentMenu == 2 && currentSub == 0) { 
                    appMode = 3; // Brightness
                } else if (currentMenu == 2 && currentSub == 1) {
                    appMode = 14; // About RootX
                } else if (currentMenu == 2 && currentSub == 2) {
                    appMode = 15; // Reboot
                }
            }
        }
        lastPress = input_millis();
        return;
    }
}












void handleNavigasiScanner(int btn) {
    if (scannerState == 0) {
        if (btn == BTN_LEFT) appMode = 0; 
        else if (btn == BTN_RIGHT || btn == BTN_OK) { 
            scannerState = 1;     
            triggerScan = true;   
            scanDone = false;     
            cursorInScanner = 0;  
            scrollPosScanner = 0; 
        }
    }
    else if (scannerState == 1) {
        if (btn == BTN_LEFT) scannerState = 0; 
    }
    else if (scannerState == 2) {
        if (btn == BTN_RIGHT) {
            if (cursorInScanner < 2 && (scrollPosScanner + cursorInScanner) < (totalWiFi - 1)) cursorInScanner++;
            else if ((scrollPosScanner + 3) < totalWiFi) scrollPosScanner++;
            else { cursorInScanner = 0; scrollPosScanner = 0; } // Nyampe bawah -> balik ke atas
        }
        else if (btn == BTN_OK) {
            if (totalWiFi > 0) {
                targetLockedIdx = scrollPosScanner + cursorInScanner; 
                targetTerkunci = listWiFi[targetLockedIdx];
                adaTarget = true; 
               // triggerTrackWifi = true;
                scannerState = 4;   
                contextCursor = 0;  
            }
        }
        else if (btn == BTN_LEFT) {
            scannerState = 0; 
            appMode = 0;      
        }
    }
    else if (scannerState == 3) {
        if (btn == BTN_LEFT) scannerState = 4; 
    }
      else if (scannerState == 4) {
        // Navigasi (cuma RIGHT), nyampe bawah balik ke atas
        if (btn == BTN_RIGHT) {
            contextCursor = (contextCursor < 4) ? contextCursor + 1 : 0;
        }
        
        else if (btn == BTN_OK) {
        if (contextCursor == 0) { appMode = 2; deauthState = 0; } 
        else if (contextCursor == 1) {
           appMode = 8;
        }
        else if (contextCursor == 2) { appMode = 5; scannerStateSta = 0; contextCursor = 0; }
        else if (contextCursor == 3) { // TRACK
            appMode = 6; 
            triggerTrack = true; // Nyalain update RSSI
        }
        else if (contextCursor == 4) { scannerState = 3; }
    }
        else if (btn == BTN_LEFT) scannerState = 2; 
    }

}




void handleNavigasiScanSta(int btn) {
    if (scannerStateSta == 0) {
        if (btn == BTN_LEFT) appMode = 1; // Balik ke WiFi Scanner
        else if (btn == BTN_RIGHT || btn == BTN_OK) {
            totalStation = 0;
            scannerStateSta = 1;
            triggerScanSta = true;
            scanStaDone = false;
        }
    } 
    else if (scannerStateSta == 2) {
        if (btn == BTN_LEFT) { appMode = 1;  triggerScanSta = false; }// Mau scan lagi
        else if (btn == BTN_RIGHT) {
            if (cursorInScanSta < 2 && (scrollPosScanner + cursorInScanSta) < (totalStation - 1)) cursorInScanSta++; 
            else if ((scrollPosScanner + 3) < totalStation) scrollPosScanner++;
            else { cursorInScanSta = 0; scrollPosScanner = 0; } // Nyampe bawah -> balik ke atas
        }
        else if (btn == BTN_OK) {
            if (totalStation > 0) {
                targetLockedIdx = scrollPosScanner + cursorInScanSta; 
                targetSta = listStation[targetLockedIdx];
                adaTargetSta = true; 
                scannerStateSta = 4;   
                contextCursor = 0;  
            }
        }
    } 
    else if (scannerStateSta == 3) {
        if (btn == BTN_LEFT || btn == BTN_OK) scannerStateSta = 4; 
    } 
    else if (scannerStateSta == 4) {
        if (btn == BTN_RIGHT) contextCursor = (contextCursor < 1) ? contextCursor + 1 : 0;
        else if (btn == BTN_OK) {
            if (contextCursor == 0) {
                // MULAI ATTACK KE HP
                isDeauthSta = true; 
                scannerStateSta = 0;
                appMode = 7; // Pindah layar ke animasi Deauth
            } 
            else if (contextCursor == 1) {
                scannerStateSta = 3; // Lihat Detail
            }
        }
        else if (btn == BTN_LEFT) { scannerStateSta = 2;}
    }
}


void handleNavigasiDeauth(int btn) {
    if (deauthState == 0) { 
        if (btn == BTN_LEFT) appMode = 0; 
        else if (btn == BTN_RIGHT || btn == BTN_OK) { 
            deauthState = 1;
            isDeauthing = true;
        }
    } 
    else if (deauthState == 1) { 
        if (btn == BTN_LEFT) { 
            esp_wifi_stop(); 
            isDeauthing = false;
            deauthState = 0;
            appMode = 1; 
        }
    }
}

// --- TARUH INI DI LUAR FUNGSI (Di atas handleInputPassword) ---


void handleNavigasiSpam(int btn) {
    if (spamState == 0) { 
        if (btn == BTN_RIGHT || btn == BTN_OK) { 
            spamState = 1;
            isSpamming = true; 
        } 
        else if (btn == BTN_LEFT) { 
            esp_wifi_stop(); 
            appMode = 0; 
            isSpamming = false;
            aktifModeSpam = 0; 
        }
    } 
    else if (spamState == 1) { 
        if (btn == BTN_LEFT) { 
            isSpamming = false;
            spamState = 0;
            appMode = 0; 
            aktifModeSpam = 0;
        }
    }
}


void handleEvilTwinInput(int btn) {
    if (evilTwinState == 0) {
        if (btn == BTN_RIGHT || btn == BTN_OK) {
            triggerEvilTwin = true;
            
        } else if (btn == BTN_LEFT) {
            appMode = 1; // Balik ke menu scanner
        }
    } else if (evilTwinState == 2) {
        if (btn == BTN_LEFT || btn == BTN_OK) {
            isEvilTwin = false;
            esp_wifi_stop();
            appMode = 1;
        } 
    } else if (evilTwinState == 1) { if (btn == BTN_LEFT) {
            appMode = 1; // Balik ke menu scanner
            isEvilTwin = false;
            esp_wifi_stop();
        }}
}


