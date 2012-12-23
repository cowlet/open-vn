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
 
/** Example script for the VN210 Simple API.  
 * 
 * This script starts a session with the VN210 allowing the user to
 * call  API methods, printing returned data to the serial console.
 *
 * ===INSTRUCTIONS===
 *
 * The API can be controlled and validated through the Arduino console 
 * using the following character commands:
 * 
 * [d] Toggle debug mode
 * [r] Reset VN210 hardware
 * [u] Update UAP values
 * [p] Provision VN210. WARNING - deconfigures radio!
 * [1] Get hardware platform info from VN210
 * [2] Get firmware version from VN210
 * [3] Get buffer length from VN210
 * [4] Get maximum SPI bus speed from VN210
 * [5] Set SPI bus speed to 1 MHz
 * [8] Set VN210 polling frequency to 60s
 *
 * Numbers 1-5 and 8 correspond to the Simple API commands in order.
 *
 * @since 22 May 2012
 * $Date: 2012-06-22 09:39:05 +0100 (Fri, 22 Jun 2012) $
 * @author Pete Baker <peteb4ker@gmail.com>
 * $Rev: 5382 $
 * @copyright University of Strathclyde 
 * @ingroup Arduino
 * @ingroup Examples
 *
 * $Id: VN210_Example.ino 5382 2012-06-22 08:39:05Z pbaker $
 */ 
#include "VN210SimpleAPI_Arduino.h"

boolean debug = false;                //!< Debug flag used to hide / show debug messages
const boolean wakeUpMode = false;     //!< Set wakeup mode of SimpleAPI

//used to calculate duration between polls
unsigned long current;
unsigned long previous;
unsigned long duration;

String na = "N/A";        //!< VN210 response code strings
String hwPlatforms[] = {"MSP430", "HSC08", "ARM7", "ARM9"}; //!< VN210 response code strings
String spiSpeeds[] = {na ,na ,na,"100k","200k", "250k", "500k", "1M", "2M"}; //!< VN210 response code strings

char * instructions = "\n== VN210 Simple API ==\n\n"
" The API can be controlled & validated via the console\n"
" using the following commands:\n\n"
"  [d] Toggle debug mode\n"
"  [i] Print these instructions\n"
"  [r] Reset VN210\n"
"  [u] Update UAP data\n"
"  [p] Provision VN210. WARNING - deconfigures radio!\n\n"
"  [1] Get hardware platform from VN210\n"
"  [2] Get firmware version\n"
"  [3] Get buffer length\n"
"  [4] Get maximum SPI speed\n"
"  [5] Set SPI speed to 1 MHz\n"
"  [8] Set VN210 polling to 60s\n\n"
" Numbers 1-5, 8 correspond to Simple API commands.\n"
" To run a command, hit a key followed by enter\n";   //!< Sketch user instructions.

/**
 * Script setup function.   Initialises the Serial console, prints
 * sketch instructions and initialises the VN210.
 *
 * Called once at startup.
 */
void setup() {
    //start the serial debug and VN210 sessions.
    Serial.begin(115200);

    printInstructions();
    VN210.begin(wakeUpMode);            // ---- VN210 API CALL ----
    
    previous = millis();
}

/**
 * Loop function.  Checks whether there is a new message from the VN210,
 * handling it when necessary.   Prints user messages depending on the
 * received (and sent) messages.
 * 
 * Also checks for user input, sending the appropriate message to the VN210
 * based upon the user choice.  See the script header for user instructions.
 */
void loop() {
    if (VN210.hasNewMessage()) {        // ---- VN210 API CALL ----
        current = millis();
        VN210.handleMessage();          // ---- VN210 API CALL ----
                
        duration = current - previous;
        previous = current;
        
        if (VN210.info.crcValid) {      // ---- VN210 API CALL ----
            //only print messages if debug is on, or if debug is off and its not a polling message
            if (debug || !VN210.receivedPollingMessage())       // ---- VN210 API CALL ----
                printMessageInfo();
        } else {
            Serial.println("CRC FAILED");
            printRxData();
        }
    }
    
    checkUserCommand();
}

/**
 * Prints instructions to the user.  Can be run by typing "i".
 */
void printInstructions() {
    Serial.println(instructions);
}

/**
 * Checks whether the user has send a command via the console.  
 * 
 * A status message is echoed to the console and the corresponding
 * Simple API command is called.
 */
void checkUserCommand() {
    if (Serial.available()) {
        Serial.print("User > ");
        
        switch(Serial.read()) {
            case 'd':                //toggle debug mode
                debug = !debug;
                Serial.print("DEBUG MODE: ");
                Serial.println((debug) ? "ON" : "OFF");
                break;
            case 'i':                //print instructions to the user
                printInstructions();
                break;
            case 'r':                //reset the VN210.
                Serial.println("Resetting..");
                VN210.begin(wakeUpMode);                    // ---- VN210 API CALL ----
                break;
            case 'u':                //update UAP values with mock data
                Serial.println("Updating UAP data values");    
                updateUAPValues();
                break;
            case 'p':
                Serial.println("Provisioning radio (takes 10s)");
                VN210.provisionRadio();
                Serial.println("Done");
                break;
            case '1':                //get the HW platform info
                Serial.println("HW platform");
                VN210.getHardwarePlatform();                 // ---- VN210 API CALL ----
                break;
            case '2':                //get the firmware version
                Serial.println("FW ver.");
                VN210.getFirmwareVersion();                  // ---- VN210 API CALL ----
                break;
            case '3':                //get the buffer length from the VN210
                Serial.println("Buffer len");
                VN210.getMaxBufferSize();                    // ---- VN210 API CALL ----
                break;
            case '4':                //get the max SPI speed from the VN210
                Serial.println("Max SPI");
                VN210.getMaxSPISpeed();                     // ---- VN210 API CALL ----
                break;
            case '5':                //set the SPI speed to 1MHz
                Serial.println("SPI speed->1MHz");
                VN210.updateSPISpeed(VN210.SPI_1MHz);        // ---- VN210 API CALL ----
                break;
            case '8':                //set the polling frequency to 60s
                Serial.println("Poll freq->60s");
                VN210.updatePollingFrequency(VN210.Poll_60s);    // ---- VN210 API CALL ----
                break;
            default:                //command unknown
                Serial.println("UNKNOWN");
        }
    }
}

/**
 * Prints the last received message to the console and duration between received 
 * messages.   Each message class has its own corresponding print message, one of 
 * which is called depending on the message class of the received message..
 */
void printMessageInfo() {
    Serial.print("[");
    Serial.print(duration);
    Serial.print(" ms] ");
    
    Serial.print("RX (");
    Serial.print(VN210.rxMessage.messageID, HEX);
    Serial.print(") -> ");
    
    switch(VN210.getMessageClass(&VN210.rxMessage)) {            // ---- VN210 API CALL ----
       case 1: 
           printPassThroughInfo();
           if (debug) printTxData();
           break;
       case 4: 
           printAPIInfo();
           break;
       case 5: 
           printACKInfo();
           break;
       case 6:
           printNACKInfo();
           printRxData();
           break;
       default: Serial.println("RESERVED");
    }
}

/**
 * Prints pass through message information.
 */
void printPassThroughInfo() {
    Serial.print("DATA_PASS-THROUGH: ");   
    
    switch(VN210.rxMessage.messageType) {            // ---- VN210 API CALL ----
        case 1: 
            Serial.println("Write req");
            break;
        case 2:
            Serial.print("Read req: ");
            
            for (int i = 0; i < VN210.rxMessage.dataSize; i++) {        // ---- VN210 API CALL ----
                Serial.print(VN210.rxMessage.data[i]);                // ---- VN210 API CALL ----
                Serial.print(" ");
            }
            
            Serial.println("");
            break;
    }
}

/**
 * Prints the transmitted message to the console.
 */
void printTxData() {
    Serial.print("TX (");
    Serial.print(VN210.txMessage.messageID, HEX);            // ---- VN210 API CALL ----
    Serial.print(") -> ");
    
    for (int i = 0; i < VN210.dl->txBuff.byteCount; i++) {   // ---- VN210 low-level API CALL ----
        Serial.print(VN210.dl->txBuff.bytes[i], HEX);
        Serial.print(" ");
    }
    
    Serial.println(" ");
}  

//note - this doesnt work as the rx buffer is reset before this is called.
void printRxData() {
    Serial.print("RX (");
    Serial.print(VN210.rxMessage.messageID, HEX);            // ---- VN210 API CALL ----
    Serial.print(") -> ");
    
    for (int i = 0; i < VN210.dl->rxBuff.byteCount; i++) {    // ---- VN210 low-level API CALL ----
        Serial.print(VN210.dl->rxBuff.bytes[i], HEX);
        Serial.print(" ");
    }
    
    Serial.println(" ");
}  

/**
 * Prints information pertaining to a received API message.
 */
void printAPIInfo() {
    Serial.print("API: ");
     
    switch(VN210.rxMessage.messageType) {
        case 1:
            Serial.print("Hardware version: ");
            Serial.println(hwPlatforms[VN210.info.hwPlatform]);        // ---- VN210 API CALL ----
            break;
        case 2: 
            Serial.print("Firmware version: ");
            Serial.println(VN210.info.firmwareVersion, HEX);            // ---- VN210 API CALL ----
            break;
        case 3: 
            Serial.print("Max buffer size: ");
            Serial.println(VN210.info.maxBufferSize);                    // ---- VN210 API CALL ----
            break;
        case 4:
            Serial.print("Max SPI speed: (");        
            //get SPI speed string from the array defined at the top of the sketch
            Serial.print(VN210.info.maxSPISpeed);
            Serial.print(") ");
            Serial.println(spiSpeeds[VN210.info.maxSPISpeed]);            // ---- VN210 API CALL ----
            break;
        //5 - update SPI speed receives ACK / NACK
        //6,7 UART Speed - not supported
        //8 - Polling speed: responded with ACK / NACK
        case 9: 
            Serial.println("Polling");            
            break;
        case 10:
            Serial.println("New firmware activated"); 
            break;
        default:
            Serial.println("Unknown");
    }
}

/**
 * Prints ACK response message information to the console.
 */
void printACKInfo() {
    Serial.print("ACK: "); 

    switch(VN210.rxMessage.messageType) {                    // ---- VN210 API CALL ----
        case 1: Serial.println("OK"); break;
        case 2: Serial.println("SENT"); break;
        case 3: Serial.println("UPDATED"); break;
        case 4: Serial.println("UPGRADED"); break;
    }
}

/**
 * Prints  NACK response information to the serial console.
 */
void printNACKInfo() {    
    Serial.print("NACK: "); 
    
    switch(VN210.rxMessage.messageType) {                    // ---- VN210 API CALL ----
        case 1: Serial.println("CRC Fail"); break;
        case 2: Serial.println("Data Overrun"); break;
        case 3: Serial.println("Packet incomplete"); break;
        case 4: Serial.println("Parity err"); break;
        case 5: Serial.println("API not init"); break;
        case 6: Serial.println("API cmd err"); break;
        case 7: Serial.println("API busy"); break;
        case 8: Serial.println("API err"); break;
        case 9: Serial.println("Stack err"); break;
        case 10: Serial.println("Unsupported"); break;    
        case 11: Serial.println("FW update err"); break;    
    }
}

/**
 * Updates the values of the User Application Process (UAP) values
 * register.  These are read periodically by the basestation through
 * the read / write through request functions.
 *
 * This method sets the analog values to the value of the millis()
 * command, cast to a float.   This is the number of milliseconds
 * since the Arduino reset.   Digital values are negated. 
 *
 * This demonstrates how to set the UAP values in a client application.
 */
void updateUAPValues() {
    Serial.print("UAP DATA - Analogs: [");
    for (int i = 0; i < 4; i++) {       
        
        //update the analog register with a new value
        VN210.uapData.analogs[i].value = (float) millis();        // ---- VN210 API CALL ----
        
        //print the new value to the console
        Serial.print(VN210.uapData.analogs[i].value);
        Serial.print((i < 3) ? ", " : "]");
    }
    
    Serial.print(" Digitals: [");
    for (int i = 0; i < 4; i++) {                
        
        //Update the digital register with a new value
        VN210.uapData.digitals[i] = ! VN210.uapData.digitals[i];    // ---- VN210 API CALL ----
        
        //print the new value to the console
        Serial.print(VN210.uapData.digitals[i], BIN);
        Serial.print((i < 3) ? ", " : "]\n");
    }
}
