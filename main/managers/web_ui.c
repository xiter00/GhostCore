/**
 * GhostCore - web_ui.c
 * 
 * Web server lokal yang berjalan di ESP32.
 * Hotspot hidden (SSID: GhostCore) -> Connect -> buka http://192.168.4.1
 * Semua kontrol via REST API JSON.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_random.h"
#include "globals.h"

static const char *TAG = "GhostCore-Web";
static httpd_handle_t g_server = NULL;

// ====================================================
// HTML UI - SINGLE PAGE APP (Embedded dalam firmware)
// ====================================================
static const char GHOSTCORE_HTML[] =
"<!DOCTYPE html>"
"<html lang='id'><head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no'>"
"<title>GhostCore Panel</title>"
"<style>"
":root{"
"--bg:#080c14;"
"--surface:#0d1117;"
"--surface2:#161b27;"
"--surface3:#1c2333;"
"--border:#21293d;"
"--accent:#00d4ff;"
"--accent2:#7c3aed;"
"--accent3:#10b981;"
"--danger:#ef4444;"
"--warn:#f59e0b;"
"--text:#e2e8f0;"
"--text2:#94a3b8;"
"--text3:#475569;"
"--glow:0 0 20px rgba(0,212,255,0.15);"
"--glowHot:0 0 30px rgba(239,68,68,0.3);"
"}"
"*{margin:0;padding:0;box-sizing:border-box;-webkit-tap-highlight-color:transparent}"
"body{background:var(--bg);color:var(--text);font-family:'SF Pro Display','Segoe UI',system-ui,sans-serif;min-height:100vh;overflow-x:hidden}"
"body::before{content:'';position:fixed;inset:0;background:radial-gradient(ellipse at 20% 0%,rgba(0,212,255,0.05) 0%,transparent 50%),radial-gradient(ellipse at 80% 100%,rgba(124,58,237,0.05) 0%,transparent 50%);pointer-events:none;z-index:0}"

/* Scrollbar */
"::-webkit-scrollbar{width:4px}::-webkit-scrollbar-track{background:var(--surface)}::-webkit-scrollbar-thumb{background:var(--border);border-radius:2px}"

/* Header */
".header{position:sticky;top:0;z-index:100;background:rgba(8,12,20,0.85);backdrop-filter:blur(20px);-webkit-backdrop-filter:blur(20px);border-bottom:1px solid var(--border);padding:12px 16px;display:flex;align-items:center;justify-content:space-between}"
".logo{display:flex;align-items:center;gap:10px}"
".logo-icon{width:32px;height:32px;background:linear-gradient(135deg,var(--accent),var(--accent2));border-radius:8px;display:flex;align-items:center;justify-content:center;font-size:16px;box-shadow:0 0 15px rgba(0,212,255,0.3)}"
".logo-text{font-size:18px;font-weight:700;background:linear-gradient(135deg,var(--accent),#fff);-webkit-background-clip:text;-webkit-text-fill-color:transparent;letter-spacing:-0.5px}"
".logo-sub{font-size:10px;color:var(--text3);letter-spacing:2px;text-transform:uppercase;margin-top:-2px}"
".header-status{display:flex;align-items:center;gap:8px}"
".status-dot{width:8px;height:8px;border-radius:50%;background:var(--accent3);box-shadow:0 0 8px var(--accent3);animation:pulse 2s infinite}"
".status-text{font-size:11px;color:var(--text2)}"
"@keyframes pulse{0%,100%{opacity:1}50%{opacity:0.5}}"

/* Main */
".main{padding:16px;max-width:480px;margin:0 auto;position:relative;z-index:1}"

/* Section */
".section{margin-bottom:20px}"
".section-title{font-size:10px;font-weight:600;color:var(--text3);letter-spacing:2px;text-transform:uppercase;margin-bottom:10px;padding-left:4px}"

/* Card */
".card{background:var(--surface);border:1px solid var(--border);border-radius:16px;overflow:hidden;transition:border-color 0.2s}"
".card:hover{border-color:rgba(0,212,255,0.2)}"
".card-header{padding:14px 16px;display:flex;align-items:center;gap:12px;border-bottom:1px solid var(--border)}"
".card-icon{width:36px;height:36px;border-radius:10px;display:flex;align-items:center;justify-content:center;font-size:18px;flex-shrink:0}"
".card-icon.wifi{background:rgba(0,212,255,0.1);border:1px solid rgba(0,212,255,0.2)}"
".card-icon.attack{background:rgba(239,68,68,0.1);border:1px solid rgba(239,68,68,0.2)}"
".card-icon.spam{background:rgba(245,158,11,0.1);border:1px solid rgba(245,158,11,0.2)}"
".card-icon.evil{background:rgba(124,58,237,0.1);border:1px solid rgba(124,58,237,0.2)}"
".card-icon.info{background:rgba(16,185,129,0.1);border:1px solid rgba(16,185,129,0.2)}"
".card-title{font-size:14px;font-weight:600;color:var(--text)}"
".card-desc{font-size:11px;color:var(--text3);margin-top:2px}"
".card-body{padding:14px 16px}"

/* WiFi List */
".wifi-item{display:flex;align-items:center;padding:10px 12px;border-radius:10px;cursor:pointer;transition:all 0.15s;margin-bottom:6px;border:1px solid transparent;position:relative;overflow:hidden}"
".wifi-item::before{content:'';position:absolute;inset:0;background:linear-gradient(90deg,transparent,rgba(0,212,255,0.03),transparent);transform:translateX(-100%);transition:transform 0.4s}"
".wifi-item:hover::before{transform:translateX(100%)}"
".wifi-item:hover{background:var(--surface2);border-color:var(--border)}"
".wifi-item.selected{background:rgba(0,212,255,0.08);border-color:rgba(0,212,255,0.3)}"
".wifi-signal{width:28px;height:28px;border-radius:8px;background:var(--surface2);display:flex;align-items:center;justify-content:center;font-size:13px;flex-shrink:0;margin-right:10px}"
".wifi-info{flex:1;min-width:0}"
".wifi-ssid{font-size:13px;font-weight:500;color:var(--text);white-space:nowrap;overflow:hidden;text-overflow:ellipsis}"
".wifi-meta{font-size:10px;color:var(--text3);margin-top:2px;display:flex;gap:8px}"
".wifi-badge{padding:2px 6px;border-radius:4px;font-size:9px;font-weight:600;letter-spacing:0.5px}"
".badge-open{background:rgba(16,185,129,0.15);color:var(--accent3)}"
".badge-wpa{background:rgba(245,158,11,0.1);color:var(--warn)}"
".rssi-bar{display:flex;align-items:center;gap:3px;margin-left:8px}"
".rssi-bar span{width:3px;border-radius:2px;background:var(--border);transition:background 0.3s}"

/* Buttons */
".btn{display:flex;align-items:center;justify-content:center;gap:8px;padding:11px 18px;border-radius:10px;font-size:13px;font-weight:600;border:none;cursor:pointer;transition:all 0.15s;letter-spacing:0.3px;white-space:nowrap}"
".btn:active{transform:scale(0.97)}"
".btn-primary{background:linear-gradient(135deg,var(--accent),#0099cc);color:#000;box-shadow:0 4px 15px rgba(0,212,255,0.2)}"
".btn-primary:hover{box-shadow:0 4px 25px rgba(0,212,255,0.35)}"
".btn-danger{background:linear-gradient(135deg,var(--danger),#c53030);color:#fff;box-shadow:0 4px 15px rgba(239,68,68,0.2)}"
".btn-danger:hover{box-shadow:0 4px 25px rgba(239,68,68,0.35)}"
".btn-warn{background:linear-gradient(135deg,var(--warn),#d97706);color:#000;box-shadow:0 4px 15px rgba(245,158,11,0.2)}"
".btn-purple{background:linear-gradient(135deg,var(--accent2),#5b21b6);color:#fff;box-shadow:0 4px 15px rgba(124,58,237,0.2)}"
".btn-ghost{background:var(--surface2);color:var(--text2);border:1px solid var(--border)}"
".btn-ghost:hover{background:var(--surface3);border-color:rgba(0,212,255,0.2);color:var(--text)}"
".btn-sm{padding:7px 12px;font-size:12px;border-radius:8px}"
".btn-full{width:100%}"
".btn-row{display:flex;gap:8px}"

/* Target Card */
".target-card{background:var(--surface2);border:1px solid rgba(0,212,255,0.2);border-radius:12px;padding:12px 14px;margin-bottom:14px;position:relative;overflow:hidden}"
".target-card::before{content:'TARGET TERKUNCI';position:absolute;top:8px;right:10px;font-size:9px;font-weight:700;color:var(--accent);letter-spacing:1px;opacity:0.7}"
".target-ssid{font-size:15px;font-weight:700;color:var(--accent);margin-bottom:6px}"
".target-details{display:grid;grid-template-columns:1fr 1fr;gap:4px}"
".target-detail-item{font-size:11px;color:var(--text3)}"
".target-detail-item span{color:var(--text2)}"

/* Attack Status */
".attack-active{background:rgba(239,68,68,0.08);border:1px solid rgba(239,68,68,0.3);border-radius:12px;padding:14px;margin-bottom:14px}"
".attack-title{font-size:12px;font-weight:700;color:var(--danger);letter-spacing:1px;text-transform:uppercase;margin-bottom:8px;display:flex;align-items:center;gap:6px}"
".attack-title::before{content:'';width:6px;height:6px;border-radius:50%;background:var(--danger);box-shadow:0 0 8px var(--danger);animation:pulse 1s infinite}"
".progress-bar{height:6px;background:var(--surface3);border-radius:3px;overflow:hidden;margin-top:8px}"
".progress-fill{height:100%;background:linear-gradient(90deg,var(--danger),#ff6b6b);border-radius:3px;animation:progress 2s linear infinite}"
"@keyframes progress{0%{width:0%}100%{width:100%}}"

/* Stats Grid */
".stats-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-bottom:14px}"
".stat-item{background:var(--surface2);border:1px solid var(--border);border-radius:10px;padding:10px 8px;text-align:center}"
".stat-val{font-size:18px;font-weight:700;color:var(--accent);line-height:1}"
".stat-label{font-size:9px;color:var(--text3);letter-spacing:1px;text-transform:uppercase;margin-top:4px}"

/* Client List */
".client-item{display:flex;align-items:center;padding:9px 12px;border-radius:8px;cursor:pointer;transition:all 0.15s;margin-bottom:4px;border:1px solid transparent}"
".client-item:hover{background:var(--surface2);border-color:var(--border)}"
".client-item.selected{background:rgba(239,68,68,0.08);border-color:rgba(239,68,68,0.3)}"
".client-mac{font-family:'SF Mono','Consolas',monospace;font-size:11px;color:var(--text2);flex:1}"
".client-rssi{font-size:11px;color:var(--text3);min-width:50px;text-align:right}"

/* Slider */
".slider-wrap{padding:4px 0}"
".slider{width:100%;height:4px;-webkit-appearance:none;background:linear-gradient(90deg,var(--accent) var(--pct,50%),var(--surface3) var(--pct,50%));border-radius:2px;outline:none;margin:12px 0}"
".slider::-webkit-slider-thumb{-webkit-appearance:none;width:18px;height:18px;border-radius:50%;background:var(--accent);box-shadow:0 0 10px rgba(0,212,255,0.4);cursor:pointer}"
".slider-val{text-align:center;font-size:22px;font-weight:700;color:var(--accent);margin-top:4px}"

/* Toast */
".toast-wrap{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);z-index:999;pointer-events:none;display:flex;flex-direction:column;gap:8px;align-items:center}"
".toast{background:var(--surface3);border:1px solid var(--border);color:var(--text);padding:10px 18px;border-radius:10px;font-size:13px;opacity:0;transform:translateY(10px);transition:all 0.3s;white-space:nowrap;backdrop-filter:blur(10px)}"
".toast.show{opacity:1;transform:translateY(0)}"
".toast.success{border-color:rgba(16,185,129,0.4);color:var(--accent3)}"
".toast.error{border-color:rgba(239,68,68,0.4);color:var(--danger)}"
".toast.warn{border-color:rgba(245,158,11,0.4);color:var(--warn)}"

/* Tabs */
".tabs{display:flex;background:var(--surface);border:1px solid var(--border);border-radius:12px;padding:4px;margin-bottom:16px}"
".tab{flex:1;padding:8px 4px;text-align:center;font-size:12px;font-weight:600;color:var(--text3);border-radius:8px;cursor:pointer;transition:all 0.2s}"
".tab.active{background:var(--surface3);color:var(--accent);box-shadow:0 2px 8px rgba(0,0,0,0.3)}"

/* Spinner */
".spinner{width:20px;height:20px;border:2px solid var(--border);border-top-color:var(--accent);border-radius:50%;animation:spin 0.8s linear infinite;display:inline-block}"
"@keyframes spin{to{transform:rotate(360deg)}}"

/* Empty State */
".empty{text-align:center;padding:30px 20px;color:var(--text3)}"
".empty-icon{font-size:36px;margin-bottom:10px;opacity:0.5}"
".empty-text{font-size:13px}"

/* Beacon Modal */
".modal{position:fixed;inset:0;background:rgba(0,0,0,0.7);backdrop-filter:blur(4px);z-index:200;display:flex;align-items:flex-end;opacity:0;pointer-events:none;transition:opacity 0.3s}"
".modal.open{opacity:1;pointer-events:all}"
".modal-sheet{background:var(--surface);border:1px solid var(--border);border-radius:20px 20px 0 0;padding:20px 16px;width:100%;transform:translateY(100%);transition:transform 0.3s}"
".modal.open .modal-sheet{transform:translateY(0)}"
".modal-title{font-size:15px;font-weight:700;color:var(--text);margin-bottom:16px;text-align:center}"
".modal-handle{width:36px;height:4px;background:var(--border);border-radius:2px;margin:0 auto 16px}"

/* Dot separator */
".dot-sep{width:3px;height:3px;border-radius:50%;background:var(--border);display:inline-block;margin:0 4px}"

/* Evil Twin Result */
".stolen-pass{font-family:'SF Mono',monospace;font-size:18px;font-weight:700;color:var(--accent3);background:rgba(16,185,129,0.1);border:1px solid rgba(16,185,129,0.2);border-radius:8px;padding:10px 14px;margin:10px 0;text-align:center;letter-spacing:2px}"
"</style></head><body>"

"<div class='header'>"
"<div class='logo'>"
"<div class='logo-icon'>👻</div>"
"<div><div class='logo-text'>GhostCore</div><div class='logo-sub'>ESP32 Control Panel</div></div>"
"</div>"
"<div class='header-status'><div class='status-dot'></div><span class='status-text' id='statusTxt'>Online</span></div>"
"</div>"

"<div class='main'>"

/* TABS */
"<div class='tabs'>"
"<div class='tab active' onclick='switchTab(0)'>📡 WiFi</div>"
"<div class='tab' onclick='switchTab(1)'>💀 Attack</div>"
"<div class='tab' onclick='switchTab(2)'>📻 Spam</div>"
"<div class='tab' onclick='switchTab(3)'>👁 Twin</div>"
"<div class='tab' onclick='switchTab(4)'>⚙️ Sys</div>"
"</div>"

/* ========= TAB 0: WIFI SCANNER ========= */
"<div id='tab0'>"
"<div class='section'>"
"<div class='section-title'>Scan & Target</div>"
"<div class='card'>"
"<div class='card-header'>"
"<div class='card-icon wifi'>📡</div>"
"<div><div class='card-title'>WiFi Scanner</div><div class='card-desc'>Scan jaringan di sekitar</div></div>"
"</div>"
"<div class='card-body'>"
"<div class='btn-row' style='margin-bottom:12px'>"
"<button class='btn btn-primary btn-full' onclick='doScan()'><span id='scanIcon'>🔍</span> <span id='scanTxt'>Scan Sekarang</span></button>"
"<button class='btn btn-ghost btn-sm' onclick='loadWifi()'>↻</button>"
"</div>"
"<div id='wifiList'><div class='empty'><div class='empty-icon'>📶</div><div class='empty-text'>Belum ada data. Tekan Scan.</div></div></div>"
"</div>"
"</div>"
"</div>"

/* Target terkunci */
"<div class='section' id='targetSection' style='display:none'>"
"<div class='section-title'>Target Terkunci</div>"
"<div class='target-card'>"
"<div class='target-ssid' id='tSSID'>-</div>"
"<div class='target-details'>"
"<div class='target-detail-item'>MAC: <span id='tMAC'>-</span></div>"
"<div class='target-detail-item'>CH: <span id='tCH'>-</span></div>"
"<div class='target-detail-item'>RSSI: <span id='tRSSI'>-</span> dBm</div>"
"<div class='target-detail-item'>ENC: <span id='tENC'>-</span></div>"
"</div>"
"</div>"
"<div class='btn-row'>"
"<button class='btn btn-ghost btn-sm' onclick='scanClients()'>👥 Clients</button>"
"<button class='btn btn-ghost btn-sm' onclick='trackTarget()'>📍 Track</button>"
"<button class='btn btn-ghost btn-sm' style='color:var(--text3)' onclick='clearTarget()'>✕ Clear</button>"
"</div>"
"</div>"

/* Client list */
"<div class='section' id='clientSection' style='display:none'>"
"<div class='section-title'>Client Stations</div>"
"<div class='card'>"
"<div class='card-header'>"
"<div class='card-icon attack'>👥</div>"
"<div><div class='card-title'>Connected Clients</div><div class='card-desc' id='clientCount'>Scan client dulu</div></div>"
"<button class='btn btn-ghost btn-sm' style='margin-left:auto' onclick='scanClients()'>Scan</button>"
"</div>"
"<div class='card-body' id='clientList'><div class='empty'><div class='empty-icon'>👤</div><div class='empty-text'>Belum ada client.</div></div></div>"
"</div>"
"</div>"

/* Track */
"<div class='section' id='trackSection' style='display:none'>"
"<div class='section-title'>RSSI Tracker</div>"
"<div class='card'>"
"<div class='card-header'><div class='card-icon info'>📍</div><div><div class='card-title'>Tracking Target</div><div class='card-desc'>Real-time signal strength</div></div></div>"
"<div class='card-body'>"
"<div class='stats-grid'>"
"<div class='stat-item'><div class='stat-val' id='trackRSSI'>-</div><div class='stat-label'>dBm</div></div>"
"<div class='stat-item'><div class='stat-val' id='trackCH'>-</div><div class='stat-label'>Channel</div></div>"
"<div class='stat-item'><div class='stat-val' id='trackUpd'>-</div><div class='stat-label'>Update</div></div>"
"</div>"
"<button class='btn btn-ghost btn-full btn-sm' onclick='stopTrack()'>⏹ Stop Tracking</button>"
"</div>"
"</div>"
"</div>"
"</div>"

/* ========= TAB 1: ATTACK ========= */
"<div id='tab1' style='display:none'>"
"<div class='section'>"
"<div class='section-title'>Deauth Attack</div>"
"<div id='noTargetWarning' class='card' style='text-align:center;padding:20px'>"
"<div style='font-size:32px;margin-bottom:8px'>⚠️</div>"
"<div style='font-size:13px;color:var(--warn)'>Pilih target WiFi dulu di tab Scan!</div>"
"</div>"
"<div id='attackPanel' style='display:none'>"
"<div class='target-card' style='margin-bottom:12px'>"
"<div class='target-ssid' id='atkSSID'>-</div>"
"<div class='target-details'>"
"<div class='target-detail-item'>MAC: <span id='atkMAC'>-</span></div>"
"<div class='target-detail-item'>CH: <span id='atkCH'>-</span></div>"
"</div>"
"</div>"
"<div id='atkStatus'></div>"
"<div class='btn-row'>"
"<button class='btn btn-danger btn-full' id='atkBtn' onclick='toggleDeauth()'>💀 Mulai Deauth</button>"
"</div>"
"</div>"
"</div>"

"<div class='section'>"
"<div class='section-title'>Kick Client</div>"
"<div class='card'>"
"<div class='card-header'><div class='card-icon attack'>🦵</div><div><div class='card-title'>Station Deauth</div><div class='card-desc'>Kick specific client dari jaringan</div></div></div>"
"<div class='card-body'>"
"<div id='kickClientList'><div class='empty'><div class='empty-icon'>👤</div><div class='empty-text'>Scan clients di tab WiFi dulu</div></div></div>"
"<div id='kickStatus'></div>"
"<button class='btn btn-danger btn-full' id='kickBtn' onclick='kickClient()' style='display:none;margin-top:10px'>🦵 Kick Client</button>"
"</div>"
"</div>"
"</div>"
"</div>"

/* ========= TAB 2: SPAM ========= */
"<div id='tab2' style='display:none'>"
"<div class='section'>"
"<div class='section-title'>Beacon Spam</div>"
"<div class='card'>"
"<div class='card-header'><div class='card-icon spam'>📻</div><div><div class='card-title'>Beacon Flood</div><div class='card-desc'>Spam SSID palsu ke udara</div></div></div>"
"<div class='card-body'>"
"<div id='spamStatus'></div>"
"<div class='btn-row'>"
"<button class='btn btn-warn btn-full' id='spamBtn' onclick='toggleSpam(1)'>📻 Beacon Spam</button>"
"<button class='btn btn-purple btn-full' id='rrBtn' onclick='toggleSpam(2)'>🎵 RickRoll</button>"
"</div>"
"</div>"
"</div>"
"</div>"
"</div>"

/* ========= TAB 3: EVIL TWIN ========= */
"<div id='tab3' style='display:none'>"
"<div class='section'>"
"<div class='section-title'>Evil Twin Attack</div>"
"<div id='noTargetWarningET' class='card' style='text-align:center;padding:20px;display:none'>"
"<div style='font-size:32px;margin-bottom:8px'>⚠️</div>"
"<div style='font-size:13px;color:var(--warn)'>Pilih target WiFi dulu di tab Scan!</div>"
"</div>"
"<div class='card'>"
"<div class='card-header'><div class='card-icon evil'>👁</div><div><div class='card-title'>Evil Twin / Captive Portal</div><div class='card-desc'>Kloning AP target + captive portal</div></div></div>"
"<div class='card-body'>"
"<div id='etTarget' style='margin-bottom:12px;display:none'>"
"<div class='target-card'><div class='target-ssid' id='etSSID'>-</div><div class='target-detail-item'>Channel: <span id='etCH'>-</span></div></div>"
"</div>"
"<div id='etStatus'></div>"
"<div id='etPassResult' style='display:none'>"
"<div style='font-size:12px;color:var(--accent3);margin-bottom:6px;font-weight:700'>🎯 PASSWORD BERHASIL DICURI!</div>"
"<div class='stolen-pass' id='etPass'>-</div>"
"<button class='btn btn-ghost btn-sm btn-full' onclick='copyPass()'>📋 Copy Password</button>"
"</div>"
"<div id='etBtnWrap'>"
"<button class='btn btn-purple btn-full' id='etBtn' onclick='toggleEvilTwin()'>👁 Start Evil Twin</button>"
"</div>"
"</div>"
"</div>"
"</div>"
"</div>"

/* ========= TAB 4: SYSTEM ========= */
"<div id='tab4' style='display:none'>"
"<div class='section'>"
"<div class='section-title'>System Info</div>"
"<div class='card'>"
"<div class='card-header'><div class='card-icon info'>💻</div><div><div class='card-title'>GhostCore OS</div><div class='card-desc'>ESP32 System Status</div></div></div>"
"<div class='card-body'>"
"<div class='stats-grid'>"
"<div class='stat-item'><div class='stat-val' id='sysFree'>-</div><div class='stat-label'>Free Heap</div></div>"
"<div class='stat-item'><div class='stat-val' id='sysUptime'>-</div><div class='stat-label'>Uptime (s)</div></div>"
"<div class='stat-item'><div class='stat-val' id='sysClients'>-</div><div class='stat-label'>AP Clients</div></div>"
"</div>"
"</div>"
"</div>"
"</div>"

"<div class='section'>"
"<div class='section-title'>Control</div>"
"<div class='card'>"
"<div class='card-header'><div class='card-icon info'>⚙️</div><div><div class='card-title'>System Control</div><div class='card-desc'>Reboot & reset operasi</div></div></div>"
"<div class='card-body'>"
"<div class='btn-row'>"
"<button class='btn btn-ghost btn-full' onclick='stopAll()'>⏹ Stop Semua</button>"
"<button class='btn btn-danger' onclick='doReboot()'>🔄 Reboot</button>"
"</div>"
"</div>"
"</div>"
"</div>"

"<div class='section'>"
"<div class='section-title'>About</div>"
"<div class='card'><div class='card-body' style='text-align:center;padding:20px'>"
"<div style='font-size:36px;margin-bottom:10px'>👻</div>"
"<div style='font-size:16px;font-weight:700;color:var(--accent);margin-bottom:4px'>GhostCore OS</div>"
"<div style='font-size:11px;color:var(--text3);margin-bottom:2px'>Version 1.0.0 • ESP32-S3</div>"
"<div style='font-size:11px;color:var(--text3)'>By Andyy • Web UI Edition</div>"
"</div></div>"
"</div>"
"</div>"

"</div>" /* /main */

/* Toast container */
"<div class='toast-wrap' id='toastWrap'></div>"

"<script>"
/* State */
"var wifiData=[];"
"var clientData=[];"
"var selTarget=null;"
"var selClient=null;"
"var activeTab=0;"
"var isDeauthing=false;"
"var isSpamming=false;"
"var spamMode=0;"
"var isEvilTwin=false;"
"var isTracking=false;"
"var trackInterval=null;"
"var pollInterval=null;"

/* Toast */
"function toast(msg,type=''){var w=document.getElementById('toastWrap');var t=document.createElement('div');t.className='toast '+(type||'');t.textContent=msg;w.appendChild(t);setTimeout(()=>t.classList.add('show'),10);setTimeout(()=>{t.classList.remove('show');setTimeout(()=>t.remove(),300)},2800);}"

/* Tab switch */
"function switchTab(i){"
"for(var j=0;j<5;j++){document.getElementById('tab'+j).style.display=j===i?'':'none';}"
"document.querySelectorAll('.tab').forEach((t,j)=>t.className='tab'+(j===i?' active':''));"
"activeTab=i;"
"if(i===1)refreshAttackPanel();"
"if(i===3)refreshEvilTwinPanel();"
"if(i===4)loadSysInfo();"
"}"

/* Signal icon */
"function sigIcon(rssi){if(rssi>=-50)return'📶';if(rssi>=-65)return'▂▄▆█';if(rssi>=-75)return'▂▄▆░';return'▂▄░░';}"
"function sigColor(rssi){if(rssi>=-50)return'var(--accent3)';if(rssi>=-65)return'var(--accent)';if(rssi>=-75)return'var(--warn)';return'var(--danger)';}"

/* WiFi Scan */
"function doScan(){"
"var icon=document.getElementById('scanIcon');var txt=document.getElementById('scanTxt');"
"icon.textContent='⏳';txt.textContent='Scanning...';"
"document.getElementById('wifiList').innerHTML=\"<div style='text-align:center;padding:20px'><div class='spinner'></div><p style='color:var(--text3);margin-top:10px;font-size:12px'>Scanning WiFi...</p></div>\";"
"fetch('/api/scan',{method:'POST'})"
".then(r=>r.json())"
".then(d=>{icon.textContent='🔍';txt.textContent='Scan Sekarang';if(d.ok){toast('Scan dimulai, tunggu 5 detik...','');setTimeout(loadWifi,5500);}else toast('Gagal scan: '+d.error,'error');})"
".catch(e=>{icon.textContent='🔍';txt.textContent='Scan Sekarang';toast('Error: '+e,'error');});"
"}"

"function loadWifi(){"
"fetch('/api/wifi')"
".then(r=>r.json())"
".then(d=>{"
"wifiData=d.list||[];"
"var el=document.getElementById('wifiList');"
"if(!wifiData.length){el.innerHTML=\"<div class='empty'><div class='empty-icon'>📶</div><div class='empty-text'>Tidak ada WiFi ditemukan.</div></div>\";return;}"
"el.innerHTML=wifiData.map((w,i)=>"
"\"`<div class='wifi-item ${selTarget&&selTarget.mac===w.mac?'selected':''}' onclick='selectTarget(${i})'>"
"<div class='wifi-signal' style='color:${sigColor(w.rssi)}'>${sigIcon(w.rssi)}</div>"
"<div class='wifi-info'>"
"<div class='wifi-ssid'>${w.ssid||'(Hidden)'}</div>"
"<div class='wifi-meta'>"
"<span>CH ${w.channel}</span><span>${w.rssi} dBm</span>"
"<span class='wifi-badge ${w.is_open?'badge-open':'badge-wpa'}'>${w.encrypt}</span>"
"</div>"
"</div>"
"</div>`\").join('');"
"})"
".catch(e=>toast('Gagal load WiFi','error'));"
"}"

"function selectTarget(i){"
"selTarget=wifiData[i];"
"fetch('/api/set_target',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({idx:i})})"
".then(r=>r.json())"
".then(d=>{"
"if(d.ok){"
"loadWifi();"
"updateTargetUI();"
"toast('Target terkunci: '+selTarget.ssid,'success');"
"}else toast('Gagal set target','error');"
"});"
"}"

"function updateTargetUI(){"
"if(!selTarget){document.getElementById('targetSection').style.display='none';return;}"
"document.getElementById('targetSection').style.display='';"
"document.getElementById('tSSID').textContent=selTarget.ssid||'(Hidden)';"
"document.getElementById('tMAC').textContent=selTarget.mac;"
"document.getElementById('tCH').textContent=selTarget.channel;"
"document.getElementById('tRSSI').textContent=selTarget.rssi;"
"document.getElementById('tENC').textContent=selTarget.encrypt;"
"}"

"function clearTarget(){"
"selTarget=null;"
"fetch('/api/clear_target',{method:'POST'});"
"document.getElementById('targetSection').style.display='none';"
"loadWifi();"
"toast('Target dihapus');"
"}"

/* Client scan */
"function scanClients(){"
"if(!selTarget){toast('Pilih target dulu!','warn');return;}"
"toast('Scanning clients 4 detik...');"
"fetch('/api/scan_clients',{method:'POST'})"
".then(r=>r.json())"
".then(d=>{if(d.ok){setTimeout(loadClients,4500);}else toast('Gagal: '+d.error,'error');});"
"}"

"function loadClients(){"
"fetch('/api/clients')"
".then(r=>r.json())"
".then(d=>{"
"clientData=d.list||[];"
"document.getElementById('clientSection').style.display='';"
"document.getElementById('clientCount').textContent=clientData.length+' client ditemukan';"
"var el=document.getElementById('clientList');"
"if(!clientData.length){el.innerHTML=\"<div class='empty'><div class='empty-icon'>👤</div><div class='empty-text'>Tidak ada client.</div></div>\";return;}"
"el.innerHTML=clientData.map((c,i)=>"
"\"`<div class='client-item ${selClient&&selClient.id===c.id?'selected':''}' onclick='selectClient(${i})'>"
"<div class='client-mac'>${c.mac_str}</div>"
"<div class='client-rssi'>${c.rssi} dBm<br><span style='font-size:9px;color:var(--text3)'>${c.paket_count} pkt</span></div>"
"</div>`\").join('');"
"refreshKickList();"
"});"
"}"

"function selectClient(i){"
"selClient=clientData[i];"
"fetch('/api/set_client',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({idx:i})})"
".then(r=>r.json())"
".then(d=>{"
"if(d.ok){loadClients();toast('Client dipilih');}else toast('Gagal','error');"
"});"
"}"

/* Track */
"function trackTarget(){"
"if(!selTarget){toast('Pilih target dulu!','warn');return;}"
"document.getElementById('trackSection').style.display='';"
"document.getElementById('trackCH').textContent=selTarget.channel;"
"fetch('/api/track_start',{method:'POST'});"
"isTracking=true;"
"if(trackInterval)clearInterval(trackInterval);"
"trackInterval=setInterval(()=>{"
"fetch('/api/track_rssi').then(r=>r.json()).then(d=>{"
"document.getElementById('trackRSSI').textContent=d.rssi;"
"document.getElementById('trackUpd').textContent=new Date().toLocaleTimeString('id-ID',{hour:'2-digit',minute:'2-digit',second:'2-digit'});"
"});"
"},800);"
"toast('Tracking dimulai');"
"}"

"function stopTrack(){"
"isTracking=false;"
"if(trackInterval)clearInterval(trackInterval);"
"fetch('/api/track_stop',{method:'POST'});"
"document.getElementById('trackSection').style.display='none';"
"toast('Tracking dihentikan');"
"}"

/* Attack panel */
"function refreshAttackPanel(){"
"if(!selTarget){"
"document.getElementById('noTargetWarning').style.display='';"
"document.getElementById('attackPanel').style.display='none';"
"}else{"
"document.getElementById('noTargetWarning').style.display='none';"
"document.getElementById('attackPanel').style.display='';"
"document.getElementById('atkSSID').textContent=selTarget.ssid||'(Hidden)';"
"document.getElementById('atkMAC').textContent=selTarget.mac;"
"document.getElementById('atkCH').textContent=selTarget.channel;"
"}"
"refreshKickList();"
"}"

"function toggleDeauth(){"
"if(!selTarget){toast('Pilih target dulu!','warn');return;}"
"if(!isDeauthing){"
"fetch('/api/deauth_start',{method:'POST'})"
".then(r=>r.json())"
".then(d=>{"
"if(d.ok){"
"isDeauthing=true;"
"document.getElementById('atkBtn').className='btn btn-ghost btn-full';"
"document.getElementById('atkBtn').innerHTML='⏹ Stop Deauth';"
"document.getElementById('atkStatus').innerHTML=\"<div class='attack-active'><div class='attack-title'>DEAUTH ACTIVE</div><div style='font-size:12px;color:var(--text3)'>Sending deauth packets...</div><div class='progress-bar'><div class='progress-fill'></div></div></div>\";"
"toast('Deauth dimulai!','warn');"
"}else toast('Gagal: '+d.error,'error');"
"});"
"}else{"
"fetch('/api/deauth_stop',{method:'POST'})"
".then(()=>{"
"isDeauthing=false;"
"document.getElementById('atkBtn').className='btn btn-danger btn-full';"
"document.getElementById('atkBtn').innerHTML='💀 Mulai Deauth';"
"document.getElementById('atkStatus').innerHTML='';"
"toast('Deauth dihentikan');"
"});"
"}"
"}"

"function refreshKickList(){"
"var el=document.getElementById('kickClientList');"
"if(!clientData.length){el.innerHTML=\"<div class='empty'><div class='empty-icon'>👤</div><div class='empty-text'>Scan clients di tab WiFi dulu</div></div>\";document.getElementById('kickBtn').style.display='none';return;}"
"el.innerHTML=clientData.map((c,i)=>"
"\"`<div class='client-item ${selClient&&selClient.id===c.id?'selected':''}' onclick='selectClient(${i});refreshKickList()'>"
"<div class='client-mac'>${c.mac_str}</div>"
"<div class='client-rssi'>${c.rssi} dBm</div>"
"</div>`\").join('');"
"if(selClient)document.getElementById('kickBtn').style.display='';"
"}"

"function kickClient(){"
"if(!selClient){toast('Pilih client dulu!','warn');return;}"
"fetch('/api/kick_client',{method:'POST'})"
".then(r=>r.json())"
".then(d=>{"
"if(d.ok){"
"document.getElementById('kickStatus').innerHTML=\"<div class='attack-active'><div class='attack-title'>KICKING CLIENT</div><div style='font-size:12px;color:var(--text3)'>Target: \"+selClient.mac_str+\"</div><div class='progress-bar'><div class='progress-fill'></div></div></div>\";"
"toast('Kick dimulai!','warn');"
"setTimeout(()=>{document.getElementById('kickStatus').innerHTML='';},8000);"
"}else toast('Gagal: '+d.error,'error');"
"});"
"}"

/* Spam */
"function toggleSpam(mode){"
"if(isSpamming&&spamMode===mode){"
"fetch('/api/spam_stop',{method:'POST'}).then(()=>{"
"isSpamming=false;spamMode=0;"
"document.getElementById('spamBtn').innerHTML='📻 Beacon Spam';"
"document.getElementById('rrBtn').innerHTML='🎵 RickRoll';"
"document.getElementById('spamStatus').innerHTML='';"
"toast('Spam dihentikan');"
"});return;"
"}"
"var body=JSON.stringify({mode:mode});"
"fetch('/api/spam_start',{method:'POST',headers:{'Content-Type':'application/json'},body:body})"
".then(r=>r.json())"
".then(d=>{"
"if(d.ok){"
"isSpamming=true;spamMode=mode;"
"var label=mode===1?'BEACON SPAM':'RICKROLL';"
"document.getElementById('spamStatus').innerHTML=\"<div class='attack-active'><div class='attack-title'>\"+label+\" RUNNING</div><div style='font-size:12px;color:var(--text3)'>Flooding airspace...</div><div class='progress-bar'><div class='progress-fill'></div></div></div>\";"
"if(mode===1){document.getElementById('spamBtn').innerHTML='⏹ Stop Spam';}else{document.getElementById('rrBtn').innerHTML='⏹ Stop RickRoll';}"
"toast(label+' dimulai!','warn');"
"}else toast('Gagal: '+d.error,'error');"
"});"
"}"

/* Evil Twin */
"function refreshEvilTwinPanel(){"
"if(selTarget){"
"document.getElementById('etTarget').style.display='';"
"document.getElementById('etSSID').textContent=selTarget.ssid||'(Hidden)';"
"document.getElementById('etCH').textContent=selTarget.channel;"
"document.getElementById('noTargetWarningET').style.display='none';"
"}else{"
"document.getElementById('noTargetWarningET').style.display='';"
"document.getElementById('etTarget').style.display='none';"
"}"
"}"

"function toggleEvilTwin(){"
"if(!selTarget){toast('Pilih target dulu!','warn');return;}"
"if(!isEvilTwin){"
"fetch('/api/evil_twin_start',{method:'POST'})"
".then(r=>r.json())"
".then(d=>{"
"if(d.ok){"
"isEvilTwin=true;"
"document.getElementById('etBtn').innerHTML='⏹ Stop Evil Twin';"
"document.getElementById('etBtn').className='btn btn-ghost btn-full';"
"document.getElementById('etStatus').innerHTML=\"<div class='attack-active'><div class='attack-title'>EVIL TWIN ACTIVE</div><div style='font-size:12px;color:var(--text3)'>Captive portal aktif. Menunggu korban...</div><div class='progress-bar'><div class='progress-fill'></div></div></div>\";"
"toast('Evil Twin aktif!','warn');"
"if(pollInterval)clearInterval(pollInterval);"
"pollInterval=setInterval(checkEvilTwinResult,3000);"
"}else toast('Gagal: '+d.error,'error');"
"});"
"}else{"
"fetch('/api/evil_twin_stop',{method:'POST'}).then(()=>{"
"isEvilTwin=false;"
"if(pollInterval)clearInterval(pollInterval);"
"document.getElementById('etBtn').innerHTML='👁 Start Evil Twin';"
"document.getElementById('etBtn').className='btn btn-purple btn-full';"
"document.getElementById('etStatus').innerHTML='';"
"toast('Evil Twin dihentikan');"
"});"
"}"
"}"

"function checkEvilTwinResult(){"
"fetch('/api/evil_twin_status')"
".then(r=>r.json())"
".then(d=>{"
"if(d.state===2&&d.password){"
"if(pollInterval)clearInterval(pollInterval);"
"isEvilTwin=false;"
"document.getElementById('etStatus').innerHTML='';"
"document.getElementById('etPassResult').style.display='';"
"document.getElementById('etPass').textContent=d.password;"
"document.getElementById('etBtn').innerHTML='👁 Start Evil Twin';"
"document.getElementById('etBtn').className='btn btn-purple btn-full';"
"toast('Password berhasil dicuri!','success');"
"}"
"});"
"}"

"function copyPass(){"
"var pass=document.getElementById('etPass').textContent;"
"if(navigator.clipboard)navigator.clipboard.writeText(pass).then(()=>toast('Password tersalin!','success'));"
"else toast(pass);"
"}"

/* System */
"function loadSysInfo(){"
"fetch('/api/sysinfo')"
".then(r=>r.json())"
".then(d=>{"
"document.getElementById('sysFree').textContent=Math.round(d.free_heap/1024)+'K';"
"document.getElementById('sysUptime').textContent=d.uptime;"
"document.getElementById('sysClients').textContent=d.ap_clients;"
"document.getElementById('statusTxt').textContent='Heap: '+Math.round(d.free_heap/1024)+'K';"
"});"
"}"

"function stopAll(){"
"fetch('/api/stop_all',{method:'POST'}).then(()=>{"
"isDeauthing=false;isSpamming=false;spamMode=0;isEvilTwin=false;isTracking=false;"
"if(pollInterval)clearInterval(pollInterval);"
"if(trackInterval)clearInterval(trackInterval);"
"document.getElementById('atkStatus').innerHTML='';"
"document.getElementById('spamStatus').innerHTML='';"
"document.getElementById('etStatus').innerHTML='';"
"document.getElementById('atkBtn').className='btn btn-danger btn-full';"
"document.getElementById('atkBtn').innerHTML='💀 Mulai Deauth';"
"document.getElementById('spamBtn').innerHTML='📻 Beacon Spam';"
"document.getElementById('rrBtn').innerHTML='🎵 RickRoll';"
"document.getElementById('etBtn').innerHTML='👁 Start Evil Twin';"
"document.getElementById('etBtn').className='btn btn-purple btn-full';"
"toast('Semua operasi dihentikan','success');"
"});"
"}"

"function doReboot(){"
"if(!confirm('Reboot ESP32?'))return;"
"fetch('/api/reboot',{method:'POST'}).then(()=>toast('Rebooting...','warn'));"
"}"

/* Init */
"window.onload=function(){"
"loadSysInfo();"
"setInterval(()=>{if(activeTab===4)loadSysInfo();},5000);"
"};"
"</script>"
"</body></html>";

// ====================================================
// HTML size check
// ====================================================
#define GHOSTCORE_HTML_LEN (sizeof(GHOSTCORE_HTML) - 1)

// ====================================================
// HANDLER: Serve halaman utama
// ====================================================
static esp_err_t handler_root(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_send(req, GHOSTCORE_HTML, GHOSTCORE_HTML_LEN);
    return ESP_OK;
}

// ====================================================
// HANDLER: POST /api/scan - Mulai WiFi scan
// ====================================================
static esp_err_t handler_scan(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    if (sedang_scan) {
        httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"Sudah scanning\"}");
        return ESP_OK;
    }
    triggerScan = true;
    scanDone = false;
    totalWiFi = 0;
    sedang_scan = true;
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

// ====================================================
// HANDLER: GET /api/wifi - Daftar WiFi hasil scan
// ====================================================
static esp_err_t handler_wifi_list(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    
    char *buf = malloc(4096);
    if (!buf) { httpd_resp_sendstr(req, "{\"list\":[]}"); return ESP_OK; }
    
    int pos = 0;
    pos += snprintf(buf + pos, 4096 - pos, "{\"total\":%d,\"list\":[", totalWiFi);
    
    for (int i = 0; i < totalWiFi && pos < 3800; i++) {
        if (i > 0) buf[pos++] = ',';
        pos += snprintf(buf + pos, 4096 - pos,
            "{\"id\":%d,\"ssid\":\"%s\",\"rssi\":%d,\"channel\":%d,"
            "\"encrypt\":\"%s\",\"is_open\":%s,\"mac\":\"%s\"}",
            listWiFi[i].id,
            listWiFi[i].ssid,
            listWiFi[i].rssi,
            listWiFi[i].channel,
            listWiFi[i].encrypt,
            listWiFi[i].is_open ? "true" : "false",
            listWiFi[i].mac
        );
    }
    snprintf(buf + pos, 4096 - pos, "]}");
    
    httpd_resp_sendstr(req, buf);
    free(buf);
    return ESP_OK;
}

// ====================================================
// HANDLER: POST /api/set_target - Set target WiFi
// ====================================================
static esp_err_t handler_set_target(httpd_req_t *req) {
    char buf[64];
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) { httpd_resp_sendstr(req, "{\"ok\":false}"); return ESP_OK; }
    buf[len] = '\0';
    
    // Parse idx dari JSON sederhana: {"idx":N}
    int idx = -1;
    char *p = strstr(buf, "\"idx\":");
    if (p) idx = atoi(p + 6);
    
    httpd_resp_set_type(req, "application/json");
    if (idx >= 0 && idx < totalWiFi) {
        targetTerkunci = listWiFi[idx];
        targetLockedIdx = idx;
        adaTarget = true;
        httpd_resp_sendstr(req, "{\"ok\":true}");
    } else {
        httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"index invalid\"}");
    }
    return ESP_OK;
}

// ====================================================
// HANDLER: POST /api/clear_target
// ====================================================
static esp_err_t handler_clear_target(httpd_req_t *req) {
    adaTarget = false;
    targetLockedIdx = -1;
    memset(&targetTerkunci, 0, sizeof(targetTerkunci));
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

// ====================================================
// HANDLER: POST /api/scan_clients
// ====================================================
static esp_err_t handler_scan_clients(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    if (!adaTarget) {
        httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"Tidak ada target\"}");
        return ESP_OK;
    }
    totalStation = 0;
    triggerScanSta = true;
    scanStaDone = false;
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

// ====================================================
// HANDLER: GET /api/clients
// ====================================================
static esp_err_t handler_clients(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    
    char *buf = malloc(2048);
    if (!buf) { httpd_resp_sendstr(req, "{\"list\":[]}"); return ESP_OK; }
    
    int pos = 0;
    pos += snprintf(buf + pos, 2048 - pos, "{\"total\":%d,\"list\":[", totalStation);
    
    for (int i = 0; i < totalStation && pos < 1800; i++) {
        if (i > 0) buf[pos++] = ',';
        char mac_str[18];
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 listStation[i].mac[0], listStation[i].mac[1], listStation[i].mac[2],
                 listStation[i].mac[3], listStation[i].mac[4], listStation[i].mac[5]);
        pos += snprintf(buf + pos, 2048 - pos,
            "{\"id\":%d,\"mac_str\":\"%s\",\"rssi\":%d,\"paket_count\":%d}",
            listStation[i].id, mac_str, listStation[i].rssi, listStation[i].paket_count
        );
    }
    snprintf(buf + pos, 2048 - pos, "]}");
    
    httpd_resp_sendstr(req, buf);
    free(buf);
    return ESP_OK;
}

// ====================================================
// HANDLER: POST /api/set_client
// ====================================================
static esp_err_t handler_set_client(httpd_req_t *req) {
    char buf[64];
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) { httpd_resp_sendstr(req, "{\"ok\":false}"); return ESP_OK; }
    buf[len] = '\0';
    
    int idx = -1;
    char *p = strstr(buf, "\"idx\":");
    if (p) idx = atoi(p + 6);
    
    httpd_resp_set_type(req, "application/json");
    if (idx >= 0 && idx < totalStation) {
        targetSta = listStation[idx];
        adaTargetSta = true;
        httpd_resp_sendstr(req, "{\"ok\":true}");
    } else {
        httpd_resp_sendstr(req, "{\"ok\":false}");
    }
    return ESP_OK;
}

// ====================================================
// HANDLER: POST /api/deauth_start
// ====================================================
static esp_err_t handler_deauth_start(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    if (!adaTarget) {
        httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"Tidak ada target\"}");
        return ESP_OK;
    }
    deauthState = 1;
    isDeauthing = true;
    deauthUdahSetup = false;
    appMode = 2;
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

// ====================================================
// HANDLER: POST /api/deauth_stop
// ====================================================
static esp_err_t handler_deauth_stop(httpd_req_t *req) {
    isDeauthing = false;
    deauthState = 0;
    appMode = 0;
    deauthUdahSetup = false;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

// ====================================================
// HANDLER: POST /api/kick_client
// ====================================================
static esp_err_t handler_kick_client(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    if (!adaTargetSta) {
        httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"Pilih client dulu\"}");
        return ESP_OK;
    }
    isDeauthSta = true;
    deauthUdahSetup = false;
    appMode = 7;
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

// ====================================================
// HANDLER: POST /api/spam_start
// ====================================================
static esp_err_t handler_spam_start(httpd_req_t *req) {
    char buf[32];
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    buf[len > 0 ? len : 0] = '\0';
    
    int mode = 1;
    char *p = strstr(buf, "\"mode\":");
    if (p) mode = atoi(p + 7);
    
    aktifModeSpam = mode;
    spamState = 1;
    isSpamming = true;
    spamUdahSetup = false;
    appMode = 4;
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

// ====================================================
// HANDLER: POST /api/spam_stop
// ====================================================
static esp_err_t handler_spam_stop(httpd_req_t *req) {
    isSpamming = false;
    spamState = 0;
    aktifModeSpam = 0;
    spamUdahSetup = false;
    appMode = 0;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

// ====================================================
// HANDLER: POST /api/track_start
// ====================================================
static esp_err_t handler_track_start(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    if (!adaTarget) {
        httpd_resp_sendstr(req, "{\"ok\":false}");
        return ESP_OK;
    }
    triggerTrack = true;
    appMode = 6;
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

// ====================================================
// HANDLER: GET /api/track_rssi
// ====================================================
static esp_err_t handler_track_rssi(httpd_req_t *req) {
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"rssi\":%d,\"ssid\":\"%s\"}",
             targetTerkunci.rssi, targetTerkunci.ssid);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, buf);
    return ESP_OK;
}

// ====================================================
// HANDLER: POST /api/track_stop
// ====================================================
static esp_err_t handler_track_stop(httpd_req_t *req) {
    triggerTrack = false;
    appMode = 0;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

// ====================================================
// HANDLER: POST /api/evil_twin_start
// ====================================================
extern void startEvilTwin(void);

static esp_err_t handler_evil_twin_start(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    if (!adaTarget) {
        httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"Pilih target dulu\"}");
        return ESP_OK;
    }
    triggerEvilTwin = true;
    stolenPassword[0] = '\0';
    evilTwinState = 0;
    appMode = 8;
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

// ====================================================
// HANDLER: POST /api/evil_twin_stop
// ====================================================
static esp_err_t handler_evil_twin_stop(httpd_req_t *req) {
    isEvilTwin = false;
    evilTwinState = 0;
    triggerEvilTwin = false;
    appMode = 0;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

// ====================================================
// HANDLER: GET /api/evil_twin_status
// ====================================================
static esp_err_t handler_evil_twin_status(httpd_req_t *req) {
    char buf[128];
    snprintf(buf, sizeof(buf), "{\"state\":%d,\"password\":\"%s\"}", evilTwinState, stolenPassword);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, buf);
    return ESP_OK;
}

// ====================================================
// HANDLER: POST /api/stop_all
// ====================================================
static esp_err_t handler_stop_all(httpd_req_t *req) {
    isDeauthing = false;
    isDeauthSta = false;
    isSpamming = false;
    isEvilTwin = false;
    triggerTrack = false;
    triggerEvilTwin = false;
    deauthState = 0;
    spamState = 0;
    aktifModeSpam = 0;
    evilTwinState = 0;
    appMode = 0;
    deauthUdahSetup = false;
    spamUdahSetup = false;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

// ====================================================
// HANDLER: GET /api/sysinfo
// ====================================================
#include "esp_system.h"
#include "esp_timer.h"

static esp_err_t handler_sysinfo(httpd_req_t *req) {
    uint32_t free_heap = esp_get_free_heap_size();
    uint32_t uptime = (uint32_t)(esp_timer_get_time() / 1000000);
    
    // Count AP clients
    wifi_sta_list_t sta_list;
    int ap_clients = 0;
    if (esp_wifi_ap_get_sta_list(&sta_list) == ESP_OK) {
        ap_clients = sta_list.num;
    }
    
    char buf[128];
    snprintf(buf, sizeof(buf), "{\"free_heap\":%lu,\"uptime\":%lu,\"ap_clients\":%d}",
             (unsigned long)free_heap, (unsigned long)uptime, ap_clients);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, buf);
    return ESP_OK;
}

// ====================================================
// HANDLER: POST /api/reboot
// ====================================================
static esp_err_t handler_reboot(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
    return ESP_OK;
}

// ====================================================
// HANDLER: Captive portal redirect (untuk DNS catch-all)
// ====================================================
static esp_err_t handler_captive(httpd_req_t *req) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
    httpd_resp_sendstr(req, "");
    return ESP_OK;
}

// ====================================================
// DAFTARKAN SEMUA URI HANDLER
// ====================================================
static const httpd_uri_t uri_handlers[] = {
    { .uri = "/",                      .method = HTTP_GET,  .handler = handler_root           },
    { .uri = "/api/scan",              .method = HTTP_POST, .handler = handler_scan            },
    { .uri = "/api/wifi",              .method = HTTP_GET,  .handler = handler_wifi_list       },
    { .uri = "/api/set_target",        .method = HTTP_POST, .handler = handler_set_target      },
    { .uri = "/api/clear_target",      .method = HTTP_POST, .handler = handler_clear_target    },
    { .uri = "/api/scan_clients",      .method = HTTP_POST, .handler = handler_scan_clients    },
    { .uri = "/api/clients",           .method = HTTP_GET,  .handler = handler_clients         },
    { .uri = "/api/set_client",        .method = HTTP_POST, .handler = handler_set_client      },
    { .uri = "/api/deauth_start",      .method = HTTP_POST, .handler = handler_deauth_start    },
    { .uri = "/api/deauth_stop",       .method = HTTP_POST, .handler = handler_deauth_stop     },
    { .uri = "/api/kick_client",       .method = HTTP_POST, .handler = handler_kick_client     },
    { .uri = "/api/spam_start",        .method = HTTP_POST, .handler = handler_spam_start      },
    { .uri = "/api/spam_stop",         .method = HTTP_POST, .handler = handler_spam_stop       },
    { .uri = "/api/track_start",       .method = HTTP_POST, .handler = handler_track_start     },
    { .uri = "/api/track_rssi",        .method = HTTP_GET,  .handler = handler_track_rssi      },
    { .uri = "/api/track_stop",        .method = HTTP_POST, .handler = handler_track_stop      },
    { .uri = "/api/evil_twin_start",   .method = HTTP_POST, .handler = handler_evil_twin_start },
    { .uri = "/api/evil_twin_stop",    .method = HTTP_POST, .handler = handler_evil_twin_stop  },
    { .uri = "/api/evil_twin_status",  .method = HTTP_GET,  .handler = handler_evil_twin_status},
    { .uri = "/api/stop_all",          .method = HTTP_POST, .handler = handler_stop_all        },
    { .uri = "/api/sysinfo",           .method = HTTP_GET,  .handler = handler_sysinfo         },
    { .uri = "/api/reboot",            .method = HTTP_POST, .handler = handler_reboot          },
    { .uri = "/*",                     .method = HTTP_GET,  .handler = handler_captive         },
};

#define NUM_HANDLERS (sizeof(uri_handlers) / sizeof(uri_handlers[0]))

// ====================================================
// START HTTP SERVER
// ====================================================
void start_web_server(void) {
    if (g_server) return; // Sudah jalan
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = NUM_HANDLERS + 2;
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.stack_size = 8192;
    config.max_open_sockets = 7;
    
    ESP_LOGI(TAG, "Memulai HTTP Server di port %d...", config.server_port);
    
    if (httpd_start(&g_server, &config) == ESP_OK) {
        for (int i = 0; i < (int)NUM_HANDLERS; i++) {
            httpd_register_uri_handler(g_server, &uri_handlers[i]);
        }
        ESP_LOGI(TAG, "HTTP Server siap! Buka http://192.168.4.1");
    } else {
        ESP_LOGE(TAG, "Gagal memulai HTTP Server!");
    }
}

// ====================================================
// SETUP AP HIDDEN + WEB SERVER
// ====================================================
void ghostcore_web_init(void) {
    ESP_LOGI(TAG, "=== GhostCore Web Init ===");
    
    // Init NVS jika belum
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // Init netif & event loop (cek kalau sudah diinit dari main)
    esp_netif_init();
    esp_event_loop_create_default();

    // Buat AP netif
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    (void)ap_netif;

    // Init WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // Set mode AP
    esp_wifi_set_mode(WIFI_MODE_AP);

    // Konfigurasi AP - HIDDEN SSID
    wifi_config_t ap_config = {
        .ap = {
            .ssid            = GHOSTCORE_AP_SSID,
            .ssid_len        = strlen(GHOSTCORE_AP_SSID),
            .password        = GHOSTCORE_AP_PASS,
            .channel         = GHOSTCORE_AP_CHANNEL,
            .authmode        = WIFI_AUTH_WPA2_PSK,
            .ssid_hidden     = GHOSTCORE_AP_HIDDEN,   // 1 = hidden!
            .max_connection  = 5,
            .beacon_interval = 100,
        },
    };

    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "AP Hidden aktif: SSID='%s' Pass='%s' CH=%d",
             GHOSTCORE_AP_SSID, GHOSTCORE_AP_PASS, GHOSTCORE_AP_CHANNEL);
    ESP_LOGI(TAG, "SSID disembunyikan (hidden). Connect manual di HP.");

    // DNS server (captive portal)
    start_dns_server();

    // HTTP server
    start_web_server();
    
    ESP_LOGI(TAG, "=== Web UI Siap di http://192.168.4.1 ===");
}
