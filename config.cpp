/**
  config.cpp - Modem persistent configuration

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

Profile::Profile() {
}

Profile::~Profile() {
}

/**
  Try to load the specified stored profile or use the factory defaults

  @param cfg the configuration structure
  @param slot the stored profile
*/
void Profile::init(CFG_t *cfg, uint8_t slot) {
  this->read(cfg, slot, true);
}

/**
  CRC8 computing

  @param inCrc partial CRC
  @param inData new data
  @return updated CRC
*/
uint8_t Profile::CRC8(uint8_t inCrc, uint8_t inData) {
  uint8_t outCrc = inCrc ^ inData;
  for (uint8_t i = 0; i < 8; ++i)
    if ((outCrc & 0x80 ) != 0) {
      outCrc <<= 1;
      outCrc ^= 0x07;
    }
    else
      outCrc <<= 1;
  return outCrc;
}

/**
  Compute the CRC8 of the configuration structure

  @param cfgTemp the configuration structure
  @return computed CRC8
*/
uint8_t Profile::crc(CFG_t *cfg) {
  // Compute the CRC8 checksum of the data
  uint8_t crc8 = 0;
  for (uint8_t i = 1; i < eeProfLen; i++)
    crc8 = this->CRC8(crc8, cfg->data[i]);
  return crc8;
}

/**
  Compare two configuration structures

  @param cfg1 the first configuration structure
  @param cfg1 the second configuration structure
  @return true if equal
*/
bool Profile::equal(CFG_t *cfg1, CFG_t *cfg2) {
  bool result = true;
  // Check CRC first
  if (cfg1->crc8 != cfg2->crc8)
    result = false;
  else
    // Compare the overlayed array of the two structures
    for (uint8_t i = 1; i < eeProfLen; i++)
      if (cfg1->data[i] != cfg2->data[i])
        result = false;
  return result;
}

/**
  Write the configuration to EEPROM, along with CRC8, if different

  @param cfg the configuration structure
  @param slot the stored profile
  @return always true
*/
bool Profile::write(CFG_t *cfg, uint8_t slot) {
  // Temporary configuration structure
  struct CFG_t cfgTemp;
  // Open EEPROM
  EEPROM.begin(EESIZE);
  // Read the data from EEPROM into the temporary structure
  for (uint8_t i = 0; i < eeProfLen; i++)
    cfgTemp.data[i] = EEPROM.read(eeAddrCfg + slot * eeProfLen + i);
  // Compute the CRC8 checksum of the read data
  uint8_t crc8 = this->crc(&cfgTemp);
  // Compute the CRC8 checksum of the actual data, make sure S1 is zero
  cfg->sregs[1] = 0;
  cfg->crc8 = this->crc(cfg);
  // Compare the new and the stored data and check if the stored data is valid
  if (not this->equal(cfg, &cfgTemp) or (cfgTemp.crc8 != crc8))
    // Write the data to EEPROM
    for (uint8_t i = 0; i < eeProfLen; i++)
      EEPROM.write(eeAddrCfg + slot * eeProfLen + i, cfg->data[i]);
  // Close EEPROM
  EEPROM.end();
  // Always return true, even if data is not written
  return true;
}

/**
  Read the configuration from EEPROM, along with CRC8, and verify

  @param cfg the configuration structure
  @param slot the stored profile
  @param useDefaults use the factory defauls in case of error
  @return CRC verification
*/
bool Profile::read(CFG_t *cfg, uint8_t slot, bool useDefaults) {
  // Temporary configuration structure
  struct CFG_t cfgTemp;
  // Open EEPROM
  EEPROM.begin(EESIZE);
  // Read the data from EEPROM into the temporary structure
  for (uint8_t i = 0; i < eeProfLen; i++)
    cfgTemp.data[i] = EEPROM.read(eeAddrCfg + slot * eeProfLen + i);
  // Compute the CRC8 checksum of the read data
  uint8_t crc8 = this->crc(&cfgTemp);
  // And compare with the read crc8 checksum, also check S1 and S2
  if (cfgTemp.crc8 == crc8 and
      cfgTemp.sregs[1] == 0 and
      cfgTemp.sregs[2] >= ' ' and
      cfgTemp.sregs[2] <= '~')
    // Copy the temporary structure to configuration and the crc8
    for (uint8_t i = 0; i < eeProfLen; i++)
      cfg->data[i] = cfgTemp.data[i];
  else if (useDefaults)
    factory(cfg);
  // Close EEPROM
  EEPROM.end();
  return (cfgTemp.crc8 == crc8);
}

/**
  Reset the configuration to factory defaults

  @param cfg the configuration structure
  @return always true
*/
bool Profile::factory(CFG_t *cfg) {
  // Set the configuration
  cfg->compro = 0x10; // ATB
  cfg->txcarr = 0x01; // ATC
  cfg->cmecho = 0x01; // ATE
  cfg->dtecho = 0x01; // ATF
  cfg->spklvl = 0x01; // ATL
  cfg->spkmod = 0x00; // ATM
  cfg->quiet  = 0x00; // ATQ
  cfg->verbal = 0x01; // ATV
  cfg->selcpm = 0x00; // ATX
  cfg->dialpt = 0x01; // ATP/T
  cfg->revans = 0x00; // AT&A
  cfg->dcdopt = 0x01; // AT&C
  cfg->dtropt = 0x01; // AT&D
  cfg->jcksel = 0x00; // AT&J
  cfg->flwctr = 0x00; // AT&K
  cfg->lnetpe = 0x00; // AT&L
  cfg->plsrto = 0x00; // AT&P
  cfg->rtsopt = 0x00; // AT&R
  cfg->dsropt = 0x00; // AT&S

  // Set the S regs
  memcpy_P(&cfg->sregs, &sRegs, 16);

  //Always return true
  return true;
};

/**
  Get the specfied S register value

  @param cfg the configuration structure
  @param reg the specified S register
  @return the register value
*/
uint8_t Profile::sregGet(CFG_t *cfg, uint8_t reg) {
  return cfg->sregs[reg];
}

/**
  Set the value to specfied S register

  @param cfg the configuration structure
  @param reg the specified S register
  @param value the value to set
*/
void Profile::sregSet(CFG_t *cfg, uint8_t reg, uint8_t value) {
  cfg->sregs[reg] = value;
}

/**
  Get the stored phone number from the specfied slot

  @param phone the phone number
  @param slot the storge slot
  @return success result
*/
uint8_t Profile::pbGet(char *phone, uint8_t slot) {
  // Open EEPROM
  EEPROM.begin(EESIZE);
  // Compute the eeprom address
  uint16_t address = eeAddrPBook + slot * eePhoneLen;
  // Get the chars
  for (uint8_t i = 0; i < eePhoneLen; i++) {
    phone[i] = EEPROM.read(address + i);
    // Minimal validation
    if (phone[i] < ' ' or phone[i] > '~') {
      phone[i] = '\0';
      break;
    }
  }
  // Close EEPROM
  EEPROM.end();
}

/**
  Set the phone number from the specfied slot

  @param phone the phone number
  @param slot the storge slot
*/
void Profile::pbSet(char *phone, uint8_t slot) {
  // Open EEPROM
  EEPROM.begin(EESIZE);
  // Compute the eeprom address
  uint16_t address = eeAddrPBook + slot * eePhoneLen;
  // Store the chars only if changed
  for (uint8_t i = 0; i < eePhoneLen; i++) {
    if (EEPROM.read(address + i) != phone[i])
      EEPROM.write(address + i, phone[i]);
    if (phone[i] == '\0') break;
  }
  // Close EEPROM
  EEPROM.end();
}

/**
  Get the stored WiFi credentials

  @param ssid the WiFi SSID
  @param pass the WiFi password
  @return success result
*/
bool Profile::wfGet(char *ssid, char *pass) {
  // Open EEPROM
  EEPROM.begin(EESIZE);
  // Get the ssid
  for (uint8_t i = 0; i < WL_SSID_MAX_LENGTH; i++)
    ssid[i] = EEPROM.read(eeAddrWiFi + i);
  ssid[WL_SSID_MAX_LENGTH] = '\0';
  // Get the password
  for (uint8_t i = 0; i < WL_WPA_KEY_MAX_LENGTH; i++)
    pass[i] = EEPROM.read(eeAddrWiFi + WL_SSID_MAX_LENGTH + i);
  pass[WL_WPA_KEY_MAX_LENGTH] = '\0';
  // Close EEPROM
  EEPROM.end();
  return (ssid[0] >= ' ' and ssid[0] <= '~' and
          pass[0] >= ' ' and pass[0] <= '~');
}

/**
  Store the WiFi credentials

  @param ssid the WiFi SSID
  @param pass the WiFi password
*/
void Profile::wfSet(char *ssid, char *pass) {
  // Open EEPROM
  EEPROM.begin(EESIZE);
  // Set the ssid
  for (uint8_t i = 0; i < WL_SSID_MAX_LENGTH; i++) {
    EEPROM.write(eeAddrWiFi + i, ssid[i]);
    if (ssid[i] == '\0') break;
  }
  // Set the password
  for (uint8_t i = 0; i < WL_WPA_KEY_MAX_LENGTH; i++) {
    EEPROM.write(eeAddrWiFi + WL_SSID_MAX_LENGTH + i, pass[i]);
    if (pass[i] == '\0') break;
  }
  // Close EEPROM
  EEPROM.end();
}
