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

  Many thanks to:
    Kamal Mostafa   https://github.com/kamalmostafa/minimodem
    Mark Qvist      https://github.com/markqvist/MicroModemGP
*/

#ifndef AFSK_H
#define AFSK_H

//#define DEBUG_RX
//#define DEBUG_RX_LVL

// The PWM pin may be 3 or 11 (Timer2)
#define PWM_PIN 3

// CPU frequency correction for sampling timer
#define F_COR (-120000L)

// Sampling frequency
#define F_SAMPLE    9600

#include <Arduino.h>
#include "config.h"
#include "fifo.h"
#include "wave.h"

// Mark and space bits
enum BIT {SPACE, MARK};
// States in RX and TX finite states machines
enum TXRX_STATE {WAIT, PREAMBLE, START_BIT, DATA_BIT, STOP_BIT, TRAIL};
// Connection direction
enum DIRECTION {ORIGINATING, ANSWERING};

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

// Frequencies, wave index steps, autocorrelation queue length
struct AFSK_FSQ_t {
  uint16_t  freq[2];  // Frequencies for SPACE and MARK
  uint8_t   step[2];  // Wave index steps for SPACE and MARK
  uint8_t   queuelen; // Autocorrelation queue length
  uint8_t   polarity; // Symbol polarity for specified queue
};

// AFSK configuration structure
struct AFSK_t {
  AFSK_FSQ_t  orig;   // Data for originating
  AFSK_FSQ_t  answ;   // Data for answering
  uint16_t    baud;   // Baud rate
  uint8_t     dtbits; // Data bits count
  uint8_t     duplex; // 0: HalfDuplex, 1: Duplex
};


// Bell103 configuration
static AFSK_t BELL103 = {
  {
    {1070, 1270}, // 934uS  14953CPU, 787uS  12598CPU
    {0, 0},
    10, 0
  },
  {
    {2025, 2225}, // 493uS   7901CPU, 449uS   7191CPU
    {0, 0},
    8,  0
  },
  300,
  8,
  1,
};

// V.21 configuration
static AFSK_t V_21 = {
  {
    {1180,  980},
    {0, 0},
    11, 0
  },
  {
    {1850, 1650},
    {0, 0},
    7, 0
  },
  300,
  8,
  1,
};

// Bell202 configuration
static AFSK_t BELL202 = {
  {
    {2200, 1200},
    {0, 0},
    4, 0
  },
  {
    {2200, 1200},
    {0, 0},
    4, 0
  },
  1200,
  8,
  0,
};

// V.23 Mode1 configuration
static AFSK_t V_23_M1 = {
  {
    {1700, 1300},
    {0, 0},
    8, 0
  },
  {
    {1700, 1300},
    {0, 0},
    8, 0
  },
  600,
  8,
  0,
};

// V.23 Mode2 configuration
static AFSK_t V_23_M2 = {
  {
    {2100, 1300},
    {0, 0},
    4, 0,
  },
  {
    {2100, 1300},
    {0, 0},
    4, 0,
  },
  1200,
  8,
  1
};

// RTTY configuration
static AFSK_t RTTY = {
  {
    {2295, 2125},
    {0, 0},
    12, 0
  },
  {
    {2295, 2125},
    {0, 0},
    12, 0,
  },
  4545,
  5,
  0
};


class AFSK {
  public:
    uint8_t dataMode  = 0;      // Modem works in data mode or in command mode
    uint8_t bias      = 0x80;   // Input line level bias
    uint8_t carrier   = 240;    // Number of carrier bits to send in preamble and trail

#ifdef DEBUG_RX_LVL
    uint8_t inLevel   = 0x00;   // Get the input level
#endif

    AFSK();
    ~AFSK();

    void init(AFSK_t afsk, CFG_t *cfg);
    void initSteps();
    void setModemType(AFSK_t afsk);
    void setDirection(uint8_t dir);
    void handle();
    void serial();

    void simFeed();             // Simulation
    void simPrint();

  private:
    AFSK_t  _afsk;
    CFG_t  *_cfg;

    uint8_t _dir = ORIGINATING;
    uint8_t fulBit, hlfBit, qrtBit, octBit;

    TX_t tx;
    RX_t rx;

    AFSK_FSQ_t *fsqTX;
    AFSK_FSQ_t *fsqRX;

    inline void DAC(uint8_t sample);
    void initHW();
    void txHandle();
    void rxHandle(uint8_t sample);
    void rxDecoder(uint8_t bt);

#ifdef DEBUG_RX_LVL
    // Count input samples and get the minimum, maximum and input level
    uint8_t inSamples = 0;
    uint8_t inMin     = 0xFF;
    uint8_t inMax     = 0x00;
#endif
};

#endif /* AFSK_H */
