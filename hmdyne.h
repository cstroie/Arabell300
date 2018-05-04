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


  For a detailed explanation, see http://arduino.stackexchange.com/a/21175

  Copyright (c) 2016 Edgar Bonet Orozco.
  Released under the terms of the MIT license:
  https://opensource.org/licenses/MIT

  https://gist.githubusercontent.com/edgar-bonet/0b03735d70366bc05fc6/raw/a93d9b09f1008db83b5232641d33cca3b387237d/homodyne.ino
*/

#ifndef HMDYNE_H
#define HMDYNE_H

#include <Arduino.h>

class HMDYNE {
  public:
    uint8_t logTau;    // log(t)

    HMDYNE(uint16_t f, uint16_t fSmpl);
    ~HMDYNE();

    uint16_t getPower(uint8_t sample);

  private:
    uint16_t _fSmpl;        // Sampling frequency
    uint16_t _frq;          // Signal frequency
    int32_t  _phInc;        // Phase increment
    uint16_t _phase;        // Phase
    volatile int16_t I, Q;  // Demodulated (I, Q) amplitudes
};

#endif /* HMDYNE_H */
