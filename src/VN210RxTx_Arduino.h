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

#include "spi_helper.h"
#include "VN210RxTx.h"

#ifndef VN210RxTx_Arduino_H_
#define VN210RxTx_Arduino_H_

//IO pin definitions for WKU and RDY
#define WKU_PIN 			9
#define RDY_RADIO_PIN 		8
#define RESET_PIN			7
#define PROVISIONING_PIN	6
#define BOOT_PIN			5

/**
 * Arduino-specific implementation of the VN210 transport layer.
 *
 * @since 24 Feb 2012
 * $Date: 2012-06-26 14:03:55 +0100 (Tue, 26 Jun 2012) $
 * @author Pete Baker <peteb4ker@gmail.com>
 * $Rev: 5406 $
 * @copyright University of Strathclyde
 * @ingroup Headers
 * @ingroup Arduino
 * @ingroup Lowlevel
 *
 * $Id: VN210RxTx_Arduino.h 5406 2012-06-26 13:03:55Z pbaker $
 */
class VN210RxTx_Arduino : public VN210RxTx {
public:
	void rxtx(void);									//receives and transmits a byte on the SPI bus
private:
	void enable();										//initialises SPI bus as slave
	void initIO();										//initialises the WKU and RESET pins
	void wakeupRadio();									//pulses the WKU line to wake the radio and start communication.
	void resetRadio();									//resets the radio.
	void provisionRadio() __attribute__ ((deprecated));	//!< Puts the radio into provisioning mode.
};

extern VN210RxTx_Arduino VN210RxTx;

#endif /* VN210RxTx_Arduino_H_ */
