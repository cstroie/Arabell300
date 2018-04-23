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
const uint8_t wvStepMax = FRQ_SAMPLE / BAUD;

// Keep track if we are sending the start (1) or
// stop bit (3), the data bits (2) or nothing (0)
uint8_t txState = 0;
uint8_t txOn = 0;
uint8_t txData;
// Momentary TX index
uint8_t txIdx = 0;
// Momentary counter
uint8_t txCount = 0;
// Keep track of what we are TX'ing now
uint8_t txIdxStep;
uint8_t txBit;
uint8_t txBitCount;




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
  for (uint8_t i = 0; i < len; i++) {
    txData = chr[i];
    txOn = 1;
    while (txOn) wvOut();
  }
}

uint8_t wvOut() {
  // First thing first: get the result
  uint8_t result = wvSample(txIdx);

  // Check if we are TX'ing
  if (txOn > 0) {

    Serial.print(txBit * 100);
    Serial.print(" ");
    Serial.print(txCount);
    Serial.print(" ");
    Serial.println(result);


    // Check if we reached the maximum
    if (txCount >= wvStepMax) {
      // Reset the counter
      txCount = 0;
      // Get the next bit to send
      if (txState == 0) {
        // We have sent nothing, start with the start bit
        txState = 1;
        txBit = 1;
      }
      else if (txState == 1) {
        // We have sent the start bit, go on with data bits
        txState = 2;
        txBit = txData & 0x01;
        txData = txData >> 1;
        txBitCount = 0;
      }
      else if (txState == 2) {
        // We are sending the data bits, count them
        txBitCount++;
        if (txBitCount < 8) {
          // Keep sending the data bits
          txBit = txData & 0x01;
          txData = txData >> 1;
        }
        else {
          // We have sent all the data bits, send the stop bit
          txState = 3;
          txBit = 0;
        }
      }
      else if (txState == 3) {
        // We have sent the stop bit, disable TX
        txState = 0;
        txBit = 1;
        txOn = 0;
      }
    }

    // Check if we switched the tone
    if (txIdxStep != wvStep[txBit]) {
      // Use the new step
      txIdxStep = wvStep[txBit];
      // Reset the counter
      txCount = 0;
    }

    // Increase the index and counter for next sample
    txIdx += wvStep[txBit];
    txCount++;



  }
  return result;
}


/**
  Main Arduino setup function
*/
void setup() {
  Serial.begin(9600);

  //Serial.println(wvMaxCount[SPACE]);
  //Serial.println(wvMaxCount[MARK]);

  /*
    txOn = 1;
    txData = 0xAA;
    for (uint16_t x = 0; x <= 400; x++) {
      uint8_t y = wvOut();
      delay(1);
    }
  */

  char text[] = "Mama are mere.";
  txSend(text, strlen(text));

}

/**
  Main Arduino loop
*/
void loop() {

}
