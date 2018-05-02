/**
  hmdyne.h - Homodyne detector

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

#ifndef HMDYNE_H
#define HMDYNE_H

#include <Arduino.h>

class HMDYNE {
  public:
    // log(t)
    uint8_t logTau;

    HMDYNE(uint16_t f, uint16_t fSmpl);
    ~HMDYNE();

    uint16_t getPower(uint8_t sample);

  private:
    // Sampling frequency
    uint16_t _fSmpl;
    // Frequency
    uint16_t _frq;
    // Phase increment
    int32_t _phInc;
    // Demodulated (I, Q) amplitudes
    volatile int16_t I, Q;
};

#endif /* HMDYNE_H */
