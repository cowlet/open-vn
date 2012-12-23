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
 * Temperature sensor application for the VN210 using a MAX6675 
 * thermocouple amplifier.
 * 
 * Uses:
 *       TimedAction library: http://arduino.cc/playground/Code/TimedAction
 *       MAX6675 library: https://github.com/adafruit/MAX6675-library
 *       Nivis VN210 SimpleAPI implementation. 
 *
 * For more information on the VN210 SimpleAPI implementation, see "VN210_Example" script.
 *
 * @since 22 May 2012
 * $Date: 2012-06-22 09:39:05 +0100 (Fri, 22 Jun 2012) $
 * @author Pete Baker <peteb4ker@gmail.com>
 * $Rev: 5382 $
 * @copyright University of Strathclyde 
 * @ingroup Arduino
 * @ingroup Examples
 *
 * $Id: VN210_MAX6675.ino 5382 2012-06-22 08:39:05Z pbaker $
 */
#include "VN210SimpleAPI_Arduino.h"
#include "TimedAction.h"
#include "max6675.h"
#include "SCADARegister.h"

//MAX6675 pin assignments
const int thermoGND = A5;
const int thermoVCC = A4;
const int thermoDO = A3;
const int thermoCS = A2;
const int thermoCLK = A1;

float tempValue;

const int SAMPLE_PERIOD_MILLIS = 1000;
const int UPDATE_PERIOD_MILLIS = 60000;

//max6675 breakout board driver
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

//timed action workers
TimedAction sampleAction = TimedAction(SAMPLE_PERIOD_MILLIS, sample);
TimedAction scadaAction = TimedAction(UPDATE_PERIOD_MILLIS, update_scada_registers);

//SCADA teperature register
SCADARegister temperatureRegister;

/**
 * Script setup function.   Initialises the Serial console, prints
 * sketch instructions and initialises the VN210.
 *
 * Called once at startup.
 */
void setup() {
    //start the serial debug and VN210 sessions.
    Serial.begin(115200);
    Serial.println("Booting MAX6675 and VN210...");
    
    initialiseSensors();
    
    //start radio
    VN210.begin(false);      

    //wait for VN210 to boot
    delay(5000); 
    
    Serial.println("Done");
} 

/**
 * Initialise sensors
 */
void initialiseSensors() {
    //use GPIO pins for power
    pinMode(thermoGND, OUTPUT);
    pinMode(thermoVCC, OUTPUT);
    digitalWrite(thermoGND, LOW);
    digitalWrite(thermoVCC, HIGH);
}    

/**
 * Main program loop.
 */
void loop() {
    if (VN210.hasNewMessage()) {           //check if there is a new message
        VN210.handleMessage();             //if so, deal with it
    }
    
    sampleAction.check();                   //check whether its time to sense
    scadaAction.check();		   //check whether its time to update SCADA
}

/**
 * Samples the temperature and adds it to the local SCADA register
 */
void sample() {
   tempValue = thermocouple.readCelsius();
    
   temperatureRegister.addValue(tempValue);		//add a new value.
}


/**
 * Set VN210 registers with temperature data.  Called by the TimedAction worker.
 */
void update_scada_registers() {
    /* Node is provisioned to use Analogs 0-2 (attributes 1, 2, 3) */
    VN210.uapData.analogs[0].value = temperatureRegister.values.maximum;
    VN210.uapData.analogs[1].value = temperatureRegister.values.minimum;
    VN210.uapData.analogs[2].value = temperatureRegister.values.average;

    Serial.println("Updated UAP (SCADA) registers to:");
    temperatureRegister.print();
}
