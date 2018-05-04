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

CFG ee;

HAYES::HAYES(CFG_t *cfg, AFSK *afsk): _cfg(cfg), _afsk(afsk) {
}

HAYES::~HAYES() {
}


/**
  Print a character array from program memory

  @param str the character array to print
  @param eol print the EOL
*/
void print_P(const char *str, bool eol = false) {
  uint8_t val;
  do {
    val = pgm_read_byte(str++);
    if (val) Serial.write(val);
  } while (val);
  if (eol) Serial.println();
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
  static uint8_t ldx = 0; // Local index, static
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
int16_t HAYES::getValidInteger(char* buf, int8_t idx, int16_t low, int16_t hgh, int16_t def = 0, uint8_t len) {
  // Get the integer value
  int16_t res = getInteger(buf, idx, len);
  // Check if valid
  if ((res < low) or (res > hgh))
    res = def;
  // Return the result
  return res;
}

int16_t HAYES::getValidInteger(int16_t low, int16_t hgh, int16_t def = 0, uint8_t len) {
  cmdResult = 1;
  // Get the integer value
  int16_t res = getInteger(buf, idx, len);
  // Check if valid
  if ((res < low) or (res > hgh)) {
    res = def;
    cmdResult = 0;
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
int8_t HAYES::getDigit(char* buf, int8_t idx) {
  // The result is signed integer
  int8_t res = HAYES_NUM_ERROR;
  // Check the pointed char
  if ((buf[idx] == '\0') or (buf[idx] == ' '))
    // If it is the last char ('\0') or space (' '), the value is zero
    res = 0;
  else if (isdigit(buf[idx]))
    // If it is a digit, get the numeric value
    res = buf[idx] - '0';
  // Return the result
  return res;
}

int8_t HAYES::getDigit(int8_t def) {
  // The result is signed integer
  int8_t value = def;
  cmdResult = 1;
  // Check the pointed char
  if ((buf[idx] == '\0') or (buf[idx] == ' '))
    // If it is the last char ('\0') or space (' '), the value is zero
    value = 0;
  else if (isdigit(buf[idx]))
    // If it is a digit, get the numeric value
    value = buf[idx] - '0';
  else
    cmdResult = 0;
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
  if ((cmdResult != 0) and ((res < low) or (res > hgh))) {
    res = def;
    cmdResult = 0;
  }
  // Return the result
  return res;
}

void HAYES::cmdPrint(char cmd, uint8_t value) {
  // Print the value
  Serial.print(cmd); Serial.print(F(": ")); Serial.println(value);
  cmdResult = 1;
}

void HAYES::cmdPrint(uint8_t value) {
  // Print the value
  cmdPrint(buf[idx - 1], value);
}

void HAYES::dispatch() {
  // Reset the command result
  cmdResult = 0;

  // Start by finding the 'AT' sequence
  char *pch = strstr_P(buf, PSTR("AT"));
  if (pch != NULL) {
    // Jump over those two chars from the start
    idx = buf - pch + 2;

    // Check the first character, could be a symbol or a letter
    switch (buf[idx++]) { // idx++ -> 1

      // ATA Answer incomming call
      case 'A':
        // TODO
        break;

      // ATB Select Communication Protocol
      case 'B':
        if (buf[idx] == '?')
          cmdPrint(_cfg->compro);
        else {
          _cfg->compro = getValidInteger(0, 31, _cfg->compro);
          if (cmdResult) {
            // Change the protocol
            //_afsk.setProtocol();
          }
        }
        break;

      // ATC Transmit carrier
      case 'C':
        if (buf[idx] == '?')
          cmdPrint(_cfg->txcarr);
        else
          _cfg->txcarr = getValidDigit(0, 1, _cfg->txcarr);
        break;

      // ATE Set local echo
      case 'E':
        if (buf[idx] == '?')
          cmdPrint(_cfg->echo);
        else
          _cfg->echo = getValidDigit(0, 1, _cfg->echo);
        break;

      // ATH Hook control
      case 'H':
        // TODO
        break;

      // ATI Show info
      case 'I': {
          uint8_t rqInfo = 0x00;
          // Get the digit value
          uint8_t value = getValidDigit(0, 7, 0);
          if (cmdResult) {
            // Specify the line to display
            rqInfo = 0x01 << value;

            // 0 Modem model and speed
            if (rqInfo & 0x01)
              print_P(DEVNAME, true);
            rqInfo = rqInfo >> 1;
            // 1 ROM checksum
            if (rqInfo & 0x01)
              Serial.println(_cfg->crc8, 16);
            rqInfo = rqInfo >> 1;
            // 2 Tests ROM checksum THEN reports it
            if (rqInfo & 0x01)
              Serial.println(_cfg->crc8, 16);
            rqInfo = rqInfo >> 1;
            // 3 Firmware revision level.
            if (rqInfo & 0x01) {
              print_P(VERSION, true);
              print_P(DATE,    true);
            }
            rqInfo = rqInfo >> 1;
            // 4 Data connection info
            rqInfo = rqInfo >> 1;
            // 5 Regional Settings
            rqInfo = rqInfo >> 1;
            // 6 Data connection info
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
        _afsk->dataMode = getValidDigit(0, 1, 0) + 1;
        break;

      // ATQ Quiet Mode
      case 'Q':
        if (buf[idx] == '?')
          cmdPrint(_cfg->quiet);
        else
          _cfg->quiet = getValidDigit(0, 1, _cfg->quiet);
        break;

      // ATV Verbose mode
      case 'V':
        if (buf[idx] == '?')
          cmdPrint(_cfg->verbos);
        else
          _cfg->verbos = getValidDigit(0, 1, _cfg->verbos);
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
        wdt_enable(WDTO_2S);
        // Wait for the prescaller time to expire
        // without sending the reset signal
        while (true);
        break;

      // Standard '&' extension
      case '&':
        switch (buf[idx++]) { // idx++ -> 2
          // Factory defaults
          case 'F':
            //cmdResult = cfg.factory();
            break;

          // Show the configuration
          case 'V':
            Serial.print(F("E: "));   Serial.print(_cfg->echo); Serial.print(F("; "));
            Serial.print(F("L: "));   Serial.print(_cfg->spklvl); Serial.print(F("; "));
            Serial.print(F("M: "));   Serial.print(_cfg->spkmod); Serial.print(F("; "));
            cmdResult = true;
            break;

          // Store the configuration
          case 'W':
            //cmdResult = cfgWriteEE();
            break;

          // Read the configuration
          case 'Y':
            cmdResult = ee.read(_cfg);
            break;
        }
        break;


      default:
        // Return OK if the command is just 'AT'
        cmdResult = (len == 0);
    }

  }
}


uint8_t HAYES::handle() {
  char c;
  // Read from serial only if there is room in buffer
  if (len < MAX_INPUT_SIZE - 1) {
    c = Serial.read();
    // Check if we have a valid character
    if (c >= 0) {
      // Uppercase
      c = toupper(c);
      // Local terminal echo
      if (_cfg->echo)
        Serial.print(c);
      // Append to buffer
      buf[len++] = c;
      // Check for EOL
      if (c == '\r' or c == '\n') {
        Serial.print("\r\n");
        // Check the next character
        c = Serial.peek();
        // If newline, consume it
        if (c == '\r' or c == '\n')
          Serial.read();
        // Make sure the last char is null
        buf[len - 1] = '\0';
        // Parse the line
        dispatch();
        // Check the line length before reporting
        if (len >= 0) {
          if (cmdResult)  Serial.println(F("OK"));
          else            Serial.println(F("ERROR"));
        }
        // Reset the buffer length
        len = 0;
      }
    }
  }
}

