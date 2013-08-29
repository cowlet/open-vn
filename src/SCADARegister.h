/*
 * SCADARegister.h
 *
 * Created on: Jun 19, 2012
 * Author: Vic Catterson, Pete Baker
 */
#include "Arduino.h";

#ifndef SCADAREGISTER_H_
#define SCADAREGISTER_H_

#define SC_FLOAT_MAX_VALUE 32767
#define SC_FLOAT_MIN_VALUE -32766

/**
 * SCADA Register class.  Provides iterative
 * max, min and average calculations.
 */
class SCADARegister {
public:
	//sensor variables
	struct scada {
	  float average;
	  float minimum;
	  float maximum;
	  unsigned int total;
	} values;

	SCADARegister();
	void reset();
	void addValue(float num);
	void print();
};

#endif /* SCADAREGISTER_H_ */
