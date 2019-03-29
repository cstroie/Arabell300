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

HAYES::HAYES(CFG_t *conf, AFSK *afsk): cfg(conf), afskModem(afsk) {
  // Try to restore the profile or use factory defaults
  profile.init(cfg);
}

HAYES::~HAYES() {
}

/**
  Print \r\n, as configured in S registers
*/
void HAYES::printCRLF() {
  Serial.write(cfg->sregs[3]);
  Serial.write(cfg->sregs[4]);
}

/**
  Print a character array from program memory

  @param str the character array to print
  @param newline print the EOL
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
  Print the startup banner
*/
void HAYES::banner() {
  printCRLF();
  print_P(DEVNAME);
  Serial.write(' ');
  print_P(VERSION);
  Serial.print(" (");
  print_P(DATE);
  Serial.print(")");
  printCRLF();
}

/**
  Get the uptime

  @param upt the time in seconds
  @param buf character array to return the text to
  @param len the maximum length of the character array
  @return uptime in seconds
*/
void HAYES::getUptime(uint32_t upt, char *buf, size_t len) {
  // Compute days, hours, minutes and seconds
  int ss =  upt % 60;
  int mm = (upt % 3600) / 60;
  int hh = (upt % 86400L) / 3600;
  int dd =  upt / 86400L;
  // Create the formatted time
  if      (dd == 0) snprintf_P(buf, len, PSTR("%02d:%02d:%02d"),              hh, mm, ss);
  else if (dd == 1) snprintf_P(buf, len, PSTR("%d day, %02d:%02d:%02d"),  dd, hh, mm, ss);
  else              snprintf_P(buf, len, PSTR("%d days, %02d:%02d:%02d"), dd, hh, mm, ss);
}


/**
  Parse the buffer and return an integer

  @param buf the char buffer to parse
  @param idx index to start with
  @param len maximum to parse, 0 for no limit
  @return the integer found or zero, also set the cmdResult
*/
int16_t HAYES::getInteger(char* buf, int8_t idx, uint8_t len = 32) {
  int16_t result  = 0;      // The result
  bool    isNeg   = false;  // Negative flag
  uint8_t sdx     = 0;      // Start index

  // Be positive
  cmdResult = RC_OK;

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
  if ((ldx - sdx <= len) and (buf[ldx] != 0)) {
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
           and (ldx - sdx <= len)
           and (buf[ldx] != 0)) {
      // Build the result
      result = (result * 10) + (buf[ldx] - '0');
      // Move forward
      ldx++;
    }
    // Buffer check
    if (ldx - sdx > len) {
      // Break because length exceeded
      cmdResult = RC_ERROR;
      result = 0;
    }
    // Check if negative
    if (isNeg)
      result = -result;
  }
  // Return the result
  return result;
}

/**
  Parse the default buffer and return the integer value if it fits
  in the specified interval or the default value if not.

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

/**
  Parse the buffer and return the integer value if it fits
  in the specified interval or the default value if not.

  @param buf the char buffer to parse
  @param idx index to start with
  @param low minimal valid value
  @param hgh maximal valid value
  @param def default value
  @param len maximum to parse, 0 for no limit
  @return the integer value or default
*/
int16_t HAYES::getValidInteger(char* buf, uint8_t idx, int16_t low, int16_t hgh, int16_t def, uint8_t len) {
  cmdResult = RC_OK;
  // Get the integer value
  int16_t res = getInteger(buf, (int8_t)idx, len);
  // Check if valid
  if ((cmdResult != RC_OK) or (res < low) or (res > hgh)) {
    res = def;
    cmdResult = RC_ERROR;
  }
  // Return the result
  return res;
}

/**
  Parse the default buffer and return one digit integer

  @param def default value
  @return the integer found or HAYES_NUM_ERROR
*/
int8_t HAYES::getDigit(int8_t def) {
  return getDigit(buf, idx, def);
}

/**
  Parse the buffer and return one digit integer

  @param buf the char buffer to parse
  @param idx index to start with
  @param def default value
  @return the integer found or HAYES_NUM_ERROR
*/
int8_t HAYES::getDigit(char* buf, uint8_t idx, int8_t def) {
  // The result is signed integer
  int8_t value = def;
  cmdResult = RC_OK;
  // Check the pointed char
  if ((buf[idx] == '\0') or (buf[idx] == ' ') or (buf[idx] == '='))
    // If it is the last char ('\0') or space (' ') or equal ('='),
    // the value is zero
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
  Parse the buffer and return the digit value if it fits
  in the specified interval or the default value if not.

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

/**
  Parse the default buffer and return the digit value if it fits
  in the specified interval or the default value if not.

  @param low minimal valid value
  @param hgh maximal valid value
  @param def default value
  @return the digit value or default
*/
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

/**
  Print a command and its value

  @param cmd the command
  @param mod the command modifier (&,+,*)
  @param value the value to print
  @param newline print a new line after
*/
void HAYES::cmdPrint(char cmd, char mod, uint8_t value, bool newline) {
  // Print the command
  if (cmd != '\0') {
    if (mod != '\0')
      Serial.print(mod);
    Serial.print(cmd);
  }
  // Print the value
  Serial.print(value);
  // Print the newline, if requested
  if (newline) printCRLF();
  else         Serial.print(F(" "));
  // Response code OK
  cmdResult = RC_OK;
}

/**
  Print a command and its value, no modifier

  @param cmd the command
  @param value the value to print
  @param newline print a new line after
*/
void HAYES::cmdPrint(char cmd, uint8_t value, bool newline) {
  cmdPrint(cmd, '\0', value, newline);
}

/**
  Print a command and its value, no modifier, get the command
  from the default buffer.

  @param value the value to print
*/
void HAYES::cmdPrint(uint8_t value) {
  // Print the value
  cmdPrint(buf[idx - 1], value);
}

/**
  Print a S register

  @param conf the configuration structure
  @param reg the specified S register
  @param newline print a new line after
*/
void HAYES::sregPrint(CFG_t *conf, uint8_t reg, bool newline) {
  // Print the command
  Serial.print(F("S"));
  if (reg < 10) Serial.print(F("0"));
  Serial.print(reg);
  Serial.print(F(":"));
  // Print the value
  if (conf->sregs[reg] < 100)
    Serial.print(F("0"));
  if (conf->sregs[reg] < 10)
    Serial.print(F("0"));
  Serial.print(conf->sregs[reg]);
  // Print the newline, if requested
  if (newline) printCRLF();
  else         Serial.print(F(" "));
  // Response code OK
  cmdResult = RC_OK;
}

/**
  Show a configuration profile

  @param conf the configuration structure
*/
void HAYES::showProfile(CFG_t *conf) {
  // Print the main configuration
  cmdPrint('B', conf->compro, false);
  cmdPrint('C', conf->txcarr, false);
  cmdPrint('E', conf->cmecho, false);
  cmdPrint('F', conf->dtecho, false);
  cmdPrint('L', conf->spklvl, false);
  cmdPrint('M', conf->spkmod, false);
  if (not conf->dialpt) Serial.print(F("P "));
  cmdPrint('Q', conf->quiet,  false);
  if (conf->dialpt) Serial.print(F("T "));
  cmdPrint('V', conf->verbal, false);
  cmdPrint('X', conf->selcpm, false);
  printCRLF();
  cmdPrint('A', '&', conf->revans, false);
  cmdPrint('C', '&', conf->dcdopt, false);
  cmdPrint('D', '&', conf->dtropt, false);
  cmdPrint('J', '&', conf->jcksel, false);
  cmdPrint('K', '&', conf->flwctr, false);
  cmdPrint('L', '&', conf->lnetpe, false);
  cmdPrint('P', '&', conf->plsrto, false);
  cmdPrint('R', '&', conf->rtsopt, false);
  cmdPrint('S', '&', conf->dsropt, false);
  printCRLF();
  // Print the S registers
  for (uint8_t reg = 0; reg < 16; reg++) {
    sregPrint(conf, reg, false);
    if (reg == 0x07 or reg == 0x0F)
      printCRLF();
  }
}

/**
  Process serial I/O in command mode: read the chars into a buffer,
  check them, echo them (uppercase), run commands and print results
*/
uint8_t HAYES::doSIO(uint8_t rcRemote) {
  char c;
  // Check if we just have to print a result
  if (rcRemote != RC_NONE) {
    // Check for some special cases
    if (rcRemote == RC_NO_CARRIER) {
      // If NO CARRIER, show the call time
      char buf[20];
      getUptime(afskModem->callTime(), buf, 20);
      printResult(rcRemote, buf);
    }
    else if (rcRemote == RC_RING and \
             cfg->sregs[1] >= cfg->sregs[0] and \
             cfg->sregs[0] > 0) {
      // Ringing and auto answer is enabled and reached,
      // so auto-answer by mangling the input buffer
      strcpy_P(buf, PSTR("ATA"));
      // Print the remote result first
      printResult(rcRemote);
      // Print the input buffer
      Serial.print(buf);
      // Send the newline
      printCRLF();
      // Parse the line
      doCommand();
      // Print the command response
      printResult(cmdResult);
    }
    else
      // Just print the remote result
      printResult(rcRemote);
    // Bail out
    return;
  }

  // Read from serial only if there is room in buffer
  if (len < MAX_INPUT_SIZE - 1) {
    c = Serial.read();
    // Check if we have a valid character
    if (c >= 0) {
      // Uppercase
      c = toupper(c);
      // Local terminal command mode echo
      if (cfg->cmecho)
        Serial.write(c);
      // Check the character
      if (c == cfg->sregs[5] and len > 0)
        // Backspace
        len--;
      else if (len == 0) {
        // First character in command buffer
        if (c == 'A')
          // First character is capital 'A'
          sChr = c;
        else {
          // Anything else
          sChr = '\0';
          buf[0] = c;
        }
        // Count up
        len++;
      }
      else if (len == 1) {
        // Second character in command buffer
        if (sChr == 'A') {
          // First character is capital 'A'
          if (c == '/') {
            // Second character is slash '/', repeat the
            // previous command, but first check its length
            if (strlen(buf) > 0) {
              // Send the newline
              printCRLF();
              // Print the buffer again
              Serial.print(buf);
              // Send the newline
              printCRLF();
              // Parse the line
              doCommand();
              // Print the command response
              printResult(cmdResult);
            }
            else {
              // No previous command
              cmdResult = RC_ERROR;
              printResult(cmdResult);
            }
            // Reset the buffer length
            len = 0;
          }
          else {
            // Second character is not slash, may be 'T', go to buffer
            buf[0] = sChr;
            buf[1] = c;
            len++;
            sChr = '\0';
          }
        }
        else {
          // Not starting with 'A', put into buffer, length up
          buf[len++] = c;
        }
      }
      else {
        // Append to buffer all characters starting with the third
        buf[len++] = c;
      }

      // Check for EOL
      if (c == '\r' or c == '\n') {
        // Flush the rest
        Serial.flush();
        // Send the newline
        printCRLF();
        // Make sure the last char is null
        buf[--len] = '\0';
        // Check the line length before processing
        if (strlen(buf) > 0) {
          // Parse the line
          doCommand();
          // Print the command response
          printResult(cmdResult);
        }
        // Reset the buffer length
        len = 0;
      }
    }
  }
}

/**
  Run one or multiple commands
*/
void HAYES::doCommand() {
  // Reset the command result
  cmdResult = RC_ERROR;

  // Start by finding the 'AT' sequence
  char *pch = strstr_P(buf, PSTR("AT"));

  if (pch != NULL) {
    // Jump over those two chars from the start
    idx = pch - buf + 2;
    // New line, just "AT"
    if (strlen(buf) <= 2)
      cmdResult = RC_OK;
    // Process the line
    while (idx < strlen(buf)) {
      this->dispatch();
      if (cmdResult == RC_ERROR)
        break;
    }
  }
}

/**
  Print the command result code or message
  http://www.messagestick.net/modem/Hayes_Ch1-2.html

  @param code the response code
*/
void HAYES::printResult(uint8_t code, char* buf) {
  if (code != RC_NONE and cfg->quiet != 1)
    if (cfg->verbal) {
      printCRLF();
      print_P(rcMsg[code]);
      if ((buf != NULL) and (cfg->selcpm != 0)) {
        Serial.write(' ');
        Serial.print(buf);
      }
      printCRLF();
    }
    else {
      Serial.print(code);
      printCRLF();
    }
}

/**
  AT commands dispatcher: each command detailed
*/
void HAYES::dispatch() {
  int8_t option;
  // Check the first character, could be a symbol or a letter
  switch (buf[idx++]) {
    // Skip over some characters
    case ' ':
    case '\0':
      break;

    // AT? Print the long help message
    case '?':
      print_P(atHelp, false);
      cmdResult = RC_OK;
      break;

    // ATA Answer incoming call
    case 'A':
      cmdResult = RC_ERROR;
      // Phase 0: Clear any ringing counters and signals
      afskModem->clearRing();
      // Phase 1: Set direction
      afskModem->setDirection(ANSWERING);
      // Phase 2: Go online
      afskModem->setLine(ON);
      // Phase 2: Answering carrier on (after a while)
      afskModem->setTxCarrier(ON);
      // Phase 3: Wait for originating carrier for S7 seconds
      if (afskModem->getRxCarrier()) {
        // Phase 4: Data mode if carrier found
        afskModem->setMode(DATA_MODE);
        if (cfg->selcpm == 0)
          cmdResult = RC_CONNECT;
        else
          cmdResult = RC_CONNECT_300;
      }
      else {
        // No carrier, go offline
        afskModem->setLine(OFF);
        cmdResult = RC_NO_CARRIER;
      }
      // Disable result codes if ATQ2
      if (cfg->quiet == 2) cmdResult = RC_NONE;
      break;

    // ATB Select Communication Protocol
    // ATB15  set ITU V.21 modem type
    // ATB16  set Bell103 modem type
    case 'B':
      if (buf[idx] == '?')
        cmdPrint(cfg->compro);
      else {
        cfg->compro = getValidInteger(0, 31, cfg->compro);
        if (cmdResult == RC_OK)
          // Change the modem type
          switch (cfg->compro) {
            case 15:  afskModem->setModemType(V_21);    break;
            case 16:  afskModem->setModemType(BELL103); break;
            // Default to Bell 103
            default:
              cfg->compro = 16;
              afskModem->setModemType(BELL103);
              cmdResult = RC_ERROR;
          }
      }
      break;

    // ATC Transmit carrier
    // ATC0  disable running TX carrier
    // ATC1  enable running TX carrier
    case 'C':
      if (buf[idx] == '?')
        cmdPrint(cfg->txcarr);
      else {
        cfg->txcarr = getValidDigit(0, 1, cfg->txcarr);
        afskModem->setTxCarrier(ON);
      }
      break;

    // ATD Call
    // ATDxnnn
    //    T     tone dialing
    //    P     pulse dialing
    //     nnn  phone number
    case 'D':
      cmdResult = RC_ERROR;
      // Phase 1: Get the dial number and parameters
      if (getDialNumber(dialNumber, sizeof(dialNumber) - 1)) {
        // Phase 2: Set direction
        afskModem->setDirection(ORIGINATING, dialReverse);
        // Phase 3: Go online
        afskModem->setLine(ON);
        // Phase 4: Wait for dialtone / busy (NO_DIALTONE / BUSY)
        // Phase 5: Dial: DTMF/Pulses
        if (afskModem->dial(dialNumber)) {
          // Phase 6: Wait for incoming carrier for S7 seconds
          if (afskModem->getRxCarrier()) {
            // Phase 7: Enable outgoing carrier
            afskModem->setTxCarrier(ON);
            // Phase 8: Enter data mode or stay in command mode
            if (dialCmdMode)
              cmdResult = RC_OK;
            else {
              afskModem->setMode(DATA_MODE);
              if (cfg->selcpm == 0)
                cmdResult = RC_CONNECT;
              else
                cmdResult = RC_CONNECT_300;
            }
          }
          else {
            // No carrier, go offline
            afskModem->setLine(OFF);
            cmdResult = RC_NO_CARRIER;
          }
        }
        else
          // Interrupted
          cmdResult = RC_ERROR;
      }
      else
        // Invalid dial number
        cmdResult = RC_ERROR;
      break;

    // ATE Set local command mode echo
    // ATE0  disable local character echo in command mode
    // ATE1  enable local character echo in command mode
    case 'E':
      if (buf[idx] == '?')
        cmdPrint(cfg->cmecho);
      else
        cfg->cmecho = getValidDigit(0, 1, cfg->cmecho);
      break;

    // ATF Set local data mode echo
    // ATF0  Half Duplex, modem echoes characters in data mode
    // ATF1  Full Duplex, modem does not echo characters in data mode
    case 'F':
      if (buf[idx] == '?')
        cmdPrint(cfg->dtecho);
      else
        cfg->dtecho = getValidDigit(0, 1, cfg->dtecho);
      break;

    // ATH Hook control
    // ATH0  force line on hook (offline)
    // ATH1  force line off hook (online)
    case 'H':
      afskModem->setLine(getValidDigit(0, 1, 0));
      if (afskModem->getLine() == OFF) {
        char buf[20];
        uint32_t upt = afskModem->callTime();
        if (upt > 0) {
          getUptime(upt, buf, 20);
          printResult(RC_NO_CARRIER, buf);
          // No other response
          cmdResult = RC_NONE;
        }
      }
      break;

    // ATI Show info
    // ATI0  device name and speed
    // ATI1  ROM checksum
    // ATI2  tests ROM checksum, then reports it
    // ATI3  firmware revision
    // ATI4  data connection info (modem features)
    // ATI5  regional settings
    // ATI6  long device description
    // ATI7  manufacturer info
    case 'I': {
        uint8_t rqInfo = 0x00;
        // Get the digit value
        uint8_t value = getValidDigit(0, 7, 0);
        if (cmdResult == RC_OK) {
          // Specify the line to display
          rqInfo = 0x01 << value;

          // 0 Device name and speed
          if (rqInfo & 0x01)
            print_P(DEVNAME, true);
          rqInfo = rqInfo >> 1;

          // 1 ROM checksum
          if (rqInfo & 0x01)
            cmdPrint('\0', cfg->crc8);
          rqInfo = rqInfo >> 1;

          // 2 Tests ROM checksum, then reports it
          if (rqInfo & 0x01) {
            struct CFG_t cfgTemp;
            cmdResult = profile.read(&cfgTemp) ? RC_OK : RC_ERROR;
          }
          rqInfo = rqInfo >> 1;

          // 3 Firmware revision
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

          // 6 Long device description
          if (rqInfo & 0x01)
            print_P(DESCRP,  true);
          rqInfo = rqInfo >> 1;

          // 7 Manufacturer info
          if (rqInfo & 0x01)
            print_P(AUTHOR,  true);
          rqInfo = rqInfo >> 1;
        }
      }
      break;

    // ATL Set speaker volume level
    // ATL0  medium volume, -9dB
    // ATL1  medium volume, -6dB
    // ATL2  medium volume, -3dB
    // ATL3  maximum volume, 0dB
    case 'L':
      if (buf[idx] == '?')
        cmdPrint(cfg->spklvl);
      else
        cfg->spklvl = getValidDigit(0, 3, cfg->spklvl);
      break;

    // ATM Speaker control
    // ATM0  speaker always off
    // ATM1  speaker on for TX
    // ATM2  speaker on for RX
    // ATM3  speaker on for both TX and RX
    case 'M':
      if (buf[idx] == '?')
        cmdPrint(cfg->spkmod);
      else
        cfg->spkmod = getValidDigit(0, 3, cfg->spkmod);
      break;

    // ATO Return to data mode
    // ATO0  back to data mode, while in command mode
    // ATO1  stay in command mode (nonsense)
    case 'O':
      // Data mode or command mode
      afskModem->setMode(getValidDigit(0, 1, 0) == 0 ? DATA_MODE : COMMAND_MODE);
      // Return CONNECT if back to data mode
      cmdResult = afskModem->getMode() == DATA_MODE ? RC_CONNECT : RC_NONE;
      break;

    // ATP Use pulse dialing for the next call
    case 'P':
      cfg->dialpt = OFF;
      cmdResult = RC_OK;
      break;

    // ATQ Quiet Mode
    // ATQ0  modem returns result codes
    // ATQ1  modem does not return result codes
    // ATQ2  modem does not return result codes for ATA command
    case 'Q':
      if (buf[idx] == '?')
        cmdPrint(cfg->quiet);
      else
        cfg->quiet = getValidDigit(0, 2, cfg->quiet);
      break;

    // ATS Addresses An S-register
    // ATSx=y  set value "y" in register "x"
    case 'S':
      // Get the register number
      _sreg = getValidInteger(0, 15, 255, (uint8_t)2);
      if (_sreg == 255) {
        _sreg = 0;
        cmdResult = RC_ERROR;
      }
      else {
        // Move the index to the last local index
        idx = ldx;
        // Check the next character
        if (buf[idx] == '?')
          sregPrint(cfg, _sreg, true);
        else if (buf[idx] == '=') {
          idx++;
          cfg->sregs[_sreg] = getValidInteger(0, 255, cfg->sregs[_sreg], (uint8_t)3);
        }
      }
      break;

    // ATT Use tone dialing for the next call
    case 'T':
      cfg->dialpt = ON;
      cmdResult = RC_OK;
      break;

    // ATV Verbose mode
    // ATV0  send numeric codes
    // ATV1  send text result codes (English)
    case 'V':
      if (buf[idx] == '?')
        cmdPrint(cfg->verbal);
      else
        cfg->verbal = getValidDigit(0, 1, cfg->verbal);
      break;

    // ATX Select call progress method
    // ATX0  basic result codes: "CONNECT" and "NO CARRIER"
    // ATX1  extended result codes: "CONNECT 300" and "NO CARRIER 00:00:00" (call time)
    case 'X':
      if (buf[idx] == '?')
        cmdPrint(cfg->selcpm);
      else
        cfg->selcpm = getValidDigit(0, 4, cfg->selcpm) == 0 ? 0 : 1;
      break;

    // ATZ MCU (and modem) reset
    case 'Z':
      // No more response messages
      cmdResult = RC_NONE;
      wdt_enable(WDTO_250MS);
      // Wait for the prescaller time to expire
      // without sending the reset signal
      while (true) {};
      break;

    // Standard '&' extension
    case '&':
      switch (buf[idx++]) {
        // AT&A Reverse answering frequencies
        // AT&A0  use receiving modem frequencies on answering
        // AT&A1  use originating modem frequencies on answering
        case 'A':
          if (buf[idx] == '?')
            cmdPrint('A', '&', cfg->revans);
          else
            cfg->revans = getValidDigit(0, 1, cfg->revans);
          break;

        // AT&C DCD Option
        // AT&C0  always keep DCD on (consider RX carrier present)
        // AT&C1  DCD follows RX carrier
        case 'C':
          if (buf[idx] == '?')
            cmdPrint('C', '&', cfg->dcdopt);
          else
            cfg->dcdopt = getValidDigit(0, 1, cfg->dcdopt);
          break;

        // AT&D DTR Option
        // AT&D0  ignore DTR
        // AT&D1  return to command mode after losing DTR
        // AT&D2  hang up, turn off auto answer, return to command mode after losing DTR
        // AT&D3  reset after losing DTR
        case 'D':
          if (buf[idx] == '?')
            cmdPrint('D', '&', cfg->dtropt);
          else
            cfg->dtropt = getValidDigit(0, 3, cfg->dtropt);
          break;

        // AT&F Load factory defaults
        case 'F':
          cmdResult = profile.factory(cfg) ? RC_OK : RC_ERROR;
          break;

        // AT&J Jack Type Selection (choose OCR2A or OCR2B)
        // AT&J0  OCR2A primary, OCR2B secondary
        // AT&J1  OCR2A secondary, OCR2B primary
        case 'J':
          if (buf[idx] == '?')
            cmdPrint('J', '&', cfg->jcksel);
          else
            cfg->jcksel = getValidDigit(0, 1, cfg->jcksel);
          break;

        // AT&K Flow Control Selection
        // AT&K0  disable flow control
        // AT&K3  enables CTS/RTS hardware flow control
        // AT&K4  enables XON/XOFF software flow control
        case 'K':
          if (buf[idx] == '?')
            cmdPrint('K', '&', cfg->flwctr);
          else
            cfg->flwctr = getValidDigit(0, 6, cfg->flwctr);
          break;

        // AT&L Line Type Selection
        // AT&L0  Selects PSTN (normal dialup)
        // AT&L1  Selects leased line (no dial, no carrier detection)
        case 'L':
          if (buf[idx] == '?')
            cmdPrint('L', '&', cfg->lnetpe);
          else
            cfg->lnetpe = getValidDigit(0, 1, cfg->lnetpe);
          break;

        // AT&P Make/Break Ratio for Pulse Dialing
        // AT&P0  Selects 39%-61% make/break ratio at 10 pulses per second (NA)
        // AT&P1  Selects 33%-67% make/break ratio at 10 pulses per second (EU)
        // AT&P2  Selects 39%-61% make/break ratio at 20 pulses per second (NA)
        // AT&P3  Selects 33%-67% make/break ratio at 20 pulses per second (EU)
        case 'P':
          if (buf[idx] == '?')
            cmdPrint('P', '&', cfg->plsrto);
          else
            cfg->plsrto = getValidDigit(0, 3, cfg->plsrto);
          break;

        // AT&R RTS/CTS Option Selection
        // AT&R0  ignore RTS
        // AT&R1  read RTS to control outgoing flow
        case 'R':
          if (buf[idx] == '?')
            cmdPrint('R', '&', cfg->rtsopt);
          else
            cfg->rtsopt = getValidDigit(0, 1, cfg->rtsopt);
          break;

        // AT&S DSR Option Selection
        // AT&S0  DSR line is always on, except when on-hook
        // AT&S1  DSR line follows CD
        case 'S':
          if (buf[idx] == '?')
            cmdPrint('S', '&', cfg->dsropt);
          else
            cfg->dsropt = getValidDigit(0, 2, cfg->dsropt);
          break;

        // AT&V Show the configuration
        // AT&V   show everything
        // AT&V0  show current profile
        // AT&V1  show stored profiles
        // AT&V2  show stored phone numbers
        case 'V':
          // Show the active profile
          if ((buf[idx] == '0') or (buf[idx] == '\0')) {
            printCRLF();
            Serial.print(F("ACTIVE PROFILE:"));
            printCRLF();
            showProfile(cfg);
          }
          // Show the stored profiles
          if ((buf[idx] == '1') or (buf[idx] == '\0')) {
            for (uint8_t slot = 0; slot < eeProfNums; slot++) {
              struct CFG_t cfgTemp;
              cmdResult = profile.read(&cfgTemp, slot, false) ? RC_OK : RC_ERROR;
              printCRLF();
              Serial.print(F("STORED PROFILE ")); Serial.print(slot); Serial.print(F(":"));
              printCRLF();
              if (cmdResult == RC_OK)
                showProfile(&cfgTemp);
            }
          }
          // Show the stored phone numbers
          if ((buf[idx] == '2') or (buf[idx] == '\0')) {
            char dn[eePhoneLen];
            printCRLF();
            Serial.print(F("TELEPHONE NUMBERS:"));
            printCRLF();
            for (uint8_t slot = 0; slot < eePhoneNums; slot++) {
              profile.pbGet(dn, slot);
              Serial.print(slot); Serial.print(F("=")); Serial.print(dn);
              printCRLF();
            }
          }
          cmdResult = RC_OK;
          break;

        // AT&W Store the configuration
        // AT&Wx  store the profile in position "x"
        case 'W':
          option = getValidDigit(0, eeProfNums - 1, 0);
          cmdResult = profile.write(cfg, option) ? RC_OK : RC_ERROR;
          break;

        // AT&Y Read the configuration
        // AT&Yx  read the profile from position "x"
        case 'Y':
          option = getValidDigit(0, eeProfNums - 1, 0);
          cmdResult = profile.read(cfg, option, false) ? RC_OK : RC_ERROR;
          break;

        // AT&Z Store Telephone Number
        // AT&Z=nnn   store phone number in position 0
        // AT&Zx=nnn  store phone number in position "x"
        case 'Z':
          cmdResult = RC_OK;
          if (buf[idx] == '=') {
            // If the current char is '=', the slot is zero
            // and the phone number starts at the next char
            option = 0;
            idx += 1;
          }
          else if (buf[idx + 1] == '=') {
            // If the next char is '=', the slot is specified before it
            // and the phone number starts at the next char
            option = getValidDigit(0, eePhoneNums - 1, 0);
            idx += 2;
          }
          else
            // The slot is zero
            option = 0;
          // Get the chars after '='
          if (buf[idx] == '?') {
            // Get the stored phone number
            char dn[eePhoneLen];
            profile.pbGet(dn, option);
            Serial.print(option); Serial.print(F("=")); Serial.print(dn);
            cmdResult = RC_OK;
          }
          else if (cmdResult == RC_OK) {
            // Get the dial number and parameters, if valid
            if (getDialNumber(dialNumber, sizeof(dialNumber) - 1)) {
              // Store the dial number if the specified position
              profile.pbSet(dialNumber, option);
            }
            else
              // Invalid dial number
              cmdResult = RC_ERROR;
          }
          break;
      }
      break;

    // Partial '+' extension
    case '+':
      // AT+FCLASS set the device for different modes (only data supported)
      // AT+FCLASS?   show current device mode
      // AT+FCLASS=?  list the supported device modes
      // AT+FCLASS=0  set the device mode to data
      if (strncmp(&buf[idx], "FCLASS", 6) == 0) {
        idx += 6;
        if (buf[idx] == '?' or
            (buf[idx] == '=' and buf[idx + 1] == '?')) {
          // No FAX capabilities
          Serial.print("0");
          printCRLF();
          // Response code OK
          cmdResult = RC_OK;
        }
        else if (buf[idx] == '=' and buf[idx + 1] == '0') {
          // Response code OK
          cmdResult = RC_OK;
        }
        else
          // Anything else is ERROR
          cmdResult = RC_ERROR;
        break;
      }
      break;
  }
}

/**
  Parse the line buffer and try to compose the dial number

  @param dn the resulting dial number
  @param len maximum dial number lenght
  @return true if succeeeded
*/
bool HAYES::getDialNumber(char *dn, size_t len) {
  bool result = true;
  uint8_t ndx = 0;

  // Reset defaults
  dialReverse = false;
  dialCmdMode = false;

  // Process until the end of line
  while (buf[idx] != '\0' and
         buf[idx] != '\r' and
         buf[idx] != '\n' and
         result == true) {

    if (buf[idx] == ' ' or
        buf[idx] == '-' or
        buf[idx] == '.' or
        buf[idx] == '(' or
        buf[idx] == ')') {
      // Skip over some characters
      idx++;
    }
    else if (buf[idx] == 'S' and ndx == 0) {
      // Callbook dialling, next digit is the callbook entry
      idx++;
      if (buf[idx] >= '0' and buf[idx] < '0' + eePhoneNums) {
        uint8_t entry = buf[idx] - '0';
        profile.pbGet(dn, entry);
        // Quick check the validity
        if (dn[0] == '\0')
          // Invalid stored number
          result = false;
      }
      else
        // No valid phonebook entry
        result = false;
      break;
    }
    else if (buf[idx] == 'T' and ndx == 0) {
      // Tone dialing mode, first character
      cfg->dialpt = ON;
      idx++;
    }
    else if (buf[idx] == 'P' and ndx == 0) {
      // Pulse dialing mode, first character
      cfg->dialpt = OFF;
      idx++;
    }
    else if (isdigit(buf[idx]) or
             (buf[idx] >= 'A' and buf[idx] <= 'D') or
             buf[idx] == '*' or
             buf[idx] == '#' or
             buf[idx] == ',') {
      // Digit or pause character
      dn[ndx++] = buf[idx++];
      // Check how many digits we have
      if (ndx > len)
        result = false;
    }
    else if (buf[idx] == 'R') {
      // Reverse, last character
      dialReverse = true;
      break;
    }
    else if (buf[idx] == ';') {
      // Stay in command mode, last character
      dialCmdMode = true;
      break;
    }
    else
      // Invalid character
      result = false;
  }

  // Make sure it ends with null character
  dn[ndx] = '\0';

  // Return the result
  return result;
}
