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
  CRC8 computing

  @param inCrc partial CRC
  @param inData new data
  @return updated CRC
*/
uint8_t Profile::CRC8(uint8_t inCrc, uint8_t inData) {
  uint8_t outCrc = inCrc ^ inData;
  for (uint8_t i = 0; i < 8; ++i) {
    if ((outCrc & 0x80 ) != 0) {
      outCrc <<= 1;
      outCrc ^= 0x07;
    }
    else {
      outCrc <<= 1;
    }
  }
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
  for (uint8_t i = 1; i < cfgLen; i++)
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
    for (uint8_t i = 1; i < cfgLen; i++)
      if (cfg1->data[i] != cfg2->data[i])
        result = false;
  return result;
}

/**
  Write the configuration to EEPROM, along with CRC8, if different
*/
bool Profile::write(CFG_t *cfg) {
  // Temporary configuration structure
  struct CFG_t cfgTemp;
  // Read the data from EEPROM into the temporary structure
  EEPROM.get(eeAddress, cfgTemp.data);
  // Compute the CRC8 checksum of the read data
  uint8_t crc8 = this->crc(&cfgTemp);
  // Compute the CRC8 checksum of the actual data
  cfg->crc8 = this->crc(cfg);
  // Compare the new and the stored data and check if the stored data is valid
  if (not this->equal(cfg, &cfgTemp) or (cfgTemp.crc8 != crc8)) {
    // Copy the actual data to the temporary structure
    for (uint8_t i = 0; i < cfgLen; i++)
      cfgTemp.data[i] = cfg->data[i];
    // Write the data
    EEPROM.put(eeAddress, cfgTemp.data);
  }
  // Always return true, even if data is not written
  return true;
}

/**
  Read the configuration from EEPROM, along with CRC8, and verify
*/
bool Profile::read(CFG_t *cfg, bool useDefaults = false) {
  // Temporary configuration structure
  struct CFG_t cfgTemp;
  // Read the data from EEPROM into the temporary structure
  EEPROM.get(eeAddress, cfgTemp);
  // Compute the CRC8 checksum of the read data
  uint8_t crc8 = this->crc(&cfgTemp);
  // And compare with the read crc8 checksum
  if (cfgTemp.crc8 == crc8) {
    // Copy the temporary structure to configuration and the crc8, too
    for (uint8_t i = 0; i < cfgLen; i++)
      cfg->data[i] = cfgTemp.data[i];
  }
  else if (useDefaults)
    factory(cfg);
  return (cfgTemp.crc8 == crc8);
}

/**
  Reset the configuration to factory defaults
*/
bool Profile::factory(CFG_t *cfg) {
  cfg->compro = 0x01; // ATB
  cfg->txcarr = 0x01; // ATC
  cfg->cmecho = 0x01; // ATE
  cfg->dtecho = 0x00; // ATF
  cfg->spklvl = 0x01; // ATL
  cfg->spkmod = 0x01; // ATM
  cfg->quiet  = 0x00; // ATQ
  cfg->verbal = 0x01; // ATV
  cfg->selcpm = 0x00; // ATX
  return true;
};

/**
  Try to load the stored profile or use the factory defaults
*/
void Profile::init(CFG_t *cfg) {
  this->read(cfg, true);
}

