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

//#define DEBUG

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
  // We really should use 300 baud...
  Serial.begin(1200);

  // Define and configure the modem
  afsk.init(BELL103, &cfg);
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
