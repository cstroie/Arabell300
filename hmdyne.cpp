/**
  hmdyne.cpp - Homodyne detector

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

#include "hmdyne.h"


HMDYNE::HMDYNE(uint16_t f, uint16_t fSmpl): _frq(f), _fSmpl(fSmpl) {
  logTau = 4;
  _phInc = (1L << 16) * _frq / _fSmpl;
}

HMDYNE::~HMDYNE() {
}

uint16_t HMDYNE::getPower(uint8_t sample) {
  int8_t x, y;
  static uint16_t phase;

  // Update the phase of the local oscillator
  phase += _phInc;
  // Multiply the sample by square waves in quadrature
  x = sample;
  if (((phase >> 8) + 0x00) & 0x80)
    x = -1 - x;
  y = sample;
  if (((phase >> 8) + 0x40) & 0x80)
    y = -1 - y;
  // First order low-pass filter.
  I += x - (I >> logTau);
  Q += y - (Q >> logTau);

  int16_t i = I >> logTau;
  int16_t q = Q >> logTau;
  return (int8_t)i * (int8_t)i + (int8_t)q * (int8_t)q;
}

