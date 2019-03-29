/**
  conn.h - Network link and serial decoding

  Copyright (C) 2019 Costin STROIE <costinstroie@eridu.eu.org>

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

#ifndef CONN_H
#define CONN_H

#include <Arduino.h>

#include "config.h"

// Connection direction
enum DIRECTION {ORIGINATING, ANSWERING};
// Command and data mode
enum MODES {COMMAND_MODE, DATA_MODE};
// Flow control
enum FLOWCONTROL {FC_NONE = 0, FC_RTSCTS = 3, FC_XONXOFF = 4};
// On / Off
enum ONOFF {OFF, ON};

class CONN {
  public:
    CONN();
    ~CONN();

    void init(CFG_t *conf);

    void setDirection(uint8_t dir, uint8_t rev = OFF);
    void setLine(uint8_t online);
    bool getLine();
    void setMode(uint8_t mode);
    bool getMode();
    void setTxCarrier(uint8_t onoff);
    void setRxCarrier(uint8_t onoff);
    bool getRxCarrier();
    bool dial(char *phone);

    void doTXRX();

    uint8_t doSIO();
    void clearRing();
    uint32_t callTime();

  private:
    CFG_t  *cfg;

    uint8_t onLine    = OFF;            // OnHook / OffHook
    uint8_t opMode    = COMMAND_MODE;   // Modem works in data mode or in command mode
    uint8_t direction = ORIGINATING;

    // Serial flow control tracking status
    bool inFlow  = false;
    bool outFlow = false;

    // Ring
    uint32_t inpRingTimeout;
    uint32_t outRingTimeout;

    // Escape char detection
    uint8_t  escCount = 0;
    uint32_t escFirst = 0;
    uint32_t escLast  = 0;
    uint16_t escGuard;
    char     escChar;

    uint32_t connTime; // Connection time
    uint32_t connCDT ; // RX carrier time out

};


#endif /* CONN_H */
