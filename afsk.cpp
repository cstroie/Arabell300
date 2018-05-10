/**
  afsk.cpp - AFSK modulation/demodulation and serial decoding

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

  Many thanks to:
    Kamal Mostafa   https://github.com/kamalmostafa/minimodem
    Mark Qvist      https://github.com/markqvist/MicroModemGP
*/

#include "afsk.h"

// The wave generator
WAVE wave;

// The DTMF wave generator
DTMF dtmf(250, 250);

// FIFOs
FIFO txFIFO(6);
FIFO rxFIFO(6);
FIFO dyFIFO(4);


AFSK::AFSK() {
}

AFSK::~AFSK() {
}

/**
  Initialize the AFSK modem

  @param x the afsk modem type
*/
void AFSK::init(AFSK_t afsk, CFG_t *cfg) {
  _cfg  = cfg;
  // Hardware init
  this->initHW();
  // Set the modem type
  this->setModemType(afsk);
}

/**
  Set the modem type

  @param afsk the afsk modem type
*/
void AFSK::setModemType(AFSK_t afsk) {
  _afsk = afsk;
  // Compute the wave index steps
  this->initSteps();
  // Start as originating
  this->setDirection(ORIGINATING);
  // Start in command mode
  this->setMode(COMMAND_MODE);
  // Compute modem specific parameters
  fulBit = F_SAMPLE / _afsk.baud;
  hlfBit = fulBit >> 1;
  qrtBit = hlfBit >> 1;
  octBit = qrtBit >> 1;
}

/**
  Compute the originating and answering samples steps

  @param x the afsk modem to compute for
*/
void AFSK::initSteps() {
  _afsk.orig.step[SPACE] = wave.getStep(_afsk.orig.freq[SPACE]);
  _afsk.orig.step[MARK]  = wave.getStep(_afsk.orig.freq[MARK]);
  _afsk.answ.step[SPACE] = wave.getStep(_afsk.answ.freq[SPACE]);
  _afsk.answ.step[MARK]  = wave.getStep(_afsk.answ.freq[MARK]);
}

/**
  Initialize the hardware
*/
void AFSK::initHW() {
  // TC1 Control Register B: No prescaling, WGM mode 12
  TCCR1A = 0;
  TCCR1B = _BV(CS10) | _BV(WGM13) | _BV(WGM12);
  // Top set for F_SAMPLE
  ICR1 = ((F_CPU + F_COR) / F_SAMPLE) - 1;

  // Vcc with external capacitor at AREF pin
  // ADC Left Adjust Result
  ADMUX = _BV(REFS0) | _BV(ADLAR);

  // Analog input A0
  DDRC  &= ~_BV(0); // Port C Data Direction Register
  PORTC &= ~_BV(0); // Port C Data Register
  DIDR0 |=  _BV(0); // Digital Input Disable Register 0

  // Timer/Counter1 Capture Event
  ADCSRB = _BV(ADTS2) | _BV(ADTS1) | _BV(ADTS0);
  // ADC Enable, ADC Start Conversion, ADC Auto Trigger Enable,
  // ADC Interrupt Enable, ADC Prescaler 16 (1MHz)
  ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADATE) | _BV(ADIE) | _BV(ADPS2);

  // Set up Timer 2 to do Pulse Width Modulation on PIN 3 or 11
  ASSR    &= ~(_BV(EXCLK) | _BV(AS2));  // Use internal clock (p.160)
  TCCR2A  |= _BV(WGM21)   | _BV(WGM20); // Set fast PWM mode  (p.157)
  TCCR2B  &= ~_BV(WGM22);

#if PWM_PIN == 11
  // Configure the PWM PIN 11 (PB3)
  PORTB &= ~(_BV(PORTB3));
  DDRD  |= _BV(PORTD3);
  // Do non-inverting PWM on pin OC2A (p.155)
  TCCR2A = (TCCR2A | _BV(COM2A1)) & ~_BV(COM2A0);
  TCCR2A &= ~(_BV(COM2B1) | _BV(COM2B0));
  // No prescaler (p.158)
  TCCR2B = (TCCR2B & ~(_BV(CS12) | _BV(CS11))) | _BV(CS10);
#else
  // Configure the PWM PIN 3 (PD3)
  PORTD &= ~(_BV(PORTD3));
  DDRD  |= _BV(PORTD3);
  // Do non-inverting PWM on pin OC2B (p.155)
  TCCR2A = (TCCR2A | _BV(COM2B1)) & ~_BV(COM2B0);
  TCCR2A &= ~(_BV(COM2A1) | _BV(COM2A0));
  // No prescaler (p.158)
  TCCR2B = (TCCR2B & ~(_BV(CS12) | _BV(CS11))) | _BV(CS10);
#endif

  // Set initial pulse width to the first sample, progresively
  for (uint8_t i = 0; i <= wave.sample(0); i++) {
    DAC(i);
    delay(1);
  }

  // Configure the leds: RX PB0, TX PB1
  DDRB |= _BV(PORTB1) | _BV(PORTB0);
}

/**
  Send the sample to the DAC

  @param sample the sample to output to DAC
*/
inline void AFSK::DAC(uint8_t sample) {
#if PWM_PIN == 11
  OCR2A = sample;
#else
  OCR2B = sample;
#endif
}

/**
  TX workhorse.  This function is called by ISR for each output sample.
  First, it gets the sample value and sends it to DAC, then prepares
  the next sample.
*/
void AFSK::txHandle() {
  // Check if we are transmitting
  if (tx.active == ON or _carrier == ON) {
    // First thing first: get the sample
    uint8_t sample = wave.sample(tx.idx);
    // Output the sample
    DAC(sample);
    // Step up the index for the next sample
    tx.idx += fsqTX->step[tx.dtbit];

    // Check if we have sent all samples for a bit
    if (tx.clk++ >= fulBit) {
      // Reset the samples counter
      tx.clk = 0;

      // One bit finished, choose the next bit and TX state
      switch (tx.state) {
        // We have been offline, prepare the transmission
        case WAIT:
          // The carrier is MARK
          tx.dtbit  = MARK;
          // Check if there is data in FIFO
          if (not txFIFO.empty()) {
            // Get one byte from FIFO
            tx.data   = txFIFO.out();
            tx.state  = PREAMBLE;
            tx.bits   = 0;
          }
          else
            // No data in FIFO
            tx.state  = WAIT;
          break;

        // We are sending the preamble carrier
        case PREAMBLE:
          // Check if we have sent the carrier long enough
          if (++tx.bits >= carBits) {
            // Carrier sent, go to the start bit (SPACE)
            tx.state  = START_BIT;
            tx.dtbit  = SPACE;
          }
          break;

        // We have sent the start bit, go on with data bits
        case START_BIT:
          tx.state  = DATA_BIT;
          tx.dtbit  = tx.data & 0x01;
          tx.data   = tx.data >> 1;
          tx.bits   = 0;
          break;

        // We are sending the data bits, keep sending until the last
        case DATA_BIT:
          // Check if we have sent all the bits
          if (++tx.bits < _afsk.dtbits) {
            // Keep sending the data bits, LSB to MSB
            tx.dtbit  = tx.data & 0x01;
            tx.data   = tx.data >> 1;
          }
          else {
            // We have sent all the data bits, go on with the stop bit
            tx.state  = STOP_BIT;
            tx.dtbit  = MARK;
          }
          break;

        // We have sent the stop bit, try to get the next byte, if any
        case STOP_BIT:
          // Check the TX FIFO
          if (txFIFO.empty()) {
            // No more data to send, go on with the trail carrier (MARK)
            tx.state  = TRAIL;
            tx.dtbit  = MARK;
            tx.bits   = 0;
          }
          else {
            // There is still data, get one byte and go to the start bit
            tx.state  = START_BIT;
            tx.dtbit  = SPACE;
            tx.data   = txFIFO.out();
          }
          break;

        // We are sending the trail carrier
        case TRAIL:
          // Check if we have sent the trail carrier long enough
          if (++tx.bits > carBits) {
            // Disable TX and wait
            tx.active = OFF;
            tx.state  = WAIT;
            // TX led off
            PORTB    &= ~_BV(PORTB1);
          }
          else if (tx.bits == carBits and _carrier == OFF) {
            // After the last trail carrier bit, send nothing
            tx.dtbit  = MARK;
            // Prepare for the future TX
            tx.idx    = 0;
            tx.clk    = 0;
          }
          // Still sending the trail carrier, check the TX FIFO again
          else if (not txFIFO.empty()) {
            // There is new data in FIFO, start over
            tx.state  = START_BIT;
            tx.dtbit  = SPACE;
            tx.data   = txFIFO.out();
          }
          break;
      }
    }
  }
  // Check if we are tone dialing
  else if (_dialing) {
    // Get the DTMF sample
    if (dtmf.getSample())
      // Send the sample to DAC
      DAC(dtmf.sample);
    else if (not txFIFO.empty())
      // Check the FIFO for dial numbers
      dtmf.send(txFIFO.out());
    else
      // Stop dialing
      _dialing = OFF;
  }
}

/**
  RX workhorse.  Called by ISR for each input sample, it autocorrelates
  the input samples for a delay queue tailored for MARK symbol,
  low-passes the result and tries to to figure out the data bit.
  Then, it sends the data bit to decoder.

  @param sample the (unsigned) sample
*/
void AFSK::rxHandle(uint8_t sample) {
  // Create the signed sample
  int8_t ss = sample - bias;

#ifdef DEBUG_RX_LVL
  // Keep sample for level measurements
  if (sample < inMin) inMin = sample;
  if (sample > inMax) inMax = sample;
  // Count 256 samples, which means 8 bits
  if (++inSamples == 0x00) {
    // Get the level
    inLevel = inMax - inMin;
    // Reset MIN and MAX
    inMin = 0xFF;
    inMax = 0x00;
  }
#endif

  // First order low-pass Chebyshev filter
  //  300:   0.16272643677832518 0.6745471264433496
  //  600:   0.28187392036298453 0.4362521592740309
  //  1200:  0.4470595850866754  0.10588082982664918

  rx.iirX[0] = rx.iirX[1];
  rx.iirX[1] = ((dyFIFO.out() - 128) * ss) >> 2;
  //rx.iirX[1] = ((dyFIFO.out() - 128) * ss) >> 3;
  rx.iirY[0] = rx.iirY[1];
  rx.iirY[1] = rx.iirX[0] + rx.iirX[1] + (rx.iirY[0] >> 1);
  //rx.iirY[1] = rx.iirX[0] + rx.iirX[1] + ((rx.iirY[0] >> 4) * 15); // *0.9

  // Keep the unsigned sample in delay FIFO
  dyFIFO.in(sample);

  // TODO Validate the RX tones
  rx.active = true; //abs(rx.iirY[1] > 1);
  if (rx.active)
    // Call the decoder
    rxDecoder(((rx.iirY[1] > 0) ? MARK : SPACE) ^ fsqRX->polarity);
  else
    // Disable the decoder and wait
    rx.state  = WAIT;
}

/**
  The RX data decoder.  Receive the decoded data bit and try
  to figure out the entire received byte.

  @param bt the decoded data bit
*/
void AFSK::rxDecoder(uint8_t bt) {
  // Keep the bitsum and the bit stream
  rx.bitsum  += bt;
  rx.stream <<= 1;
  rx.stream  |= bt;

  // Count the received samples
  rx.clk++;

  // Check the RX decoder state
  switch (rx.state) {
    // Check each sample for a HIGH->LOW transition
    case WAIT:
      // Check the transition
      if ((rx.stream & 0x03) == 0x02) {
        // We have a transition, let's assume the start bit begins here,
        // but we need a validation, which will be done in PREAMBLE state
        rx.state  = PREAMBLE;
        rx.clk    = 0;
        rx.bitsum = 0;
      }
      break;

    // Validate the start bit after half the samples have been collected
    case PREAMBLE:
      // Check if we have collected enough samples
      if (rx.clk >= hlfBit) {
        // Check the average level of decoded samples: less than a quarter
        // of them may be HIGHs; the bitsum must be lesser than octBit
        if (rx.bitsum > octBit) {
          // Too many HIGH, this is not a start bit
          rx.state  = WAIT;
          // RX led off
          PORTB    &= ~_BV(PORTB0);
        }
        else {
          // Could be a start bit, keep on going and check again at the end
          rx.state  = START_BIT;
          // RX led on
          PORTB    |= _BV(PORTB0);
        }
      }
      break;

    // Other states than WAIT and PREAMBLE (for each sample)
    default:
      // Check if we have received all the samples required for a bit
      if (rx.clk >= fulBit) {
        // Check the RX decoder state
        switch (rx.state) {
          // We have received the start bit
          case START_BIT:
#ifdef DEBUG_RX
            rxFIFO.in('S');
            //rxFIFO.in(rx.bitsum > hlfBit) ? '#' : '_');
            rxFIFO.in((rx.bitsum >> 2) + 'A');
#endif
            // Check the average level of decoded samples: less than a quarter
            // of them may be HIGH, the bitsum must be lesser than qrtBit
            if (rx.bitsum > qrtBit) {
              // Too many HIGHs, this is not a start bit
              rx.state  = WAIT;
              // RX led off
              PORTB    &= ~_BV(PORTB0);
            }
            else {
              // This is a start bit, go on to data bits
              rx.state  = DATA_BIT;
              rx.clk    = 0;
              rx.bitsum = 0;
              rx.bits   = 0;
            }
            break;

          // We have received a data bit
          case DATA_BIT:
            // Keep the received bits, LSB first, shift right
            rx.data = rx.data >> 1;
            // The received data bit value is the average of all decoded
            // samples.  We count the HIGH samples, threshold at half
            rx.data |= rx.bitsum > hlfBit ? 0x80 : 0x00;
#ifdef DEBUG_RX
            rxFIFO.in(47 + rx.bits);
            //rxFIFO.in(rx.bitsum > hlfBit ? '#' : '_');
            rxFIFO.in((rx.bitsum >> 2) + 'A');
#endif
            // Check if we are still receiving the data bits
            if (++rx.bits < _afsk.dtbits) {
              // Prepare for a new bit: reset the clock and the bitsum
              rx.clk    = 0;
              rx.bitsum = 0;
            }
            else {
              // Go on with the stop bit, count only half the samples
              rx.state  = STOP_BIT;
              rx.clk    = hlfBit;
              rx.bitsum = 0;
            }
            break;

          // We have received the first half of the stop bit
          case STOP_BIT:
#ifdef DEBUG_RX
            rxFIFO.in('T');
            rxFIFO.in((rx.bitsum >> 2) + 'A');
            rxFIFO.in(' ');
#endif
            // Check the average level of decoded samples: at least half
            // of them must be HIGH, the bitsum must be more than qrtBit
            // (remember we have only the first half of the stop bit)
            if (rx.bitsum > qrtBit)
              // Push the data into FIFO
              rxFIFO.in(rx.data);
#ifdef DEBUG_RX
            rxFIFO.in(10);
#endif
            // Start over again
            rx.state  = WAIT;
            // RX led off
            PORTB    &= ~_BV(PORTB0);
            break;
        }
      }
  }
}

/**
  Check the serial I/O and transmit the data, respectively check
  the received data and send it to the serial port.
*/
bool AFSK::doSIO() {
  // The charcter on the serial line
  uint8_t c;
  // The escape characters counters
  static uint8_t  escCount = 0;
  static uint32_t escFirst = 0;
  static uint32_t escLast  = 0;
  // Characters waiting on the serial input
  bool available = (Serial.available() != 0);

  // Check if we saw the escape string "+++"
  if (escCount == 3) {
    // We did, we did taw the escape string!
    // Check for the guard silence
    if (millis() - escLast > 1000) {
      // This is it, go in command mode
      this->setMode(COMMAND_MODE);
      escCount = 0;
    }
    else if (available) {
      // We saw the string recently, check if there is any
      // other character on the line.
      c = Serial.peek();
      if (c == '\r' or c == '\n')
        // Ignore CR and LF.
        Serial.read();
      else
        // There is something else, ignore the escape string
        escCount = 0;
    }
  }

  // Check if there is any data on serial port
  if (available) {
    // Check for "+++" escape sequence
    if (Serial.peek() == '+') {
      // Check when we saw the first '+'
      if (millis() - escFirst > 1000) {
        // This is the first, keep the time
        escCount = 1;
        escFirst = millis();
      }
      else {
        // Not the first, count them up until three
        escCount++;
        if (escCount == 3) {
          // This is the last, keep the time and go wait
          // for the guard silence
          escLast = millis();
        }
      }
    }
  }

  if (this->_mode != COMMAND_MODE) {
    if (available) {
      // There is data on serial port, process it normally
      if (not txFIFO.full()) {
        // FIFO not full, we can send the data
        c = Serial.read();
        if (txFIFO.in(c))
          // Local data mode echo
          if (_cfg->dtecho)
            Serial.print((char)c);
        // Keep transmitting
        tx.active = ON;
        // TX led on
        PORTB |= _BV(PORTB1);
      }
    }

    // Check if there is any data in RX FIFO
    if (not rxFIFO.empty()) {
      // Get the byte and send it to serial line
      c = rxFIFO.out();
      Serial.write(c);
    }
    // Return true, to let the caller know we have processed the data
    return true;
  }
  else
    // We have not processed the data
    return false;
}

/**
  Handle both the TX and RX, if in data mode
*/
void AFSK::doTXRX() {
  if (this->_online) {
    this->txHandle();
    this->rxHandle(ADCH);
  }
}

/**
  Set the connection direction

  @param dir connection direction ORIGINATING, ANSWERING
*/
void AFSK::setDirection(uint8_t dir, uint8_t rev) {
  // Keep the direction
  _dir = dir;
  // Stop the carrier
  this->setCarrier(OFF);
  // Create TX/RX pointers to ORIGINATING/ANSWERING parameters
  if ((_dir == ORIGINATING and rev == OFF) or
      (_dir == ANSWERING and _cfg->revans == ON)) {
    fsqTX = &_afsk.orig;
    fsqRX = &_afsk.answ;
  }
  else {
    fsqTX = &_afsk.answ;
    fsqRX = &_afsk.orig;
  }
  // Go offline
  this->setOnline(OFF);
}

/**
  Set the online status

  @param online online status
*/
void AFSK::setOnline(uint8_t online) {
  // Keep the online status
  _online = online;

  if (online == OFF) {
    // Command mode
    this->setMode(COMMAND_MODE);
    // Clear the FIFOs
    rxFIFO.clear();
    txFIFO.clear();
    // Prepare the delay queue for RX
    dyFIFO.clear();
    for (uint8_t i = 0; i < fsqRX->queuelen; i++)
      dyFIFO.in(bias);
  }
}

/**
  Set the modem mode

  @param mode command mode or data mode
*/
void AFSK::setMode(uint8_t mode) {
  // Keep the mode
  this->_mode = mode;
}

/**
  Enable or disable the carrier going at runtime

  @param onoff carrier mode
*/
void AFSK::setCarrier(uint8_t onoff) {
  _carrier = onoff & _cfg->txcarr;
}

/**
  Dial a number

  @param number the number to dial
*/
void AFSK::dial(char *buf) {
  // Disable the carrier
  this->setCarrier(OFF);
  // Store the dial number in TX FIFO
  txFIFO.clear();
  while (*buf != 0)
    txFIFO.in(*buf++);
  // Start dialing
  _dialing = ON;
  // Block until dialing is over
  while (_dialing == ON)
    delay(10);
}

/**
  Test case simulation: feed the RX demodulator
*/
void AFSK::simFeed() {
  // Simulation
  static uint8_t idx = 0;
  uint8_t bt = (millis() / 1000) % 2;

  int8_t x = wave.sample(idx);

  /*
    int8_t y = bs2225(x);
    Serial.print(lp200(((int16_t)x * x) >> 8));
    Serial.print(",");
    Serial.println(lp200(((int16_t)y * y) >> 8));
  */

  /*
    Serial.print(x);
    Serial.print(",");
    Serial.print(hm0.getPower(x));
    Serial.print(",");
    Serial.print(hm1.getPower(x));
    Serial.print(",");
    Serial.print(bt * 10);
    Serial.println();
  */

  //Serial.println(idx);

  rxHandle(x);
  idx += fsqRX->step[bt];
}

/**
  Test case simulation: print demodulated data
*/
void AFSK::simPrint() {
  static uint32_t next = millis();
  if (millis() > next) {
    //Serial.print(rx.iirX[1]);
    //Serial.print(",");
    Serial.println(rx.iirY[1]);
    next += 100;
  }
}
