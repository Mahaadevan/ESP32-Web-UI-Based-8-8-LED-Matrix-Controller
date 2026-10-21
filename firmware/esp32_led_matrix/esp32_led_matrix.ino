// ESP32 direct-drive 8x8 LED matrix + WiFi AP + WebSocket UI
// Libs: WebSockets (Markus Sattler). No MAX72xx needed.
// Connect phone to WiFi AP below, open http://192.168.4.1
// WS protocol (port 81): first msg AUTH:<pw>, then
//   F:<16hex>  frame | A:<fps>:<16hex*n> animation | B:<0-15> brightness | S stop | C clear
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "esp_timer.h"
#include "ui.h"

#define AP_SSID  "Matrix-ESP32"
#define AP_PASS  "matrix1234"      // min 8 chars
#define AUTH_PW  "matrix1234"

// 1 = rows driven via NPN transistors (row HIGH = active). 0 = rows wired straight to GPIO (row LOW = active)
#define USE_TRANSISTORS 1
#define ROW_ON  (USE_TRANSISTORS ? HIGH : LOW)
#define COL_ON  HIGH            // columns = LED anodes (via resistors)

const uint8_t ROWS[8] = {32, 14, 19, 26, 15, 18, 2, 17};
const uint8_t COLS[8] = {27, 4, 16, 33, 5, 25, 12, 13};

volatile uint8_t fb[8];          // bit7 = leftmost column
volatile uint8_t onTicks = 4;    // brightness: 1..8 of 8 ticks

WebServer http(80);
WebSocketsServer ws(81);
bool authed[WEBSOCKETS_SERVER_CLIENT_MAX];
uint8_t frames[64][8];
uint8_t nf = 0, cur = 0;
uint16_t delayMs = 200;
bool playing = false;
uint32_t last = 0;

// multiplex scanner: 150us tick, 8 ticks/row => ~104 Hz full refresh
void scanTick(void *) {
  static uint8_t r = 0, t = 0;
  if (t == 0) {
    uint8_t d = fb[r];
    for (uint8_t c = 0; c < 8; c++)
      digitalWrite(COLS[c], ((d >> (7 - c)) & 1) ? !COL_ON : COL_ON);
    digitalWrite(ROWS[r], ROW_ON);
  } else if (t == onTicks) {
    digitalWrite(ROWS[r], !ROW_ON);
  }
  if (++t == 8) {
    t = 0;
    digitalWrite(ROWS[r], !ROW_ON);
    r = (r + 1) & 7;
  }
}

uint8_t hx(char c) { return c <= '9' ? c - '0' : (c | 32) - 'a' + 10; }
void show(const uint8_t *d) { for (int r = 0; r < 8; r++) fb[r] = d[r]; }

bool loadFrames(const String &s, int n) {
  if (s.length() < n * 16) return false;
  for (int i = 0; i < n; i++)
    for (int r = 0; r < 8; r++)
      frames[i][r] = hx(s[i * 16 + r * 2]) << 4 | hx(s[i * 16 + r * 2 + 1]);
  return true;
}

void onWs(uint8_t n, WStype_t t, uint8_t *p, size_t len) {
  if (t == WStype_DISCONNECTED) { authed[n] = false; return; }
  if (t != WStype_TEXT) return;
  String m((char *)p, len);
  if (!authed[n]) {
    if (m.startsWith("AUTH:") && m.substring(5) == AUTH_PW) {
      authed[n] = true;
      ws.sendTXT(n, "OK");
    } else {
      delay(1000);
      ws.sendTXT(n, "DENY");
      ws.disconnect(n);
    }
    return;
  }
  switch (m[0]) {
    case 'F':
      if (loadFrames(m.substring(2), 1)) { playing = false; show(frames[0]); }
      break;
    case 'A': {
      int c = m.indexOf(':', 2);
      if (c < 0) break;
      int fps = constrain(m.substring(2, c).toInt(), 1, 60);
      String d = m.substring(c + 1);
      int cnt = min((int)(d.length() / 16), 64);
      if (cnt > 0 && loadFrames(d, cnt)) { nf = cnt; cur = 0; delayMs = 1000 / fps; playing = true; }
      break;
    }
    case 'B': onTicks = 1 + constrain(m.substring(2).toInt(), 0, 15) * 7 / 15; break;
    case 'S': playing = false; break;
    case 'C': playing = false; for (int r = 0; r < 8; r++) fb[r] = 0; break;
  }
}
// ---- IP scroller ----
const uint8_t FONT[11][5] = {
  {0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},{0x42,0x61,0x51,0x49,0x46},
  {0x21,0x41,0x45,0x4B,0x31},{0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},
  {0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},{0x36,0x49,0x49,0x49,0x36},
  {0x06,0x49,0x49,0x29,0x1E},{0x00,0x60,0x60,0x00,0x00}   // 10 = '.'
};
uint8_t txt[160];
int txtLen = 0, scrollPos = 0;
uint32_t lastScroll = 0;
bool wasIdle = true;

void buildIpText() {
  String s = WiFi.softAPIP().toString();
  txtLen = 8;                                   // leading blank
  for (int i = 0; i < 8; i++) txt[i] = 0;
  for (unsigned i = 0; i < s.length(); i++) {
    char ch = s[i];
    int g = (ch == '.') ? 10 : ch - '0';
    if (g < 0 || g > 10) continue;
    for (int k = 0; k < 5; k++) txt[txtLen++] = FONT[g][k];
    txt[txtLen++] = 0;
  }
  for (int i = 0; i < 8; i++) txt[txtLen++] = 0; // trailing blank
}

bool anyAuthed() {
  for (int i = 0; i < WEBSOCKETS_SERVER_CLIENT_MAX; i++) if (authed[i]) return true;
  return false;
}
void setup() {
  for (uint8_t i = 0; i < 8; i++) {
    pinMode(ROWS[i], OUTPUT); digitalWrite(ROWS[i], !ROW_ON);
    pinMode(COLS[i], OUTPUT); digitalWrite(COLS[i], !COL_ON);
  }
  esp_timer_handle_t tm;
  esp_timer_create_args_t a = {};
  a.callback = scanTick; a.name = "scan";
  esp_timer_create(&a, &tm);
  esp_timer_start_periodic(tm, 150);

  WiFi.softAP(AP_SSID, AP_PASS);
  buildIpText();
  http.on("/", []() { http.send_P(200, "text/html", UI_HTML); });
  http.begin();
  ws.begin();
  ws.onEvent(onWs);
}

/*void loop() {
  ws.loop();
  http.handleClient();
  if (playing && millis() - last >= delayMs) {
    last = millis();
    show(frames[cur]);
    cur = (cur + 1) % nf;
  }
}*/
void loop() {
  ws.loop();
  http.handleClient();

  bool idle = !anyAuthed();
  if (idle) {
    playing = false;
    if (millis() - lastScroll >= 120) {
      lastScroll = millis();
      for (int r = 0; r < 8; r++) {
        uint8_t v = 0;
        for (int c = 0; c < 8; c++)
          if ((txt[(scrollPos + c) % txtLen] >> r) & 1) v |= 0x80 >> c;
        fb[r] = v;
      }
      scrollPos = (scrollPos + 1) % txtLen;
    }
  } else if (wasIdle) {
    for (int r = 0; r < 8; r++) fb[r] = 0;
  }
  wasIdle = idle;

  if (playing && millis() - last >= delayMs) {
    last = millis();
    show(frames[cur]);
    cur = (cur + 1) % nf;
  }
}
