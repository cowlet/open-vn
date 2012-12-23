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

#include "VN210SimpleAPI.h"

/**
 * Class constructor.  Must be passed in a transport
 * layer instance.   Initialises the zero payload to zero.
 */
VN210SimpleAPI::VN210SimpleAPI(VN210RxTx * dl) : zeroPayload (MSG_DATA_ZERO_VALUE) {
	this->dl = dl;		//handle to the transport layer.
}

/**
 * SimpleAPI instantiation method.  starts
 * the transport layer, signalling the VN210.
 *
 * The VN210 seems to take about 5 before sending polling messages
 * after boot.
 */
void VN210SimpleAPI::begin(bool wakeupSupportEnabled) {
	this->dl->wakeupViaHWEnabled(wakeupSupportEnabled);

	//TODO can API_STX be defined statically in the message struct?
	txMessage.STX = API_STX;
	rxMessage.STX = API_STX;
	rxMessage.messageType = 0;

	//point the transport layer response to this response.
	this->dl->rxMessage = &this->rxMessage;

	//register new message flag with the transport layer.  this allows DL to tell the API when a new message is received.
	//this should give the DL a pointer to the API new message flag.
	this->dl->registerNewMessageFlag(& this->hasNewMessageFlag);

	//initialise the transport layer
	this->dl->begin();
}

/**
 * API command. Requests that the SPI bus is updated to the given speed.
 *
 * This method doesn't return anything as its effect is completely transparent to the microcontroller.
 */
void VN210SimpleAPI::updateSPISpeed(VN210_SPISpeed speed) {
	//cast speed pointer to a byte pointer - its just one byte.
	this->send(MSG_HEADER_API_REQUEST, API_UPDATE_SPI_SPEED, MSG_DATA_ONE_BYTE_SIZE, (uint8_t*) &speed);
}

/**
 * API command. Requests that the rate at which the radio polls the application processor is amended.
 *
 * Returns the response header, which will either signify an ACK or NACK
 */
void VN210SimpleAPI::updatePollingFrequency(VN210_PollingFrequency freq) {
	this->send(MSG_HEADER_API_REQUEST, API_UPDATE_POLLING_FREQ, MSG_DATA_ONE_BYTE_SIZE, (uint8_t*) &freq);
}

/**
 * API command.  Requests the maximum buffer size of the radio.
 */
void VN210SimpleAPI::getMaxBufferSize(void) {
	this->send(MSG_HEADER_API_REQUEST, API_MAX_BUFFER, MSG_DATA_ONE_BYTE_SIZE, (uint8_t*) &zeroPayload);
}

/**
 * API command. Requests the maximum supported SPI speed of the radio.
 */
void VN210SimpleAPI::getMaxSPISpeed(void) {
	this->send(MSG_HEADER_API_REQUEST, API_MAX_SPI_SPEED, MSG_DATA_ONE_BYTE_SIZE, (uint8_t*) &zeroPayload);
}

/**
 * API command.  Requests the hardware platform ID from the radio.
 */
void VN210SimpleAPI::getHardwarePlatform(void) {
	this->send(MSG_HEADER_API_REQUEST, API_HW_PLATFORM, MSG_DATA_ONE_BYTE_SIZE, (uint8_t*) &zeroPayload);
}

/**
 * API command.  Request the firmware version from the radio.
 *
 * This is returned as 2 bytes: MSB is the major version, LSB is the minor version.
 */
void VN210SimpleAPI::getFirmwareVersion(void) {
	return this->send(MSG_HEADER_API_REQUEST, API_FW_VERSION, MSG_DATA_ONE_BYTE_SIZE, (uint8_t*) &zeroPayload);
}

/**
 * Data pass-through method. Handles a write request from the radio, putting the
 * data into the local uapData store.
 */
void VN210SimpleAPI::writeDataRequest(void) {
	uint8_t volatile * ptr = rxMessage.data;
	uint8_t attributeID;

	for (int i = 0; i < rxMessage.dataSize / 5; i++) {
		attributeID = *ptr++;

		switch (attributeID) {			//get the attribute ID and increment the pointer
			case 1:
			case 2:
			case 3:
			case 4:
				for (int j = 3; j >=0; j--) {
					this->uapData.analogs[attributeID - 1].bytes[j] = *ptr++;
				}
				break;
			case 16:
			case 17:
			case 18:
			case 19:
				this->uapData.digitals[attributeID - 16] = ptr[3];
				ptr += 4;
				break;
		}
	}

	this->send(MSG_CLASS_ACK | MSG_TYPE_RESPONSE, ACK_DATA_RECEIVED, MSG_DATA_ZERO_BYTE_SIZE, NULL);
}

/**
 * Data pass-through method. Handles a read request from the radio for data,
 * preparing data to be sent via readDataResponse().
 */
void VN210SimpleAPI::readDataRequest(void) {
	uint8_t attributeID;
	uint8_t attributeCount = rxMessage.dataSize;
	uint8_t * buff = this->dataBuffer;		//get a pointer to the buffer to use for writing

	//get the value for each of the requested attributes
	for (int i = 0; i < attributeCount; i++) {
		attributeID = rxMessage.data[i];
		*buff = attributeID; 		//get the next attribute ID and put it into the data buffer
		buff++;

		switch (attributeID) {
			case 1:
			case 2:
			case 3:
			case 4:
				for (int j = 3; j >= 0; j--) {
					*buff = this->uapData.analogs[attributeID - 1].bytes[j];
					buff++;
				}
				break;
			case 16:
			case 17:
			case 18:
			case 19:
				for (int j = 0; j < 3; j++) *buff++ = 0;				//pad three zeros
				*buff = this->uapData.digitals[attributeID - 16];	//set the LSB
				buff++;
				break;
		}
	}

	//respond to read request
	this->readDataResponse(attributeCount, this->dataBuffer);
}

/**
 * Data Pass-through method.  Responds to a request for data attributes from the VN210.
 *
 * - attributeCount is the number of attributes to send
 * - attributes is an array of 4-byte attribute values, length (attributeCount * 4).
 */
void VN210SimpleAPI::readDataResponse(uint8_t attributeCount, uint8_t* attributes) {
	this->send(MSG_CLASS_DATA_PASSTHROUGH | MSG_TYPE_RESPONSE, READ_DATA_RESPONSE, attributeCount * 5, attributes);
}

/**
 * Sends a message to the VN210 radio.  Also handles message reception so after this returns
 * there should be a new response message.
 *
 */
void VN210SimpleAPI::send(uint8_t messageHeader, uint8_t type, uint8_t dataSize, uint8_t *data) {
	txMessage.header = messageHeader;
	txMessage.messageType = type;
	txMessage.messageID = rxMessage.messageID;		//reuse the message ID received from the radio
	txMessage.dataSize = dataSize;
	txMessage.data = data;

	this->dl->sendMsg(&txMessage);
}

/**
 * Checks to see whether there is a new message available from the radio
 * which is not a response to an API command.
 *
 * If there is a new message, it is parsed.  This decouples message parsing from
 * the low-level communications channel (which may be interrupt driven).
 */
bool VN210SimpleAPI::hasNewMessage() {
	bool hasNewMessage = this->hasNewMessageFlag;		//read the flag, its going to be reset when parsed

	if (hasNewMessage) {
		this->info.crcValid = this->dl->parseMessage();
	}

	return hasNewMessage;
}

/**
 * Handles all requests from the VN210 radio.
 */
void VN210SimpleAPI::handleMessage() {
	//handle messages
	switch (this->getMessageClass(&rxMessage)) {
		case DATA_PASS_THROUGH:
			switch (this->rxMessage.messageType) {
				case WRITE_DATA_REQUEST:
					this->writeDataRequest();
					break;
				case READ_DATA_REQUEST:
					this->readDataRequest();
			}
			break;
		case API_COMMAND:
			switch (this->rxMessage.messageType) {
				case API_HW_PLATFORM:					//got HW platform code - copy it to flags
					this->info.hwPlatform = this->rxMessage.data[1];
					break;
				case API_FW_VERSION:				//got API FW version. copy to flags.
					this->info.firmwareVersion = (this->rxMessage.data[0] << 8) | this->rxMessage.data[1];
					break;
				case API_MAX_BUFFER:
					this->info.maxBufferSize = (this->rxMessage.data[0] << 8) | this->rxMessage.data[1];
					break;
				case API_MAX_SPI_SPEED:
					this->info.maxSPISpeed = this->rxMessage.data[0];
					break;
				case API_POLLING:
					break;
				case API_FW_ACTIVATION_REQ: /* TODO: do something */ break;
			}
			break;
		case ACK:
			break; //TODO handle ACKs
		case NACK:
			break; //TODO handle NACKS
	}
}

/**
 * Returns the message class from the header of the specified message.
 */
uint8_t VN210SimpleAPI::getMessageClass(VN210_APIMessage * message) {
	return message->header >> 4;
}

/**
 * Returns true if the most recent message was a polling message, false otherwise.
 */
bool VN210SimpleAPI::receivedPollingMessage(void) {
	 return (getMessageClass(&rxMessage) == API_COMMAND) && (rxMessage.messageType == API_POLLING);
}

/**
 * Puts the radio into provisioning mode.
 *
 * This must pull the provisioning pin of the radio low for at least
 * 10 seconds, otherwise be pulled high.
 *
 * NOTE:
 *  - This method has a 10 second delay which will temporarily halt the application.
 *  - Not supported on VS210 boards - VN210 only.
 *
 * @deprecated Provisioning should be done via a button press only.  Calling this method
 * unconfigures the radio, so it shouldn't really be down to a method call.
 */
void VN210SimpleAPI::provisionRadio(void) {
	this->dl->provisionRadio();
}
