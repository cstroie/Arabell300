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

// Result codes
enum RESULT_CODES {RC_OK, RC_CONNECT, RC_RING, RC_NO_CARRIER, RC_ERROR,
                   RC_CONNECT_1200, RC_NO_DIALTONE, RC_BUSY, RC_NO_ANSWER,
                   RC_NONE = 255
                  };
const char rcOK[]           PROGMEM = "OK";
const char rcCONNECT[]      PROGMEM = "CONNECT";
const char rcRING[]         PROGMEM = "RING";
const char rcNO_CARRIER[]   PROGMEM = "NO CARRIER";
const char rcERROR[]        PROGMEM = "ERROR";
const char rcCONNECT_1200[] PROGMEM = "CONNECT 1200";
const char rcNO_DIALTONE[]  PROGMEM = "NO DIALTONE";
const char rcBUSY[]         PROGMEM = "BUSY";
const char rcNO_ANSWER[]    PROGMEM = "NO ANSWER";
const char* const rcMsg[] = {rcOK, rcCONNECT, rcRING, rcNO_CARRIER, rcERROR,
                             rcCONNECT_1200, rcNO_DIALTONE, rcBUSY, rcNO_ANSWER
                            };



class HAYES {
  public:
    HAYES(CFG_t *cfg, AFSK *afsk);
    ~HAYES();

    void printCRLF();
    void print_P(const char *str, bool newline = false);

    int16_t getInteger(char* buf, int8_t idx, uint8_t len = 32);
    int16_t getValidInteger(char* buf, uint8_t idx, int16_t low, int16_t hgh, int16_t def = 0, uint8_t len = 32);
    int16_t getValidInteger(int16_t low, int16_t hgh, int16_t def = 0, uint8_t len = 32);
    int8_t  getDigit(char* buf, uint8_t idx, int8_t def = HAYES_NUM_ERROR);
    int8_t  getDigit(int8_t def = HAYES_NUM_ERROR);
    int8_t  getValidDigit(char* buf, int8_t idx, int8_t low, int8_t hgh, int8_t def = HAYES_NUM_ERROR);
    int8_t  getValidDigit(int8_t low, int8_t hgh, int8_t def = HAYES_NUM_ERROR);


    uint8_t doSIO();
    void    doCommand();
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
    // Local index
    uint8_t ldx = 0;
    // Numeric value
    int8_t value = 0;

    // One specific S register
    uint8_t _sreg = 0;

    // The result status of the last command
    uint8_t cmdResult = RC_OK;

    void    cmdPrint(char cmd, uint8_t value, bool newline = true);
    void    cmdPrint(char cmd, char mod, uint8_t value, bool newline = true);
    void    cmdPrint(uint8_t value);
    void    sregPrint(CFG_t *cfg, uint8_t reg, bool newline = false);


    uint8_t dialCmdMode = 0;
    uint8_t dialReverse = 0;
    char    dialNumber[21];
    bool    getDialNumber(char *dn, size_t len);


    void    showProfile(CFG_t *cfg);

    void    printResult(uint8_t code);
};

#endif /* HAYES_H */
