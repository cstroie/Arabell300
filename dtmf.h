/**
  dtmf.h - DTMF generator

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

#ifndef DTMF_H
#define DTMF_H

#define ROWSCOLS 4

#include <Arduino.h>
#include "config.h"
#include "wave.h"


//     DTMF frequencies
//
//      1209 1336 1477 1633
//  697    1    2    3    A
//  770    4    5    6    B
//  852    7    8    9    C
//  941    *    0    #    D

static const uint16_t frqRows[ROWSCOLS] = { 697,  770,  852,  941};
static const uint16_t frqCols[ROWSCOLS] = {1209, 1336, 1477, 1633};

static const char dtmfRowsCols[ROWSCOLS][ROWSCOLS] = {
  { '1', '2', '3', 'A'},
  { '4', '5', '6', 'B'},
  { '7', '8', '9', 'C'},
  { '*', '0', '#', 'D'}
};

// States in DTMF finite states machines
enum DTMF_STATE {DTMF_DISB, DTMF_WAVE, DTMF_SLNC};

class DTMF {
  public:
    DTMF(uint8_t pulse = 40, uint8_t pause = 40);
    ~DTMF();

    // The resulting sample
    uint8_t sample;

    bool getSample();
    char send(char chr);
    uint8_t send(char *chrs, size_t len);
    void setDuration(uint8_t pulse, uint8_t pause = 0);

  private:
    // The DTMF wave generator
    WAVE wave;
    // Wave steps (Q6.2)
    uint8_t stpRows[ROWSCOLS];
    uint8_t stpCols[ROWSCOLS];
    // Wave indices for row and col frequencies (Q14.2)
    uint16_t rowIdx, colIdx;
    // Identified row and col
    uint8_t row, col;
    // Wave generator status
    uint8_t state = DTMF_DISB;

    // Pulse and pause length (in samples) and counter
    uint16_t lenPulse;
    uint16_t lenPause;
    uint16_t counter;


    bool getRowCol(char chr);
};

#endif /* DTMF_H */
