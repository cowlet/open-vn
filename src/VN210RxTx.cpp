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

#include "VN210RxTx.h"

/**
 * VN210RxTx instantiation method..
 *
 * Resets the radio and initialises the transport layer to allow message-based
 * communication with the VN210.
 */
void VN210RxTx::begin() {
	this->resetTransmitBuffer();
	this->resetReceiveBuffer();

	this->enable();
	this->initIO();

	this->resetRadio();
}

/**
 * Sends a message to the VN210.
 *
 * This only actually packs the message into the transmit buffer.  Messages
 * are sent from this buffer at the same time as the VN210 sends a message as
 * the VN210 is the SPI master, and therefore clocks both the MOSI and MISO
 * SPI lines.
 */
void VN210RxTx::sendMsg(VN210_APIMessage* msg) {
	this->resetTransmitBuffer();
	uint16_t crc = VN210_CRC_INITIAL_VALUE;

	//add next byte to the tx buffer and update CRC
	txBuff.bytes[txBuff.byteCount++] = msg->STX;
	crc = _crc_xmodem_update(crc, addToTxBuffer(msg->header));
	crc = _crc_xmodem_update(crc, addToTxBuffer(msg->messageType));
	crc = _crc_xmodem_update(crc, addToTxBuffer(msg->messageID));
	crc = _crc_xmodem_update(crc, addToTxBuffer(msg->dataSize));

	for (uint8_t i = 0; i < msg->dataSize; i++) {
		crc = _crc_xmodem_update(crc, addToTxBuffer(msg->data[i]));
	}

	msg->crc.value = crc;

	//copy the 2 CRC bytes into the buffer. MSB first.
	addToTxBuffer(msg->crc.bytes[1]);
	addToTxBuffer(msg->crc.bytes[0]);

	//if using wakeup mode, signal to the VN210 that we have a packet to send.
	this->wakeupRadio();
}

/**
 * Adds a byte to the transmit buffer, escaping it if necessary.  The unescaped byte
 * is returned as-is so it can be CRC'd.  NOTE: don't add the first STX character
 * using this method - it should not be escaped!
 */
uint8_t VN210RxTx::addToTxBuffer(uint8_t b) {
	if (b == API_STX) {
		txBuff.bytes[txBuff.byteCount++] = API_CHX;			//return the escaped char
		txBuff.bytes[txBuff.byteCount++] = 0x0E;				//add the escaped char
	} else if (b == API_CHX) {
		txBuff.bytes[txBuff.byteCount++] = API_CHX;			//return the escaped char
		txBuff.bytes[txBuff.byteCount++] = 0x0D;				//add the escaped char
	} else {
		txBuff.bytes[txBuff.byteCount++] = b;
	}

	return b;		//return b as is - this is just a trick to cut down on code in the sendMsg function
}

/**
 * Parses a message from the receive buffer, copying the buffer contents into
 * the response frame.
 */
bool VN210RxTx::parseMessage() {
	//pull the CRC out of the message. MSB first.
	rxMessage->crc.bytes[0] = rxBuff.bytes[rxBuff.byteCount - 1];
	rxMessage->crc.bytes[1] = rxBuff.bytes[rxBuff.byteCount - VN210_CRC_SIZE];

	//calculate the CRC from the payload
	uint16_t crc = VN210_CRC_INITIAL_VALUE;
	for (int i = 1; i < rxBuff.byteCount - 2; i++) {
		crc = _crc_xmodem_update(crc, rxBuff.bytes[i]);
	}

	bool crcIsValid = (rxMessage->crc.value == crc);

	if (crcIsValid) {
		rxBuff.idx = 0;		//reset the buffer index

		//iterate through the receive buffer byte by byte
		//See 3.1.3 for the API message format.

		rxMessage->STX = rxBuff.bytes[rxBuff.idx++];
		rxMessage->header = rxBuff.bytes[rxBuff.idx++];
		rxMessage->messageType = rxBuff.bytes[rxBuff.idx++];
		rxMessage->messageID = rxBuff.bytes[rxBuff.idx++];
		rxMessage->dataSize = rxBuff.bytes[rxBuff.idx++];
		rxMessage->data = &rxBuff.bytes[rxBuff.idx];		//response data points to the receive buffer
	}

	this->resetReceiveBuffer();

	return crcIsValid;
}

/**
 * Handles the received byte, putting it into the receive buffer and dealing with
 * escape characters.
 *
 * Parses the message when the entire frame has been received.
 *
 * NOTE: If the RF processor detects a valid incoming message in progress
 * (from the application processor), it will keep sending the STX character
 * until it receives the complete message.  [This keeps clocking the SPI bus].
 *
 * See 3.1.3.2 for more info.
 */
void VN210RxTx::receiveByte(uint8_t rxb) {
	if (rxb == API_CHX) {					//if we see the escape character
		rxBuff.escape = true;						//set the escape flag
	} else {
		//check for the start character to reset frame position. this aborts the packet.
		if (rxb == API_STX || rxBuff.byteCount == VN210_BUFFER_SIZE) { 	//also reset if the buffer is full.
			this->resetReceiveBuffer();
		}

		//not an escape char
		if (rxBuff.escape == true) {				//previous char was an escape
			rxBuff.escape = false;					//reset escape flag.

			//3.1.3.2 - special chars are ones-complemented if they immediately follow an escape
			if (rxb == 0x0E)						//if its 1s-complement of STX (0x0E), replace with STX
				rxb = API_STX;
			else if (rxb == 0x0D)					//if its 1s-complement of CHX (0x0D), replace with CHX
				rxb = API_CHX;
		}

		rxBuff.bytes[rxBuff.byteCount++] = rxb;			//write the byte to the receive buffer
	}

	//check whether we have an entire message yet.
	if (rxBuff.byteCount >= VN210_FRAME_SIZE_MINUS_DATA) {	//if we have enough bytes to constitute a message

		//check how many data bytes there are to find how big the frame should be
		if (rxBuff.byteCount == (rxBuff.bytes[VN210_DATASIZE_FRAME_FIELD_INDEX] + VN210_FRAME_SIZE_MINUS_DATA)) {

			//inform the API that we have a new message via flag.
			*this->hasNewMessageForAPI = true;
		}
	}
}

/**
 * Registers a 'new message flag' with the transport layer.  Once a new message has been received and
 * is available for processing, this flag will be set true.
 */
void VN210RxTx::registerNewMessageFlag(bool * newMessageFlagPtr) {
	this->hasNewMessageForAPI = newMessageFlagPtr;
}

/**
 * Checks whether there is a message currently queued to send.  This enalbles
 * the API to determine whether an ACK should immediately be sent or whether
 * an existing message will take its place, as per the spec
 */
bool VN210RxTx::hasMessageToSend(void) {
	return txBuff.byteCount > 0;
}

/**
 * Resets the receive buffer.
 */
void VN210RxTx::resetReceiveBuffer(void) {
	rxBuff.idx = 0;
	rxBuff.byteCount = 0;
	rxBuff.escape = false;

	*this->hasNewMessageForAPI = false;
}

/**
 * Resets the transmit buffer.
 */
void VN210RxTx::resetTransmitBuffer(void) {
	txBuff.idx = 0;
	txBuff.byteCount = 0;
	txBuff.escape = false;
}

/**
 * Sets the operating mode to either wakeup via HW support, or no wakeup support
 */
void VN210RxTx::wakeupViaHWEnabled(bool wakeupSupportEnabled) {
	this->wakeupSupportEnabled = wakeupSupportEnabled;
}
