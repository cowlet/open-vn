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

/**
 * Structs and enums related to the Nivis VN210 ISA100.11a radio
 *
 * @since 16 Feb 2012
 * $Date: 2012-06-22 09:39:05 +0100 (Fri, 22 Jun 2012) $
 * @author Pete Baker <peteb4ker@gmail.com>
 * $Rev: 5382 $
 * @copyright University of Strathclyde
 * @ingroup headers
 *
 * $Id: VN210.h 5382 2012-06-22 08:39:05Z pbaker $
 */

/**
 * @mainpage
 *
 * This library is an implementation of the Simple API for the Nivis VN210 and VS210 ISA100.11a industrial
 * wireless radios.
 *
 */
#include <stdint.h>

#ifndef VN210_H_
#define VN210_H_

//width of WKU pulse to signal VN210 radio
#define WKU_PULSE_WIDTH_MS 2

#define VN210_CRC_INITIAL_VALUE 0xFFFF
#define VN210_CRC_SIZE 2
#define VN210_PROVISIONING_DURATION_MS 11000  //!< Duration required to pull provisioning pin low to provision radio
#define BOOT_PIN_ISA100_FIRMWARE_BOOT 	1	  //!< Boot pin assignment for ISA100.11a firmware boot.

// Section 3.1.3.2 - Special characters
#define API_STX 0xF1
#define API_CHX 0xF2

/**
 * VN210 API message structure.  This is defined in both the Simple and Full APIs.
 */
typedef struct {
	uint8_t STX;				//!< Message start character.
	uint8_t header;				//!< Message header byte. Contains the message class and request / response flag.
	uint8_t messageType;		//!< Defines the type of message.  Specific to different message classes.
	uint8_t messageID;			//!< The ID of the current message.  Responses must match the request message ID.
	uint8_t dataSize;			//!< The number of data bytes contained within the message.
	uint8_t volatile * data;	//!< Pointer to the data payload.  This is of length "dataSize"
	union {
		uint16_t value;
		uint8_t bytes[VN210_CRC_SIZE];
	} crc;						//!< Cyclic redundancy check field.  Read / write to the .value field.
} VN210_APIMessage;

#endif /* VN210_H_ */
