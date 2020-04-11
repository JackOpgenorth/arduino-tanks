/* 
This file contains all the code that is being executed on the arduino.
*/ 



// most constants and files included below were copied from major assignment 2

#include <Arduino.h>
#include "bull_tank.h"
// core graphics library (written by Adafruit)
#include <Adafruit_GFX.h>

// Hardware-specific graphics library for MCU Friend 3.5" TFT LCD shield
#include <SD.h>
#include <math.h>

#include "lcd_image.h"

#define SD_CS 10
#define JOY_VERT  A9 // should connect A9 to pin VRx
#define JOY_HORIZ A8 // should connect A8 to pin VRy
#define JOY_SEL   53


#define JOY_CENTER   512
#define JOY_DEADZONE 64

#define CURSOR_SIZE 9
#define BULLET_SIZE 4

#define DISPLAY_WIDTH  480
#define DISPLAY_HEIGHT 320

#define BACK_WIDTH 2048
#define BACK_HEIGHT 2048


#define YP A3 // must be an analog pin, use "An" notation!
#define XM A2 // must be an analog pin, use "An" notation!
#define YM 9  // can be a digital pin
#define XP 8  // can be a digital pin


#define TS_MINX 100
#define TS_MINY 110
#define TS_MAXX 960
#define TS_MAXY 910

// thresholds to determine if there was a touch
#define MINPRESSURE   10
#define MAXPRESSURE 1000

#define BLOCK_SIZE  10

extern MCUFRIEND_kbv tft;

extern TouchScreen ts;

lcd_image_t backImage = {"yeg-big.lcd", BACK_WIDTH, BACK_HEIGHT};




// much of the setup function is what was used in the major assignments

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
	tft.drawLine(DISPLAY_WIDTH/2, 0, DISPLAY_WIDTH/2, DISPLAY_HEIGHT, TFT_BLUE);

	tft.fillRect(50 - CURSOR_SIZE/2, 150 - CURSOR_SIZE/2,
        CURSOR_SIZE, CURSOR_SIZE, TFT_RED);
	tft.fillRect(300 - CURSOR_SIZE/2, 150 - CURSOR_SIZE/2,
        CURSOR_SIZE, CURSOR_SIZE, TFT_RED);


	tft.setTextColor(TFT_BLACK);
	tft.setTextSize(3);

	tft.fillRect(210, 0, 60, 25, TFT_RED);

	//arduino score box
	tft.setCursor(217, 3);
	tft.print(0);

	//desktop score box
	tft.setCursor(248, 3);
	tft.print(0);
}


/*
	Description: If a tank has been hit since the last time the function was called, it redraws
				the score box at the top of the screen with the new values
	Arguments: both of the tank stuctures, by reference
	Outputs: none

*/
void displayScores(struct tank &ard_tank, struct tank &desk_tank) {

	// change text options
	tft.setTextColor(TFT_BLACK);
	tft.setTextSize(3);

	// only change desktop tank score if it has changed
	if ((ard_tank.deaths != ard_tank.old_deaths) && (ard_tank.deaths < 10)) {

		tft.fillRect(240, 0, 30, 25, TFT_RED);

		//arduino score
		tft.setCursor(248, 3);
		tft.print(ard_tank.deaths);

		// reset old death value
		ard_tank.old_deaths = ard_tank.deaths;
	}

	// only change desktop tank score if it has changed, score 10+
	else if (ard_tank.deaths != ard_tank.old_deaths) {

		tft.fillRect(240, 0, 45, 25, TFT_RED);

		//arduino score
		tft.setCursor(248, 3);
		tft.print(ard_tank.deaths);

		// reset old death value
		ard_tank.old_deaths = ard_tank.deaths;
	}

	// only change arduino tank score if has changed
	if ((desk_tank.deaths != desk_tank.old_deaths) && (desk_tank.deaths < 10)) {

		tft.fillRect(210, 0, 30, 25, TFT_RED);

		//desktop score
		tft.setCursor(217, 3);
		tft.print(desk_tank.deaths);

		// reset old death value
		desk_tank.old_deaths = desk_tank.deaths;
	}

	// only change arduino tank score if has changed, score 10+
	if (desk_tank.deaths != desk_tank.old_deaths) {

		tft.fillRect(195, 0, 45, 25, TFT_RED);

		//desktop score
		tft.setCursor(200, 3);
		tft.print(desk_tank.deaths);

		// reset old death value
		desk_tank.old_deaths = desk_tank.deaths;
	}
}


/*
   Description: very simple function to check if a square (not a bullet!)
   				hit the boundry of the screen
   Arguments: x: x position we want to check
   			  y: y position we want to check


   Returns: 0 if we hit a boundry, 1 otherwise
*/
bool check_boundries(int &x, int &y){
	if (x > DISPLAY_WIDTH - 4){
		x = DISPLAY_WIDTH - 4;
		return 0;
	}
	if (x < 4){
		x = 4;
		return 0;
	}
	if (y > DISPLAY_HEIGHT - 4){
		y = DISPLAY_HEIGHT - 4;
		return 0;
	}
	if (y < 4){
		y = 4;
		return 0;
	}
	return 1;
}


/*
	Description: call when screen has been tapped, computes everything necissary to set up
				and fire a bullet
	Arguments: touchX: x-coordinate of screen tap; touchY: y-coordinate of tap; the tank 
				object (the tank firing the bullet); reference to the bullet array of the current
				tank; cooldown: reference to current cooldown of the tank
	Returns: nothing
*/
void process_shot(int touchX, int touchY, tank &Atank, bullet *bulls, int &cooldown){
	if (Atank.bullets >= 2 ||  millis() - cooldown < 1000){return;}
	bullet bull = bulls[Atank.bullets];

	bull.x = Atank.x;
	bull.y = Atank.y;

	bull.fire(touchX, touchY);

	bulls[Atank.bullets] = bull;
	bulls[Atank.bullets].gracePeriod = true;

	Atank.bullets += 1;
	cooldown = millis();


}


/*
   Description: Reads for keyboard input from the desktop and updates the desktop square accordingly

   Arguments: deskTank: the desktop square

   Returns: nothing
*/
void readDesktop(tank& deskTank){
	if (Serial.available()){
		//testing("MESSAGE");
		char incoming = Serial.read();// w, a, s, or d
		deskTank.desktopUpdate(incoming);
	}
}



/*
   Description: Reads information about a rectangle from the desktop and then draws said rectangle
   Arguments: nothing


   Returns:nothing
*/
void readRect(){
	char rectInfo[50]; // to hold message from desktop
	int i = 0;
	while(1){
		if (Serial.available()){
			char incoming = Serial.read();
			if (incoming == '\n'){ // newlines signals end of message
				break;
			}
			// read in one char at a time
			rectInfo[i] = incoming;
			i++;
			rectInfo[i] = 0;

		}
	}
	// parse message
	String rectStr = rectInfo;
	String xStr = rectStr.substring(0, rectStr.indexOf(','));
	rectStr = rectStr.substring(rectStr.indexOf(" ", 1));
	String yStr = rectStr.substring(1, rectStr.indexOf(','));
	rectStr = rectStr.substring(rectStr.indexOf(" ", 1));
	String wStr = rectStr.substring(1, rectStr.indexOf(','));
	rectStr = rectStr.substring(rectStr.indexOf(" ", 1));

	String lStr = rectStr.substring(1, rectStr.indexOf(','));
	

	// draw rectangle
	tft.fillRect(xStr.toInt(), yStr.toInt(), wStr.toInt(), lStr.toInt(), TFT_BLUE );

	Serial.println("D");


}


/*
   Description: reads in rectangles from the desktop until it is done sending rectangles
   Arguments: none


   Returns: none
*/

void wait_for_rectangles(){
	bool waiting = 1;
	char incoming;
	//Serial.println(Serial.available());

	unsigned long timeout = millis();
	while (waiting){ // keep waiting until we get a signal from the desktop
		if (Serial.available()){

			incoming = Serial.read();
			//Serial.println(incoming);

			if (incoming == 'R'){// desktop says its sending a rectangle
				
				readRect();
			}

			if (incoming == 'F' || millis() - timeout > 2000){// desktop says its done sending reactangles
				break;
			}
		}
	}
}


/*
 Description: sends info about a point to the desktop, which then tells the arduino 
 				wheather or not this is a "valid" point. (i.e have we just crashed into a 
 				rectangle)
 Arguments:   x: x position we want to check
 			  y: y position we want to check


 Returns: incoming: will tell us wheather the point is valid or not	
*/			
char check_xy(int x, int y){
	// sending point to dekstop
	Serial.print("P ");
	Serial.print(x);
	Serial.print(", ");
	Serial.println(y);
	char incoming;

	// wait for response
	while (1){
		if (Serial.available()){
			incoming = Serial.read();
			if (incoming == 'V'){// point is valid
				return incoming;
			}
			else if (incoming == 'N'){// point is not valid

				// need to wait just a split second until we are ready to read
				// in the next part of the message
				while (!Serial.available()){
					continue;
				}
				incoming = Serial.read();
				return incoming; //'S' if we hit the side or a recangle or 'T' if we hit the top
								 // or bottom
			}
		}
	}

}



int main(){
	setup();

	// let the desktop know we are ready
	Serial.println("Y\n");

	wait_for_rectangles();// read in and draw all the rectangles
	
	// get tanks ready
	tank thisTank(50,150);
	tank deskTank(300, 150);


	// get bullets ready
	int ardiCooldown = 0, deskCooldown = 0;
 	bullet ardiBull[2] =
 	{
 		bullet(20, 20), // the starting position of the bullets don't actually matter
 		bullet(20, 20)


 	};

 	bullet deskBull[2] = 
 	{
  	bullet(20, 20),
 		bullet(20, 20)

 	};

 	// main loop
	while(1){

		thisTank.ardiUpdate(); // moves the arduino's square according to joystick

		if (Serial.available()){// check if desktop is sending somthing
			char incoming = Serial.read();

			if (incoming == 'M'){ //check if desktop is telling us to move its square
				readDesktop(deskTank);
			}	
		}


		TSPoint touch = ts.getPoint();
		pinMode(YP, OUTPUT);
		pinMode(XM, OUTPUT);

		if ( (touch.z > MINPRESSURE && touch.z < MAXPRESSURE )){// screen is touched
			int touchX = map(touch.y, TS_MINX, TS_MAXX, DISPLAY_WIDTH - 1, 0);
			int touchY = map(touch.x, TS_MINY, TS_MAXY, DISPLAY_HEIGHT - 1, 0);

			if (touchX > DISPLAY_WIDTH/2){// left side, arduino fires
				process_shot(touchX, touchY, thisTank, ardiBull, ardiCooldown);
			}
			else if (touchX < DISPLAY_WIDTH/2){// right side, desktop fires
				process_shot(touchX, touchY, deskTank, deskBull, deskCooldown);
			}

		}


		for (int i = 0; i < 2; i++){
			//update all the bullets
			ardiBull[i].updateBullet(thisTank.bullets);
			deskBull[i].updateBullet(deskTank.bullets);

			displayScores(thisTank, deskTank);

			//arduino tank
			ardiBull[i].checkCollision(thisTank, thisTank.bullets);
			deskBull[i].checkCollision(thisTank, deskTank.bullets);

			//desktop tank
			ardiBull[i].checkCollision(deskTank, thisTank.bullets);
			deskBull[i].checkCollision(deskTank, deskTank.bullets);
		}
	}
}

