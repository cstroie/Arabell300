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

HAYES::HAYES(CFG_t cfg): _cfg(cfg) {
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
int16_t HAYES::getValidInteger(char* buf, int8_t idx, int16_t low, int16_t hgh, int16_t def = 0, uint8_t len = 32) {
  // Get the integer value
  int16_t res = getInteger(buf, idx, len);
  // Check if valid
  if ((res < low) or (res > hgh))
    res = def;
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
  // Make sure the pointed char is a digit and get the numeric value
  if (isdigit(buf[idx]))
    res = buf[idx] - '0';
  // Return the result
  return res;
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
int8_t HAYES::getValidDigit(char* buf, int8_t idx, int8_t low, int8_t hgh, int8_t def = HAYES_NUM_ERROR) {
  // Get the digit value
  int8_t res = getDigit(buf, idx);;
  // Check if valid
  if ((res != HAYES_NUM_ERROR) and
      ((res < low) or (res > hgh)))
    res = def;
  // Return the result
  return res;
}


uint8_t HAYES::handle() {
  // String buffer
  char buf[35] = "";
  // Line buffer length
  int8_t len = -1;
  // Buffer index
  uint8_t idx = 0;
  // Numeric value
  int8_t value = 0;
  // Command result
  bool result = false;
  // Start by finding the 'AT' sequence
  // TODO allow for lowercase 'at'
  if (Serial.find("AT")) {
    // Read until newline, no more than 32 chararcters
    len = Serial.readBytesUntil('\r', buf, 32);
    buf[len] = '\0';
    // Uppercase
    while (buf[idx++])
      buf[idx] = toupper(buf[idx]);
    // Local terminal echo
    if (_cfg.echo) {
      Serial.print(F("AT")); Serial.println(buf);
    }
    // Reset the buffer index
    idx = 0;
    // Check the first character, could be a symbol or a letter
    switch (buf[idx++]) { // idx++ -> 1
      // ATE Set local echo
      case 'E':
        if (buf[idx] == '?') {
          // Get local echo
          Serial.print(F("E: ")); Serial.println(_cfg.echo);
          result = true;
        }
        else if (len == idx) {
          _cfg.echo = 0x00;
          result = true;
        }
        else {
          // Get the digit value
          value = getValidDigit(buf, idx, 0, 1, HAYES_NUM_ERROR);
          if (value != HAYES_NUM_ERROR) {
            // Set echo on or off
            _cfg.echo = value;
            result = true;
          }
        }
        break;

      // ATI Show info
      case 'I': {
          uint8_t rqInfo = 0x00;
          if (len == idx or buf[idx] == '0') {
            // Display all info
            rqInfo = 0x03;
            result = true;
          }
          else {
            // Get the digit value
            value = getValidDigit(buf, idx, 1, 7, HAYES_NUM_ERROR);
            if (value != HAYES_NUM_ERROR) {
              // Specify the line to display
              rqInfo = 0x01 << (value - 1);
              result = true;
            }
          }
          if (result) {
            if (rqInfo & 0x01) print_P(DEVNAME, true);  rqInfo = rqInfo >> 1;
            if (rqInfo & 0x01) print_P(VERSION, true);  rqInfo = rqInfo >> 1;
            if (rqInfo & 0x01) print_P(AUTHOR,  true);  rqInfo = rqInfo >> 1;
            if (rqInfo & 0x01) print_P(DATE,    true);  rqInfo = rqInfo >> 1;
            if (rqInfo & 0x01) Serial.println(_cfg.crc8, 16);  rqInfo = rqInfo >> 1;
          }
        }
        break;

      // ATL Set speaker volume level
      case 'L':
        if (buf[idx] == '?') {
          // Get speaker volume level
          Serial.print(F("L: ")); Serial.println(_cfg.spkl);
          result = true;
        }
        else if (len == idx) {
          _cfg.spkl = 0x00;
          result = true;
        }
        else {
          // Get the digit value
          value = getValidDigit(buf, idx, 0, 3, HAYES_NUM_ERROR);
          if (value != HAYES_NUM_ERROR) {
            // Set speaker volume
            _cfg.spkl = value;
            result = true;
          }
        }
        break;

      // ATM Speaker control
      case 'M':
        if (buf[idx] == '?') {
          // Get speaker mode
          Serial.print(F("M: ")); Serial.println(_cfg.spkm);
          result = true;
        }
        else if (len == idx) {
          _cfg.spkm = 0x00;
          result = true;
        }
        else {
          // Get the digit value
          value = getValidDigit(buf, idx, 0, 3, HAYES_NUM_ERROR);
          if (value != HAYES_NUM_ERROR) {
            // Set speaker on or off mode
            _cfg.spkm = value;
            result = true;
          }
        }
        break;

      // ATQ Quiet Mode
      case 'Q':
        if (buf[idx] == '?') {
          // Get quiet mode
          Serial.print(F("Q: ")); Serial.println(_cfg.scqt);
          result = true;
        }
        else if (len == idx) {
          _cfg.scqt = 0x00;
          result = true;
        }
        else {
          // Get the digit value
          value = getValidDigit(buf, idx, 0, 1, HAYES_NUM_ERROR);
          if (value != HAYES_NUM_ERROR) {
            // Set console quiet mode off or on
            _cfg.scqt = value;
            result = true;
          }
        }
        break;

      // ATZ Reset
      case 'Z':
        wdt_enable(WDTO_2S);
        // Wait for the prescaller time to expire
        // without sending the reset signal by using
        // the wdt_reset() method
        while (true);
        break;

      default:
        result = (len == 0);
    }


    // Last response line
    if (len >= 0) {
      if (result) Serial.println(F("OK"));
      else        Serial.println(F("ERROR"));


      Serial.flush();
    }
  }
}

