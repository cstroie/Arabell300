/**
  wave.cpp - Wave generator

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

#include "wave.h"

WAVE::WAVE() {
}

WAVE::~WAVE() {
}

/**
  Get an instant wave sample

  @param idx the wave sample index
*/
uint8_t WAVE::sample(uint8_t idx) {
  // Work on half wave
  uint8_t hIdx = idx & 0x7F;
  // Check if in second quarter wave
  if (hIdx >= qart)
    // Descending slope
    hIdx = half - hIdx - 1;
  // Get the value
  uint8_t result = wavelut[hIdx];
  // Check if in second half wave
  if (idx >= half)
    // Under X axis
    result = 0xFF - result;
  return result;
}

/**
  Get an instant wave sample

  @param idx the wave sample index as Q14.2
*/
uint8_t WAVE::sample(uint16_t idx) {
  // Get the integer part of the index
  uint8_t idxi = (uint8_t)(idx >> 2);
  // Get the fractional part of the index
  uint8_t idxq = idx & 0x03;
  // Get the first sample (at idxi)
  uint8_t result = this->sample(idxi);
  // Do the linear interpolation only if the fractional part
  // is greater than zero
  if (idxq > 0) {
    // Get the next sample (at idxi + 1)
    uint8_t next = this->sample(idxi++);
    // Do the linear interpolation
    uint8_t delta = next - result;
    switch (idxq) {
      case 1:
        result = result + delta;
        break;
      case 2:
        result = result + delta + delta;
        break;
      case 3:
        result = next - delta;
        break;
    }
  }
  // Return the result
  return result;
}

/**
  Compute the samples step for the given frequency, as Q6.2

  @param freq the frequency
  @return the step
*/
uint8_t WAVE::getStep(uint16_t freq) {
  return (uint8_t)(((uint32_t)freq * (this->full << 3) / F_SAMPLE + 1) >> 1);
}
