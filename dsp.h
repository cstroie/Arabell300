/**
  dsp.h - Various digital filters

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


  Copyright (c) 2018 MicroModeler.

  A non-exclusive, nontransferable, perpetual, royalty-free license is granted to the Licensee to
  use the following Information for academic, non-profit, or government-sponsored research purposes.
  Use of the following Information under this License is restricted to NON-COMMERCIAL PURPOSES ONLY.
  Commercial use of the following Information requires a separately executed written license agreement.

  This Information is distributed WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef DSP_H
#define DSP_H

/**
  Linear interpolation

  @param v0 previous value (8 bits)
  @param v1 next value     (8 bits)
  @param t                 (4 bits)
  @result interpolated value
*/
uint8_t lerp(uint8_t v0, uint8_t v1, uint8_t t) {
  return ((int16_t)v0 * (0x10 - t) + (int16_t)v1 * t) >> 4;
}

/**
  BandStop filter for 2225Hz
  S:2190 T:2250 R:0.2
*/
int8_t bs2225(int8_t x0) {
  // Coefficients
  const int8_t b0 = 126,
               b1 = -30,
               b2 = 126,
               a1 =  15,
               a2 = -62;

  int16_t acc;
  int8_t w1, w2;

  // Run feedback part of filter
  acc  = (int16_t)w2 * a2;
  acc += (int16_t)w1 * a1;
  acc += (int16_t)x0 << 5;

  int16_t w0 = acc >> 6;

  // Run feedforward part of filter
  acc  = (int16_t)w0 * b0;
  acc += (int16_t)w1 * b1;
  acc += (int16_t)w2 * b2;

  // Shuffle history buffer
  w2 = w1;
  w1 = w0;

  // Return output
  return (acc >> 8);
}

/**
  LowPass filter for 200Hz
*/
int8_t lp200(int8_t x0) {
  // Coefficients
  const int8_t b0 =  38,
               b1 =  76,
               b2 =  38,
               a1 = 111,
               a2 = -49;

  int16_t acc;
  static int8_t w1, w2;

  // Run feedback part of filter
  acc  = (int16_t)w2 * a2;
  acc += (int16_t)w1 * a1;
  acc += (int16_t)x0;

  int16_t w0 = acc >> 6;

  // Run feedforward part of filter
  acc  = (int16_t)w0 * b0;
  acc += (int16_t)w1 * b1;
  acc += (int16_t)w2 * b2;

  // Shuffle history buffer
  w2 = w1;
  w1 = w0;

  // Return output
  return (acc >> 7);
}

/**
  LowPass filter for 600Hz
  S:600 R:0.2
*/
int8_t lp600(int8_t x0) {
  // Coefficients
  const int8_t b0 = 13,
               b1 = 13,
               b2 = 0,
               a1 = 37,
               a2 = 0;

  int16_t acc;
  static int8_t w1, w2;

  // Run feedback part of filter
  acc  = (int16_t)w2 * a2;
  acc += (int16_t)w1 * a1;
  acc += (int16_t)x0 << 4;

  int16_t w0 = acc >> 6;

  // Run feedforward part of filter
  acc  = (int16_t)w0 * b0;
  acc += (int16_t)w1 * b1;
  acc += (int16_t)w2 * b2;

  // Shuffle history buffer
  w2 = w1;
  w1 = w0;

  // Return output
  return (acc >> 7);
}

#endif /* DSP_H */
