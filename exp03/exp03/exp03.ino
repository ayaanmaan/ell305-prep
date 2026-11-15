#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int sw1Pin = 2;
const int sw2Pin = 3;
const int freqPotPin = A0;
const int ampPotPin = A1;
const int outPin = 9;

uint8_t sine_table[256];
volatile uint16_t phase = 0;
volatile uint16_t phase_inc = 0;
volatile uint8_t wave_state = 0;
volatile uint8_t amp_val = 255;

void setup() {
  pinMode(sw1Pin, INPUT_PULLUP);
  pinMode(sw2Pin, INPUT_PULLUP);
  pinMode(outPin, OUTPUT);

  // Generate sine lookup table in RAM to save flash space
  for (int i = 0; i < 256; i++) {
    sine_table[i] = (sin(i * 2.0 * PI / 256.0) + 1.0) * 127.5;
  }

  // _BV(val) equivalent to 1 << val

  // Timer 1: Fast PWM, 8-bit, non-inverting on Pin 9 (DDS DAC)
  TCCR1A = _BV(COM1A1) | _BV(WGM10);
  TCCR1B = _BV(CS10) | _BV(WGM12); // No prescaler -> 62.5kHz carrier

  // Timer 2: 50kHz interrupt for DDS sampling and wave generation
  TCCR2A = _BV(WGM21); // CTC mode
  TCCR2B = _BV(CS21);  // Prescaler 8 -> 2MHz clock
  OCR2A = 39;          // 2MHz / 40 = 50kHz sample rate
  TIMSK2 = _BV(OCIE2A);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(WHITE);
}

void loop() {
  // Read switches (0 = pressed to GND, 1 = unpressed via pullup)
  uint8_t s1 = digitalRead(sw1Pin);
  uint8_t s2 = digitalRead(sw2Pin);
  uint8_t current_state = (s1 << 1) | s2;

  int freq_raw = analogRead(freqPotPin);
  int amp_raw = analogRead(ampPotPin);

  // Map analog readings to DDS parameters
  uint16_t new_phase_inc = map(freq_raw, 0, 1023, 10, 2500); 
  uint8_t new_amp_val = map(amp_raw, 0, 1023, 0, 255);

  // Atomic update for ISR variables
  noInterrupts();
  wave_state = current_state;
  phase_inc = new_phase_inc;
  amp_val = new_amp_val;
  interrupts();

  // Calculate true frequency for display output
  float freq_hz = (new_phase_inc * 50000.0) / 65536.0;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("State: ");
  
  switch (current_state) {
    case 0b00: display.println("00 (Square)"); break;
    case 0b01: display.println("01 (Sine)"); break;
    case 0b10: display.println("10 (PWM)"); break;
    case 0b11: display.println("11 (Triangle)"); break;
  }
  
  display.print("Freq: "); display.print(freq_hz, 1); display.println(" Hz");
  display.print("Gain/Duty: "); display.print((new_amp_val / 255.0) * 100, 0); display.println("%");
  display.display();
  
  delay(100);
}

// DDS Core: Fires at 50kHz to output the next wave sample
ISR(TIMER2_COMPA_vect) {
  phase += phase_inc;
  uint8_t index = phase >> 8;
  uint8_t sample = 0;

  switch (wave_state) {
    case 0b00: // Square
      sample = (index < 128) ? 255 : 0;
      sample = (sample * amp_val) >> 8; // Amplitude scaling
      break;
    case 0b01: // Sine
      sample = sine_table[index];
      sample = (sample * amp_val) >> 8;
      break;
    case 0b10: // PWM (Amplitude pot defines duty cycle width)
      sample = (index < amp_val) ? 255 : 0;
      break;
    case 0b11: // Triangle
      sample = (index < 128) ? (index * 2) : (255 - ((index - 128) * 2));
      sample = (sample * amp_val) >> 8;
      break;
  }
  
  OCR1A = sample; // Output to Timer 1 PWM (Pin 9)
}