/**
  config.h - Modem persistent configuration

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

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <EEPROM.h>

// Include local configuration
#include "local.h"

// Sampling frequency
#define F_SAMPLE    9600

// Software name and vesion
const char DEVNAME[]  PROGMEM = "Arabell300";
const char VERSION[]  PROGMEM = "v3.50";
const char AUTHOR[]   PROGMEM = "Costin Stroie <costinstroie@eridu.eu.org>";
const char DESCRP[]   PROGMEM = "Arduino based Bell 103 and ITU V.21 AFSK modem";
const char FTRS[]     PROGMEM = "a0020400080004000\r\nb000008\r\nr1001000000000000";
const char DATE[]     PROGMEM = __DATE__;


// Modem configuration
const uint16_t  eeAddress   = 0x0080; // EEPROM start address for configuration store
const uint8_t   eeProfNums  = 4;      // Number of configuration profiles to store
const uint8_t   eeProfLen   = 32;     // Reserved profile lenght
const uint8_t   eePhoneNums = 4;      // Number of phone numbers to store
const uint8_t   eePhoneLen  = 32;     // Reserved phone number lenght
struct CFG_t {
  union {
    struct {
      uint8_t crc8  : 8;  // CRC8 of the following data
      uint8_t compro: 5;  // ATB Select communication protocol
      uint8_t txcarr: 1;  // ATC Keep a carrier going when transmitting
      uint8_t cmecho: 1;  // ATE Local command mode echo
      uint8_t dtecho: 1;  // ATF Half/Full duplex and local data mode echo
      uint8_t spklvl: 2;  // ATL Speaker volume level
      uint8_t spkmod: 2;  // ATM Speaker mode
      uint8_t quiet : 2;  // ATQ Quiet mode
      uint8_t verbal: 1;  // ATV Verbose mode
      uint8_t selcpm: 1;  // ATX Select call progress method
      uint8_t dialpt: 1;  // ATP/T Select pulse/tone dialing
      uint8_t revans: 1;  // AT&A Reverse answering frequencies
      uint8_t dcdopt: 1;  // AT&C DCD option selection
      uint8_t dtropt: 2;  // AT&D DTR option selection
      uint8_t jcksel: 1;  // AT&J Jack type selection
      uint8_t flwctr: 3;  // AT&K Flow control selection
      uint8_t lnetpe: 1;  // AT&L Line type slection
      uint8_t plsrto: 2;  // AT&P Make/Break ratio for pulse dialing
      uint8_t rtsopt: 1;  // AT&R RTS/CTS option selection
      uint8_t dsropt: 2;  // AT&S DSR option selection

      uint8_t sregs[16];  // The S registers
    };
    uint8_t data[eeProfLen];
  };
};

// The S registers
const uint8_t sRegs[16] PROGMEM = {0,     //  0 Rings to Auto-Answer
                                   0,     //  1 Ring Counter
                                   '+',   //  2 Escape Character
                                   '\r',  //  3 Carriage Return Character
                                   '\n',  //  4 Line Feed Character
                                   '\b',  //  5 Backspace Character
                                   2,     //  6 Wait Time for Dial Tone
                                   50,    //  7 Wait Time for Carrier
                                   2,     //  8 Pause Time for Dial Delay Modifier
                                   6,     //  9 Carrier Detect Response Time (tenths of a second)
                                   14,    // 10 Carrier Loss Disconnect Time (tenths of a second)
                                   95,    // 11 DTMF Tone Duration
                                   50,    // 12 Escape Prompt Delay
                                   0,     // 13 Reserved
                                   0,     // 14 General Bit Mapped Options Status
                                   0      // 15 Reserved
                                  };


class Profile {
  public:
    Profile();
    ~Profile();

    // Init, read, write and reset profile
    void    init (CFG_t *cfg, uint8_t slot = 0);
    bool    read (CFG_t *cfg, uint8_t slot = 0, bool useDefaults = false);
    bool    write(CFG_t *cfg, uint8_t slot = 0);
    bool    factory(CFG_t *cfg);

    // S registers data and functions
    uint8_t sregGet(CFG_t *cfg, uint8_t reg);
    void    sregSet(CFG_t *cfg, uint8_t reg, uint8_t value);

    // Phone numbers storage
    uint8_t pbGet(char *phone, uint8_t slot);
    void pbSet(char *phone, uint8_t slot);

  private:
    uint8_t crc(CFG_t *cfg);
    uint8_t CRC8(uint8_t inCrc, uint8_t inData);
    bool    equal(CFG_t *cfg1, CFG_t *cfg2);
};

#endif /* CONFIG_H */
