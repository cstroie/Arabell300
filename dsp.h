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
*/

#ifndef DSP_H
#define DSP_H

/**
  BandStop filter for 2225Hz
  S:2190 T:2250 R:0.2
*/
int8_t bs2225(int8_t x0) {
  // Coefficients
  static int8_t b0 = 126,
                b1 = -30,
                b2 = 126,
                a1 =  15,
                a2 = -62;

  static int16_t acc;
  static int8_t w1, w2;

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
  static int8_t b0 =  38,
                b1 =  76,
                b2 =  38,
                a1 = 111,
                a2 = -49;

  static int16_t acc;
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
  static int8_t b0 = 13,
                b1 = 13,
                b2 = 0,
                a1 = 37,
                a2 = 0;

  static int16_t acc;
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
