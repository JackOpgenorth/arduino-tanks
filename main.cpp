#include <Arduino.h>

// core graphics library (written by Adafruit)
#include <Adafruit_GFX.h>

// Hardware-specific graphics library for MCU Friend 3.5" TFT LCD shield
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <SD.h>

#define SD_CS 10
#define JOY_VERT  A9 // should connect A9 to pin VRx
#define JOY_HORIZ A8 // should connect A8 to pin VRy
#define JOY_SEL   53



#define JOY_CENTER   512
#define JOY_DEADZONE 64

#define CURSOR_SIZE 9

#define DISPLAY_WIDTH  480
#define DISPLAY_HEIGHT 320



#define YP A3 // must be an analog pin, use "An" notation!
#define XM A2 // must be an analog pin, use "An" notation!
#define YM 9  // can be a digital pin
#define XP 8  // can be a digital pin





MCUFRIEND_kbv tft;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

/*void redrawCursor(uint16_t colour, int x, int y);*/


struct tank
{

	int x, y;


	tank(int inputx, int inputy){
		x = inputx;
		y = inputy;
	}




	void redrawCursor(uint16_t colour, int &x, int &y) {
  		tft.fillRect(x - CURSOR_SIZE/2, y - CURSOR_SIZE/2,
        CURSOR_SIZE, CURSOR_SIZE, colour);
	}

	void update(){
		//obtaining the velocity by scaling down the values obtained from the x and y pins
		//so we won't go too fast, then subtracting so that while the joystick is
		//centered we will always get a velocity of 0.
		int velX = abs(0.002 * analogRead(JOY_HORIZ) - 10);
		int velY = abs(0.002 * analogRead(JOY_VERT) - 10);
		Serial.println(velX);
		// so we only redraw the screen when we are moving
		bool moving = false;

		int xVal = analogRead(JOY_HORIZ);
		int yVal = analogRead(JOY_VERT);
		int buttonVal = digitalRead(JOY_SEL);

		// for redrawing later
		int oldY = y;
		int oldX = x;

		// now move the cursor
		if (yVal < JOY_CENTER - JOY_DEADZONE) {
		y -= (1 + velY - 9); // decrease the y coordinate of the cursor
		moving = true;
		}
		else if (yVal > JOY_CENTER + JOY_DEADZONE) {
		y += 1 + velY - 6;
		moving = true;
		}

		// remember the x-reading increases as we push left
		if (xVal > JOY_CENTER + JOY_DEADZONE) {
		x -= (1 + velX - 6);
		moving = true;
		}
		else if (xVal < JOY_CENTER - JOY_DEADZONE) {
		x += 1 + velX - 9;
		moving = true;
		}


		// check if at edge, if so, redraw next section

		// draw a small patch of the Edmonton map at the old cursor position before
		// drawing a red rectangle at the new cursor position

		// redrawing patch at old position.
		if (moving){


			tft.fillRect(oldX - CURSOR_SIZE/2, oldY - CURSOR_SIZE/2,
			CURSOR_SIZE, CURSOR_SIZE, TFT_BLACK);

			redrawCursor(TFT_RED, x, y);
		}

		delay(20);

		}


};



tank thisTank(50, 50);


void setup(){
	init();

	Serial.begin(9600);

	pinMode(JOY_SEL, INPUT_PULLUP);

	//    tft.reset();             // hardware reset
	uint16_t ID = tft.readID();    // read ID from display
	Serial.print("ID = 0x");
	Serial.println(ID, HEX);
	if (ID == 0xD3D3) ID = 0x9481; // write-only shield

	// must come before SD.begin() ...
	tft.begin(ID);                 // LCD gets ready to work

	Serial.print("Initializing SD card...");
	if (!SD.begin(SD_CS)) {
		Serial.println("failed! Is it inserted properly?");
		while (true) {}
	}
	Serial.println("OK!");

	tft.setRotation(1);

	tft.fillScreen(TFT_BLACK);
}





int main(){
	setup();
	while(1){
		thisTank.update();
	}

}