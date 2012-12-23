/**
 * Copyright (C) 2012 University of Strathclyde
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "VN210RxTx_Arduino.h"
#include "Arduino.h"

//global instance of the VN210 rxtx layer. This is used
//by the API implementation, and can also be called by the
//user if they really really want.
VN210RxTx_Arduino VN210RxTx = VN210RxTx_Arduino();

/**
 * Initialises the wakeup, reset, provisioning and boot pins.
 *
 * This method must configure:
 *
 * - WKU, RESET, PROVISIONING and BOOT pins as outputs
 * - WKU as digital LOW
 * - RESET, PROVISIONING and BOOT as digital HIGH
 *
 * The BOOT pin should be set before the reset pin to ensure
 * that the correct firmware is loaded at boot time.
 */
void VN210RxTx_Arduino::initIO() {
	pinMode(9, OUTPUT);

	//ensure that ISA100.11a firmware is loaded at boot time.
	pinMode(BOOT_PIN, OUTPUT);
	digitalWrite(BOOT_PIN, BOOT_PIN_ISA100_FIRMWARE_BOOT);

	//set reset pin as output:high
	pinMode(RESET_PIN, OUTPUT);
	digitalWrite(RESET_PIN, HIGH);

	//set the provisioning pin as output:high
	pinMode(PROVISIONING_PIN, OUTPUT);
	digitalWrite(PROVISIONING_PIN, HIGH);
}

/**
 * Initialises the SPI bus as a slave.
 *
 * SimpleAPI Section 2.3.3 - Clock polarity - CPOL bit of SPCR = 0.  This differs from the SIMPLE API
 * 							 manual as the Arduino / Nivis definition is different. When the clock is idle,
 * 							 the voltage level is zero.  For Arduino, this is CPOL=0.
 * 			               - Clock phase = 0 - CPHA bit of SPCR = 0
 * 			               - MSB first - DORD = 0
 *
 * Clock speed is set by the radio. Don't need to worry about it on this side.
 *
 * For more information see Section 18.5 of the ATmega 328P manual and section 2.92 of
 * application note Arduino151 - SPI communication controlled by interrupts.
 *
 * Offloads this function to the spi_helper library, which also sets up the correct SPI interrupt.
 */
void VN210RxTx_Arduino::enable() {
	setup_spi(SPI_MODE_0, SPI_MSB, SPI_INTERRUPT, SPI_SLAVE);
}

/**
 * Performs a soft-reset of the VN210 radio.
 */
void VN210RxTx_Arduino::resetRadio() {
	digitalWrite(RESET_PIN, LOW);
	delay(2);
	digitalWrite(RESET_PIN, HIGH);
}

/**
 * Puts the radio into provisioning mode.
 *
 * This must pull the provisioning pin of the radio low for at least
 * 10 seconds, otherwise be pulled high.
 *
 * NOTE: This method has a 10 second delay which will temporarily halt the application!
 *
 * @deprecated Provisioning should be done via a button press only.  Calling this method
 * unconfigures the radio, so it shouldn't really be down to a method call.
 */
void VN210RxTx_Arduino::provisionRadio() {
	digitalWrite(PROVISIONING_PIN, LOW);
	delay(VN210_PROVISIONING_DURATION_MS);
	digitalWrite(PROVISIONING_PIN, HIGH);
}

/**
 * Wakes up the VN210 causing it to send a poll message to the application processor.
 *
 * See Section 2.3.1 of the VN210 Simple API documentation
 */
void VN210RxTx_Arduino::wakeupRadio() {
	digitalWrite(9, HIGH);

	delay(2);

	digitalWrite(9, LOW);
}

/**
 * Receives and transmits bytes on the SPI bus.
 *
 * As the host uC is configured as SPI slave, all transmits occur
 * at the whim of the SPI master device.   Therefore, when the master
 * sends a byte, the slave must send one in return.
 *
 * More info can be found in Atmel application note Arduino151.
 */
void VN210RxTx_Arduino::rxtx() {
	uint8_t rxb;

	//read and send the bytes on the SPI bus
	rxb = received_from_spi(txBuff.idx < txBuff.byteCount ? txBuff.bytes[txBuff.idx++] : 0x00);

	//if the entire message has been sent, reset the transmit buffer so that no more messages are sent
	if ((txBuff.idx > 0) && (txBuff.idx == txBuff.byteCount)) this->resetTransmitBuffer();

	this->receiveByte(rxb);
}

/**
 * AVR SPI interrupt routine.   Receive / transmit data by
 * calling the transport rxtx() method.
 *
 * NOTE: this may not work on the new Arduino Due boards - it may require
 * an architecture specific conditional compilation block.
 */
ISR(SPI_STC_vect) {
	VN210RxTx.rxtx();
}

