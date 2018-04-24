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

// Sampling frequency
#define FRQ_SAMPLE  9600
#define BAUD        300
#define STARTBITS   1
#define STOPBITS    1
// Mark and space
#define MARK        1
#define SPACE       0

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
                          (wvPtsFull * (uint32_t)txMark + FRQ_SAMPLE / 2) / FRQ_SAMPLE  // 34
                         };
const uint8_t txMaxSamples = FRQ_SAMPLE / BAUD;

enum TX_STATE {HEAD, STARTBIT, DATABIT, STOPBIT, TAIL};

// Keep track if we are sending the start (1) or
// stop bit (3), the data bits (2) or nothing (0)
uint8_t txState = HEAD;
uint8_t txOn = 0;
uint8_t txData;
// Momentary TX index
uint8_t txIdx = 0;
// Momentary counter
uint8_t txCountSamples = 0;
// Keep track of what we are TX'ing now
uint8_t txIdxStep;
uint8_t txBit;
uint8_t txBitCount;

uint8_t txCarrierCount = 0;
uint8_t txCarrierMax = 10; // BAUD


// Queue
uint8_t qQueue[16] = {0};
uint8_t qIdxIn = 0;
uint8_t qIdxOut = 0;

inline bool qFull() {
  return (qIdxOut > qIdxIn) ? (qIdxOut - qIdxIn == 1) : (qIdxOut - qIdxIn + 16 == 1);
}

inline bool qEmpty() {
  return qIdxIn == qIdxOut;
}

inline bool qLen() {
  return (qIdxIn < qIdxOut) ? (16 + qIdxIn - qIdxOut) : (qIdxIn - qIdxOut);
}

inline bool qPush(uint8_t x) {
  if (not qFull()) {
    qQueue[qIdxIn] = x;
    qIdxIn = (qIdxIn + 1) & 0x0F;
  }
}

inline uint8_t qPop() {
  uint8_t x = 0;
  if (not qEmpty()) {
    x = qQueue[qIdxOut];
    qIdxOut = (qIdxOut + 1) & 0x0F;
  }
  return x;
}

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
  txState = HEAD;
  txCarrierCount = 0;
  txBit = MARK;
  txIdx = 0;
  for (uint8_t i = 0; i < len; i++) {
    qPush(chr[i]);
  }
  while (txOn) wvOut();
}

uint8_t wvOut() {
  // First thing first: output the sample
  uint8_t result = wvSample(txIdx);

  // Check if we are TX'ing
  if (txOn > 0) {

    //Serial.print(txBit * 100);
    //Serial.print(" ");
    //Serial.print(txCountSamples);
    //Serial.print(" ");
    Serial.print((result >> 4) & 0x0F, 16);
    Serial.print(result & 0x0F, 16);


    // Check if we have sent all samples for a bit
    if (txCountSamples >= txMaxSamples) {
      // Reset the samples counter
      txCountSamples = 0;
      // Get the next bit to send, according to current state
      switch (txState) {
        case HEAD:  // We are sending the head carrier
          if (++txCarrierCount >= txCarrierMax) {
            // Carrier sent, go to the start bit
            txState = STARTBIT;
            txBit = SPACE;
          }
          break;
        case STARTBIT:  // We have sent the start bit, go on with data bits
          txState = DATABIT;
          txBit = txData & 0x01;
          txData = txData >> 1;
          txBitCount = 0;
          break;
        case DATABIT:
          // We are sending the data bits, count them
          if (++txBitCount < 8) {
            // Keep sending the data bits
            txBit = txData & 0x01;
            txData = txData >> 1;
          }
          else {
            // We have sent all the data bits, send the stop bit
            txState = STOPBIT;
            txBit = MARK;
          }
          break;
        case STOPBIT:
          // We have sent the stop bit, get the next byte
          if (qEmpty()) {
            txState = TAIL;
            txBit = MARK;
            txCarrierCount = 0;
          }
          else {
            txState = STARTBIT;
            txBit = SPACE;
            txData = qPop();
            Serial.println();
          }
          break;
        case TAIL:
          // We are sending the tail (carrier)
          if (++txCarrierCount >= txCarrierMax)
            txOn = 0;
          break;
      }

      // Check if we switched the bit
      if (txIdxStep != wvStep[txBit]) {
        // Use the new step
        txIdxStep = wvStep[txBit];
        // Reset the samples counter
        txCountSamples = 0;
      }
    }

    // Increase the index and counter for next sample
    txIdx += txIdxStep;
    txCountSamples++;
  }
  return result;
}


/**
  Main Arduino setup function
*/
void setup() {
  Serial.begin(9600);

  char text[] = "Mama are mere.";
  txSend(text, strlen(text));
}

/**
  Main Arduino loop
*/
void loop() {

}
