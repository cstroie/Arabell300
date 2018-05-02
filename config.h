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

// Software name and vesion
const char DEVNAME[]  PROGMEM = "Bell103";
const char VERSION[]  PROGMEM = "v1.31";
const char AUTHOR[]   PROGMEM = "Costin Stroie <costinstroie@eridu.eu.org>";
const char DATE[]     PROGMEM = __DATE__;

// Modem configuration
struct CFG_t {
  union {
    struct {
      uint8_t txcarr: 1;  // Keep a carrier going when transmitting
      uint8_t font: 4;  // Display font
      uint8_t brgt: 4;  // Display brightness (manual)
      uint8_t mnbr: 4;  // Minimal display brightness (auto)
      uint8_t mxbr: 4;  // Minimum display brightness (auto)
      uint8_t aubr: 1;  // Manual/Auto display brightness adjustment
      uint8_t tmpu: 1;  // Temperature units
      uint8_t spkm: 2;  // Speaker mode
      uint8_t spkl: 2;  // Speaker volume level
      uint8_t echo: 1;  // Local echo
      uint8_t dst:  1;  // DST flag
      int8_t  kvcc: 8;  // Bandgap correction factor (/1000)
      int8_t  ktmp: 8;  // MCU temperature correction factor (/100)
      uint8_t scqt: 1;  // Serial console quiet mode (negate)
      uint8_t bfst: 5;  // First hour to beep
      uint8_t blst: 5;  // Last hour to beep
    };
    uint8_t data[8];    // We use 8 bytes in the structure
  };
  uint8_t crc8;         // CRC8
};


class CFG {
  public:
    CFG();
    ~CFG();
};

#endif /* CONFIG_H */
