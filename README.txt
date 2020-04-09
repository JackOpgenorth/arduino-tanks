Names: Jack Opgenorth, Noah Batiuk
ID: 1580266, 1579662
CMPUT 275 Wi20

Final project: Arduino tanks

Included files:
	arduino/ containing:
		main.cpp
		bull_tank.cpp
		bull_tank.h
		Makefile
	Desktop/ containing:
		desktop.cpp
		serialport.cpp
		serialport.h
		Makefile
	README

Desktop Makefile Targets:
	- make desktop: Links all of the object files into a executable call "desktop"
	- make desktop.o: Compiles the object file for desktop.cpp
	- make serialport.o: Compiles the object file for serialport.cpp
	- make clean: Removes object files and executables
	
Client is just the unmodified arduino makefile that was used in class

Wiring Instructions
	Joystick:
		VRx <--> A8
		VRy <--> A9
		SW  <--> Digital Pin 53
		GND <--> GND
		+5V <--> 5V

	
Running Instructions:
	1. Have one terminal open in the desktop directory and one open in the arduino directory
	2. Compile the desktop executable by typing "make" into the desktop terminal
	3. Compile and upload the arduino program by typing "make upload" in the arduino terminal (Make sure this is done BEFORE the desktop executable is run)
	4. Run the desktop executable by typing "./desktop" in the desktop terminal
	5. Begin inputting by moving the joystick and tapping the screen. To make a tank shoot, tap the opposite side of the screen from where it started

Notes and Assumptions:
	Due to the pandemic, we were unable to meet up and could not implement
'	the arduino-arduino communication aspect of our project. This was replaced with
	desktop-arduino communication

	To make it so we could have keyboard input without pressing enter, some code was taken off the internet
	since this was much more complicated than we thought it would be. The source is cited here:
	https://stackoverflow.com/questions/18281412/check-keypress-in-c-on-linux
	and also in desktop.cpp, where it is used

	As mention in the Instructions, a tank shoots only if you tap the opposite side of the screen from where it started.




	

