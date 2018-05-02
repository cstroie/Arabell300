/**
  Bell103 - An Arduino-based Bell 103-compatible modem (300 baud)

  Copyright (C) 2018 Costin STROIE <costinstroie@eridu.eu.org>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Bell 103: 300 8N1
*/

#include <util/atomic.h>

#include "afsk.h"

//#define DEBUG





// Phase increments for SPACE and MARK in RX (fixed point arithmetics)
const int32_t phSpceInc = (1L << 16) * BELL103.frqAnsw[SPACE] / F_SAMPLE;   // 13824
const int32_t phMarkInc = (1L << 16) * BELL103.frqAnsw[MARK] / F_SAMPLE;    // 15189
const uint8_t logTau    = 4;                                // tau = 64 / SAMPLING_FREQ = 6.666 ms

// Demodulated (I, Q) amplitudes for SPACE and Mark
volatile int16_t rxSpceI, rxSpceQ, rxMarkI, rxMarkQ;

// Power for SPACE and Mark
uint16_t pwSpce, pwMark;



// Modem configuration
CFG_t cfg;

#ifdef DEBUG_RX_LVL
// Count input samples and get the minimum, maximum and input level
uint8_t inSamples = 0;
int8_t  inMin     = 0x7F;
int8_t  inMax     = 0x80;
uint8_t inLevel   = 0x00;
#endif

// Define the modem
AFSK afsk;


/**
  ADC Interrupt vector, called for each sample, which calls both the handlers
*/
ISR(ADC_vect) {
  TIFR1 = _BV(ICF1);
  afsk.handle();
}

/**
  Main Arduino setup function
*/
void setup() {
  Serial.begin(9600);


  // TC1 Control Register B: No prescaling, WGM mode 12
  TCCR1A = 0;
  TCCR1B = _BV(CS10) | _BV(WGM13) | _BV(WGM12);
  // Top set for 9600 baud
  ICR1 = ((F_CPU + F_COR) / F_SAMPLE) - 1;

  // Vcc with external capacitor at AREF pin, ADC Left Adjust Result
  ADMUX = _BV(REFS0) | _BV(ADLAR);

  // Analog input A0
  // Port C Data Direction Register
  DDRC  &= ~_BV(0);
  // Port C Data Register
  PORTC &= ~_BV(0);
  // Digital Input Disable Register 0
  DIDR0 |= _BV(0);

  // Timer/Counter1 Capture Event
  ADCSRB = _BV(ADTS2) | _BV(ADTS1) | _BV(ADTS0);
  // ADC Enable, ADC Start Conversion, ADC Auto Trigger Enable,
  // ADC Interrupt Enable, ADC Prescaler 16
  ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADATE) | _BV(ADIE) | _BV(ADPS2);


#ifdef PWM_DAC
  // Set up Timer 2 to do pulse width modulation on the PWM PIN
  // Use internal clock (datasheet p.160)
  ASSR &= ~(_BV(EXCLK) | _BV(AS2));

  // Set fast PWM mode  (p.157)
  TCCR2A |= _BV(WGM21) | _BV(WGM20);
  TCCR2B &= ~_BV(WGM22);

#if PWM_PIN == 11
  // Configure the PWM PIN 11 (PB3)
  PORTB &= ~(_BV(PORTB3));
  DDRD  |= _BV(PORTD3);
  // Do non-inverting PWM on pin OC2A (p.155)
  // On the Arduino this is pin 11.
  TCCR2A = (TCCR2A | _BV(COM2A1)) & ~_BV(COM2A0);
  TCCR2A &= ~(_BV(COM2B1) | _BV(COM2B0));
  // No prescaler (p.158)
  TCCR2B = (TCCR2B & ~(_BV(CS12) | _BV(CS11))) | _BV(CS10);
  // Set initial pulse width to the first sample, progresively
  for (uint8_t i = 0; i <= wvSmpl[0]; i++)
    OCR2A  = i;
#else
  // Configure the PWM PIN 3 (PD3)
  PORTD &= ~(_BV(PORTD3));
  DDRD  |= _BV(PORTD3);
  // Do non-inverting PWM on pin OC2B (p.155)
  // On the Arduino this is pin 3.
  TCCR2A = (TCCR2A | _BV(COM2B1)) & ~_BV(COM2B0);
  TCCR2A &= ~(_BV(COM2A1) | _BV(COM2A0));
  // No prescaler (p.158)
  TCCR2B = (TCCR2B & ~(_BV(CS12) | _BV(CS11))) | _BV(CS10);
  // Set initial pulse width to the first sample, progresively
  for (uint8_t i = 0; i <= wvSmpl[0]; i++)
    OCR2B  = i;
#endif // PWM_PIN
#else
  // Configure resistor ladder DAC
  DDRD |= 0xF8;
#endif

  // Leds
  DDRB |= _BV(PORTB1) | _BV(PORTB0);


  // Modem configuration
  cfg.txCarrier = 1;  // Keep a carrier going when transmitting


  // Define and configure the afsk
  afsk.init(BELL103, cfg);

}

/**
  Main Arduino loop
*/
void loop() {

  afsk.serialHandle();

#ifdef DEBUG_RX_LVL
  static uint32_t next = millis();
  if (millis() > next) {
    Serial.println(inLevel);
    next += 8 * 1000 / BAUD;
  }
#endif

  /*
    // Simulation
    static uint8_t rxIdx = 0;
    rxHandle((wave.sample(rxIdx) - 128) / 2);
    rxIdx += BELL103.stpAnsw[SPACE];
  */

#ifdef DEBUG
  static uint32_t next = millis();
  if (millis() > next) {
    Serial.print(rx.iirX[1]); Serial.print(" "); Serial.println(rx.iirY[1]);
    next += 100;
  }
#endif
}
