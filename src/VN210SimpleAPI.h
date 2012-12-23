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
#include "VN210RxTx.h"
#include <string.h>

#ifndef VN210SIMPLEAPI_H_
#define VN210SIMPLEAPI_H_

// Section 3.1.3.1 Message Header
// bits 7-4: message class
// bit  3  : request / response
// bits 2-0: reserved

// data buffer size: 4 analogs, 4 digitals and 8 IDs: (8 * 4) + 4 = 24;
#define UAP_ANALOGS_COUNT 4
#define UAP_DIGITALS_COUNT 4
#define UAP_ATTRIBUTES_COUNT (UAP_ANALOGS_COUNT + UAP_DIGITALS_COUNT)
#define UAP_ATTRIBUTE_SIZE_BYTES 4
#define UAP_ATTRIBUTES_BUFFER_SIZE (UAP_ATTRIBUTES_COUNT + (UAP_ATTRIBUTES_COUNT * UAP_ATTRIBUTE_SIZE_BYTES))

// common message header and payload macros
#define MSG_HEADER_API_REQUEST (MSG_TYPE_REQUEST | MSG_CLASS_API_COMMAND)
#define MSG_DATA_ZERO_VALUE 0
#define MSG_DATA_ZERO_BYTE_SIZE 0
#define MSG_DATA_ONE_BYTE_SIZE 1

/**
 * Implementation of the Nivis VN210 Simple API.  This class
 * is architecture-independent, and relies upon an architecture-specific
 * implementation of the transport layer for communication.
 *
 * To use this class, instantiate a suitable transport layer and pass
 * it in as an argument to the constructor of a new Simple API instance.
 *
 * @since 16 Feb 2012
 * $Date: 2012-06-22 09:39:05 +0100 (Fri, 22 Jun 2012) $
 * @author Pete Baker <peteb4ker@gmail.com>
 * $Rev: 5382 $
 * @copyright University of Strathclyde
 * @ingroup SimpleAPI
 * @ingroup Headers
 *
 * $Id: VN210SimpleAPI.h 5382 2012-06-22 08:39:05Z pbaker $
 */
class VN210SimpleAPI {
public:

	/**
	 * Enumeration of API message classes.
	 */
	enum MessageClass {
		DATA_PASS_THROUGH = 1,				//!< Data pass-through request / response.   Used for sending sensor data.
		//RESERVED = 2,
		//RESERVED = 3,
		API_COMMAND = 4,					//!< API command message class.  Corresponds to Simple API commands.
		ACK = 5,							//!< Acknowledgement message class.
		NACK = 6							//!< Non-acknowledge message class.
	};

	/**
	 * Enumeration of all read/write passthrough, acknowledgement and API message types.
	 * Used by the API to route messages to the appropriate local functions.
	 *
	 * Defined in Section 3.1.3.3 - Data-pass through command message types and
	 * Section 3.1.3.4 - API_COMMANDS message types
	 */
	enum MessageType {
		WRITE_DATA_REQUEST = 1,
		READ_DATA_REQUEST = 2,
		READ_DATA_RESPONSE = 3,

		ACK_DATA_RECEIVED = 1,
		ACK_SENT_VIA_RF = 2,
		ACK_API_CHANGE_OK = 3,
		ACK_FW_UPGRADE_OK = 4,

		API_HW_PLATFORM = 1,			//!< Reads API Hardware version from RF modem
		API_FW_VERSION = 2,				//!< Reads API Communication Protocol version from RF modem
		API_MAX_BUFFER = 3,				//!< Reads API buffer size from RF modem
		API_MAX_SPI_SPEED = 4,			//!< Reads max SPI speed from RF modem
		API_UPDATE_SPI_SPEED = 5,		//!< Sets SPI speed to RF modem
		API_MAX_UART_SPEED = 6,			//!< Reads max UART speed from RF modem
		API_UPDATE_UART_SPEED = 7,		//!< Sets UART speed to RF modem
		API_UPDATE_POLLING_FREQ = 8,	//!< Sets POLLING frequency of RF modem (for SPI only)
		API_POLLING = 9,				//!< Polling message (for SPI only)
		API_FW_ACTIVATION_REQ = 10,		//!< Notifies application processor about new firmware activation
	};

	/**
	 * VN210 polling frequency options, mapped to their corresponding code.
	 * These are defined in Section 3.1.3.4.8 - Polling frequency data values for SPI non-wakeup mode
	 */
	enum VN210_PollingFrequency {
		//1-3 not used
		Poll_500ms = 4,
		Poll_1s = 5,
		Poll_60s = 6
	};

	/**
	 * Enumeration of VN210 supported SPI bus speeds, mapped to their
	 * corresponding code. These are defined in Section 3.1.3.4.5 - SPI speed options
	 */
	enum VN210_SPISpeed {
		SPI_100KHz = 4,
		SPI_200KHz,
		SPI_250KHz,
		SPI_500KHz,
		SPI_1MHz,
		SPI_2MHz,
		SPI_MAX_SPEED = SPI_2MHz
	};

	/**
	 * UAP analog register structure.   Float values can be written
	 * to 'value' and read bytewise from 'bytes'
	 */
	typedef struct {
		union {
			uint8_t bytes[UAP_ATTRIBUTE_SIZE_BYTES];
			float value;
		};
	} Analog;

	/**
	 * Local copy of the UAP data object holding
	 * analog and digital data values.
	 */
	typedef struct {
		Analog analogs[UAP_ANALOGS_COUNT];	//!< 4 float "analog" registers
		bool digitals[UAP_DIGITALS_COUNT];	//!< 4 binary "digital" registers
	} LocalUAPData;

	/**
	 * Local data registers containing sensor information that will be read by the radio.
	 */
	LocalUAPData uapData;

	/**
	 * Local VN210 stack information which is written
	 * to by API call responses.
	 */
	typedef struct {
		bool crcValid;											//!< Flag indicating whether the last message had a valid CRC.  Writing to it has no effect.
		uint8_t hwPlatform;										//!< Radio hardware platform code.  Call getHardwarePlatform() once before reading this.  Writing to it has no effect.
		uint16_t maxBufferSize;									//!< The size of the radio's receive buffer. Call getMaxBufferSize() once before reading this. Writing to it has no effect.
		uint8_t maxSPISpeed;									//!< Code indicating the maximum supported speed of the radio.  Call getMaxSPISpeed() once before reading this.  Writing to it has no effect.
		uint16_t firmwareVersion;								//!< The firmware version code.  Top byte is major version, second byte is minor version.  Call getFirmwareVersion() once before reading this.  Writing to it has no effect.
	} VN210Properties;

	VN210Properties info;										//!< VN210 stack information.  These are not set until the corresponding API call has been made.

	VN210RxTx * dl;												//!< VN210 transport layer pointer.

	VN210_APIMessage txMessage;									//!< The message to be transmitted to the radio
	VN210_APIMessage rxMessage;									//!< The last received message from the radio

	VN210SimpleAPI(VN210RxTx * dl);

	void begin(bool wakeupSupportEnabled);						//API instantiation method. Resets the radio and sets up the API.

	//Application processor API commands
	void updatePollingFrequency(VN210_PollingFrequency freq);	//updates rate at which radio polls the application processor
	void updateSPISpeed(VN210_SPISpeed speed);					//updates the speed of the SPI bus
	void getHardwarePlatform(void);							 	//fetches the hardware platform code from the radio
	void getFirmwareVersion(void);								//fetches the firmware version from the radio
	void getMaxBufferSize(void);								//fetches the max buffer size from the radio
	void getMaxSPISpeed(void);									//fetches the max spi speed of the radio.

	//utility commands
	uint8_t getMessageClass(VN210_APIMessage * message);		//returns the message class from the header of the specified message.
	bool hasNewMessage(void);									//checks whether the radio has sent a message
	void handleMessage();										//handles messages - the highest level of the protocol
	bool receivedPollingMessage(void);							//Returns true if the most recent message was a polling message, false otherwise.
	void provisionRadio(void) __attribute__ ((deprecated));		//Puts the radio into provisioning mode. Deprecated.
private:

	/**
	 * Enumeration of header message classes.  Used with VN210HeaderRequestResponse to build message headers
	 */
	enum VN210HeaderMessageClass {
		MSG_CLASS_DATA_PASSTHROUGH = 0x10,		//see passthrough message types
		//RESERVED = 0x20,
		MSG_CLASS_API_COMMAND = 0x40,			//see Full API specification
		MSG_CLASS_ACK = 0x50,
		MSG_CLASS_NACK = 0x60
	};

	/**
	 * Enumeration of header message request / response bits. Used with VN210HeaderMessageClass to build message
	 * headers.
	 */
	enum VN210HeaderRequestResponse {
		MSG_TYPE_REQUEST = 0x00,		//bit 3, 0
		MSG_TYPE_RESPONSE = 0x08,		//bit 3, 1
	};

	uint8_t const zeroPayload;										//!< Often a zero-value 1 byte payload is required. This is it.

	uint8_t dataBuffer[UAP_ATTRIBUTES_BUFFER_SIZE];					//!< Buffer used to store data to be sent to the radio

	bool hasNewMessageFlag;											//!< Flag indicating whether we have a new message.

	//pass-through data commands
	void writeDataRequest(void);									//handles writing to the AP by the radio
	void readDataRequest(void);										//handles reading from the AP by the radio
	void readDataResponse(uint8_t attributeCount, uint8_t *dataBytes);	//sends attribute values to the radio

	//utility methods
	void send(uint8_t messageHeader, uint8_t type, uint8_t dataSize, uint8_t *data);
};

#endif /* VN210SIMPLEAPI_H_ */
