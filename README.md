# Arabell300

Old school Arduino modem, Bell 103 and ITU V.21 compatible, at 300 baud.

## Overview

Arabell300 is a software implementation of a 300 baud modem for Arduino platforms. It supports both Bell 103 (300 baud) and ITU V.21 (300 baud) standards, making it compatible with vintage terminal equipment and bulletin board systems (BBS).

## Features

- **Modem Standards**: Bell 103 and ITU V.21 compatibility
- **Baud Rate**: 300 baud operation
- **Hardware**: Designed for Arduino microcontrollers
- **Protocol Support**: Full duplex communication with Hayes-compatible AT command set
- **Audio Interface**: Audio frequency shift keying (AFSK) modulation/demodulation
- **DTMF Support**: Dual-tone multi-frequency signaling
- **Configurable Profiles**: EEPROM-based configuration storage

## Technical Specifications

- **Bell 103**:
  - Originate: 1070 Hz (MARK), 1270 Hz (SPACE)
  - Answer: 2025 Hz (MARK), 2225 Hz (SPACE)
- **V.21**:
  - Channel 1: 1180 Hz (MARK), 980 Hz (SPACE)
  - Channel 2: 1850 Hz (MARK), 1650 Hz (SPACE)
- **Sample Rate**: 7680 Hz
- **Data Encoding**: Asynchronous serial with start/stop bits

## Hardware Requirements

- Arduino board (ATmega328P or compatible)
- Audio input/output circuitry
- Optional: Speaker for monitoring
- Optional: LED indicators for status

## Software Architecture

The modem implementation consists of several key components:

- **AFSK**: Audio Frequency Shift Keying modulation/demodulation
- **DTMF**: Dual-Tone Multi-Frequency signal generation
- **Wave**: Waveform generation and sampling
- **FIFO**: First-In-First-Out buffer management
- **Config**: EEPROM-based configuration profiles
- **Hayes**: AT command interpreter and response handling

## Usage

Connect to the modem using a terminal program at 300 baud. The modem supports standard Hayes-compatible AT commands for configuration and operation.

## Debugging

The firmware includes several debug modes that can be enabled:
- `DEBUG`: General debugging output
- `DEBUG_EE`: EEPROM contents display
- `DEBUG_RX_LVL`: Receive level monitoring

## License

This project is released under the MIT License.
