/**
  wave.h - Wave generator

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

#ifndef WAVE_H
#define WAVE_H

#include <Arduino.h>

// The first quarter wave samples LUT
const uint8_t wavelut[] = {
  0x80, 0x83, 0x86, 0x89, 0x8c, 0x8f, 0x92, 0x95,
  0x98, 0x9b, 0x9e, 0xa2, 0xa5, 0xa7, 0xaa, 0xad,
  0xb0, 0xb3, 0xb6, 0xb9, 0xbc, 0xbe, 0xc1, 0xc4,
  0xc6, 0xc9, 0xcb, 0xce, 0xd0, 0xd3, 0xd5, 0xd7,
  0xda, 0xdc, 0xde, 0xe0, 0xe2, 0xe4, 0xe6, 0xe8,
  0xea, 0xeb, 0xed, 0xee, 0xf0, 0xf1, 0xf3, 0xf4,
  0xf5, 0xf6, 0xf8, 0xf9, 0xfa, 0xfa, 0xfb, 0xfc,
  0xfd, 0xfd, 0xfe, 0xfe, 0xfe, 0xff, 0xff, 0xff
};

class WAVE {
  public:
    // Samples count for quarter, half and full wave
    uint8_t   qart = sizeof(wavelut) / sizeof(*wavelut);;
    uint8_t   half = qart + qart;
    uint16_t  full = half + half;

    WAVE();
    ~WAVE();

    // Get the wave sample specified by index
    uint8_t sample(uint8_t idx);
};

#endif /* WAVE_H */
