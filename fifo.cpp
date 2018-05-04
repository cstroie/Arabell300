/**
  fifo.cpp - Simple FIFO

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

#include <stddef.h>
#include <util/atomic.h>
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

uint8_t FIFO::_full() {
  return ((i_out > i_in) ? (i_out - i_in == 1) : (i_out - i_in + _size == 1));
}

uint8_t FIFO::full() {
  uint8_t result;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    result = this->_full();
  }
  return result;
}

uint8_t FIFO::_empty() {
  return (i_in == i_out);
}

uint8_t FIFO::empty() {
  uint8_t result;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    result = this->_empty();
  }
  return result;
}

uint8_t FIFO::_len() {
  return ((i_out > i_in) ? (_size + i_in - i_out) : (i_in - i_out));
}

uint8_t FIFO::len() {
  uint8_t result;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    result = this->_len();
  }
  return result;
}

uint8_t FIFO::_clear() {
  i_in  = 0;
  i_out = 0;
}

uint8_t FIFO::clear() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    this->_clear();
  }
}

uint8_t FIFO::_in(uint8_t x) {
  if (not this->_full()) {
    buf[i_in] = x;
    i_in = (i_in + 1) & _mask;
  }
}

uint8_t FIFO::in(uint8_t x) {
  uint8_t result;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    result = this->_in(x);
  }
  return result;
}

uint8_t FIFO::_out() {
  uint8_t x = 0;
  if (i_in != i_out) {
    x = buf[i_out];
    i_out = (i_out + 1) & _mask;
  }
  return x;
}

uint8_t FIFO::out() {
  uint8_t result;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    result = this->_out();
  }
  return result;
}

uint8_t FIFO::_peek() {
  return buf[i_out];
}

uint8_t FIFO::peek() {
  uint8_t result;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    result = this->_peek();
  }
  return result;
}
