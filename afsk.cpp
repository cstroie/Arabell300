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
    Kamal Mostafa    https://github.com/kamalmostafa/minimodem
    Mark Qvist       https://github.com/markqvist/MicroModemGP
    Francesco Sacchi https://github.com/develersrl/bertos/blob/master/bertos/net/afsk.c
*/

#include "afsk.h"

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))


// The wave generator
WAVE wave;

// The DTMF wave generator
DTMF dtmf;

// FIFOs
const uint8_t fifoSize = 4;
const uint8_t fifoLow =  1 << (fifoSize - 2);
const uint8_t fifoMed =  1 << (fifoSize - 1);
const uint8_t fifoHgh = (1 << fifoSize) - fifoLow;
FIFO txFIFO(fifoSize);
FIFO rxFIFO(fifoSize);
FIFO dyFIFO(4);


AFSK::AFSK() {
}

AFSK::~AFSK() {
}

/**
  Initialize the AFSK modem

  @param x the afsk modem type
*/
void AFSK::init(AFSK_t afsk, CFG_t *conf) {
  cfg = conf;
  // Hardware init
  this->initHW();
  // Set the modem type
  this->setModemType(afsk);
  // Set the DTMF pulse and pause durations (S11)
  dtmf.setDuration(cfg->sregs[11]);
  // Set the escape character and the guard time
  escChar  = cfg->sregs[2];
  escGuard = cfg->sregs[12] * 20;
}

/**
  Set the modem type

  @param afsk the afsk modem type
*/
void AFSK::setModemType(AFSK_t afsk) {
  cfgAFSK = afsk;
  // Compute the wave index steps
  this->initSteps();
  // Go offline, switch to command mode
  this->setLine(OFF);
  // Start as originating modem
  this->setDirection(ORIGINATING);
  // Compute modem specific parameters
  fulBit = F_SAMPLE / cfgAFSK.baud;
  hlfBit = fulBit >> 1;
  qrtBit = hlfBit >> 1;
  octBit = qrtBit >> 1;
  // Compute CarrierDetect threshold
  cdTotal = F_SAMPLE / 10 * cfg->sregs[9];
  cdTotal = cdTotal - (cdTotal >> 4);
}

/**
  Compute the originating and answering samples steps, as Q8.8

  @param x the afsk modem to compute for
*/
void AFSK::initSteps() {
  cfgAFSK.orig.step[SPACE] = wave.getStep(cfgAFSK.orig.freq[SPACE]);
  cfgAFSK.orig.step[MARK]  = wave.getStep(cfgAFSK.orig.freq[MARK]);
  cfgAFSK.answ.step[SPACE] = wave.getStep(cfgAFSK.answ.freq[SPACE]);
  cfgAFSK.answ.step[MARK]  = wave.getStep(cfgAFSK.answ.freq[MARK]);
}

/**
  Initialize the hardware
*/
void AFSK::initHW() {
  //Disable interrupts
  cli();

  // disable Timer0 !!! delay() is now not available
  //cbi (TIMSK0, TOIE0);

  // TC1 Control Register B: No prescaling, WGM mode 12
  TCCR1A = 0;
  TCCR1B = _BV(CS10) | _BV(WGM13) | _BV(WGM12);
  // Top set for F_SAMPLE
  ICR1 = ((F_CPU + F_COR) / F_SAMPLE) - 1;

  // ADC Left Adjust Result
#ifdef AREF_EXT
  // 3.3V at AREF pin with external capacitor
  ADMUX = _BV(ADLAR);
#else
  // Vcc with external capacitor at AREF pin
  ADMUX = _BV(REFS0) | _BV(ADLAR);
#endif

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
  ASSR    &= ~(_BV(EXCLK) | _BV(AS2));  // Use internal clock (p.213)
  TCCR2A  |=   _BV(WGM21) | _BV(WGM20); // Set fast PWM mode  (p.205)
  TCCR2B  &= ~(_BV(WGM22));

  // Configure the PWM PIN 11 (PB3)
  PORTB &= ~(_BV(PORTB3));
  DDRB  |=   _BV(PORTB3);
  // Configure the PWM PIN 3 (PD3)
  PORTD &= ~(_BV(PORTD3));
  DDRD  |=   _BV(PORTD3);
  // Do non-inverting PWM on pin OC2A and OC2B (p.205)
  TCCR2A = (TCCR2A | _BV(COM2A1) | _BV(COM2B1)) & (~_BV(COM2A0) | ~_BV(COM2B0));
  // No prescaler (p.206)
  TCCR2B = (TCCR2B & ~(_BV(CS22) | _BV(CS21))) | _BV(CS20);

  // Configure the leds: RX PB0(8), TX PB1(9), CD PB2(10), OH PB4(12), RI PB5(13)
  DDRB  |= _BV(PORTB5) | _BV(PORTB4) | _BV(PORTB2) | _BV(PORTB1) | _BV(PORTB0);
  // Configure RTS/CTS
  DDRD  |=   _BV(PORTD7);  // CTS
  DDRD  &= ~(_BV(PORTD6)); // RTS
  PORTD |=   _BV(PORTD6) | _BV(PORTD7);
  // Configure DTR/DSR
  DDRD  |=   _BV(PORTD5);  // DSR
  DDRD  &= ~(_BV(PORTD4)); // DTR
  PORTD |=   _BV(PORTD4) | _BV(PORTD5);
  // Configure ring trigger (2)
  DDRD  &= ~(_BV(PORTD2));
  PORTD |=   _BV(PORTD2);

  // Set initial PWM to the first sample
  priDAC(wave.sample((uint8_t)0));
  secDAC(wave.sample((uint8_t)0));

  // Enable interrupts
  sei();
}

/**
  Test the leds

  @param onoff set the leds on or off
*/
void AFSK::setLeds(uint8_t onoff) {
  if (onoff)
    PORTB |=  (_BV(PORTB5) | _BV(PORTB4) | _BV(PORTB2) | _BV(PORTB1) | _BV(PORTB0));
  else
    PORTB &= ~(_BV(PORTB5) | _BV(PORTB4) | _BV(PORTB2) | _BV(PORTB1) | _BV(PORTB0));
}

/**
  Clear any ringing counters and signals.
*/
void AFSK::clearRing() {
  // Reset the signaling timeout
  outRingTimeout = 0;
  // Reset the counter
  cfg->sregs[1] = 0;
  // Stop sending signal to RI
  PORTB &= ~(_BV(PORTB5));
}

/**
  Handle both the TX and RX
*/
void AFSK::doTXRX() {
  //Disable interrupts
  cli();
  // Get the sample first
  rxSample = ADCH;
  if (this->onLine) {
    // Handle TX (constant delay)
    this->txHandle();
    // Handle RX
    this->rxHandle(rxSample);
  }
  // Handle the audio monitor
  this->spkHandle();
  // Enable interrupts
  sei();
}

/**
  Send the sample to the primary DAC

  @param sample the sample to output to DAC
*/
inline void AFSK::priDAC(uint8_t sample) {
  switch (selDAC) {
    case 1:
      OCR2B = sample;
      break;
    default:
      OCR2A = sample;
      break;
  }
}

/**
  Send the sample to the secondary DAC

  @param sample the sample to output to DAC
*/
inline void AFSK::secDAC(uint8_t sample) {
  switch (selDAC) {
    case 0:
      OCR2B = sample;
      break;
    default:
      OCR2A = sample;
      break;
  }
}


/**
  TX workhorse.  This function is called by ISR for each output sample.
  First, it gets the sample value and sends it to DAC, then prepares
  the next sample.
*/
void AFSK::txHandle() {
  // Check if we are transmitting
  if (tx.active == ON or tx.carrier == ON) {
    // First thing first: get the sample, index is Q8.8
    txSample = wave.sample(tx.idx);
    // Output the sample
    priDAC(txSample);
    // Step up the index for the next sample
    tx.idx += fsqTX->step[tx.dtbit];

    // Check if we have sent all samples for a bit
    if (tx.clk++ >= fulBit) {
      // Reset the samples counter
      tx.clk = 0;

      // One bit finished, choose the next bit and TX state
      switch (tx.state) {
        // Do nothing
        case NOP:
          break;

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
            // TX led on
            PORTB |= _BV(PORTB1);
          }
          else
            // No data in FIFO
            tx.state  = WAIT;
          break;

        // We are sending the preamble carrier
        case PREAMBLE:
          // Check if we have sent the carrier long enough
          if (++tx.bits >= carBits or tx.carrier == ON) {
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
          if (++tx.bits < cfgAFSK.dtbits) {
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
            PORTB &= ~(_BV(PORTB1));
          }
          else if (tx.bits == carBits and tx.carrier == OFF) {
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
  else if (this->isDialing) {
    // One time static
    static char dialChar = '\0';
    // Check each dial character
    if (dialChar == ',') {
      // Pause for S8 seconds
      if (_commaCnt++ >= _commaMax) {
        dialChar = '\0';
        _commaCnt = 0;
      }
    }
    else if (dtmf.getSample()) {
      // Get the DTMF sample and send to DAC
      txSample = dtmf.sample;
      priDAC(txSample);
    }
    else if (not txFIFO.empty()) {
      // Check the FIFO for dial numbers
      dialChar = txFIFO.out();
      if (dialChar != ',')
        dtmf.send(dialChar);
    }
    else
      // Stop dialing
      this->isDialing = OFF;
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
  // And the signed delayed sample
  int8_t ds = dyFIFO.out() - bias;

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

  // First order low-pass Chebyshev filter, 600Hz
  //  300:   0.16272643677832518 0.6745471264433496
  //  600:   0.28187392036298453 0.4362521592740309
  //  1200:  0.4470595850866754  0.10588082982664918

  rx.iirX[0] = rx.iirX[1];
  rx.iirX[1] = (ds * ss) >> 2;

  rx.iirY[0] = rx.iirY[1];
  rx.iirY[1] = rx.iirX[0] + rx.iirX[1] + (rx.iirY[0] >> 1);

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
    // Do nothing
    case NOP:
      break;

    // Detect the incoming carrier
    case CARRIER:
      // Check if the sample is valid
      if (bt != 0) {
        // Count the received samples
        if (++cdCount >= cdTotal) {
          // Reached the maximum, carrier is valid
          this->setRxCarrier(ON);
          // Reuse the counter as call timer
          cdCount = millis();
          // Wait for the first start bit
          rx.state = WAIT;
        }
      }
      else
        // Reset the counter
        cdCount = 0;
      break;

    // Carrier lost
    case NO_CARRIER:
      break;

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
      else if (rx.stream == 0xFF) {
        // Carrier only, update the carrier loss timeout
        cdTOut = millis() + cfg->sregs[10] * 100UL;
      }
      // Check for carrier timeout
      if (millis() > cdTOut) {
        // Report NO CARRIER if &C1, &L0 and timeout set
        if ((cfg->dcdopt != 0) and (cfg->sregs[10] != 0) and (cfg->lnetpe != 1)) {
          // Disable the CD flag and led
          this->setRxCarrier(OFF);
          // Stay in NO_CARRIER until SIO moves it to NOP
          rx.state = NO_CARRIER;
        }
      }
      break;

    // Validate the start bit after half the samples have been collected
    case PREAMBLE:
      // Check if we have collected enough samples
      if (rx.clk >= hlfBit) {
        // Check the average level of decoded samples: less than a quarter
        // of them may be HIGHs; the bitsum must be lesser than octBit
        if (rx.bitsum > octBit)
          // Too many HIGH, this is not a start bit
          rx.state  = WAIT;
        else
          // Could be a start bit, keep on going and check again at the end
          rx.state  = START_BIT;
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
            }
            else {
              // This is a start bit, go on to data bits
              rx.state  = DATA_BIT;
              rx.data   = 0;
              rx.clk    = 0;
              rx.bitsum = 0;
              rx.bits   = 0;
              // RX led on
              PORTB |= _BV(PORTB0);
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
            if (++rx.bits < cfgAFSK.dtbits) {
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
            PORTB &= ~(_BV(PORTB0));
            break;
        }
      }
  }
}

/**
  Check the serial I/O and transmit the data, respectively check
  the received data and send it to the serial port.
*/
uint8_t AFSK::doSIO() {
  // The charcter on the serial line
  uint8_t c;
  // The result (if unchanged, it's command mode, hayes will process)
  uint8_t result = 255;
  // The escape characters counters
  static uint8_t  escCount = 0;
  static uint32_t escFirst = 0;
  static uint32_t escLast  = 0;
  static uint32_t lstChar  = 0;
  static uint32_t now      = 0;
  // Characters waiting on the serial input
  bool inAvlb = (Serial.available() != 0);

  // The time
  now = millis();

  // Check the RING input, only when offline, 10 times per second
  if (this->onLine == OFF and this->opMode == COMMAND_MODE and
      (now >= inpRingTimeout or inpRingTimeout == 0)) {
    inpRingTimeout = now + 100UL;
    // Check if ringing
    if (not (PIND & _BV(PORTD2))) {
      // Ringing
      if (now > outRingTimeout or outRingTimeout == 0) {
        // Update the timeout
        outRingTimeout = now + 2000UL;
        // Send signal to RI
        PORTB |= _BV(PORTB5);
        // Increment the counter
        cfg->sregs[1] += 1;
        // RC_RING
        result = 2;
      }
    }
    else if (cfg->sregs[1] != 0) {
      // No ringing, but the counter is not zero, so it was
      // ringing and stopped; clear everything
      this->clearRing();
    }
    if (result != 255)
      return result;
  }

  // Check if we saw the escape string "+++"
  if (escCount == 3) {
    // We did, we did taw the escape string!
    // Check for the guard silence (S12)
    if (now - escLast > escGuard) {
      // This is it, go in command mode
      escCount = 0;
      this->setMode(COMMAND_MODE);
      // RC_OK
      result = 0;
    }
    else if (inAvlb) {
      // We just saw the full string (still in after guard time),
      // and there is something more on the line
      c = Serial.peek();
      if (c == '\r' or c == '\n')
        // Ignore CR and LF.
        Serial.read();
      else {
        // There is something else, transmit the escape string
        // to the other part and stay in data mode
        for (uint8_t i = escCount; i > 0; i--) {
          // Send the chars
          txFIFO.in(escChar);
          // Local datamode echo only on half duplex
          if (cfg->dtecho == OFF)
            Serial.write(escChar);
        }
        // Reset the counter and the first mark
        escCount = 0;
        escFirst = 0;
        lstChar  = now;
      }
    }
  }
  else if (escCount > 0) {
    // There were at most two escape chars, check if it took too long
    if (now - escFirst > escGuard) {
      // No more '+' chars and timed out
      for (uint8_t i = escCount; i > 0; i--) {
        // Send the chars
        txFIFO.in(escChar);
        // Local datamode echo only on half duplex
        if (cfg->dtecho == OFF)
          Serial.write(escChar);
      }
      // Reset the counter
      escCount = 0;
      escFirst = 0;
      lstChar  = now;
    }
  }

  // Check if there is any data on serial port
  if (inAvlb) {
    // Check for "+++" escape sequence (S2)
    if (Serial.peek() == escChar) {
      // Check when we saw the first '+' (S12)
      if (now - escFirst > escGuard) {
        // The first is older than the guard time, this may be a new first,
        // so check the before guard time too
        if (now - lstChar >= escGuard) {
          // This is the first, the last char was long ago, keep the time
          escCount = 1;
          escFirst = now;
          // Remove it from the buffer and make it unavailable
          Serial.read();
          inAvlb = false;
        }
      }
      else {
        // The last '+' was seen recently, count them up until three.
        // If this is the last, keep the time and wait for the guard silence
        if (++escCount == 3)
          escLast = now;
        // Remove it from the buffer and make it unavailable
        Serial.read();
        inAvlb = false;
      }
    }
  }

  // Check RX carrier
  if (rx.state == NO_CARRIER) {
    // The RX carrier has been lost, disable RX
    rx.state = NOP;
    // Go in command mode
    this->setMode(COMMAND_MODE);
    // RC_NO_CARRIER
    result = 3;
  }

  // Only in data mode
  if (this->opMode != COMMAND_MODE) {
    // Data mode, say to hayes we have processed the data
    result = 254;

    // Check DTR and act accordingly
    if (cfg->dtropt > 0) {
      // Check DTR
      if (not (PIND & _BV(PORTD4))) {
        // DTR low
        switch (cfg->dtropt) {
          case 1: // Return to command mode
            // Go in command mode
            this->setMode(COMMAND_MODE);
            // RC_OK
            result = 0;
            break;
          case 2: // Hang up, turn off auto answer, return to command mode
            // No auto-answer
            cfg->sregs[0] = 0;
            // Go offline and command mode
            this->setLine(OFF);
            // RC_NO_CARRIER
            result = 3;
            break;
          case 3: // Reset
            wdt_enable(WDTO_250MS);
            while (true) {};
            break;
        }
      }
    }

    // Flow control for outgoing (to DTE)
    switch (cfg->flwctr) {
      case FC_XONXOFF:
        // Check if the next character is a XON/XOFF flow control,
        // only if flow control is enabled
        c = Serial.peek();
        if (c == 0x13) {
          // XOFF
          Serial.read();
          outFlow = true;
        }
        else if (c == 0x10) {
          // XON
          Serial.read();
          outFlow = false;
        };
        break;
      case FC_RTSCTS:
        // RTS/CTS flow control
        outFlow = (cfg->rtsopt == 0) and (not (PIND & _BV(PORTD6)));
        break;
    }

    // Check if the FIFO
    if (txFIFO.len() < fifoHgh) {
      // The FIFO is not getting full, so check if we can take the byte
      if (inAvlb and (txFIFO.len() < fifoMed or (not inFlow))) {
        // There is data on serial port, process it normally
        c = Serial.read();
        if (txFIFO.in(c))
          // Local datamode echo only on half duplex
          if (cfg->dtecho == OFF)
            Serial.write((char)c);
        // Keep the time
        lstChar = now;
        // Keep transmitting
        tx.active = ON;
      }
    }
    else if (not inFlow and cfg->flwctr != FC_NONE) {
      // FIFO is getting full, check the flow control
      if (cfg->flwctr == FC_XONXOFF)
        // XON/XOFF flow control: XOFF
        Serial.write(0x13);
      else if (cfg->flwctr == FC_RTSCTS)
        // RTS/CTS flow control
        PORTD &= ~_BV(PORTD7);
      // Stop flow
      inFlow = true;
    }

    // Anytime, try to disable flow control, if we can
    if (inFlow and txFIFO.len() < fifoLow) {
      if (cfg->flwctr == FC_XONXOFF)
        // XON/XOFF flow control: XON
        Serial.write(0x11);
      else if (cfg->flwctr == FC_RTSCTS)
        // RTS/CTS flow control
        PORTD |= _BV(PORTD7);
      // Resume flow
      inFlow = false;
    }

    // Check if there is any data in RX FIFO
    if ((not rxFIFO.empty()) and (not outFlow)) {
      // Get the byte and send it to serial line
      c = rxFIFO.out();
      Serial.write(c);
    }
  }

  // Return the result (for hayes.doSIO, to print it)
  return result;
}

/**
  Handle the audio monitor (speaker)

  @param txSample the TX sample
  @param rxSample the RX sample
*/
void AFSK::spkHandle() {
  switch (cfg->spkmod) {
    case 1:
      secDAC(txSample >> (4 - cfg->spklvl));
      break;
    case 2:
      secDAC(rxSample >> (4 - cfg->spklvl));
      break;
    case 3:
      secDAC(txSample >> (4 - cfg->spklvl) +
             rxSample >> (4 - cfg->spklvl));
      break;
    default:
      break;
  }
}

/**
  Set the connection direction

  @param dir connection direction ORIGINATING, ANSWERING
*/
void AFSK::setDirection(uint8_t dir, uint8_t rev) {
  // Keep the direction
  direction = dir;
  // Stop the TX carrier
  this->setTxCarrier(OFF);
  // Create TX/RX pointers to ORIGINATING/ANSWERING parameters
  if ((direction == ORIGINATING and rev == OFF) or
      (direction == ANSWERING and cfg->revans == ON)) {
    fsqTX = &cfgAFSK.orig;
    fsqRX = &cfgAFSK.answ;
  }
  else {
    fsqTX = &cfgAFSK.answ;
    fsqRX = &cfgAFSK.orig;
  }
  // Clear the FIFOs
  rxFIFO.clear();
  txFIFO.clear();
  // Prepare the delay queue for RX
  dyFIFO.clear();
  for (uint8_t i = 0; i < fsqRX->queuelen; i++)
    dyFIFO.in(bias);
}

/**
  Set the online status

  @param online online status
*/
void AFSK::setLine(uint8_t online) {
  // Keep the online status
  onLine = online;

  if (online == OFF) {
    // OH led off
    PORTB &= ~_BV(PORTB4);
    // CD off
    this->setRxCarrier(OFF);
    // DSR always on if &S0, except when line is off
    if (cfg->dsropt == OFF)
      PORTD &= ~_BV(PORTD5);
    // Command mode
    this->setMode(COMMAND_MODE);
  }
  else {
    // Select the jack
    selDAC = cfg->jcksel;
    // OH led on
    PORTB |= _BV(PORTB4);
    // DSR always on if &S0
    if (cfg->dsropt == OFF)
      PORTD |= _BV(PORTD5);
  }
}

/**
  Get the online status

  @return online status
*/
bool AFSK::getLine() {
  return this->onLine;
}

/**
  Set the modem mode

  @param mode command mode or data mode
*/
void AFSK::setMode(uint8_t mode) {
  // Keep the mode
  this->opMode = mode;
}

/**
  Get the modem mode

  @return modem mode
*/
bool AFSK::getMode() {
  return this->opMode;
}

/**
  Enable or disable the TX carrier going at runtime

  @param onoff carrier mode
*/
void AFSK::setTxCarrier(uint8_t onoff) {
  tx.carrier = onoff & cfg->txcarr;
}

/**
  Signal the RX carrier detection

  @param onoff carrier mode
*/
void AFSK::setRxCarrier(uint8_t onoff) {
  rx.carrier = onoff;
  if (onoff == ON) {
    // CD led on
    PORTB |= _BV(PORTB2);
    // For now, DSR follows CD if &S1
    if (cfg->dsropt == ON)
      PORTD |= _BV(PORTD5);
  }
  else {
    // CD led off
    PORTB &= ~_BV(PORTB2);
    // For now, DSR follows CD if &S1
    if (cfg->dsropt == ON)
      PORTD &= ~_BV(PORTD5);
  }
}

/**
  Check the incoming carrier

  @return the carrier detection status
*/
bool AFSK::getRxCarrier() {
  // If the value specified in S7 is zero or &C0 or &L1,
  // don't wait for the carrier, report as found
  if ((cfg->sregs[7] == 0) or (cfg->dcdopt == 0) or (cfg->lnetpe == 1)) {
    // Don't detect the carrier, go directly to WAIT
    this->setRxCarrier(ON);
    rx.state = WAIT;
    // Reuse the carrier counter as call timer
    cdCount = millis();
  }
  else {
    // Use the decoder to check for carrier
    this->setRxCarrier(OFF);
    rx.state = CARRIER;
    cdCount = 0;
    // Check the carrier for at most S7 seconds
    cdTOut = millis() + cfg->sregs[7] * 1000UL;
    while (millis() <= cdTOut)
      // Stop checking if there is any char on serial
      // or the carrier has been detected
      if (Serial.available() or rx.carrier == ON)
        break;
    // No RX if carrier not detected
    if (not rx.carrier)
      rx.state = NOP;
  }
  // Return the carrier status (only true/false)
  return rx.carrier;
}

/**
  Dial a number

  @param phone the number to dial
  @return true if completed, false if interrupted
*/
bool AFSK::dial(char *phone) {
  bool result = true;
  // If leased line (&L1), do not dial
  if (cfg->lnetpe == 0) {
    // Disable the TX carrier
    this->setTxCarrier(OFF);
    // Sanitize S8 and set the comma delay value
    if (cfg->sregs[8] > 6)
      cfg->sregs[8] = 2;
    _commaMax = F_SAMPLE * cfg->sregs[8];
    _commaCnt = 0;
    // Clear TX FIFO for storing the dial number into
    txFIFO.clear();
    // Prepend and append comma-delays
    txFIFO.in(',');
    while (*phone != 0)
      txFIFO.in(*phone++);
    txFIFO.in(',');
    // Start dialing
    this->isDialing = ON;
    // Block until dialing is over
    while (this->isDialing == ON) {
      // Stop dialing if there is any char on serial
      if (Serial.available()) {
        this->isDialing = OFF;
        result = false;
      }
      // Busy delay
      delay(10);
    }
  }
  return result;
}

/**
  Get the call time

  @return call duration in seconds
*/
uint32_t AFSK::callTime() {
  uint32_t result = 0;
  if (cdCount != 0) {
    result = (millis() - cdCount) / 1000;
    cdCount = 0;
  }
  return result;
}

/**
  Test case simulation: feed the RX demodulator
*/
void AFSK::simFeed() {
  // Simulation
  static uint16_t idx = 0;
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
  //Serial.println(x);

  rxHandle(x);
  idx += fsqRX->step[bt];
  delay(100);
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
