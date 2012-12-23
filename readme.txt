== OVERVIEW ==

This folder contains an eclipse C++ project of a driver and API implementation for the NIVIS VN210 ISA100.11a
radio.  Currently the implementation is focused towards Arduino devices, but can support other devices too as the core functionality
is abstracted.

AVR SPI support is supplied by the "SPI Helper" library under the MIT licence:
 - Project homepage: http://code.google.com/p/rocketnumbernine/

== PROJECT FILES ==

 * examples/VN210_Example/VN210_Example.ino				Example Arduino script.  Provides access to all API functions.
 * examples/VN210_MAX6675/VN210_MAX6675.ino				Sensor application example based on a MAX6675 thermocouple amplifier.
 
 * VN210.h												Base project header file with VN210-specific structs.
 * VN210RxTx_Arduino.cpp								Arduino-specific implementation of the VN210 transport layer
 * VN210RxTx_Arduino.h									Arduino-specific header for the VN210 transport layer
 * VN210RxTx.cpp										Abstract implementation of the VN210 transport layer. 
 														Contains everything apart from architecture-specific stuff.
 * VN210RxTx.h											Abstract declaration of the VN210 transport layer.
 * VN210SimpleAPI_Arduino.h								Arduino architecture SimpleAPI wrapper.

 * spi_hepler.c											AVR SPI Helper library source
 * spi_helper.h											AVR SPI Helper library header

== Using the library ==

The project can also be used with Arduino.  Check out the project and symlink the "src" directory to
your <Arduino/libraries> folder, for instance, in OSX:

 # ln -s src ~/Documents/Arduino/libraries/VN210SimpleAPI
 
After you restart the Arduino app, the library and example script will be available for use.

== Development ==

To extend the API, you may need to set up your eclipse (or other) environment for AVR-GCC support.


[ Copyright (C) 2012 University of Strathclyde ]

