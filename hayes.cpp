/**
  hayes.cpp - AT-Hayes commands interface

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

#include "hayes.h"

Profile profile;

HAYES::HAYES(CFG_t *cfg, AFSK *afsk): _cfg(cfg), _afsk(afsk) {
  // Try to restore the profile or use factory defaults
  profile.init(_cfg);
}

HAYES::~HAYES() {
}

/**
  Print \r\n, as configured in S registers
*/
void HAYES::printCRLF() {
  Serial.write(_cfg->sregs[3]);
  Serial.write(_cfg->sregs[4]);
}

/**
  Print a character array from program memory

  @param str the character array to print
  @param eol print the EOL
*/
void HAYES::print_P(const char *str, bool newline) {
  uint8_t val;
  do {
    val = pgm_read_byte(str++);
    if (val) Serial.write(val);
  } while (val);
  if (newline) printCRLF();
}

/**
  Parse the buffer and return an integer

  @param buf the char buffer to parse
  @param idx index to start with
  @param len maximum to parse, 0 for no limit
  @return the integer found or zero
*/
int16_t HAYES::getInteger(char* buf, int8_t idx, uint8_t len = 32) {
  int16_t result = 0;     // The result
  bool isNeg = false;     // Negative flag
  uint8_t sdx = 0;        // Start index

  // If the specified index is negative, use the last local index;
  // if it is positive, re-set the local index
  if (idx >= 0) {
    // Use the new index
    ldx = idx;
    sdx = idx;
  }
  // Preamble: find the first sign or digit character
  while ((ldx - sdx < len)
         and (buf[ldx] != 0)
         and (buf[ldx] != '-')
         and (buf[ldx] != '+')
         and (not isdigit(buf[ldx])))
    ldx++;
  // At this point, check if we have ended the buffer or the length
  if ((ldx - sdx < len) and (buf[ldx] != 0)) {
    // Might start with '+' or '-', keep the sign and move forward
    // only if the sign is the first character ever
    if (buf[ldx] == '-') {
      if (idx >= 0) isNeg = true;
      ldx++;
    }
    else if (buf[ldx] == '+') {
      isNeg = false;
      ldx++;
    }
    // Parse the digits
    while ((isdigit(buf[ldx]))
           and (ldx - sdx < len)
           and (buf[ldx] != 0)) {
      // Build the result
      result = (result * 10) + (buf[ldx] - '0');
      // Move forward
      ldx++;
    }
    // Check if negative
    if (isNeg)
      result = -result;
  }
  // Return the result
  return result;
}

/**
  Parse the buffer and return the integer value if it fits in the specified interval
  or the default value if not.

  @param buf the char buffer to parse
  @param idx index to start with
  @param low minimal valid value
  @param hgh maximal valid value
  @param def default value
  @param len maximum to parse, 0 for no limit
  @return the integer value or default
*/
int16_t HAYES::getValidInteger(int16_t low, int16_t hgh, int16_t def, uint8_t len) {
  return getValidInteger(buf, idx, low, hgh, def, len);
}

int16_t HAYES::getValidInteger(char* buf, uint8_t idx, int16_t low, int16_t hgh, int16_t def, uint8_t len) {
  cmdResult = RC_OK;
  // Get the integer value
  int16_t res = getInteger(buf, (int8_t)idx, len);
  // Check if valid
  if ((res < low) or (res > hgh)) {
    res = def;
    cmdResult = RC_ERROR;
  }
  // Return the result
  return res;
}

/**
  Parse the buffer and return one digit integer

  @param buf the char buffer to parse
  @param idx index to start with
  @return the integer found or HAYES_NUM_ERROR
*/
int8_t HAYES::getDigit(int8_t def) {
  return getDigit(buf, idx, def);
}

int8_t HAYES::getDigit(char* buf, uint8_t idx, int8_t def) {
  // The result is signed integer
  int8_t value = def;
  cmdResult = RC_OK;
  // Check the pointed char
  if ((buf[idx] == '\0') or (buf[idx] == ' '))
    // If it is the last char ('\0') or space (' '), the value is zero
    value = 0;
  else if (isdigit(buf[idx]))
    // If it is a digit, get the numeric value
    value = buf[idx] - '0';
  else
    cmdResult = RC_ERROR;
  // Index up
  idx++;
  // Return the resulting value
  return value;
}

/**
  Parse the buffer and return the digit value if it fits in the specified interval
  or the default value if not.

  @param buf the char buffer to parse
  @param idx index to start with
  @param low minimal valid value
  @param hgh maximal valid value
  @param def default value
  @return the digit value or default
*/
int8_t HAYES::getValidDigit(char* buf, int8_t idx, int8_t low, int8_t hgh, int8_t def) {
  // Get the digit value
  int8_t res = getDigit(buf, idx);
  // Check if valid
  if ((res != HAYES_NUM_ERROR) and
      ((res < low) or (res > hgh)))
    res = def;
  // Return the result
  return res;
}

int8_t HAYES::getValidDigit(int8_t low, int8_t hgh, int8_t def) {
  // Get the digit value
  int8_t res = getDigit(def);
  // Check if valid
  if ((cmdResult == RC_OK) and ((res < low) or (res > hgh))) {
    res = def;
    cmdResult = RC_ERROR;
  }
  // Return the result
  return res;
}

void HAYES::cmdPrint(char cmd, char mod, uint8_t value, bool newline) {
  // Print the command
  if (cmd != '\0') {
    if (mod != '\0')
      Serial.print(mod);
    Serial.print(cmd);
    Serial.print(F(": "));
  }
  // Print the value
  Serial.print(value);
  // Print the newline, if requested
  if (newline) printCRLF();
  else         Serial.print(F("; "));
  // Response code OK
  cmdResult = RC_OK;
}

void HAYES::cmdPrint(char cmd, uint8_t value, bool newline) {
  cmdPrint(cmd, '\0', value, newline);
}

void HAYES::cmdPrint(uint8_t value) {
  // Print the value
  cmdPrint(buf[idx - 1], value);
}

/**
  Print a S register
*/
void HAYES::sregPrint(uint8_t r, bool newline) {
  // Print the command
  Serial.print(F("S"));
  if (r < 10) Serial.print(F("0"));
  Serial.print(r);
  Serial.print(F(": "));
  // Print the value
  if (_cfg->sregs[r] < 100)
    Serial.print(F("0"));
  if (_cfg->sregs[r] < 10)
    Serial.print(F("0"));
  Serial.print(_cfg->sregs[r]);
  // Print the newline, if requested
  if (newline) printCRLF();
  else         Serial.print(F("; "));
  // Response code OK
  cmdResult = RC_OK;
}

void HAYES::showProfile(CFG_t *cfg) {
  // Print the main configuration
  cmdPrint('B', cfg->compro, false);
  cmdPrint('C', cfg->txcarr, false);
  cmdPrint('E', cfg->cmecho, false);
  cmdPrint('F', cfg->dtecho, false);
  cmdPrint('L', cfg->spklvl, false);
  cmdPrint('M', cfg->spkmod, false);
  cmdPrint('Q', cfg->quiet,  false);
  cmdPrint('V', cfg->verbal, false);
  cmdPrint('X', cfg->selcpm, false);
  printCRLF();
  cmdPrint('A', '&', cfg->revans, false);
  cmdPrint('C', '&', cfg->dcdopt, false);
  cmdPrint('D', '&', cfg->dtropt, false);
  cmdPrint('K', '&', cfg->flwctr, false);
  cmdPrint('L', '&', cfg->lnetpe, false);
  cmdPrint('P', '&', cfg->plsrto, false);
  cmdPrint('R', '&', cfg->rtsopt, false);
  cmdPrint('S', '&', cfg->dsropt, false);
  printCRLF();
  // Print the S registers
  for (uint8_t r = 0; r < 16; r++) {
    sregPrint(r, false);
    if (r == 0x07 or r == 0x0F)
      printCRLF();
  }
}

void HAYES::doCommand() {
  // Reset the command result
  cmdResult = RC_ERROR;

  // Start by finding the 'AT' sequence
  char *pch = strstr_P(buf, PSTR("AT"));

  if (pch != NULL) {
    // Jump over those two chars from the start
    idx = pch - buf + 2;
    // New line, just "AT"
    if (len <= 2)
      cmdResult = RC_OK;
    // Process the line
    while (idx < len) {
      this->dispatch();
      if (cmdResult == RC_ERROR)
        break;
    }
  }
}


void HAYES::dispatch() {
  // Check the first character, could be a symbol or a letter
  switch (buf[idx++]) {
    // Skip over some characters
    case ' ':
    case '\0':
      break;

    // ATA Answer incomming call
    case 'A':
      cmdResult = RC_NO_CARRIER;
      // Phase 1: Set direction
      _afsk->setDirection(ANSWERING);
      // Phase 2: Go online
      _afsk->setOnline(ON);
      // Phase 2: Carrier on (after a while)
      _afsk->setCarrier(ON);
      // Phase 3: Check originating carrier for S7 seconds
      // Phase 4: Data mode if carrier found
      _afsk->setMode(DATA_MODE);
      cmdResult = RC_CONNECT;
      break;

    // ATB Select Communication Protocol
    case 'B':
      if (buf[idx] == '?')
        cmdPrint(_cfg->compro);
      else {
        _cfg->compro = getValidInteger(0, 31, _cfg->compro);
        if (cmdResult == RC_OK) {
          // Change the modem type
          switch (_cfg->compro) {
            case  5:  _afsk->setModemType(V_23_M2); break;
            case 15:  _afsk->setModemType(V_21);    break;
            case 16:  _afsk->setModemType(BELL103); break;
            case 23:  _afsk->setModemType(V_23_M1); break;
            default:
              // Default to Bell 103
              _cfg->compro = 16;
              _afsk->setModemType(BELL103);
              cmdResult = RC_ERROR;
          }
        }
      }
      break;

    // ATC Transmit carrier
    case 'C':
      if (buf[idx] == '?')
        cmdPrint(_cfg->txcarr);
      else {
        _cfg->txcarr = getValidDigit(0, 1, _cfg->txcarr);
        _afsk->setCarrier(ON);
      }
      break;

    // ATD Call
    case 'D':
      // TODO phases
      cmdResult = RC_ERROR;
      // Phase 1: Set direction
      _afsk->setDirection(ORIGINATING);
      // Phase 2: Go online, check dialtone / busy (NO_DIALTONE / BUSY)
      _afsk->setOnline(ON);
      // Phase 3: Dial: DTMF/Pulses
      // Phase 4: Wait for RX carrier (NO_CARRIER)
      // Phase 5: Enable TX carrier (if not already)
      _afsk->setCarrier(ON);
      // Phase 6: Enter data mode
      _afsk->setMode(DATA_MODE);
      cmdResult = RC_CONNECT;
      break;

    // ATE Set local command mode echo
    case 'E':
      if (buf[idx] == '?')
        cmdPrint(_cfg->cmecho);
      else
        _cfg->cmecho = getValidDigit(0, 1, _cfg->cmecho);
      break;

    // ATF Set local data mode echo
    case 'F':
      if (buf[idx] == '?')
        cmdPrint(_cfg->dtecho);
      else
        _cfg->dtecho = getValidDigit(0, 1, _cfg->dtecho);
      break;

    // ATH Hook control
    case 'H':
      _afsk->setOnline(getValidDigit(0, 1, 0));
      break;

    // ATI Show info
    case 'I': {
        uint8_t rqInfo = 0x00;
        // Get the digit value
        uint8_t value = getValidDigit(0, 7, 0);
        if (cmdResult == RC_OK) {
          // Specify the line to display
          rqInfo = 0x01 << value;

          // 0 Modem model and speed
          if (rqInfo & 0x01)
            print_P(DEVNAME, true);
          rqInfo = rqInfo >> 1;

          // 1 ROM checksum
          if (rqInfo & 0x01)
            cmdPrint('\0', _cfg->crc8);
          rqInfo = rqInfo >> 1;

          // 2 Tests ROM checksum THEN reports it
          if (rqInfo & 0x01) {
            struct CFG_t cfgTemp;
            cmdResult = profile.read(&cfgTemp) ? RC_OK : RC_ERROR;
          }
          rqInfo = rqInfo >> 1;

          // 3 Firmware revision level.
          if (rqInfo & 0x01) {
            print_P(VERSION, true);
            print_P(DATE,    true);
          }
          rqInfo = rqInfo >> 1;

          // 4 Data connection info
          if (rqInfo & 0x01)
            print_P(FTRS,  true);
          rqInfo = rqInfo >> 1;

          // 5 Regional Settings
          rqInfo = rqInfo >> 1;

          // 6 Data connection info
          if (rqInfo & 0x01)
            print_P(DESCRP,  true);
          rqInfo = rqInfo >> 1;

          // 7 Manufacturer and model info
          if (rqInfo & 0x01)
            print_P(AUTHOR,  true);
          rqInfo = rqInfo >> 1;
        }
      }
      break;

    // ATL Set speaker volume level
    case 'L':
      if (buf[idx] == '?')
        cmdPrint(_cfg->spklvl);
      else
        _cfg->spklvl = getValidDigit(0, 3, _cfg->spklvl);
      break;

    // ATM Speaker control
    case 'M':
      if (buf[idx] == '?')
        cmdPrint(_cfg->spkmod);
      else
        _cfg->spkmod = getValidDigit(0, 3, _cfg->spkmod);
      break;

    // ATO Return to data mode
    case 'O':
      // No more response messages
      cmdResult = RC_NONE;
      // Data mode
      _afsk->setMode(getValidDigit(0, 1, 0) + 1);
      break;

    // ATQ Quiet Mode
    case 'Q':
      if (buf[idx] == '?')
        cmdPrint(_cfg->quiet);
      else
        _cfg->quiet = getValidDigit(0, 1, _cfg->quiet);
      break;

    // ATS Addresses An S-register
    case 'S':
      // Get the register number
      _sreg = getValidInteger(0, 15, 255);
      if (_sreg == 255) {
        _sreg = 0;
        cmdResult = RC_ERROR;
      }
      else {
        // Move the index to the last local index
        idx = ldx;
        // Check the next character
        if (buf[idx] == '?')
          sregPrint(_sreg, true);
        else if (buf[idx] == '=') {
          _cfg->sregs[_sreg] = getValidInteger(0, 255, _cfg->sregs[_sreg]);
        }
      }
      break;

    // ATV Verbose mode
    case 'V':
      if (buf[idx] == '?')
        cmdPrint(_cfg->verbal);
      else
        _cfg->verbal = getValidDigit(0, 1, _cfg->verbal);
      break;

    // ATX Select call progress method
    case 'X':
      if (buf[idx] == '?')
        cmdPrint(_cfg->selcpm);
      else
        _cfg->selcpm = getValidDigit(0, 1, _cfg->selcpm);
      break;

    // ATZ Reset
    case 'Z':
      // No more response messages
      cmdResult = RC_NONE;
      //FIXME wdt_enable(WDTO_2S);
      // Wait for the prescaller time to expire
      // without sending the reset signal
      //FIXME while (true);
      break;

    // Standard '&' extension
    case '&':
      switch (buf[idx++]) {
        // Reverse answering frequencies
        case 'A':
          if (buf[idx] == '?')
            cmdPrint('A', '&', _cfg->revans);
          else
            _cfg->revans = getValidDigit(0, 1, _cfg->revans);
          break;

        // DCD Option
        case 'C':
          if (buf[idx] == '?')
            cmdPrint('C', '&', _cfg->dcdopt);
          else
            _cfg->dcdopt = getValidDigit(0, 1, _cfg->dcdopt);
          break;

        // DTR Option
        case 'D':
          if (buf[idx] == '?')
            cmdPrint('D', '&', _cfg->dtropt);
          else
            _cfg->dtropt = getValidDigit(0, 3, _cfg->dtropt);
          break;

        // Factory defaults
        case 'F':
          cmdResult = profile.factory(_cfg) ? RC_OK : RC_ERROR;
          break;

        // Flow Control Selection
        case 'K':
          if (buf[idx] == '?')
            cmdPrint('K', '&', _cfg->flwctr);
          else
            _cfg->flwctr = getValidDigit(0, 6, _cfg->flwctr);
          break;

        // Line Type Selection
        case 'L':
          if (buf[idx] == '?')
            cmdPrint('L', '&', _cfg->lnetpe);
          else
            _cfg->lnetpe = getValidDigit(0, 1, _cfg->lnetpe);
          break;

        // Make/Break Ratio for Pulse Dialing
        case 'P':
          if (buf[idx] == '?')
            cmdPrint('P', '&', _cfg->plsrto);
          else
            _cfg->plsrto = getValidDigit(0, 3, _cfg->plsrto);
          break;

        // RTS/CTS Option Selection
        case 'R':
          if (buf[idx] == '?')
            cmdPrint('R', '&', _cfg->rtsopt);
          else
            _cfg->rtsopt = getValidDigit(0, 1, _cfg->rtsopt);
          break;

        // DSR Option Selection
        case 'S':
          if (buf[idx] == '?')
            cmdPrint('S', '&', _cfg->dsropt);
          else
            _cfg->dsropt = getValidDigit(0, 2, _cfg->dsropt);
          break;

        // Show the configuration
        case 'V': {
            Serial.println(F("ACTIVE PROFILE:"));
            showProfile(_cfg);
            Serial.println();
            struct CFG_t cfgTemp;
            cmdResult = profile.read(&cfgTemp) ? RC_OK : RC_ERROR;
            Serial.println(F("STORED PROFILE:"));
            showProfile(&cfgTemp);
          }
          break;

        // Store the configuration
        case 'W':
          cmdResult = profile.write(_cfg) ? RC_OK : RC_ERROR;
          break;

        // Read the configuration
        case 'Y':
          cmdResult = profile.read(_cfg) ? RC_OK : RC_ERROR;
          break;
      }
      break;
  }
}

uint8_t HAYES::doSIO() {
  char c;
  // Read from serial only if there is room in buffer
  if (len < MAX_INPUT_SIZE - 1) {
    c = Serial.read();
    // Check if we have a valid character
    if (c >= 0) {
      // Uppercase
      c = toupper(c);
      // Local terminal command mode echo
      if (_cfg->cmecho)
        Serial.print(c);
      // Append to buffer
      buf[len++] = c;
      // Check for EOL
      if (c == '\r' or c == '\n') {
        //printCRLF();
        // Check the next character
        c = Serial.peek();
        // If newline, consume it
        if (c == '\r' or c == '\n')
          Serial.read();
        // Make sure the last char is null
        len--;
        buf[len] = '\0';
        // Parse the line
        doCommand();
        // Check the line length before reporting
        if (len >= 0)
          // Print the command response
          printResult(cmdResult);
        // Reset the buffer length
        len = 0;
      }
    }
  }
}

/**
  Print the command result code or message
  http://www.messagestick.net/modem/Hayes_Ch1-2.html

  @param code the response code
*/
void HAYES::printResult(uint8_t code) {
  if (code != RC_NONE and _cfg->quiet != 1)
    if (_cfg->verbal) {
      printCRLF();
      print_P(rcMsg[code]);
      printCRLF();
    }
    else {
      Serial.print(code);
      printCRLF();
    }
}
