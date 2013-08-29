/*
 * SCADARegister.cpp
 *
 * Created on: Jun 19, 2012
 * Author: Vic Catterson, Pete Baker
 */

#include "SCADARegister.h"

/**
 * Initialises the scada register, resetting the values to defaults.
 */
SCADARegister::SCADARegister() {
	this->reset();
}

/**
 * Resets the scada register values
 */
void SCADARegister::reset() {
	values.total = 0;
	values.average = 0;
	values.maximum = SC_FLOAT_MIN_VALUE; 	
    values.minimum = SC_FLOAT_MAX_VALUE;
}

/**
 * Adds a value to the SCADA register, updating
 * the max, min and average values.
 */
void SCADARegister::addValue(float num) {
  if (num > values.maximum)
    values.maximum = num;

  if (num < values.minimum)
    values.minimum = num;

  float sum = values.average * values.total; // sum of values so far
  values.average = (sum + num)/++values.total; // calc new average AFTER incrementing total
}

/**
 * Prints the SCADA register values to the console.  Requires a working Serial instance!
 */
void SCADARegister::print() {
  Serial.print("Max: ");
  Serial.print(values.maximum);
  Serial.print(", min: ");
  Serial.print(values.minimum);
  Serial.print(", ave: ");
  Serial.print(values.average);
  Serial.print(", total: ");
  Serial.println(values.total, DEC);
}
