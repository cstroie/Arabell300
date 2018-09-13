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
  state = 0;
}

DTMF::~DTMF() {
}

/**
  Set DTMF digit duration and the silence interval between adjacent digits
*/
void DTMF::setDuration(uint8_t pulse, uint8_t pause) {
  // Make the pause equal to pulse, if not specified
  if (pause == 0) pause = pulse;
  // Compute the wave steps
  for (uint8_t i = 0; i < ROWSCOLS; i++) {
    stpRows[i] = wave.getStep(frqRows[i]);
    stpCols[i] = wave.getStep(frqCols[i]);
  }
  // Compute the pulse and pause samples
  lenPulse = (uint32_t)pulse * F_SAMPLE / 1000;
  lenPause = (uint32_t)pause * F_SAMPLE / 1000;
}

bool DTMF::getSample() {
  if (state == 1) {
    sample = (wave.sample((uint8_t)((rowIdx >> 2) & 0x00FF)) >> 1) +
             (wave.sample((uint8_t)((colIdx >> 2) & 0x00FF)) >> 1);
    // Step up the indices for the next samples
    rowIdx += stpRows[row];
    colIdx += stpCols[col];
    if (counter < lenPulse)
      counter++;
    else {
      state = 2;
      counter = 0;
      sample = 0x80;
    }
    return true;
  }
  else if (state == 2) {
    if (counter < lenPause)
      counter++;
    else {
      state = 0;
      counter = 0;
    }
    return true;
  }
  return false;
}

char DTMF::send(char chr) {
  if (getRowCol(chr)) {
    state = 1;
    counter = 0;
    return chr;
  }
  else {
    state = 0;
    return '\0';
  }
}

uint8_t DTMF::send(char *chrs, size_t len) {
  for (uint8_t i = 0; i < len; i++)
    if (chrs[i] != '\0')
      this->send(chrs[i]);
    else
      break;
}

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
