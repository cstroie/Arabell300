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

#include "fifo.h"

FIFO::FIFO(uint8_t bitsize): _bitsize(bitsize) {
  _size = (1 << _bitsize);
  _mask = _size - 1;
  buf = (uint8_t*)malloc(_size);
}

FIFO::~FIFO() {
  free(buf);
}

uint8_t FIFO::full() {
  return (_out > _in) ? (_out - _in == 1) : (_out - _in + _size == 1);
}

uint8_t FIFO::empty() {
  return _in == _out;
}

uint8_t FIFO::len() {
  return (_in < _out) ? (_size + _in - _out) : (_in - _out);
}

uint8_t FIFO::in(uint8_t x) {
  if (not this->full()) {
    buf[_in] = x;
    _in = (_in + 1) & _mask;
  }
}

uint8_t FIFO::out() {
  uint8_t x = 0;
  if (_in != _out) {
    x = buf[_out];
    _out = (_out + 1) & _mask;
  }
  return x;
}
