#include <Wire.h>
#include <LiquidCrystal.h>
#include <RTClib.h>

LiquidCrystal lcd(12, 11, 7, 6, 5, 4);
RTC_DS1307 rtc;

// ----- HARDWARE PINS -----
const int pinPadj = 2;
const int pinPPlus = 3;
const int pinPMinus = 8;
const int pinBuzzer = 9;
const int pinWaterSensor = 10; // Sensor for water supply
const int pinPump = 13;        // Output to Transistor buffer

// ----- STATE VARIABLES -----
int setupState = 0; 
int alarmHour = 0;
int alarmMinute = 0;
int alarmSecond = 0;
bool alarmActive = false;

// ----- PUMP CONTROL VARIABLES -----
bool pumpActive = false;
uint32_t pumpStartTime = 0;
unsigned long waterStopTimer = 0;

// ----- DEBOUNCE TRACKING -----
bool lastPadj = HIGH, lastPlus = HIGH, lastMinus = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void setup() {
  pinMode(pinPadj, INPUT_PULLUP);
  pinMode(pinPPlus, INPUT_PULLUP);
  pinMode(pinPMinus, INPUT_PULLUP);
  pinMode(pinWaterSensor, INPUT_PULLUP); 
  pinMode(pinBuzzer, OUTPUT);
  pinMode(pinPump, OUTPUT);

  lcd.begin(16, 2);
  rtc.begin();
  if (!rtc.isrunning()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void loop() {
  bool readingPadj = digitalRead(pinPadj);
  bool readingPlus = digitalRead(pinPPlus);
  bool readingMinus = digitalRead(pinPMinus);
  bool waterFlowing = !digitalRead(pinWaterSensor); // LOW (closed) = Water is flowing

  // 1. READ BUTTONS (ALARM SETUP)
  if (millis() - lastDebounceTime > debounceDelay) {
    if (readingPadj == LOW && lastPadj == HIGH) {
      setupState++;
      if (setupState > 4) { setupState = 0; alarmActive = true; }
      lcd.clear();
      lastDebounceTime = millis();
    }
    if (setupState > 1) { 
      if (readingPlus == LOW && lastPlus == HIGH) {
        if (setupState == 2) alarmHour = (alarmHour + 1) % 24;
        if (setupState == 3) alarmMinute = (alarmMinute + 1) % 60;
        if (setupState == 4) alarmSecond = (alarmSecond + 1) % 60;
        lastDebounceTime = millis();
      }
      if (readingMinus == LOW && lastMinus == HIGH) {
        if (setupState == 2) alarmHour = (alarmHour == 0) ? 23 : alarmHour - 1;
        if (setupState == 3) alarmMinute = (alarmMinute == 0) ? 59 : alarmMinute - 1;
        if (setupState == 4) alarmSecond = (alarmSecond == 0) ? 59 : alarmSecond - 1;
        lastDebounceTime = millis();
      }
    }
  }
  lastPadj = readingPadj; lastPlus = readingPlus; lastMinus = readingMinus;

  // 2. FETCH TIME & CONTROL LOGIC
  DateTime now = rtc.now();

  if (setupState == 0) {
    // --- Alarm Trigger Logic ---
    if (alarmActive && now.hour() == alarmHour && now.minute() == alarmMinute && now.second() == alarmSecond) {
      digitalWrite(pinBuzzer, HIGH);
      
      // Switch on pump when alarm matches
      if (!pumpActive && waterFlowing) {
        pumpActive = true;
        pumpStartTime = now.unixtime(); // Record Unix timestamp (seconds)
      }
    } else {
      digitalWrite(pinBuzzer, LOW); // Turn buzzer off after 1 second
    }

    // --- Pump Safety Control Logic ---
    if (pumpActive) {
      digitalWrite(pinPump, HIGH); // Buffer engages motor

      // Safety Cutoff 1: 1 hour (3600 seconds) has elapsed[cite: 1]
      if (now.unixtime() - pumpStartTime >= 3600) {
        pumpActive = false;
      }

      // Safety Cutoff 2: Water supply stops[cite: 1]
      if (!waterFlowing) {
        if (waterStopTimer == 0) {
          waterStopTimer = millis(); // Start the 1-second countdown
        } else if (millis() - waterStopTimer >= 1000) {
          pumpActive = false; // Switch off after 1 second[cite: 1]
        }
      } else {
        waterStopTimer = 0; // Reset timer if water starts flowing again
      }
    } else {
      digitalWrite(pinPump, LOW);
      waterStopTimer = 0;
    }
  }

  // 3. LCD DISPLAY
  lcd.setCursor(0, 0);
  if (setupState == 0) {
    lcd.print("Time: "); printDigits(now.hour()); lcd.print(":"); printDigits(now.minute()); lcd.print(":"); printDigits(now.second());
    lcd.setCursor(0, 1);
    if (pumpActive) lcd.print("Pump: ON FLOWING");
    else if (alarmActive) { lcd.print("Alrm: "); printDigits(alarmHour); lcd.print(":"); printDigits(alarmMinute); lcd.print(":"); printDigits(alarmSecond); } 
    else lcd.print("Alarm: OFF      ");
  } 
  else if (setupState == 1) { lcd.print("Alarm Setting"); lcd.setCursor(0,1); lcd.print("Mode Activated"); } 
  else {
    lcd.print("Set Alarm: ");
    if (setupState == 2) lcd.print("HR "); if (setupState == 3) lcd.print("MIN"); if (setupState == 4) lcd.print("SEC");
    lcd.setCursor(0, 1); printDigits(alarmHour); lcd.print(":"); printDigits(alarmMinute); lcd.print(":"); printDigits(alarmSecond);
  }
}

void printDigits(int digits) {
  if (digits < 10) lcd.print('0');
  lcd.print(digits);
}