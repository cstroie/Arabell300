/**
  dtmf.cpp - DTMF generator

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

#include "dtmf.h"

DTMF::DTMF(uint8_t pulse, uint8_t pause) {
  // Set the duration
  this->setDuration(pulse, pause);
  // Initialize the indices
  rowIdx = 0;
  colIdx = 0;
  // Initilize the state
  state = DTMF_DISB;
}

DTMF::~DTMF() {
}

/**
  Set DTMF digit duration and the silence interval between adjacent digits
*/
void DTMF::setDuration(uint8_t pulse, uint8_t pause) {
  // Make the pause equal to pulse, if not specified
  if (pause == 0) pause = pulse;
  // Compute the wave steps, as Q6.2
  for (uint8_t i = 0; i < ROWSCOLS; i++) {
    stpRows[i] = wave.getStep(frqRows[i]);
    stpCols[i] = wave.getStep(frqCols[i]);
  }
  // Compute the pulse and pause samples
  lenPulse = (uint32_t)pulse * F_SAMPLE / 1000;
  lenPause = (uint32_t)pause * F_SAMPLE / 1000;
}

/**
  Get the next DTMF sample, according to the DTMF state
*/
bool DTMF::getSample() {
  if (state == DTMF_WAVE) {
    sample = (wave.sample(rowIdx) >> 1) +
             (wave.sample(colIdx) >> 1);
    // Step up the indices for the next samples
    rowIdx = (rowIdx + stpRows[row]) & 0x03FF;
    colIdx = (colIdx + stpCols[col]) & 0x03FF;
    // Step up the samples counter to the pulse length
    if (++counter > lenPulse) {
      // Silence and reset the sample and the counter
      state = DTMF_SLNC;
      counter = 0;
      sample = 0x80;
    }
    return true;
  }
  else if (state == DTMF_SLNC) {
    // Step up the counter to pause lenght
    if (++counter > lenPause) {
      // Disable and reset the counter
      state = DTMF_DISB;
      counter = 0;
    }
    return true;
  }
  return false;
}

/**
  Start the DTMF generator to send a char

  @param chr the character to send
*/
char DTMF::send(char chr) {
  if (getRowCol(chr)) {
    // Start the wave
    state = DTMF_WAVE;
    counter = 0;
    return chr;
  }
  else {
    // Disable
    state = DTMF_DISB;
    return '\0';
  }
}

/**
  Send the buffer as DTMF tones

  @param chrs the buffer to send
  @param len the buffer length
*/
uint8_t DTMF::send(char *chrs, size_t len) {
  for (uint8_t i = 0; i < len; i++)
    if (chrs[i] != '\0')
      this->send(chrs[i]);
    else
      break;
}

/**
  Get the row and column of the character in the DTMF table

  @param chr the character to test
*/
bool DTMF::getRowCol(char chr) {
  for (row = 0; row < ROWSCOLS; row++) {
    for (col = 0; col < ROWSCOLS; col++) {
      if (chr == dtmfRowsCols[row][col]) {
        return true;
      }
    }
  }
  return false;
}
