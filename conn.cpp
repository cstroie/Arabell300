/**
  conn.cpp - Network link and serial decoding

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

#include "conn.h"
#include "fifo.h"

// FIFOs
const uint8_t fifoSize = 4;
const uint8_t fifoLow =  1 << (fifoSize - 2);
const uint8_t fifoMed =  1 << (fifoSize - 1);
const uint8_t fifoHgh = (1 << fifoSize) - fifoLow;
FIFO txFIFO(fifoSize);
FIFO rxFIFO(fifoSize);


CONN::CONN() {
}

CONN::~CONN() {
}

/**
  Initialize the modem

  @param conf the configuration
*/
void CONN::init(CFG_t *conf) {
  cfg = conf;
  // Hardware init
  //FIXME this->initHW();
  // Set the modem type
  //FIXME this->setModemType(afsk);
  // Set the escape character and the guard time
  escChar  = cfg->sregs[2];
  escGuard = cfg->sregs[12] * 20;
}


/**
  Clear any ringing counters and signals.
*/
void CONN::clearRing() {
  // Reset the signaling timeout
  outRingTimeout = 0;
  // Reset the counter
  cfg->sregs[1] = 0;
  // Stop sending signal to RI
  //PORTB &= ~(_BV(PORTB5));
}


/**
  Handle both the TX and RX
*/
void CONN::doTXRX() {
  if (this->onLine) {
    // Handle TX (constant delay)
    // FIXME this->txHandle();
    // Handle RX
    // FIXME this->rxHandle(rxSample);
  }
}

/**
  Check the serial I/O and transmit the data, respectively check
  the received data and send it to the serial port.
*/
uint8_t CONN::doSIO() {
  // The charcter on the serial line
  uint8_t c;
  // The result (if unchanged, it's command mode, hayes will process)
  uint8_t result = 255;
  // The time the last char has been seen
  static uint32_t lstChar  = 0;
  // The time the function started
  static uint32_t now      = 0;
  // Characters waiting on the serial input
  bool inAvlb = (Serial.available() != 0);

  // The time
  now = millis();

  // Check the RING input, only when offline, 10 times per second
  if (this->onLine == OFF and this->opMode == COMMAND_MODE and
      (now >= inpRingTimeout or inpRingTimeout == 0)) {
    inpRingTimeout = now + 100UL;
    // Check if ringing
    // FIXME Check for available incoming connections
    if (false) {
      // Ringing
      if (now > outRingTimeout or outRingTimeout == 0) {
        // Update the timeout
        outRingTimeout = now + 2000UL;
        // Send signal to RI
        //PORTB |= _BV(PORTB5);
        // Increment the counter
        cfg->sregs[1] += 1;
        // RC_RING
        result = 2;
      }
    }
    else if (cfg->sregs[1] != 0) {
      // No ringing, but the counter is not zero, so it was
      // ringing and stopped; clear everything
      this->clearRing();
    }
    if (result != 255)
      return result;
  }

  // Check if we saw the escape string "+++"
  if (escCount == 3) {
    // We did, we did taw the escape string!
    // Check for the guard silence (S12)
    if (now - escLast > escGuard) {
      // This is it, go in command mode
      escCount = 0;
      this->setMode(COMMAND_MODE);
      // RC_OK
      result = 0;
    }
    else if (inAvlb) {
      // We just saw the full string (still in after guard time),
      // and there is something more on the line
      c = Serial.peek();
      if (c == '\r' or c == '\n')
        // Ignore CR and LF.
        Serial.read();
      else {
        // There is something else, transmit the escape string
        // to the other part and stay in data mode
        for (uint8_t i = escCount; i > 0; i--) {
          // Send the chars
          txFIFO.in(escChar);
          // Local datamode echo only on half duplex
          if (cfg->dtecho == OFF)
            Serial.write(escChar);
        }
        // Reset the counter and the first mark
        escCount = 0;
        escFirst = 0;
        lstChar  = now;
      }
    }
  }
  else if (escCount > 0) {
    // There were at most two escape chars, check if it took too long
    if (now - escFirst > escGuard) {
      // No more '+' chars and timed out
      for (uint8_t i = escCount; i > 0; i--) {
        // Send the chars
        txFIFO.in(escChar);
        // Local datamode echo only on half duplex
        if (cfg->dtecho == OFF)
          Serial.write(escChar);
      }
      // Reset the counter
      escCount = 0;
      escFirst = 0;
      lstChar  = now;
    }
  }

  // Check if there is any data on serial port
  if (inAvlb) {
    // Check for "+++" escape sequence (S2)
    if (Serial.peek() == escChar) {
      // Check when we saw the first '+' (S12)
      if (now - escFirst > escGuard) {
        // The first is older than the guard time, this may be a new first,
        // so check the before guard time too
        if (now - lstChar >= escGuard) {
          // This is the first, the last char was long ago, keep the time
          escCount = 1;
          escFirst = now;
          // Remove it from the buffer and make it unavailable
          Serial.read();
          inAvlb = false;
        }
      }
      else {
        // The last '+' was seen recently, count them up until three.
        // If this is the last, keep the time and wait for the guard silence
        if (++escCount == 3)
          escLast = now;
        // Remove it from the buffer and make it unavailable
        Serial.read();
        inAvlb = false;
      }
    }
  }

  // Check RX carrier
  /*
    if (rx.state == NO_CARRIER) {
    // The RX carrier has been lost, disable RX
    rx.state = NOP;
    // Go in command mode
    this->setMode(COMMAND_MODE);
    // RC_NO_CARRIER
    result = 3;
    }
  */

  // Only in data mode
  if (this->opMode != COMMAND_MODE) {
    // Data mode, say to hayes we have processed the data
    result = 254;

    // Check DTR and act accordingly
    if (cfg->dtropt > 0) {
      // Check DTR
      // FIXME
      if (false) {
        // DTR low
        switch (cfg->dtropt) {
          case 1: // Return to command mode
            // Go in command mode
            this->setMode(COMMAND_MODE);
            // RC_OK
            result = 0;
            break;
          case 2: // Hang up, turn off auto answer, return to command mode
            // No auto-answer
            cfg->sregs[0] = 0;
            // Go offline and command mode
            this->setLine(OFF);
            // RC_NO_CARRIER
            result = 3;
            break;
          case 3: // Reset
            // FIXME wdt_enable(WDTO_250MS);
            while (true) {};
            break;
        }
      }
    }

    // Flow control for outgoing (to DTE)
    switch (cfg->flwctr) {
      case FC_XONXOFF:
        // Check if the next character is a XON/XOFF flow control,
        // only if flow control is enabled
        c = Serial.peek();
        if (c == 0x13) {
          // XOFF
          Serial.read();
          outFlow = true;
        }
        else if (c == 0x10) {
          // XON
          Serial.read();
          outFlow = false;
        };
        break;
      case FC_RTSCTS:
        // RTS/CTS flow control
        // FIXME
        // outFlow = (cfg->rtsopt == 0) and (not (PIND & _BV(PORTD6)));
        outFlow = false;
        break;
    }

    // Check if the FIFO
    if (txFIFO.len() < fifoHgh) {
      // The FIFO is not getting full, so check if we can take the byte
      if (inAvlb and (txFIFO.len() < fifoMed or (not inFlow))) {
        // There is data on serial port, process it normally
        c = Serial.read();
        if (txFIFO.in(c))
          // Local datamode echo only on half duplex
          if (cfg->dtecho == OFF)
            Serial.write((char)c);
        // Keep the time
        lstChar = now;
        // Keep transmitting
        // FIXME
        // tx.active = ON;
      }
    }
    else if (not inFlow and cfg->flwctr != FC_NONE) {
      // FIFO is getting full, check the flow control
      if (cfg->flwctr == FC_XONXOFF)
        // XON/XOFF flow control: XOFF
        Serial.write(0x13);
      else if (cfg->flwctr == FC_RTSCTS) {
        // RTS/CTS flow control
        // FIXME
        // PORTD &= ~_BV(PORTD7);
      }
      // Stop flow
      inFlow = true;
    }

    // Anytime, try to disable flow control, if we can
    if (inFlow and txFIFO.len() < fifoLow) {
      if (cfg->flwctr == FC_XONXOFF)
        // XON/XOFF flow control: XON
        Serial.write(0x11);
      else if (cfg->flwctr == FC_RTSCTS) {
        // RTS/CTS flow control
        // FIXME
        // PORTD |= _BV(PORTD7);
      }
      // Resume flow
      inFlow = false;
    }

    // Check if there is any data in RX FIFO
    if ((not rxFIFO.empty()) and (not outFlow)) {
      // Get the byte and send it to serial line
      c = rxFIFO.out();
      Serial.write(c);
    }
  }

  // Return the result (for hayes.doSIO, to print it)
  return result;
}

/**
  Set the connection direction

  @param dir connection direction ORIGINATING, ANSWERING
*/
void CONN::setDirection(uint8_t dir, uint8_t rev) {
  // Keep the direction
  direction = dir;
  // Stop the TX carrier
  this->setTxCarrier(OFF);
  // Create TX/RX pointers to ORIGINATING/ANSWERING parameters
  if ((direction == ORIGINATING and rev == OFF) or
      (direction == ANSWERING and cfg->revans == ON)) {
    // FIXME
    //fsqTX = &cfgAFSK.orig;
    //fsqRX = &cfgAFSK.answ;
  }
  else {
    // FIXME
    //fsqTX = &cfgAFSK.answ;
    //fsqRX = &cfgAFSK.orig;
  }
  // Clear the FIFOs
  rxFIFO.clear();
  txFIFO.clear();
}

/**
  Set the online status

  @param online online status
*/
void CONN::setLine(uint8_t online) {
  // Keep the online status
  onLine = online;

  if (online == OFF) {
    // OH led off
    //PORTB &= ~_BV(PORTB4);
    // CD off
    this->setRxCarrier(OFF);
    // DSR always on if &S0, except when line is off
    // FIXME
    //if (cfg->dsropt == OFF)
    //  PORTD &= ~_BV(PORTD5);
    // Command mode
    this->setMode(COMMAND_MODE);
  }
  else {
    // OH led on
    // FIXME PORTB |= _BV(PORTB4);
    // DSR always on if &S0
    // FIXME
    // if (cfg->dsropt == OFF)
    //  PORTD |= _BV(PORTD5);
  }
}

/**
  Get the online status

  @return online status
*/
bool CONN::getLine() {
  return this->onLine;
}

/**
  Set the modem mode

  @param mode command mode or data mode
*/
void CONN::setMode(uint8_t mode) {
  // Keep the mode
  this->opMode = mode;
}

/**
  Get the modem mode

  @return modem mode
*/
bool CONN::getMode() {
  return this->opMode;
}

/**
  Enable or disable the TX carrier going at runtime

  @param onoff carrier mode
*/
void CONN::setTxCarrier(uint8_t onoff) {
  //tx.carrier = onoff & cfg->txcarr;
}

/**
  Signal the RX carrier detection

  @param onoff carrier mode
*/
void CONN::setRxCarrier(uint8_t onoff) {
  /*
    rx.carrier = onoff;
    if (onoff == ON) {
    // CD led on
    PORTB |= _BV(PORTB2);
    // For now, DSR follows CD if &S1
    if (cfg->dsropt == ON)
      PORTD |= _BV(PORTD5);
    }
    else {
    // CD led off
    PORTB &= ~_BV(PORTB2);
    // For now, DSR follows CD if &S1
    if (cfg->dsropt == ON)
      PORTD &= ~_BV(PORTD5);
    }
  */
}

/**
  Check the incoming carrier

  @return the carrier detection status
*/
bool CONN::getRxCarrier() {
  // If the value specified in S7 is zero or &C0 or &L1,
  // don't wait for the carrier, report as found
  if ((cfg->sregs[7] == 0) or (cfg->dcdopt == 0) or (cfg->lnetpe == 1)) {
    // Don't detect the carrier, go directly to WAIT
    this->setRxCarrier(ON);
    //rx.state = WAIT;
    // Reuse the carrier counter as call timer
    connTime = millis();
  }
  else {
    // Use the decoder to check for carrier
    this->setRxCarrier(OFF);
    //rx.state = CARRIER;
    connTime = 0;
    // Check the carrier for at most S7 seconds
    connCDT = millis() + cfg->sregs[7] * 1000UL;
    while (millis() <= connCDT)
      // Stop checking if there is any char on serial
      // or the carrier has been detected
      // FIXME if (Serial.available() or rx.carrier == ON)
      if (Serial.available() or true)
        break;
    // No RX if carrier not detected
    //if (not rx.carrier)
    //  rx.state = NOP;
  }
  // Return the carrier status (only true/false)
  //FIXME return rx.carrier;
  return true;
}

/**
  Dial a number

  @param phone the number to dial
  @return true if completed, false if interrupted
*/
bool CONN::dial(char *phone) {
  bool result = true;
  /*
    // If leased line (&L1), do not dial
    if (cfg->lnetpe == 0) {
    // Disable the TX carrier
    this->setTxCarrier(OFF);
    // Sanitize S8 and set the comma delay value
    if (cfg->sregs[8] > 6)
      cfg->sregs[8] = 2;
    _commaMax = F_SAMPLE * cfg->sregs[8];
    _commaCnt = 0;
    // Clear TX FIFO for storing the dial number into
    txFIFO.clear();
    // Prepend and append comma-delays
    txFIFO.in(',');
    while (*phone != 0)
      txFIFO.in(*phone++);
    txFIFO.in(',');
    // Start dialing
    this->isDialing = ON;
    // Block until dialing is over
    while (this->isDialing == ON) {
      // Stop dialing if there is any char on serial
      if (Serial.available()) {
        this->isDialing = OFF;
        result = false;
      }
      // Busy delay
      delay(10);
    }
    }
  */
  return result;
}

/**
  Get the call time

  @return call duration in seconds
*/
uint32_t CONN::callTime() {
  uint32_t result = 0;
  if (connTime != 0) {
    result = (millis() - connTime) / 1000;
    connTime = 0;
  }
  return result;
}
