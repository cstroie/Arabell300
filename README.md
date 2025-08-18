# Arabell300

Old school Arduino modem, Bell 103 and ITU V.21 compatible, at 300 baud.

## Overview

Arabell300 is a software implementation of a 300 baud modem for Arduino platforms. It supports both Bell 103 (300 baud) and ITU V.21 (300 baud) standards, making it compatible with vintage terminal equipment and bulletin board systems (BBS).

## Features

- **Modem Standards**: Bell 103 and ITU V.21 compatibility
- **Baud Rate**: 300 baud operation
- **Hardware**: Designed for Arduino microcontrollers (ATmega328P or compatible)
- **Protocol Support**: Full duplex communication with Hayes-compatible AT command set
- **Audio Interface**: Audio frequency shift keying (AFSK) modulation/demodulation
- **DTMF Support**: Dual-tone multi-frequency signaling for dialing
- **Configurable Profiles**: EEPROM-based configuration storage with multiple profiles
- **Flow Control**: Hardware (RTS/CTS) and software (XON/XOFF) flow control
- **Phone Book**: Storage of up to 8 phone numbers in EEPROM
- **Call Progress Monitoring**: Detection of dial tone, busy signals, and carrier

## Technical Specifications

### Modem Standards

- **Bell 103**:
  - Originate: 1070 Hz (MARK), 1270 Hz (SPACE)
  - Answer: 2025 Hz (MARK), 2225 Hz (SPACE)
- **V.21**:
  - Channel 1: 1180 Hz (MARK), 980 Hz (SPACE)
  - Channel 2: 1850 Hz (MARK), 1650 Hz (SPACE)

### Signal Processing

- **Sample Rate**: 9600 Hz
- **Data Encoding**: Asynchronous serial with start/stop bits
- **Data Bits**: 8 bits
- **Implementation**: Software-based AFSK modem with digital signal processing

## Hardware Requirements

### Core Components

- Arduino board (ATmega328P or compatible)
- Audio input/output circuitry
- Low-pass filter for audio input
- Audio coupling capacitors

### Optional Components

- Speaker for monitoring transmitted/received audio
- LED indicators for status (RX, TX, CD, OH, RI)
- Hardware flow control connections (RTS/CTS)
- DTR/DSR control lines

## Software Architecture

The modem implementation consists of several key components:

- **AFSK**: Audio Frequency Shift Keying modulation/demodulation and serial decoding
- **DTMF**: Dual-Tone Multi-Frequency signal generation for dialing
- **Wave**: Waveform generation and sampling using lookup tables
- **FIFO**: First-In-First-Out buffer management for serial data
- **Config**: EEPROM-based configuration profiles and phone number storage
- **Hayes**: AT command interpreter and response handling

## Hayes Compatible Commands

The modem supports a comprehensive set of Hayes-compatible AT commands:

### Basic Commands
- `ATA` - Answer incoming call
- `ATD` - Dial phone number (ATDT for tone, ATDP for pulse)
- `ATH` - Hook control (on/off line)
- `ATO` - Return to data mode
- `ATZ` - Reset modem

### Configuration Commands
- `ATB` - Select communication protocol (15=V.21, 16=Bell103)
- `ATC` - Transmit carrier control
- `ATE` - Local command mode echo
- `ATF` - Local data mode echo (duplex control)
- `ATL` - Speaker volume level
- `ATM` - Speaker control
- `ATP/T` - Pulse/Tone dialing selection
- `ATQ` - Quiet mode (result code suppression)
- `ATV` - Verbose mode (text vs numeric result codes)
- `ATX` - Call progress method (basic/extended result codes)

### Extended Commands
- `AT&A` - Reverse answering frequencies
- `AT&C` - DCD (Carrier Detect) option
- `AT&D` - DTR (Data Terminal Ready) option
- `AT&F` - Load factory defaults
- `AT&J` - Jack type selection (PWM output pins)
- `AT&K` - Flow control selection
- `AT&L` - Line type selection (PSTN/leased line)
- `AT&P` - Pulse dialing make/break ratio
- `AT&R` - RTS (Request To Send) option
- `AT&S` - DSR (Data Set Ready) option
- `AT&V` - View configuration profiles
- `AT&W` - Store configuration profile
- `AT&Y` - Load configuration profile
- `AT&Z` - Store/Retrieve phone numbers

### S-Register Configuration
- S0: Rings to Auto-Answer
- S1: Ring Counter
- S2: Escape Character (default '+')
- S3: Carriage Return Character
- S4: Line Feed Character
- S5: Backspace Character
- S6: Wait Time for Dial Tone
- S7: Wait Time for Carrier
- S8: Pause Time for Dial Delay Modifier
- S9: Carrier Detect Response Time
- S10: Carrier Loss Disconnect Time
- S11: DTMF Tone Duration
- S12: Escape Prompt Delay

## Usage

### Basic Operation

1. Connect to the modem using a terminal program at 300 baud
2. Use Hayes-compatible AT commands for configuration and operation
3. Dial numbers using `ATDT<phone_number>` for tone dialing
4. Answer calls with `ATA` when receiving a RING indication

### Example Commands

```AT
AT&F                    # Load factory defaults
ATB16                   # Set Bell 103 mode
ATL1                    # Set medium speaker volume
ATM3                    # Enable speaker for TX and RX
ATDT5551234             # Dial phone number using tone dialing
ATA                     # Answer incoming call
+++"                    # Escape to command mode from data mode
ATO                     # Return to data mode
AT&W0                   # Save current configuration to profile 0
AT&Y0                   # Load configuration from profile 0
```

### Data Mode Operation

Once connected, the modem operates in full duplex mode at 300 baud. The escape sequence `+++` (with proper guard time) can be used to return to command mode.

## Debugging

The firmware includes several debug modes that can be enabled by uncommenting definitions in the source code:

- `DEBUG`: General debugging output
- `DEBUG_EE`: EEPROM contents display
- `DEBUG_RX_LVL`: Receive level monitoring
- `DEBUG_RX`: Receive bit decoding visualization

## Building and Flashing

1. Open the Arabell300.ino file in the Arduino IDE
2. Select the appropriate board (Arduino Uno/Nano recommended)
3. Compile and upload to your Arduino
4. Connect the audio circuitry to the appropriate pins

## License

This project is released under the GNU General Public License v3.0.

## Author

Costin STROIE <costinstroie@eridu.eu.org>
