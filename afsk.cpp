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
*/

#include "afsk.h"

// The wave generator
WAVE wave;

// FIFOs
FIFO txFIFO(6);
FIFO rxFIFO(6);
FIFO delayFIFO(4);

AFSK::AFSK() {
}

AFSK::~AFSK() {
}

/**
  Initialize the AFSK modem

  @param x the afsk modem type
*/
void AFSK::init(AFSK_t afskMode, CFG_t cfg) {
  _afsk = afskMode;
  _cfg  = cfg;
  this->initSteps();

  cli();
  for (uint8_t i = 0; i < 8; i++)
    delayFIFO.in(128);
  sei();
  //delayFIFO.in(128);
}

/**
  Compute the originating and answering samples steps

  @param x the afsk modem to compute for
*/
void AFSK::initSteps() {
  for (uint8_t b = SPACE; b <= MARK; b++) {
    _afsk.stpOrig[b] = (wave.full * (uint32_t)_afsk.frqOrig[b] + F_SAMPLE / 2) / F_SAMPLE;
    _afsk.stpAnsw[b] = (wave.full * (uint32_t)_afsk.frqAnsw[b] + F_SAMPLE / 2) / F_SAMPLE;
  }
}

/**
  Send the sample to the DAC

  @param sample the sample to output to DAC
*/
inline void AFSK::DAC(uint8_t sample) {
#ifdef PWM_DAC
  // Use the 8-bit PWM DAC
#if PWM_PIN == 11
  OCR2A = sample;
#else
  OCR2B = sample;
#endif //PWM_PIN
#else
  // Use the 4-bit resistor ladder DAC
  PORTD = (sample & 0xF0) | _BV(3);
#endif //PWM_DAC
}

/**
  TX workhorse.  This function is called by ISR for each output sample.
  Immediately after starting, it gets the sample value and sends it to DAC.
*/
void AFSK::txHandle() {
  // First thing first: get the sample
  uint8_t sample = wave.sample(tx.idx);

  // Check if we are transmitting
  if (tx.active == 1 or _cfg.txCarrier == 1) {
    // Output the sample
    DAC(sample);
    // Step up the index for the next sample
    tx.idx += BELL103.stpOrig[tx.dtbit];

    // Check if we have sent all samples for a bit
    if (tx.clk++ >= BIT_SAMPLES) {
      // Reset the samples counter
      tx.clk  = 0;

      // One bit finished, check the state and choose the next bit and state
      switch (tx.state) {
        // We have been offline, prepare the transmission
        case WAIT:
          tx.dtbit  = MARK;
          // Get data from FIFO, if any
          if (not txFIFO.empty()) {
            tx.data   = txFIFO.out();
            tx.state  = PREAMBLE;
            tx.bits   = 0;
          }
          else
            tx.state  = WAIT;
          break;

        // We are sending the preamble carrier
        case PREAMBLE:
          // Check if we have sent the carrier long enough
          if (++tx.bits >= CARR_BITS) {
            // Carrier sent, go to the start bit
            tx.state  = START_BIT;
            tx.dtbit  = SPACE;
          }
          break;

        // We have sent the start bit, go on with data bits, LSB first
        case START_BIT:
          tx.state  = DATA_BIT;
          tx.dtbit  = tx.data & 0x01;
          tx.data   = tx.data >> 1;
          tx.bits   = 0;
          break;

        // We are sending the data bits, keep sending until MSB
        case DATA_BIT:
          // Check if we have sent all the bits
          if (++tx.bits < DATA_BITS) {
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
            // No more data to send, go on with the trail carrier
            tx.state  = TRAIL;
            tx.dtbit  = MARK;
            tx.bits   = 0;
          }
          else {
            // There is still data, get one byte and return to the start bit
            tx.state  = START_BIT;
            tx.dtbit  = SPACE;
            tx.data   = txFIFO.out();
          }
          break;

        // We are sending the trail carrier
        case TRAIL:
          // Check if we have sent the trail carrier long enough
          if (++tx.bits > CARR_BITS) {
            // Disable TX and go offline
            tx.active = 0;
            tx.state  = WAIT;
            // TX led off
            PORTB    &= ~_BV(PORTB1);
          }
          else if (tx.bits == CARR_BITS and _cfg.txCarrier != 1) {
            // After the last trail carrier bit, send isoelectric line
            tx.dtbit  = NONE;
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
}

/**
  RX workhorse.  This function is called by ISR for each input sample.
  It computes the power of the two RX signals, validates them and
  compared to each other to figure out the data bit.
  Then, it sends the data bit to decoder.

  @param sample the (signed) sample
*/
void AFSK::rxHandle(int8_t sample) {
#ifdef DEBUG_RX_LVL
  // Keep sample for level measurements
  if (sample < inMin) inMin = sample;
  if (sample > inMax) inMax = sample;
  // Count 256 samples, which means 8 bits
  if (++inSamples == 0x00) {
    // Get the level
    inLevel = inMax - inMin;
    // Reset MIN and MAX
    inMin = 0x7F;
    inMax = 0x80;
  }
#endif

  rx.iirX[0] = rx.iirX[1];
  rx.iirX[1] = ((delayFIFO.out() - 128) * sample) >> 2;
  //rx.iirX[1] = ((delayFIFO.out() - 128) * sample) >> 1;
  rx.iirY[0] = rx.iirY[1];
  rx.iirY[1] = rx.iirX[0] + rx.iirX[1] + (rx.iirY[0] >> 1);
  //rx.iirY[1] = rx.iirX[0] + rx.iirX[1] + (rx.iirY[0] / 10);
  delayFIFO.in(sample + 128);

  // Validate the RX tones
  rx.active = 1; //abs(rx.iirY[1] > 1);
  if (rx.active) {
    // Call the decoder
    rxDecoder(((rx.iirY[1] > 0) ? MARK : SPACE));
  }
  else {
    // Disable the decoder and clear all partially received data
    rx.state  = WAIT;
  }
}

/**
  The RX data decoder.  It gets the decoded data bit and tries to figure out
  the entire received byte.

  @param bt the decoded data bit
*/
void AFSK::rxDecoder(uint8_t bt) {
  static uint8_t rxLed = 0;

  // Keep the bitsum and bit stream
  rx.bitsum  += sample;
  rx.stream <<= 1;
  rx.stream  |= sample;

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
      if (rx.clk >= BIT_SAMPLES / 2) {
        // Check the average level of decoded samples: less than a quarter of
        // them may be HIGHs; the bitsum should be lesser than BIT_SAMPLES/8
        if (rx.bitsum > BIT_SAMPLES / 8) {
          // Too many HIGH, this is not a start bit, go offline
          rx.state  = WAIT;
          // RX led off
          PORTB  &= ~_BV(PORTB0);
        }
        else {
          // Could be a start bit, keep on going and check again at the end
          rx.state  = START_BIT;
          // RX led on
          PORTB  |= _BV(PORTB0);
        }
      }
      break;

    // Other states than WAIT and PREAMBLE
    default:
      // Check if we have received all the samples required for a bit
      if (rx.clk >= BIT_SAMPLES) {

        // Check the RX decoder state
        switch (rx.state) {
          // We have received the start bit
          case START_BIT:
#ifdef DEBUG_RX
            rxFIFO.in('S');
            //rxFIFO.in((rx.bitsum > BIT_SAMPLES / 2) ? '#' : '_');
            rxFIFO.in((rx.bitsum >> 2) + 'A');
#endif
            // Check the average level of decoded samples: less than a quarter
            // of them may be HIGH, so, the bitsum should be lesser than
            // BIT_SAMPLES/4
            if (rx.bitsum > BIT_SAMPLES / 4) {
              // Too many HIGHs, this is not a start bit
              rx.state  = WAIT;
              // RX led off
              PORTB  &= ~_BV(PORTB0);
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
            // Keep received bits, LSB first
            rx.data >>= 1;
            // The received data bit value is the average of all decoded
            // samples.  We count the HIGH samples, threshold at half
            rx.data  |= rx.bitsum > BIT_SAMPLES / 2 ? 0x80 : 0x00;
#ifdef DEBUG_RX
            rxFIFO.in(47 + rx.bits);
            //rxFIFO.in((rx.bitsum > BIT_SAMPLES / 2) ? '#' : '_');
            rxFIFO.in((rx.bitsum >> 2) + 'A');
#endif
            // Check if we are still receiving the data bits
            if (++rx.bits < DATA_BITS) {
              // Prepare for a new bit
              rx.clk    = 0;
              rx.bitsum = 0;
            }
            else {
              // Go on with the stop bit, count only half the samples
              rx.state  = STOP_BIT;
              rx.clk    = BIT_SAMPLES / 2;
              rx.bitsum = 0;
            }
            break;

          // We have received the stop bit
          case STOP_BIT:
            // Push the data into FIFO
#ifdef DEBUG_RX
            rxFIFO.in('T');
            rxFIFO.in((rx.bitsum >> 2) + 'A');
            rxFIFO.in(' ');
#endif
            if (rx.bitsum > BIT_SAMPLES / 4)
              rxFIFO.in(rx.data);
#ifdef DEBUG_RX
            rxFIFO.in(10);
#endif
            // Start over again
            rx.state  = WAIT;
            // RX led off
            PORTB  &= ~_BV(PORTB0);
            break;
        }
      }
  }
}

/**
  Check the serial I/O and send the data to TX, respectively check the
  RX data and send it to serial.
*/
void AFSK::serialHandle() {
  uint8_t c;
  // Check any data on serial port
  if (Serial.available() > 0) {
    // There is data on serial port
    if (not txFIFO.full()) {
      // FIFO not full, we can send the data
      c = Serial.read();
      txFIFO.in(c);
      // Keep transmitting
      tx.active = 1;
      // TX led on
      PORTB |= _BV(PORTB1);
    }
  }

  // Check if there is any data in RX FIFO
  if (not rxFIFO.empty()) {
    c = rxFIFO.out();
    Serial.write(c);
  }
}

/**
  Handle both the TX and RX
*/
void AFSK::handle() {
  this->txHandle();
  this->rxHandle(ADC - bias);
}
