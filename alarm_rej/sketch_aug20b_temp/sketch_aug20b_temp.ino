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
const int pinWaterSensor = 10;
const int pinPump = 13;
const int pinTempPot = A0; // Potentiometer simulating temperature sensor

// ----- STATE VARIABLES -----
int setupState = 0; 
int alarmHour = 0, alarmMinute = 0, alarmSecond = 0;
bool alarmActive = false;

bool pumpActive = false;
uint32_t pumpStartTime = 0;
unsigned long waterStopTimer = 0;

int lastLoggedMinute = -1; 

// ----- DEBOUNCE TRACKING -----
bool lastPadj = HIGH, lastPlus = HIGH, lastMinus = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void setup() {
  Serial.begin(9600);
  pinMode(pinPadj, INPUT_PULLUP);
  pinMode(pinPPlus, INPUT_PULLUP);
  pinMode(pinPMinus, INPUT_PULLUP);
  pinMode(pinWaterSensor, INPUT_PULLUP); 
  pinMode(pinBuzzer, OUTPUT);
  pinMode(pinPump, OUTPUT);

  lcd.begin(16, 2);
  rtc.begin();
  if (!rtc.isrunning()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
  Serial.println("--- Ambient Temperature Log Started ---");
}

void loop() {
  bool readingPadj = digitalRead(pinPadj);
  bool readingPlus = digitalRead(pinPPlus);
  bool readingMinus = digitalRead(pinPMinus);
  bool waterFlowing = !digitalRead(pinWaterSensor); 

  // 1. BUTTON & ALARM SETUP LOGIC
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

  // 2. TIME, ALARM, PUMP, & TEMPERATURE LOGGING
  DateTime now = rtc.now();

  if (setupState == 0) {
    if (alarmActive && now.hour() == alarmHour && now.minute() == alarmMinute && now.second() == alarmSecond) {
      digitalWrite(pinBuzzer, HIGH);
      if (!pumpActive && waterFlowing) {
        pumpActive = true;
        pumpStartTime = now.unixtime(); 
      }
    } else {
      digitalWrite(pinBuzzer, LOW);
    }

    if (pumpActive) {
      digitalWrite(pinPump, HIGH); 
      if (now.unixtime() - pumpStartTime >= 3600) pumpActive = false;
      if (!waterFlowing) {
        if (waterStopTimer == 0) waterStopTimer = millis(); 
        else if (millis() - waterStopTimer >= 1000) pumpActive = false;
      } else {
        waterStopTimer = 0;
      }
    } else {
      digitalWrite(pinPump, LOW);
      waterStopTimer = 0;
    }

    // Read Potentiometer and map 0-1023 to a 0°C - 50°C temperature range
    int rawVal = analogRead(pinTempPot);
    float simulatedTempC = map(rawVal, 0, 1023, 0, 500) / 10.0;

    // 15-Minute Interval Logger
    int currentMin = now.minute();
    if ((currentMin == 0 || currentMin == 15 || currentMin == 30 || currentMin == 45) && currentMin != lastLoggedMinute) {
      lastLoggedMinute = currentMin;

      Serial.print("LOG [");
      printDigits(now.hour()); Serial.print(":");
      printDigits(currentMin); Serial.print("] - Temp: ");
      Serial.print(simulatedTempC);
      Serial.println(" C");
    }
    if (currentMin != 0 && currentMin != 15 && currentMin != 30 && currentMin != 45) {
      lastLoggedMinute = -1; 
    }
  }

  // 3. LCD DISPLAY
  lcd.setCursor(0, 0);
  if (setupState == 0) {
    lcd.print("Time: "); printDigits(now.hour()); lcd.print(":"); printDigits(now.minute()); lcd.print(":"); printDigits(now.second());
    lcd.setCursor(0, 1);
    
    int rawVal = analogRead(pinTempPot);
    float displayTemp = map(rawVal, 0, 1023, 0, 500) / 10.0;
    lcd.print("Temp: "); lcd.print(displayTemp, 1); lcd.print("C   ");
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