= LICENCE = 

These designs are under the Creative Commons Attribution Sharealike licence 3.0

- http://creativecommons.org/licenses/by-sa/2.0/

= OVERVIEW = 

* nivis-vn210.lbr: Eagle library components for the VN210
* vn210-breakout.*: Schematic and layout for the VN210 breakout.  THis is the small board that you solder the VN210 radio to.
* vn210-shield.*: Schematic and layout for the VN210 shield.  This connects the VN210 breakout to an arduino.

= INSTALLING =

1) Download and install eagle cadsoft.
2) Check out this folder into your eagle project folder.   
3) This requires components from the SparkFun eagle library.  Download it from:

-  https://github.com/sparkfun/SparkFun-Eagle-Library

= MAKING A NEW SHIELD DESIGN =

The shield connects the VN210 breakout to an Ardunio.  It is possible to implement new layout for the
same schematic to connect the breakout board to other microcontrollers should you need to.  The same
schematic can be used (for 5V microcontrollers anyway).  3V microcontrollers dont need the interfacing diodes.