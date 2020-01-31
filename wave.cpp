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

  @param idx the wave sample index as Q8.8
*/
uint8_t WAVE::sample(uint16_t idx) {
  return this->sample((uint8_t)(idx / 256));
}

/**
  Compute the samples step for the given frequency, as Q8.8

  @param freq the frequency
  @return the step
*/
uint16_t WAVE::getStep(uint16_t freq) {
  return (uint16_t)(((uint32_t)freq * this->full * (1 << 9) / F_SAMPLE + 1) >> 1);
}
