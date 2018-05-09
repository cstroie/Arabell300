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

// CPU frequency correction for sampling timer
#define F_COR (-120000L)

// Sampling frequency
#define F_SAMPLE    9600


#include <Arduino.h>
// EEPROM
#include <EEPROM.h>


// Software name and vesion
const char DEVNAME[]  PROGMEM = "Bell103";
const char VERSION[]  PROGMEM = "v2.21";
const char AUTHOR[]   PROGMEM = "Costin Stroie <costinstroie@eridu.eu.org>";
const char DESCRP[]   PROGMEM = "Arduino emulated standard 300 baud modem";
const char DATE[]     PROGMEM = __DATE__;

// Modem configuration
const uint8_t cfgLen = 32;
struct CFG_t {
  union {
    struct {
      uint8_t crc8  : 8;  // CRC8 of the following data
      uint8_t compro: 5;  // ATB Select Communication Protocol
      uint8_t txcarr: 1;  // ATC Keep a carrier going when transmitting
      uint8_t cmecho: 1;  // ATE Local command mode echo
      uint8_t dtecho: 1;  // ATF Local data mode echo
      uint8_t spklvl: 2;  // ATL Speaker volume level
      uint8_t spkmod: 2;  // ATM Speaker mode
      uint8_t quiet : 1;  // ATQ Quiet mode
      uint8_t verbal: 1;  // ATV Verbose mode
      uint8_t selcpm: 1;  // ATX Select call progress method
      uint8_t revans: 1;  // AT&A Reverse answering frequencies
    };
    uint8_t data[cfgLen];
  };
};



class Profile {
  public:
    Profile();
    ~Profile();

    // EEPROM address to store the configuration to
    uint16_t  eeAddress = 0x0080;

    bool    write(CFG_t *cfg);
    bool    read(CFG_t *cfg, bool useDefaults = false);
    bool    factory(CFG_t *cfg);
    void    init(CFG_t *cfg);

  private:
    uint8_t crc(CFG_t *cfg);
    uint8_t CRC8(uint8_t inCrc, uint8_t inData);
    bool    equal(CFG_t *cfg1, CFG_t *cfg2);
};

#endif /* CONFIG_H */
