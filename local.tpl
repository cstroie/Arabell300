/**
  local.h - Local configuration

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

#ifndef LOCAL_H
#define LOCAL_H

// Debug mode
//#define DEBUG

// EEPROM debug mode
//#define DEBUG_EE

// RX/TX debug
//#define DEBUG_RX_LVL

// CPU frequency correction for sampling timer
#define F_COR (0L)

// The PWM pin may be 3 or 11 (Timer2)
#define PWM_PIN 3

#endif /* LOCAL_H */
