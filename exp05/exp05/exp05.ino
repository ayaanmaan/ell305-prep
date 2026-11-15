#include <avr/pgmspace.h>

// Custom DAC Pin Mapping (Ordered from LSB to MSB)
// DAC Pin:   12, 11, 10, 9, 8, 7, 6, 5 (MSB)
// Ardu Pin:   9,  8,  7, 6, 2, 3, 4, 5
const int dacPins[8] = {9, 8, 7, 6, 2, 3, 4, 5};

const int potAmpPin = A0;  // Amplitude (Volume) Control
const int potFreqPin = A1; // Frequency (Pitch/Speed) Control

// 64-sample Sine Wave (8-bit PCM, centered at 128)
const uint8_t audio_data[64] PROGMEM = {
  128, 140, 153, 165, 177, 188, 199, 209, 218, 226, 234, 240, 245, 250, 253, 254,
  255, 254, 253, 250, 245, 240, 234, 226, 218, 209, 199, 188, 177, 165, 153, 140,
  128, 115, 102,  90,  78,  67,  56,  46,  37,  29,  21,  15,  10,   5,   2,   1,
    0,   1,   2,   5,  10,  15,  21,  29,  37,  46,  56,  67,  78,  90, 102, 115
};

void setup() {
  for (int i = 0; i < 8; i++) {
    pinMode(dacPins[i], OUTPUT);
  }
}

void loop() {
  // Read potentiometer values (0 to 1023)
  int ampReading = analogRead(potAmpPin);
  int freqReading = analogRead(potFreqPin);

  // Scale amplitude reading into a multiplier (0.0 to 1.0)
  float amplitudeMultiplier = ampReading / 1023.0;

  // Scale frequency reading into a delay multiplier (e.g., 50 to 2000 microseconds)
  // Higher delay = slower playback (lower pitch)
  int delayMicrosec = map(freqReading, 0, 1023, 50, 2000);

  // Output the audio array
  for (int i = 0; i < 64; i++) {
    // 1. Fetch the raw sample from PROGMEM
    uint8_t rawSample = pgm_read_byte(&audio_data[i]);
    
    // 2. Amplitude Modulation
    // Remove the 128 DC offset, scale the wave, then add the DC offset back
    float ac_component = rawSample - 128.0;
    uint8_t modulatedSample = (uint8_t)((ac_component * amplitudeMultiplier) + 128);

    // 3. Write 8-bit value to your custom DAC pins
    for (int bit = 0; bit < 8; bit++) {
      digitalWrite(dacPins[bit], (modulatedSample >> bit) & 0x01);
    }

    // 4. Frequency Modulation
    delayMicroseconds(delayMicrosec);
  }
}