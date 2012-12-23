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
 
/** VN210 Master poll simulator
 *
 * This application simulates a VN210 radio sending polling messages
 * to the Simple API implementation.  This allows observation of
 * received packets at the master side.
 * The Master Poller application generally shouldn't be run.  
 * It was used during API development to verify the API functions,
 * and is designed to be run on a second Arduino connected to an 
 * Arduino running the Simple API.  It implements the message
 * examples listed in the Simple API specification, effectively 
 * running a set of unit tests comparing expected and actual 
 * received bytes from the Simple API application processor node.
 *
 * The script acts as a set of unit tests.  If all you see is dots, it works as expected.
 *
 * @since 22 May 2012
 * $Date: 2012-06-22 09:39:05 +0100 (Fri, 22 Jun 2012) $
 * @author Pete Baker <peteb4ker@gmail.com>
 * $Rev: 5382 $
 * @copyright University of Strathclyde 
 * @ingroup Arduino
 * @ingroup Examples
 *
 * $Id: VN210_MasterPoller.ino 5382 2012-06-22 08:39:05Z pbaker $
 */
#include <SPI.h>
#include <util/crc16.h>

#define ACK_HEADER 0x58
#define WRITE_MSG_ID 0x03
#define READ_MSG_ID 0x04
#define DATA_RECEIVED_OK 0x01
#define DATA_PASSTHROUGH_RESPONSE 0x18
#define READ_DATA_RESPONSE 0x03
#define BYTE_DELAY_MICROS 40 //measured experimentally using scope on VN210 clock line

  union {
    uint16_t value;
    uint8_t bytes[2];
  } actual;

byte receiveBuffer[64];
int rxIndex = 0;		//receive index

//byte string for a "poll message".  Its padded by escape characters to make sure all messages can be received from the AP.
byte pollMessage[] = {0xF1, 0x48, 0x09, 0x00, 0x00, 0xF2, 0x0E, 0xE,0xF1,0xF1,0xF1,0xF1,0xF1,0xF1,0xF1,0xF1,0xF1,0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1,0xF1, 0xF1, 0xF1, 0xF1,0xF1, 0xF1, 0xF1, 0xF1,0xF1, 0xF1, 0xF1, 0xF1,0xF1, 0xF1, 0xF1, 0xF1,0xF1, 0xF1, 0xF1, 0xF1,0xF1, 0xF1, 0xF1, 0xF1,0xF1, 0xF1, 0xF1, 0xF1,0xF1, 0xF1, 0xF1, 0xF1};

//byte string to write analog data to the AP 
byte writeAnalogMessage[] = {0xF1, 0x10, 0x01, WRITE_MSG_ID, 0x05, 0x01, 0xF2, 0x0D, 0x34, 0x56, 0xF2, 0x0E, 0x0D, 0xEA};    //fix last 2 crc bytes

//byte string to seed analog data to the AP which is read by the "read analog" message
byte seedData[] = {0xF1, 0x10, 0x01, WRITE_MSG_ID, 0x05, 0x01, 0x40, 0xF2, 0x0E, 0xF2, 0x0D, 0x13, 0xD0, 0x82};

//byte string for a message to read specific analog data from the AP
byte readAnalogMessage[] = {0xF1, 0x10, 0x02, READ_MSG_ID, 0x01, 0x01, 0x07, 0xEE};

//byte string to read all the data from the AP.
byte readAllDataMessage[] = {0xF1, 0x10, 0x02, READ_MSG_ID, 0x08, 0x01, 0x02, 0x03, 0x04, 0x10, 0x11, 0x12, 0x13, 0xDF, 0xB1};

//length of the poll message.
int pollBytes = 64;

//crc result, used for comparison.
uint32_t crcResult;

//slave select pin number.
const int slaveSelectPin = 10;

//setup method.  called once.
void setup() {
    //set up serial console
    Serial.begin(115200);
    
    //set up SPI comms
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
    SPI.setClockDivider(SPI_CLOCK_DIV128);
    SPI.begin();
    
    pinMode(slaveSelectPin, OUTPUT);
    
    Serial.println("Running unit tests 5 times at 125 kHz");
    runTests5Times();    //at 125kHz SPI CLK (default is 100 but arduino clock divider doesnt do this
    SPI.end();

    Serial.println("\nRunning unit tests 5 times at 2 MHz");
    SPI.setClockDivider(SPI_CLOCK_DIV8);
    SPI.begin();
    runTests5Times();    //at 2MHz SPI CLK
    
    Serial.println("");
}

//runs all the tests 5 times.  once isnt enough.
void runTests5Times() {
        for (int i = 0; i < 5; i++) {
    testWriteAnalogData();
    testReadAnalogData();
    testReadAllData();
    //printMessage();
    }
}

//loop method.  called forever. does nothing.
void loop() {


 
}

//asserts a test condition, printing a message
void assert(String msg, boolean condition) {
  if (condition) {
    Serial.print(".");
  } else {
    Serial.print("\n"+msg+" FAILED\n");
    printMessage();
  }    
}

/**
 * Tests writing data to the application processor.
 *
 * This test first writes, then checks that a valid ACK
 * is sent back from the AP.
 */
void testWriteAnalogData() {    
    //clear buffer
    clearBuffer();
    
    //write data to the analog proce
    sendMessage(writeAnalogMessage, 14);
    delay(500);    //standard wait time
    
    //get the response
    poll();
    crc();
    
    //here you can see that individual bytes are taken out of the receive buffer and compared to the
    //expected value.  this is how this test application works.
    // 
    // NOTE: the first byte in the buffer is junk - start at index 1.
    assert("Write analog received ACK", receiveBuffer[2] == ACK_HEADER);
    assert("Data received ok", receiveBuffer[3] == DATA_RECEIVED_OK);
    assert("Write analog correct message ID", receiveBuffer[4] == WRITE_MSG_ID);
}

//writes analog data to the application processor
void testReadAnalogData() {
    //clear buffer
    clearBuffer();
  
    //seed data to read
    sendMessage(seedData, 14);
    delay(500);    //standard wait time
    poll();
    crc();
    
    assert("Seed analog received ACK", receiveBuffer[2] == ACK_HEADER);
    assert("Data received ok", receiveBuffer[3] == DATA_RECEIVED_OK);
    assert("write analog correct message ID", receiveBuffer[4] == WRITE_MSG_ID);
    
    //request analog value
    sendMessage(readAnalogMessage, 8);
    delay(500);    //standard wait time
    poll();
    crc();
    
    assert("Read analog correct header", receiveBuffer[2] == DATA_PASSTHROUGH_RESPONSE);
    assert("Read analog correct msg type", receiveBuffer[3] ==  READ_DATA_RESPONSE);
    assert("Read analog correct message ID", receiveBuffer[4] == READ_MSG_ID);
    assert("Read analog data size", receiveBuffer[5] == 5);
}

//tests that all the data in the AP registers can be read correctly.
void testReadAllData() {
  clearBuffer();
 
  sendMessage(readAllDataMessage, 15); 
  delay(500);    //standard wait time
  poll();
  crc();
}

/**
 * Polls the Simple API implementation, receiving a message
 */
void poll() {
    sendMessage(pollMessage, pollBytes);
}

//writes a message to the SPI bus, synchronously reading a message into the receive buffer.
void sendMessage(byte* buff, int length) {
    digitalWrite(slaveSelectPin, LOW); 			//chip select low
    boolean escape = false;
    byte rxb;
    rxIndex = 0;
    
    for (int i = 0; i < length; i++) {
        rxb = SPI.transfer(buff[i]);

		if (escape == true) {					//if previous character was escape
			escape = false;
			if (rxb == 0x0E)					//if its 1s-complement of STX (0x0E), replace with STX
				receiveBuffer[rxIndex++] = 0xF1;
			else if (rxb == 0x0D)					//if its 1s-complement of CHX (0x0D), replace with CHX
				receiveBuffer[rxIndex++] = 0xF2;
		} else {
			if (rxb == 0xF2) {					//if this char is escape
				escape = true;					//set the escape flag
			} else {							//otherwise 
				receiveBuffer[rxIndex++] = rxb;	//receive the byte
			}
		}

		delayMicroseconds(BYTE_DELAY_MICROS);								//wait 20us between bytes. it probably doesnt need to be this long
	}     
   
    digitalWrite(slaveSelectPin, HIGH);			//chip select high
} 

//clears the buffer of incoming messages
void clearBuffer() {
    for (int i = 0; i < 64; i++) {
		receiveBuffer[i] = 0;
	}
}

//calculates the CRC of the receive message, comparing it to the expected value
void crc() {
  int crcByteCount = 4 + receiveBuffer[5];		//receivebuffer[5] has the message length
  
  uint8_t * ptr = receiveBuffer + 2;			//start at index 2
  
  uint16_t expected = 0xFFFF;
  
  for (int i = 0; i < crcByteCount; i++) {
	expected = _crc_xmodem_update(expected, *ptr);
	ptr++; 
  }
  
  //get the bytes and put them in the actual crc byte array
  actual.bytes[1] = *ptr; ptr++;
  actual.bytes[0] = *ptr;

  assert("CRC", expected == actual.value);		//compare the actual value to the expected
}

/**
 * Prints the receive buffer to the serial console.
 */
void printMessage() {
    Serial.print("RX > ");
    
    //first byte is junk
    for (int i = 0; i < pollBytes; i++) {
        Serial.print(receiveBuffer[i], HEX);
        Serial.print(" ");
    }
    Serial.println(" ");
}
