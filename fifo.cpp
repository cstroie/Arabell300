/**
  fifo.cpp - Simple FIFO

  Copyright (C) 2019 Costin STROIE <costinstroie@eridu.eu.org>

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

#include <stddef.h>
#include "fifo.h"

FIFO::FIFO(uint8_t bitsize): _bitsize(bitsize) {
  if (bitsize >= 8) {
    // Limit to 256
    _size = 0x00;
    _mask = 0xFF;
    buf = (uint8_t*)malloc(0x0100);
  }
  else {
    _size = (1 << _bitsize);
    _mask = _size - 1;
    buf = (uint8_t*)malloc(_size);
  }
}

FIFO::~FIFO() {
  free(buf);
}

bool FIFO::full() {
  return ((_size + i_out - i_in) & _mask) == 1;
}

bool FIFO::empty() {
  return (i_in == i_out);
}

uint8_t FIFO::len() {
  return (_size + i_in - i_out) & _mask;
}

void FIFO::clear() {
  i_in  = 0;
  i_out = 0;
}

bool FIFO::in(uint8_t x) {
  if (not this->full()) {
    buf[i_in] = x;
    i_in = (i_in + 1) & _mask;
    return true;
  }
  return false;
}

uint8_t FIFO::out() {
  uint8_t x = 0;
  if (i_in != i_out) {
    x = buf[i_out];
    i_out = (i_out + 1) & _mask;
  }
  return x;
}

uint8_t FIFO::peek() {
  return buf[i_out];
}
