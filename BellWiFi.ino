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

// Configuration
#include "config.h"
// Connection
#include "conn.h"
// AT Hayes commands
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
  // Do not save the last WiFi settings
  WiFi.persistent(false);
  // Set the host name
  WiFi.hostname("BellWiFi");
  // Set the mode
  WiFi.mode(WIFI_STA);

  // Serial port configuration
  Serial.begin(115200);
  delay(100);
  e2hex();

  // Define and configure the modem
  conn.init(&cfg);

  // All LEDs on
  //afsk.setLeds(ON);

  // Initialize Hayes interpreter
  hayes.init();

  // All LEDs off
  //delay(1000);
  //afsk.setLeds(OFF);
}

/**
  Main Arduino loop
*/
void loop() {
  // Handle TX/RX
  conn.doTXRX();
  yield();

  // Check the serial port and handle data
  uint8_t sioResult = conn.doSIO();
  if      (sioResult == 255)  hayes.doSIO();
  else if (sioResult != 254)  hayes.doSIO(sioResult);
}

void e2hex() {
  // Open EEPROM
  EEPROM.begin(EESIZE);
  char buf[16];
  char val[4];
  uint8_t data;
  // Start with a new line
  Serial.print("\r\n");
  // All EEPROM bytes
  for (uint16_t addr = 0; addr < EESIZE;) {
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
  // Close EEPROM
  EEPROM.end();
}
