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

#include "VN210.h"
#include <string.h>
#include <util/crc16.h>		//for data CRC
#include <util/delay.h>

#ifndef VN210RxTx_H_
#define VN210RxTx_H_

//VN210 frame related things.
#define VN210_BUFFER_SIZE 114	//matches max buffer size of VN210
#define VN210_DATASIZE_FRAME_FIELD_INDEX 4
#define VN210_FRAME_SIZE_MINUS_DATA 7

/**
 * Transport layer implementation for the SPI-based communication protocol
 * between a Nivis VN210 ISA100.11a radio and an microcontroller
 * processor (uC).
 *
 * This class is abstract. Its necessary to implement all the architecture-specific
 * bits in a separate class.  This is an attempt to make the implementation portable.
 *
 * @since 16 Feb 2012
 * $Date: 2012-06-22 09:39:05 +0100 (Fri, 22 Jun 2012) $
 * @author Pete Baker <peteb4ker@gmail.com>
 * $Rev: 5382 $
 * @copyright University of Strathclyde
 * @ingroup Lowlevel
 * @ingroup Headers
 *
 * $Id: VN210RxTx.h 5382 2012-06-22 08:39:05Z pbaker $
 */
class VN210RxTx {
public:
	void begin();													//!< Instantiates the library

	void sendMsg(VN210_APIMessage* msg);							//!< Sends a message to the VN210 radio
	bool parseMessage();											//!< Parses a message from the receive buffer
	void registerNewMessageFlag(bool * newMessageFlagPtr);			//!< Registers a flag to set in the API when a new message is available

	bool hasMessageToSend();										//!< Returns true if there is a message to send, false otherwise

	void wakeupViaHWEnabled(bool wakeupSupportEnabled);				//!< Sets flag indicating whether hardware wakeup is enabled in the radio firmware.

	/**
	 * Communications buffer implementation.  One of these
	 * is used for both receiving and transmitting data.
	 *
	 * NOTE: you may see a compiler error as no explicit volatile
	 * copy constructor is defined.  Try a different compiler as
	 * this is not required.
	 */
	typedef struct {
		uint8_t bytes[VN210_BUFFER_SIZE];							//!< Buffer byte array.
		uint8_t byteCount;											//!< The number of bytes in the buffer.
		uint8_t escape;												//!< Flag indicating whether the current byte is an escape (API_CHX) character.
		uint8_t idx;												//!< Iterator used for reading data back out of the buffer
	} Buffer;

	volatile Buffer rxBuff;										 	//!< Receive buffer
	volatile Buffer txBuff;											//!< Transmit buffer

	VN210_APIMessage * rxMessage;									//!< Pointer to the receive message

	//abstract method - architecture dependent

	/**
	 * Abstract method. Receives and transmits a byte on the SPI bus.
	 *
	 * This method should be called by a native interrupt, and
	 * use receiveByte(uint8) to read bytes into the receive buffer.
	 *
	 * This method must also reset the transmit buffer when a full message
	 * has been sent.
	 */
	virtual void rxtx(void) = 0;

	/**
	 * Performs a soft-reset of the radio by pulling the reset pin low for 2ms.
	 *
	 * Only supported on VN210 boards - not VS210.
	 */
	virtual void resetRadio() = 0;

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
	virtual void provisionRadio() __attribute__ ((deprecated)) = 0;
protected:
	void receiveByte(uint8_t);										//!< Deals with the received byte, putting it into the rx frame.
	void resetReceiveBuffer(void);									//!< Resets the receive buffer
	void resetTransmitBuffer(void);									//!< Resets the transmit buffer
	uint8_t addToTxBuffer(uint8_t b);								//!< Adds a byte to the transmit buffer, handling character escaping
private:
	bool wakeupSupportEnabled;										//!< Flag indicating whether to use wakeup support

	bool * hasNewMessageForAPI;										//!< Pointer to new message flag, used by Simple API.

	/**
	 * Abstract method.  Initialises the wakeup, reset, provisioning and boot pins.
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
	virtual void initIO() = 0;

	/**
	 * Initialises the SPI bus as a slave.
	 * Abstract method.
	 *
	 * This method should set the relevant
	 * registers or call native methods to correctly configure
	 * the SPI bus:
	 *
	 *  - SPI MODE0
	 *  - MSB FIRST
	 *  - INTERRUPT ON SS
	 *  - SPI SLAVE
	 */
	virtual void enable() = 0;

	/**
	 * Abstract method.  Wakes up the radio.
	 *
	 * This method must send a 1-2ms pulse on the WKU pin. This will
	 * wake the radio and cause a polling message to be sent.
	 */
	virtual void wakeupRadio() = 0;
};

#endif /* VN210RxTx_H_ */
