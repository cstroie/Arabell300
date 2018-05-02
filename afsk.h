/**
  afsk.h - AFSK modulation/demodulation and serial decoding

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
*/

#ifndef AFSK_H
#define AFSK_H

//#define DEBUG_RX
//#define DEBUG_RX_LVL

// Use the PWM DAC (8 bits, one output PIN, uses Timer2) or
// the resistor ladder (4 bits, 4 PINS)
#define PWM_DAC

// The PWM pin may be 3 or 11 (Timer2)
#define PWM_PIN 3

// CPU frequency correction for sampling timer
#define F_COR (-120000L)

// Sampling frequency
#define F_SAMPLE    9600
#define BAUD        300
#define DATA_BITS   8
#define STRT_BITS   1
#define STOP_BITS   1
#define CARR_BITS   240
#define BIT_SAMPLES (F_SAMPLE / BAUD)


#include <Arduino.h>

#include "config.h"
#include "fifo.h"
#include "wave.h"



// Mark and space bits
enum BIT {SPACE, MARK, NONE};
// States in RX and TX finite states machines
enum TXRX_STATE {PREAMBLE, START_BIT, DATA_BIT, STOP_BIT, TRAIL, WAIT};


// Transmission related data
struct TX_t {
  uint8_t active  = 0;        // currently transmitting or not
  uint8_t state   = WAIT;     // TX state (TXRX_STATE enum)
  uint8_t dtbit   = MARK;     // currently transmitting data bit
  uint8_t data    = 0;        // transmitting data bits, shift out, LSB first
  uint8_t bits    = 0;        // counter of already transmitted bits
  uint8_t idx     = 0;        // wave samples index (start with first sample)
  uint8_t clk     = 0;        // samples counter for each bit
};

// Receiving and decoding related data
struct RX_t {
  uint8_t active  = 0;        // currently receiving proper signals or not
  uint8_t state   = WAIT;     // RX decoder state (TXRX_STATE enum)
  uint8_t data    = 0;        // the received data bits, shift in, LSB first
  uint8_t bits    = 0;        // counter of received data bits
  uint8_t stream  = 0;        // last 8 decoded bit samples
  uint8_t bitsum  = 0;        // sum of the last decoded bit samples
  uint8_t clk     = 0;        // samples counter for each bit
  int16_t iirX[2] = {0, 0};   // IIR Filter X cells
  int16_t iirY[2] = {0, 0};   // IIR Filter Y cells
};


struct AFSK_t {
  uint16_t  frqOrig[2]; // Frequencies for originating
  uint16_t  frqAnsw[2]; // Frequencies for answering
  uint8_t   stpOrig[2]; // Wave index steps for originating
  uint8_t   stpAnsw[2]; // Wave index steps for answering
  uint16_t  baud;       // Baud rate
  uint8_t   duplex;     // 0: HalfDuplex, 1: Duplex
};

// Bell103 frequencies (Hz)
static AFSK_t BELL103 = {
  {1070, 1270}, // 934uS  14953CPU, 787uS  12598CPU
  {2025, 2225}, // 493uS   7901CPU, 449uS   7191CPU
  {0, 0},
  {0, 0},
  300,
  1,
};

// V.21 frequencies (Hz)
static AFSK_t V_21 = {
  {1180,  980},
  {1850, 1650},
  {0, 0},
  {0, 0},
  300,
  1,
};

// Bell202 frequencies (Hz)
static AFSK_t BELL202 = {
  {2200, 1200},
  {2200, 1200},
  {0, 0},
  {0, 0},
  1200,
  0,
};

// V.23 Mode1 frequencies (Hz)
static AFSK_t V_23_M1 = {
  {1700, 1300},
  {1700, 1300},
  600,
  0,
};

// V.23 Mode2 frequencies (Hz)
static AFSK_t V_23_M2 = {
  {2100, 1300},
  {2100, 1300},
  {0, 0},
  {0, 0},
  1200,
  1,
};

// RTTY frequencies (Hz)
static AFSK_t RTTY = {
  {2295, 2125},
  {2295, 2125},
  {0, 0},
  {0, 0},
  4545,
  0,
};




class AFSK {
  public:

#ifdef DEBUG_RX_LVL
    // Get the input level
    uint8_t inLevel   = 0x00;
#endif

    AFSK();
    ~AFSK();

    void init(AFSK_t afskMode, CFG_t cfg);
    void initSteps();
    void handle();
    void serialHandle();

  private:
    AFSK_t  _afsk;
    CFG_t   _cfg;

    uint8_t bias = 0x80;

    TX_t tx;
    RX_t rx;

#ifdef DEBUG_RX_LVL
    // Count input samples and get the minimum, maximum and input level
    uint8_t inSamples = 0;
    int8_t  inMin     = 0x7F;
    int8_t  inMax     = 0x80;
#endif

    inline void DAC(uint8_t sample);
    void txHandle();
    void rxHandle(int8_t sample);
    void rxDecoder(uint8_t bt);

};

#endif /* AFSK_H */
