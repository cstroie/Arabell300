/**
  Arabell300 - Arduino Bell 103 and ITU V.21 modem

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

//#define DEBUG
//#define DEBUG_EE

#include <util/atomic.h>

#include "config.h"
#include "afsk.h"
#include "hayes.h"

// Persistent modem configuration
CFG_t cfg;

// The modem and all its related runtime configuration
AFSK afsk;

// The AT-Hayes command interface
HAYES hayes(&cfg, &afsk);


/**
  ADC Interrupt vector, called for each sample
*/
ISR(ADC_vect) {
  TIFR1 = _BV(ICF1);
#ifndef DEBUG
  afsk.doTXRX();
#endif
}

/**
  Main Arduino setup function
*/
void setup() {
  // Serial port configuration
#ifndef DEBUG_EE
  Serial.begin(300);
#else
  Serial.begin(115200);
  e2hex();
#endif

  // Define and configure the modem
  afsk.init(BELL103, &cfg);

  // Banner
  hayes.banner();
}

/**
  Main Arduino loop
*/
void loop() {
  // Check the serial port and handle data
  if (not afsk.doSIO())
    hayes.doSIO();

#ifdef DEBUG_RX_LVL
  static uint32_t next = millis();
  if (millis() > next) {
    Serial.println(afsk.inLevel);
    next += 8 * 1000 / BAUD;
  }
#endif

#ifdef DEBUG
  afsk.simFeed();
  afsk.simPrint();
#endif
}

#ifdef DEBUG_EE
void e2hex() {
  char buf[16];
  char val[4];
  uint8_t data;
  // Start with a new line
  Serial.print("\r\n");
  // All EEPROM bytes
  for (uint16_t addr = 0; addr <= E2END;) {
    // Init the buffer
    memset(buf, 0, 16);
    // Use the buffer to display the address
    sprintf_P(buf, PSTR("%04X: "), addr);
    Serial.print(buf);
    // Iterate over bytes, 2 sets of 8 bytes
    for (uint8_t set = 0; set < 2; set++) {
      for (uint8_t byt = 0; byt < 8; byt++) {
        // Read EEPROM
        data = EEPROM.read(addr);
        // Prepare and print the hex dump
        sprintf_P(val, PSTR("%02X "), data);
        Serial.print(val);
        // Prepare the ASCII dump
        if (data < 32 or data > 127)
          data = '.';
        buf[addr & 0x0F] = data;
        // Increment the address
        addr++;
      }
      // Print a separator
      Serial.write(' ');
    }
    // Print the ASCII column
    for (uint8_t idx = 0; idx < 16; idx++) {
      Serial.write(buf[idx]);
    }
    // New line
    Serial.print("\r\n");
  }
}
#endif
