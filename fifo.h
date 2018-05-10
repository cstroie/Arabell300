/**
  fifo.h - Simple FIFO

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

#ifndef FIFO_H
#define FIFO_H

#include <Arduino.h>

class FIFO {
  public:
    FIFO(uint8_t bitsize = 4);
    ~FIFO();
    bool    full();
    bool    empty();
    uint8_t len();
    void    clear();
    bool    in(uint8_t x);
    uint8_t out();
    uint8_t peek();

  private:
    uint8_t* buf;
    uint8_t _bitsize;
    uint8_t _size;
    uint8_t _mask;
    uint8_t i_in  = 0;
    uint8_t i_out = 0;

    bool    _full();
    bool    _empty();
    uint8_t _len();
    void    _clear();
    bool    _in(uint8_t x);
    uint8_t _out();
    uint8_t _peek();
};

#endif /* FIFO_H */
