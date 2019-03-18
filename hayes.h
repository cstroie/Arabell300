/**
  hayes.h - AT-Hayes commands interface

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

#ifndef HAYES_H
#define HAYES_H

// Maximum input buffer size
#define MAX_INPUT_SIZE 65

// Several Hayes related globals
#define HAYES_NUM_ERROR -128


#include <Arduino.h>
#include <avr/wdt.h>

#include "config.h"
#include "afsk.h"

// Result codes
enum RESULT_CODES {RC_OK, RC_CONNECT, RC_RING, RC_NO_CARRIER, RC_ERROR,
                   RC_CONNECT_300, RC_NO_DIALTONE, RC_BUSY, RC_NO_ANSWER,
                   RC_NONE = 255
                  };
const char rcOK[]           PROGMEM = "OK";
const char rcCONNECT[]      PROGMEM = "CONNECT";
const char rcRING[]         PROGMEM = "RING";
const char rcNO_CARRIER[]   PROGMEM = "NO CARRIER";
const char rcERROR[]        PROGMEM = "ERROR";
const char rcCONNECT_300[]  PROGMEM = "CONNECT 300";
const char rcNO_DIALTONE[]  PROGMEM = "NO DIALTONE";
const char rcBUSY[]         PROGMEM = "BUSY";
const char rcNO_ANSWER[]    PROGMEM = "NO ANSWER";
const char* const rcMsg[] = {rcOK, rcCONNECT, rcRING, rcNO_CARRIER, rcERROR,
                             rcCONNECT_300, rcNO_DIALTONE, rcBUSY, rcNO_ANSWER
                            };

const char atHelp[] PROGMEM = {"AT-Commands\r\n"
                               "ATA Answer incoming call\r\n"
                               "ATB Select Communication Protocol\r\n"
                               " ATB15 set ITU V.21 modem type\r\n"
                               " ATB16 set Bell103 modem type\r\n"
                               "ATC Transmit carrier\r\n"
                               " ATC0 disable running TX carrier\r\n"
                               " ATC1 enable running TX carrier\r\n"
                               "ATD Call\r\n"
                               " ATDTnnn tone dialing, nnn is the phone number\r\n"
                               " ATDPnnn pulse dialing, nnn is phone number\r\n"
                               "ATE Set local command mode echo\r\n"
                               " ATE0 disable local character echo in command mode\r\n"
                               " ATE1 enable local character echo in command mode\r\n"
                               "ATF Set local data mode echo\r\n"
                               " ATF0 Half Duplex, modem echoes characters in data mode\r\n"
                               " ATF1 Full Duplex, modem does not echo characters in data mode\r\n"
                               "ATH Hook control\r\n"
                               " ATH0 force line on hook (off-line)\r\n"
                               " ATH1 force line off hook (on-line)\r\n"
                               "ATI Show info\r\n"
                               " ATI0 device name and speed\r\n"
                               " ATI1 ROM checksum\r\n"
                               " ATI2 tests ROM checksum, then reports it\r\n"
                               " ATI3 firmware revision\r\n"
                               " ATI4 data connection info (modem features)\r\n"
                               " ATI5 regional settings\r\n"
                               " ATI6 long device description\r\n"
                               " ATI7 manufacturer info\r\n"
                               "ATL Set speaker volume level\r\n"
                               " ATL0 medium volume, -9dB\r\n"
                               " ATL1 medium volume, -6dB\r\n"
                               " ATL2 medium volume, -3dB\r\n"
                               " ATL3 maximum volume, 0dB\r\n"
                               "ATM Speaker control\r\n"
                               " ATM0 speaker always off\r\n"
                               " ATM1 speaker on for TX\r\n"
                               " ATM2 speaker on for RX\r\n"
                               " ATM3 speaker on for both TX and RX\r\n"
                               "ATO Return to data mode\r\n"
                               " ATO0 back to data mode, while in command mode\r\n"
                               " ATO1 stay in command mode (nonsense)\r\n"
                               "ATP Use pulse dialing for the next call\r\n"
                               "ATQ Quiet Mode\r\n"
                               " ATQ0 modem returns result codes\r\n"
                               " ATQ1 modem does not return result codes\r\n"
                               " ATQ2 modem does not return result codes for ATA command\r\n"
                               "ATS Addresses An S-register\r\n"
                               " ATSx=y set value y in register x\r\n"
                               "ATT Use tone dialing for the next call\r\n"
                               "ATV Verbose mode\r\n"
                               " ATV0 send numeric codes\r\n"
                               " ATV1 send text result codes (English)\r\n"
                               "ATX Select call progress method\r\n"
                               " ATX0 basic result codes: CONNECT and NO CARRIER\r\n"
                               " ATX1 extended result codes: CONNECT 300 and NO CARRIER 00:00:00 (call time)\r\n"
                               "ATZ MCU (and modem) reset\r\n"
                               "\r\n"
                               "AT&A Reverse answering frequencies\r\n"
                               " AT&A0 use receiving modem frequencies on answering\r\n"
                               " AT&A1 use originating modem frequencies on answering\r\n"
                               "AT&C DCD Option\r\n"
                               " AT&C0 always keep DCD on (consider RX carrier present)\r\n"
                               " AT&C1 DCD follows RX carrier\r\n"
                               "AT&D DTR Option\r\n"
                               " AT&D0 ignore DTR\r\n"
                               " AT&D1 return to command mode after losing DTR\r\n"
                               " AT&D2 hang up, turn off auto answer, return to command mode after losing DTR\r\n"
                               " AT&D3 reset after losing DTR\r\n"
                               "AT&F Load factory defaults\r\n"
                               "AT&J Jack Type Selection (choose OCR2A or OCR2B)\r\n"
                               " AT&J0 OCR2A primary, OCR2B secondary\r\n"
                               " AT&J1 OCR2A secondary, OCR2B primary\r\n"
                               "AT&K Flow Control Selection\r\n"
                               " AT&K0 disable flow control\r\n"
                               " AT&K3 enables CTS/RTS hardware flow control\r\n"
                               " AT&K4 enables XON/XOFF software flow control\r\n"
                               "AT&L Line Type Selection\r\n"
                               " AT&L0 Selects PSTN (normal dial-up)\r\n"
                               " AT&L1 Selects leased line (no dial, no carrier detection)\r\n"
                               "AT&P Make/Break Ratio for Pulse Dialing\r\n"
                               " AT&P0 Selects 39%-61% make/break ratio at 10 pulses per second (NA)\r\n"
                               " AT&P1 Selects 33%-67% make/break ratio at 10 pulses per second (EU)\r\n"
                               " AT&P2 Selects 39%-61% make/break ratio at 20 pulses per second (NA)\r\n"
                               " AT&P3 Selects 33%-67% make/break ratio at 20 pulses per second (EU)\r\n"
                               "AT&R RTS/CTS Option Selection\r\n"
                               " AT&R0 ignore RTS\r\n"
                               " AT&R1 read RTS to control outgoing flow\r\n"
                               "AT&S DSR Option Selection\r\n"
                               " AT&S0 DSR line is always on, except when on-hook\r\n"
                               " AT&S1 DSR line follows CD\r\n"
                               "AT&V Show the configuration (everything)\r\n"
                               " AT&V0 show current profile\r\n"
                               " AT&V1 show stored profiles\r\n"
                               " AT&V2 show stored phone numbers\r\n"
                               "AT&W Store the configuration\r\n"
                               " AT&Wx store the profile in position x\r\n"
                               "AT&Y Read the configuration\r\n"
                               " AT&Yx read the profile from position x\r\n"
                               "AT&Z Store Telephone Number\r\n"
                               " AT&Z=nnn store phone number nnn in position 0\r\n"
                               " AT&Zx=nnn store phone number nnn in position x\r\n"
                               "\r\n"
                               "AT+FCLASS set the device for different modes (only data supported)\r\n"
                               " AT+FCLASS? show current device mode\r\n"
                               " AT+FCLASS=? list the supported device modes\r\n"
                               " AT+FCLASS=0 set the device mode to data\r\n"
                               "\r\n"
                               "\r\n"
                               "SReg  Description\r\n"
                               "   0  Rings to Auto-Answer\r\n"
                               "   1  Ring Counter\r\n"
                               "   2  Escape Character\r\n"
                               "   3  Carriage Return Character\r\n"
                               "   4  Line Feed Character\r\n"
                               "   5  Backspace Character\r\n"
                               "   6  Wait Time for Dial Tone\r\n"
                               "   7  Wait Time for Carrier\r\n"
                               "   8  Pause Time for Dial Delay Modifier\r\n"
                               "   9  Carrier Detect Response Time (tenths of a second)\r\n"
                               "  10  Carrier Loss Disconnect Time (tenths of a second)\r\n"
                               "  11  DTMF Tone Duration\r\n"
                               "  12  Escape Prompt Delay\r\n"
                               "  13  Reserved\r\n"
                               "  14  General Bit Mapped Options Status\r\n"
                               "  15  Reserved\r\n"
                              };


class HAYES {
  public:
    HAYES(CFG_t *conf, AFSK *afsk);
    ~HAYES();

    void printCRLF();
    void print_P(const char *str, bool newline = false);
    void banner();

    void getUptime(uint32_t secs, char *buf, size_t len);

    int16_t getInteger(char* buf, int8_t idx, uint8_t len = 32);
    int16_t getValidInteger(char* buf, uint8_t idx, int16_t low, int16_t hgh, int16_t def = 0, uint8_t len = 32);
    int16_t getValidInteger(int16_t low, int16_t hgh, int16_t def = 0, uint8_t len = 32);
    int8_t  getDigit(char* buf, uint8_t idx, int8_t def = HAYES_NUM_ERROR);
    int8_t  getDigit(int8_t def = HAYES_NUM_ERROR);
    int8_t  getValidDigit(char* buf, int8_t idx, int8_t low, int8_t hgh, int8_t def = HAYES_NUM_ERROR);
    int8_t  getValidDigit(int8_t low, int8_t hgh, int8_t def = HAYES_NUM_ERROR);


    uint8_t doSIO(uint8_t rcRemote = RC_NONE);
    void    doCommand();
    void    dispatch();

  private:
    CFG_t *cfg;
    AFSK  *afskModem;

    // Input buffer
    char buf[MAX_INPUT_SIZE];
    // Starting character
    char sChr = '\0';
    // Line buffer length
    int8_t len = 0;
    // Buffer index
    uint8_t idx = 0;
    // Local index
    uint8_t ldx = 0;
    // Numeric value
    int8_t value = 0;

    // One specific S register
    uint8_t _sreg = 0;

    // The result status of the last command
    uint8_t cmdResult = RC_OK;

    void    cmdPrint(char cmd, uint8_t value, bool newline = true);
    void    cmdPrint(char cmd, char mod, uint8_t value, bool newline = true);
    void    cmdPrint(uint8_t value);
    void    sregPrint(CFG_t *conf, uint8_t reg, bool newline = false);
    void    printResult(uint8_t code, char* buf = NULL);

    uint8_t dialCmdMode = 0;
    uint8_t dialReverse = 0;
    char    dialNumber[eePhoneLen];
    bool    getDialNumber(char *dn, size_t len);


    void    showProfile(CFG_t *conf);

};

#endif /* HAYES_H */
