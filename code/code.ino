#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define BUTTON_PIN  18
#define BUZZER_INV  15
#define DEBOUNCE_MS 50

LiquidCrystal_I2C lcd(0x27, 16, 2);

uint8_t peerMAC[] = {0xC0, 0xCD, 0xDC, 0x84, 0xBC, 0x90};

// ─── TIMING ───────────────────────────────────────────────────
constexpr uint16_t DOT_MAX         = 400;
constexpr uint16_t SEND_HOLD       = 7000;
constexpr uint16_t LETTER_GAP      = 1500;
constexpr uint16_t WORD_GAP        = 3000;
constexpr uint16_t SENT_LOCKOUT_MS = 1000;
constexpr uint16_t RX_DISPLAY_MS   = 5000;

// ─── BUZZER TIMING ────────────────────────────────────────────
constexpr uint16_t BUZZ_DOT      = 150;
constexpr uint16_t BUZZ_DASH     = 450;
constexpr uint16_t BUZZ_SYM_GAP  = 150;
constexpr uint16_t BUZZ_LET_GAP  = 450;
constexpr uint16_t BUZZ_WORD_GAP = 1050;

// ─── BUFFER SIZES ─────────────────────────────────────────────
#define MSG_MAX    64
#define MORSE_MAX  16
#define QUEUE_MAX  384

// ─── MORSE TABLE ──────────────────────────────────────────────
struct MorseEntry { const char* code; char ch; };
const MorseEntry MT[] = {
  {".-",   'A'}, {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'},
  {".",    'E'}, {"..-.", 'F'}, {"--.",  'G'}, {"....", 'H'},
  {"..",   'I'}, {".---", 'J'}, {"-.-",  'K'}, {".-..", 'L'},
  {"--",   'M'}, {"-.",   'N'}, {"---",  'O'}, {".--.", 'P'},
  {"--.-", 'Q'}, {".-.",  'R'}, {"...",  'S'}, {"-",   'T'},
  {"..-",  'U'}, {"...-", 'V'}, {".--",  'W'}, {"-..-", 'X'},
  {"-.--", 'Y'}, {"--..", 'Z'}
};
constexpr uint8_t MT_SIZE = sizeof(MT) / sizeof(MT[0]);

char decodeMorse(const char* code) {
  for (uint8_t i = 0; i < MT_SIZE; i++)
    if (strcmp(code, MT[i].code) == 0) return MT[i].ch;
  return '?';
}

const char* encodeMorse(char c) {
  c = toupper(c);
  for (uint8_t i = 0; i < MT_SIZE; i++)
    if (MT[i].ch == c) return MT[i].code;
  return nullptr;
}

// ─── STATE ────────────────────────────────────────────────────
enum DisplayMode { MODE_READY, MODE_RECEIVED, MODE_SENT, MODE_ERROR };
DisplayMode   currentMode  = MODE_READY;
unsigned long displayTimer = 0;
unsigned long sentLockout  = 0;
bool          inLockout    = false;

// ─── BUFFERS ──────────────────────────────────────────────────
char    typingMorse[MORSE_MAX] = {};
char    txMessage[MSG_MAX + 1] = {};
uint8_t typingMorseLen = 0;
uint8_t txMessageLen   = 0;

volatile bool rxReady = false;
char          rxBuffer[MSG_MAX + 1]  = {};
char          rxMessage[MSG_MAX + 1] = {};

// ─── BUZZER STATE ─────────────────────────────────────────────
enum BuzzState { BUZZ_IDLE, BUZZ_BEEPING, BUZZ_SYM_PAUSE, BUZZ_LET_PAUSE, BUZZ_WRD_PAUSE };
BuzzState     buzzState    = BUZZ_IDLE;
unsigned long buzzTimer    = 0;
char          buzzQueue[QUEUE_MAX] = {};
uint16_t      buzzQueueLen = 0;
uint16_t      buzzQueueIdx = 0;

// ─── BUTTON STATE ─────────────────────────────────────────────
unsigned long pressStart       = 0;
unsigned long lastRelease      = 0;
unsigned long lastDebounceTime = 0;
bool pressed            = false;
bool letterPending      = false;
bool spaceAdded         = false;
bool sendHandled        = false;
bool ignoreNextRelease  = false;   // ← KEY FIX: eat the release after send
bool lastRawReading     = HIGH;
bool debouncedReading   = HIGH;

// ─── DEBOUNCE ─────────────────────────────────────────────────
bool readButton() {
  bool raw = digitalRead(BUTTON_PIN);
  if (raw != lastRawReading) {
    lastDebounceTime = millis();
    lastRawReading   = raw;
  }
  if (millis() - lastDebounceTime >= DEBOUNCE_MS)
    debouncedReading = raw;
  return debouncedReading;
}

// ─── LCD ──────────────────────────────────────────────────────
void lcdShow(const char* line1, const char* line2 = nullptr) {
  lcd.clear();
  lcd.print(line1);
  if (line2 && line2[0] != '\0') {
    lcd.setCursor(0, 1);
    char tmp[17];
    strncpy(tmp, line2, 16);
    tmp[16] = '\0';
    lcd.print(tmp);
  }
}

// ─── INPUT RESET ──────────────────────────────────────────────
void resetInput() {
  memset(typingMorse, 0, sizeof(typingMorse));
  memset(txMessage,   0, sizeof(txMessage));
  typingMorseLen  = 0;
  txMessageLen    = 0;
  spaceAdded      = false;
  letterPending   = false;
  pressed         = false;
  sendHandled     = false;
  lastRelease     = millis();
  pressStart      = millis();
  // NOTE: ignoreNextRelease is NOT cleared here
  // it must survive into the first loop tick after lockout ends
}

// ─── BUZZER ───────────────────────────────────────────────────
void buzzerOn()  { digitalWrite(BUZZER_INV, LOW);  }
void buzzerOff() { digitalWrite(BUZZER_INV, HIGH); }

// ─── BUILD BUZZ QUEUE ─────────────────────────────────────────
void buildBuzzQueue(const char* text) {
  buzzQueueLen = 0;
  buzzQueueIdx = 0;
  buzzState    = BUZZ_IDLE;
  memset(buzzQueue, 0, sizeof(buzzQueue));
  bool firstChar = true;

  for (uint16_t i = 0; text[i] != '\0'; i++) {
    char c = text[i];
    if (c == ' ') {
      if (buzzQueueLen > 0 && buzzQueue[buzzQueueLen - 1] == '/')
        buzzQueue[buzzQueueLen - 1] = '|';
      else if (buzzQueueLen > 0 && buzzQueueLen < QUEUE_MAX - 1)
        buzzQueue[buzzQueueLen++] = '|';
      firstChar = true;
      continue;
    }
    const char* code = encodeMorse(c);
    if (!code) continue;
    if (!firstChar && buzzQueueLen < QUEUE_MAX - 1)
      buzzQueue[buzzQueueLen++] = '/';
    firstChar = false;
    for (uint8_t j = 0; code[j] != '\0'; j++) {
      if (j > 0 && buzzQueueLen < QUEUE_MAX - 1)
        buzzQueue[buzzQueueLen++] = ' ';
      if (buzzQueueLen < QUEUE_MAX - 1)
        buzzQueue[buzzQueueLen++] = code[j];
    }
  }
  buzzQueue[buzzQueueLen] = '\0';
}

// ─── BUZZER TICK ──────────────────────────────────────────────
void buzzerTick() {
  if (buzzState == BUZZ_IDLE) {
    if (buzzQueueIdx >= buzzQueueLen) { buzzerOff(); return; }
    char sym = buzzQueue[buzzQueueIdx++];
    if      (sym == '.' || sym == '-') { buzzerOn();  buzzTimer = millis(); buzzState = BUZZ_BEEPING;   }
    else if (sym == ' ')               {              buzzTimer = millis(); buzzState = BUZZ_SYM_PAUSE; }
    else if (sym == '/')               { buzzerOff(); buzzTimer = millis(); buzzState = BUZZ_LET_PAUSE; }
    else if (sym == '|')               { buzzerOff(); buzzTimer = millis(); buzzState = BUZZ_WRD_PAUSE; }
    return;
  }
  unsigned long now = millis();
  if      (buzzState == BUZZ_BEEPING)   { char p = buzzQueue[buzzQueueIdx-1]; if (now-buzzTimer >= (p=='-'?BUZZ_DASH:BUZZ_DOT)) { buzzerOff(); buzzTimer=now; buzzState=BUZZ_SYM_PAUSE; } }
  else if (buzzState == BUZZ_SYM_PAUSE) { if (now - buzzTimer >= BUZZ_SYM_GAP)  buzzState = BUZZ_IDLE; }
  else if (buzzState == BUZZ_LET_PAUSE) { if (now - buzzTimer >= BUZZ_LET_GAP)  buzzState = BUZZ_IDLE; }
  else if (buzzState == BUZZ_WRD_PAUSE) { if (now - buzzTimer >= BUZZ_WORD_GAP) buzzState = BUZZ_IDLE; }
}

// ─── RX CALLBACK ──────────────────────────────────────────────
void onReceive(const esp_now_recv_info* info, const uint8_t* data, int len) {
  uint8_t safeLen = (len > MSG_MAX) ? MSG_MAX : len;
  memcpy(rxBuffer, data, safeLen);
  rxBuffer[safeLen] = '\0';
  rxReady = true;
}

// ─── SEND ─────────────────────────────────────────────────────
void sendMessage() {
  if (typingMorseLen > 0) {
    char d = decodeMorse(typingMorse);
    if (txMessageLen < MSG_MAX) { txMessage[txMessageLen++] = d; txMessage[txMessageLen] = '\0'; }
    memset(typingMorse, 0, sizeof(typingMorse));
    typingMorseLen = 0;
  }
  while (txMessageLen > 0 && txMessage[txMessageLen - 1] == ' ')
    txMessage[--txMessageLen] = '\0';

  // ── Set lockout and flag to swallow the upcoming button release ──
  sentLockout         = millis();
  inLockout           = true;
  displayTimer        = millis();
  ignoreNextRelease   = true;   // ← the send hold-release WILL fire; eat it

  if (txMessageLen == 0) {
    lcdShow("Nothing", "to send!");
    currentMode = MODE_ERROR;
    return;
  }

  esp_err_t r = esp_now_send(peerMAC, (uint8_t*)txMessage, txMessageLen);
  if (r == ESP_OK) {
    currentMode = MODE_SENT;
    lcdShow("Sent:", txMessage);
    Serial.print("TX: "); Serial.println(txMessage);
  } else {
    currentMode = MODE_ERROR;
    lcdShow("Send FAILED!", "Check peer");
  }
}

// ─── SETUP ────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_INV, OUTPUT);
  buzzerOff();

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcdShow("Morse Chat", "Starting...");

  WiFi.mode(WIFI_STA);
  Serial.print("MAC: "); Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    lcdShow("ESP-NOW", "Init Failed!");
    while (true) delay(1000);
  }
  esp_now_register_recv_cb(onReceive);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peerMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    lcdShow("Peer", "Add Failed!");
    while (true) delay(1000);
  }

  inLockout          = false;
  ignoreNextRelease  = false;
  resetInput();
  lcdShow("Morse Chat", "Ready");
}

// ─── LOOP ─────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // ── RX ──────────────────────────────────────────────────────
  if (rxReady) {
    rxReady = false;
    memcpy(rxMessage, rxBuffer, MSG_MAX + 1);
    Serial.print("RX: "); Serial.println(rxMessage);
    currentMode  = MODE_RECEIVED;
    displayTimer = now;
    lcdShow("Received:", rxMessage);
    buildBuzzQueue(rxMessage);
  }

  // ── BUZZER ──────────────────────────────────────────────────
  buzzerTick();

  // ── LOCKOUT ─────────────────────────────────────────────────
  if (inLockout) {
    // Still drain the button state so we catch and discard the release
    bool btnDown = (readButton() == LOW);

    // ── CATCH AND DISCARD the hold-release during lockout ─────
    if (!btnDown && pressed) {
      pressed = false;
      if (ignoreNextRelease) {
        ignoreNextRelease = false;   // swallow it — record nothing
      }
    }

    // ── Lockout expired → wipe and go Ready ───────────────────
    if (now - sentLockout >= SENT_LOCKOUT_MS) {
      inLockout = false;
      resetInput();
      ignoreNextRelease = false;   // ensure cleared even if release never came
      currentMode = MODE_READY;
      lcdShow("Morse Chat", "Ready");
    }
    return;
  }

  // ── BUTTON (only runs when NOT in lockout) ───────────────────
  bool btnDown = (readButton() == LOW);

  // ── PRESSED ─────────────────────────────────────────────────
  if (btnDown && !pressed) {
    pressed     = true;
    sendHandled = false;
    pressStart  = now;
  }

  // ── HOLD 7s → SEND ──────────────────────────────────────────
  if (btnDown && pressed && !sendHandled && (now - pressStart >= SEND_HOLD)) {
    sendHandled = true;
    pressed     = false;
    sendMessage();
    return;
  }

  // ── RELEASED ────────────────────────────────────────────────
  if (!btnDown && pressed) {
    pressed = false;

    if (ignoreNextRelease) {
      ignoreNextRelease = false;   // safety: clear if it somehow arrives here
    } else if (!sendHandled) {
      unsigned long dur = now - pressStart;
      char sym = (dur < DOT_MAX) ? '.' : '-';
      if (typingMorseLen < MORSE_MAX - 1) {
        typingMorse[typingMorseLen++] = sym;
        typingMorse[typingMorseLen]   = '\0';
      }
      lastRelease   = now;
      spaceAdded    = false;
      letterPending = true;
      lcdShow("Morse:", typingMorse);
    }
  }

  // ── LETTER GAP ──────────────────────────────────────────────
  if (!pressed && letterPending && (now - lastRelease >= LETTER_GAP)) {
    char d = decodeMorse(typingMorse);
    if (txMessageLen < MSG_MAX) { txMessage[txMessageLen++] = d; txMessage[txMessageLen] = '\0'; }
    memset(typingMorse, 0, sizeof(typingMorse));
    typingMorseLen = 0;
    letterPending  = false;
    lcdShow("Msg:", txMessage);
  }

  // ── WORD GAP ────────────────────────────────────────────────
  if (!pressed && !spaceAdded && !letterPending &&
      txMessageLen > 0 && txMessage[txMessageLen - 1] != ' ' &&
      (now - lastRelease >= WORD_GAP)) {
    if (txMessageLen < MSG_MAX) { txMessage[txMessageLen++] = ' '; txMessage[txMessageLen] = '\0'; }
    spaceAdded = true;
    lcdShow("Msg:", txMessage);
  }

  // ── RX TIMEOUT ──────────────────────────────────────────────
  if (currentMode == MODE_RECEIVED && (now - displayTimer >= RX_DISPLAY_MS)) {
    currentMode = MODE_READY;
    lcdShow(txMessageLen > 0 ? "Msg:" : "Morse Chat",
            txMessageLen > 0 ? txMessage : "Ready");
  }
}