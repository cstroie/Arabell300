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

#include "config.h"
#include "conn.h"
#include "hayes.h"

// Persistent modem configuration
CFG_t cfg;

// The modem and all its related runtime configuration
CONN conn;

// The AT-Hayes command interface
HAYES hayes(&cfg, &conn);


/**
  Main Arduino setup function
*/
void setup() {
  // Serial port configuration
  Serial.begin(115200);

  // Define and configure the modem
  //afsk.init(BELL103, &cfg);

  // All LEDs on
  //afsk.setLeds(ON);

  // Banner
  hayes.banner();

  // All LEDs off
  delay(1000);
  //afsk.setLeds(OFF);
}

/**
  Main Arduino loop
*/
void loop() {
  // Handle TX/RX
  conn.doTXRX();

  // Check the serial port and handle data
  uint8_t sioResult = conn.doSIO();
  if      (sioResult == 255)  hayes.doSIO();
  else if (sioResult != 254)  hayes.doSIO(sioResult);
}
