#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>

#define SCREEN_ADDRESS 0x3C
#define BTN_PADJ   2
#define BTN_PMINUS 3
#define BTN_PPLUS  4
#define BUZZER_PIN 8

Adafruit_SSD1306 display(128, 64, &Wire, -1);
RTC_DS1307 rtc;

int alarmHr = 0, alarmMin = 0, alarmSec = 0;
int mode = 0;                
bool ringing = false;
unsigned long ringStart = 0;
unsigned long lastClockDraw = 0;

void drawClock() {
  DateTime now = rtc.now();
  char t[9], a[22];
  sprintf(t, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  sprintf(a, "Alarm: %02d:%02d:%02d", alarmHr, alarmMin, alarmSec);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Current Time");
  display.setTextSize(2);
  display.setCursor(0, 16);
  display.println(t);
  display.setTextSize(1);
  display.setCursor(0, 50);
  display.println(a);
  display.display();
}

void drawSetting() {
  char t[9];
  sprintf(t, "%02d:%02d:%02d", alarmHr, alarmMin, alarmSec);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ALARM SETTING");
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.println(t);
  display.setTextSize(1);
  display.setCursor(0, 52);
  switch (mode) {
    case 1: display.println("Padj to change"); break;
    case 2: display.println("Mode: HR");  break;
    case 3: display.println("Mode: MIN"); break;
    case 4: display.println("Mode: SEC"); break;
  }
  display.display();
}

void setup() {
  pinMode(BTN_PADJ,   INPUT_PULLUP);
  pinMode(BTN_PPLUS,  INPUT_PULLUP);
  pinMode(BTN_PMINUS, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.display();

  rtc.begin();
  drawClock();
}

void loop() {
  DateTime now = rtc.now();

  // Trigger alarm
  if (!ringing && mode == 0 &&
      now.hour() == alarmHr &&
      now.minute() == alarmMin &&
      now.second() == alarmSec) {
    ringing = true;
    ringStart = millis();
    digitalWrite(BUZZER_PIN, HIGH);
    drawClock();
  }

  // Auto-stop after 3 seconds
  if (ringing && millis() - ringStart >= 3000) {
    ringing = false;
    digitalWrite(BUZZER_PIN, LOW);
    drawClock();
  }

  // Read buttons with edge detection
  static bool lastPadj = HIGH, lastPplus = HIGH, lastPminus = HIGH;
  bool padj   = digitalRead(BTN_PADJ)   == LOW && lastPadj   == HIGH;
  bool pplus  = digitalRead(BTN_PPLUS)  == LOW && lastPplus  == HIGH;
  bool pminus = digitalRead(BTN_PMINUS) == LOW && lastPminus == HIGH;
  lastPadj   = digitalRead(BTN_PADJ);
  lastPplus  = digitalRead(BTN_PPLUS);
  lastPminus = digitalRead(BTN_PMINUS);

  if (padj) {
    mode = (mode + 1) % 5;
    if (mode == 0) drawClock();
    else drawSetting();
  }

  if (mode >= 2 && (pplus || pminus)) {
    int d = pplus ? 1 : -1;
    if (mode == 2) alarmHr  = (alarmHr  + d + 24) % 24;
    if (mode == 3) alarmMin = (alarmMin + d + 60) % 60;
    if (mode == 4) alarmSec = (alarmSec + d + 60) % 60;
    drawSetting();
  }

  // Refresh the clock once per second
  if (mode == 0 && millis() - lastClockDraw >= 1000) {
    lastClockDraw = millis();
    drawClock();
  }
}