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

// ----- STATE VARIABLES -----
int setupState = 0; 
int alarmHour = 0;
int alarmMinute = 0;
int alarmSecond = 0;
bool alarmActive = false;

// ----- DEBOUNCE TRACKING -----
bool lastPadj = HIGH, lastPlus = HIGH, lastMinus = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void setup() {
  pinMode(pinPadj, INPUT_PULLUP);
  pinMode(pinPPlus, INPUT_PULLUP);
  pinMode(pinPMinus, INPUT_PULLUP);
  pinMode(pinBuzzer, OUTPUT);

  lcd.begin(16, 2);
  rtc.begin();
  if (!rtc.isrunning()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void loop() {
  bool readingPadj = digitalRead(pinPadj);
  bool readingPlus = digitalRead(pinPPlus);
  bool readingMinus = digitalRead(pinPMinus);

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
    } else {
      digitalWrite(pinBuzzer, LOW); // Turn buzzer off after 1 second
    }
  }

  // 3. LCD DISPLAY
  lcd.setCursor(0, 0);
  if (setupState == 0) {
    lcd.print("Time: "); printDigits(now.hour()); lcd.print(":"); printDigits(now.minute()); lcd.print(":"); printDigits(now.second());
    lcd.setCursor(0, 1);
    if (alarmActive) { lcd.print("Buzz: "); printDigits(alarmHour); lcd.print(":"); printDigits(alarmMinute); lcd.print(":"); printDigits(alarmSecond); } 
    else lcd.print("Alarm: OFF      ");
  } 
  else if (setupState == 1) { lcd.print("Alarm Setting"); lcd.setCursor(0,1); lcd.print("Mode Activated"); } 
  else {
    lcd.print("Set Alarm: ");
    if (setupState == 2) lcd.print("Hr"); if (setupState == 3) lcd.print("Min"); if (setupState == 4) lcd.print("Sec");
    lcd.setCursor(0, 1); printDigits(alarmHour); lcd.print(":"); printDigits(alarmMinute); lcd.print(":"); printDigits(alarmSecond);
  }
}

void printDigits(int digits) {
  if (digits < 10) lcd.print('0');
  lcd.print(digits);
}