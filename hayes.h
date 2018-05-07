/**
  hayes.h - AT-Hayes commands interface

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

#ifndef HAYES_H
#define HAYES_H

// Maximum input buffer size
#define MAX_INPUT_SIZE 65

// Several Hayes related globals
#define HAYES_NUM_ERROR -128


#include <Arduino.h>
#include <avr/wdt.h>

#include "config.h"
#include "afsk.h"

class HAYES {
  public:
    HAYES(CFG_t *cfg, AFSK *afsk);
    ~HAYES();

    int16_t getInteger(char* buf, int8_t idx, uint8_t len = 32);
    int16_t getValidInteger(char* buf, int8_t idx, int16_t low, int16_t hgh, int16_t def = 0, uint8_t len = 32);
    int16_t getValidInteger(int16_t low, int16_t hgh, int16_t def = 0, uint8_t len = 32);
    int8_t  getDigit(char* buf, int8_t idx);
    int8_t  getDigit(int8_t def = HAYES_NUM_ERROR);
    int8_t  getValidDigit(char* buf, int8_t idx, int8_t low, int8_t hgh, int8_t def = HAYES_NUM_ERROR);
    int8_t  getValidDigit(int8_t low, int8_t hgh, int8_t def = HAYES_NUM_ERROR);


    uint8_t handle();
    void    dispatch();

  private:
    CFG_t *_cfg;
    AFSK  *_afsk;

    // Input buffer
    char buf[MAX_INPUT_SIZE];
    // Line buffer length
    int8_t len = 0;
    // Buffer index
    uint8_t idx = 0;
    // Numeric value
    int8_t value = 0;

    // The result status of the last command
    uint8_t cmdResult = 0;

    void    cmdPrint(char cmd, uint8_t value, bool newline = true);
    void    cmdPrint(uint8_t value);
    void    showProfile(CFG_t *cfg);
};

#endif /* HAYES_H */
