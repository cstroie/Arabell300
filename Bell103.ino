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

  Bell 103:
    300 baud
    1 bit start
    1 bit stop
    0 parity bits
    8 data bits
    bit length: 3.33ms


*/

#include "fifo.h"


// Sampling frequency
#define FRQ_SAMPLE  9600
#define BAUD        300
#define DATABITS    8
#define STARTBITS   1
#define STOPBITS    1
// Mark and space
enum TX_BIT {SPACE, MARK, NONE};

// Mark and space, TX and RX frequencies
const uint16_t txSpce = 1070;  // 934uS  14953CPU
const uint16_t txMark = 1270;  // 787uS  12598CPU
const uint16_t rxSpce = 2025;  // 493uS   7901CPU
const uint16_t rxMark = 2225;  // 449uS   7191CPU

// Here are only the first quarter wave points
const uint8_t wvForm[] = {0x80, 0x83, 0x86, 0x89, 0x8c, 0x8f, 0x92, 0x95,
                          0x98, 0x9b, 0x9e, 0xa2, 0xa5, 0xa7, 0xaa, 0xad,
                          0xb0, 0xb3, 0xb6, 0xb9, 0xbc, 0xbe, 0xc1, 0xc4,
                          0xc6, 0xc9, 0xcb, 0xce, 0xd0, 0xd3, 0xd5, 0xd7,
                          0xda, 0xdc, 0xde, 0xe0, 0xe2, 0xe4, 0xe6, 0xe8,
                          0xea, 0xeb, 0xed, 0xee, 0xf0, 0xf1, 0xf3, 0xf4,
                          0xf5, 0xf6, 0xf8, 0xf9, 0xfa, 0xfa, 0xfb, 0xfc,
                          0xfd, 0xfd, 0xfe, 0xfe, 0xfe, 0xff, 0xff, 0xff
                         };
// Count wavepoints quarter, half and full
const uint8_t   wvPtsQart = sizeof(wvForm) / sizeof(*wvForm);
const uint8_t   wvPtsHalf = 2 * wvPtsQart;
const uint16_t  wvPtsFull = 2 * wvPtsHalf;
// Wave index steps
const uint8_t wvStep[] = {(wvPtsFull * (uint32_t)txSpce + FRQ_SAMPLE / 2) / FRQ_SAMPLE, // 29
                          (wvPtsFull * (uint32_t)txMark + FRQ_SAMPLE / 2) / FRQ_SAMPLE, // 34
                          0
                         };
const uint8_t txMaxSamples = FRQ_SAMPLE / BAUD; // Number of samples to send for each serial bit
uint8_t txCountSamples = 0;        // Samples counter for each serial bit

enum TX_STATE {HEADCARR, STARTBIT, DATABIT, STOPBIT, TAILCARR, OFFCARR};

// Keep track if we are sending the start (1) or
// stop bit (3), the data bits (2) or nothing (0)
uint8_t txState = OFFCARR;
uint8_t txOn = 0;
uint8_t txData;
// Momentary TX index
uint8_t txIdx = 0;
// Keep track of what we are TX'ing now
uint8_t txIdxStep;
uint8_t txBit = NONE;
uint8_t txBitCount;

uint8_t txCarrierCount  = 0;
uint8_t txCarrierMax    = 240;  // 800ms


// FIFO
FIFO txFIFO(5);


/**
  Linear interpolation

  @param v0 previous value (8 bits)
  @param v1 next value     (8 bits)
  @param t                 (4 bits)
  @result interpolated value
*/
uint8_t lerp(uint8_t v0, uint8_t v1, uint8_t t) {
  return ((int16_t)v0 * (0x10 - t) + (int16_t)v1 * t) >> 4;
}

/**
  Output an instant wave value

  @param t the wave phase (0..255)
*/
inline uint8_t wvSample(uint8_t idx) {
  // Work on half wave
  uint8_t hIdx = idx & 0x7F;
  // Check if in second quarter wave
  if (hIdx >= wvPtsQart)
    // Descending slope
    hIdx = wvPtsHalf - hIdx - 1;
  // Get the value
  uint8_t result = wvForm[hIdx];
  // Check if in second half wave
  if (idx >= wvPtsHalf)
    // Under X axis
    result = 0xFF - result;
  return result;
}

/**
  Send char array
*/
uint8_t txSend(char* chr, size_t len) {
  txOn = 1;
  //txState = OFFCARR;
  for (uint8_t i = 0; i < len; i++) {
    txFIFO.in(chr[i]);
  }
  //while (true) wvOut();
}

uint8_t wvOut() {
  // First thing first: get the sample
  uint8_t result = wvSample(txIdx);

  // Check if we are TX'ing
  if (txOn > 0) {
    // Output the sample
    PORTD = (result & 0xF0) | _BV(3);

    //Serial.print(txBit * 100);
    //Serial.print(" ");
    //Serial.print(txCountSamples);
    //Serial.print(" ");
    //Serial.print(txState * 100);
    //Serial.print(" ");
    //Serial.println(result);

    //Serial.print((result >> 4) & 0x0F, 16);
    //Serial.print(result & 0x0F, 16);


    // Check if we have sent all samples for a bit.
    if (txCountSamples >= txMaxSamples) {
      // Reset the samples counter
      txCountSamples = 0;

      // One bit finished, check the state and choose the next bit and state
      switch (txState) {

        // We have been off, prepare the transmission
        case OFFCARR:
          txState = HEADCARR;
          txBit = MARK;
          txCarrierCount = 0;
          txIdx = 0;
          if (not txFIFO.empty())
            txData = txFIFO.out();
          break;

        // We are sending the head carrier
        case HEADCARR:
          // Check if we have sent the carrier long enough
          if (++txCarrierCount >= txCarrierMax) {
            // Carrier sent, go to the start bit
            txState = STARTBIT;
            txBit = SPACE;
          }
          break;

        // We have sent the start bit, go on with data bits
        case STARTBIT:
          txState = DATABIT;
          txBitCount = 0;
          // LSB first
          txBit = txData & 0x01;
          txData = txData >> 1;
          break;

        // We are sending the data bits, count them
        case DATABIT:
          // Check if we have sent all the bits
          if (++txBitCount < DATABITS) {
            // Keep sending the data bits, LSB to MSB
            txBit = txData & 0x01;
            txData = txData >> 1;
          }
          else {
            // We have sent all the data bits, send the stop bit
            txState = STOPBIT;
            txBit = MARK;
          }
          break;

        // We have sent the stop bit, try to get the next byte
        case STOPBIT:
          // Check the TX FIFO
          if (txFIFO.empty()) {
            // No more data to send, go on with the tail carrier
            txState = TAILCARR;
            txBit = MARK;
            txCarrierCount = 0;
          }
          else {
            // There is more data, get it and return to the start bit
            txState = STARTBIT;
            txBit = SPACE;
            txData = txFIFO.out();
            //Serial.println();
          }
          break;

        // We are sending the tail carrier
        case TAILCARR:
          // Check if we have sent the carrier long enough
          if (++txCarrierCount > txCarrierMax) {
            // Disable TX
            txOn = 0;
            txState = OFFCARR;
          }
          else if (txCarrierCount == txCarrierMax) {
            // After the last tail carrier bit, send isoelectric line
            txBit = NONE;
            // Prepare for the next TX
            txIdx = 0;
            txCountSamples = 0;
          }
          // Check the TX FIFO again...
          else if (not txFIFO.empty()) {
            // There is new data in FIFO, start over
            txState = STARTBIT;
            txBit = SPACE;
            txData = txFIFO.out();
          }
          break;
      }

      // Check if we switched the bit
      if (txIdxStep != wvStep[txBit]) {
        // Use the step of the new bit
        txIdxStep = wvStep[txBit];
        // Reset the samples counter
        txCountSamples = 0;
      }
    }

    // Increase the index and counter for next sample
    txIdx += txIdxStep;
    txCountSamples++;
  }
  //return result;
}

void checkSerial() {
  // Check any command on serial port
  uint8_t c = Serial.peek();
  if (c != 0xFF) {
    // There is data on serial port
    if (not txFIFO.full()) {
      // We can send the data
      c = Serial.read();
      txFIFO.in(c);
      // Keep TX'ing
      txOn = 1;
    }
  }
}

// ADC Interrupt vector
ISR(ADC_vect) {
  TIFR1 = _BV(ICF1);
  wvOut();
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
  ICR1 = (F_CPU / FRQ_SAMPLE) - 1;

  // Vcc with external capacitor at AREF pin, ADC Left Adjust Result
  ADMUX = _BV(REFS0) | _BV(ADLAR);

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

  // DAC init
  DDRD |= 0xF8;
}

/**
  Main Arduino loop
*/
void loop() {
  checkSerial();
}
