/**
  Bell103 - An Arduino-based Bell 103-compatible modem (300 baud)

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

  Bell 103: 300 8N1
*/

#include <util/atomic.h>

#include "afsk.h"

#define DEBUG

// Modem configuration
CFG_t cfg;

// Define the modem
AFSK afsk;

// The AT-Hayes interface
#include "hayes.h"
HAYES hayes(&cfg, &afsk);


/**
  ADC Interrupt vector, called for each sample, which calls both the handlers
*/
ISR(ADC_vect) {
  TIFR1 = _BV(ICF1);
#ifndef DEBUG
  afsk.handle();
#endif
}

/**
  Main Arduino setup function
*/
void setup() {
  Serial.begin(9600);

  // Modem configuration
  cfg.txcarr = 0;  // Keep a carrier going when transmitting

  afsk.online = 0;

  // Define and configure the afsk
  afsk.init(BELL103, &cfg);
}

/**
  Main Arduino loop
*/
void loop() {
  // Check the serial port and handle data or command mode
  if (afsk.online)
    afsk.serialHandle();
  else
    hayes.handle();

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
