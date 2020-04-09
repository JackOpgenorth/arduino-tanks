#include <Arduino.h>
#include "bull_tank.h"
// core graphics library (written by Adafruit)
#include <Adafruit_GFX.h>

// Hardware-specific graphics library for MCU Friend 3.5" TFT LCD shield
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <SD.h>
#include <math.h>


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

MCUFRIEND_kbv tft;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

bool check_boundries(int &x, int &y);
char check_xy(int x, int y); 


tank::tank(int inputx, int inputy){
	this->x = inputx;
	this->y = inputy;
	deaths = 0, old_deaths = 0;
	bullets = 0;
}

/*
	   Description: redraws the square at the specified posititon,
	   				taken entirely from major assignment 1

	   Arguments: colour: what colour we want the square
	   			  x: x position of where we will be drawing
	   			  y: y position of where we will bed drawing
	   Returns: nothing
*/
void tank::redrawCursor(uint16_t colour, int &x, int &y) {
		tft.fillRect(x - CURSOR_SIZE/2, y - CURSOR_SIZE/2,
    CURSOR_SIZE, CURSOR_SIZE, colour);
}


/*
	   Description: Updates the position of the arduino tank based
	   				off of the joystick, much of this was taken from major
	   				assignment 1

	   Arguments: none
	   Returns: none
*/

void tank::ardiUpdate(){


	// reading from joystuck and manipulating the value found so we get
	// an appropiate velocity
	int velX = abs(0.002 * analogRead(JOY_HORIZ) - 10);
	int velY = abs(0.002 * analogRead(JOY_VERT) - 10);

	
	bool moving = false; // so we only redraw the screen when we are moving


	int xVal = analogRead(JOY_HORIZ);
	int yVal = analogRead(JOY_VERT);
	int buttonVal = digitalRead(JOY_SEL);

	// for redrawing later
	int oldY = y;
	int oldX = x;

	// below we change the x and y position according to the velocity
	// the random literals in the expressions were just chosen so that the square would
	// not move super fast or slow


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

	//  moving right
	else if (xVal < JOY_CENTER - JOY_DEADZONE) {
		x += 1 + velX - 9;
		moving = true;
	}


	check_boundries(x, y); // make sure we have not hit the top or bottom

	// redrawing patch at old position.
	if (moving){
		tft.fillRect(oldX - CURSOR_SIZE/2, oldY - CURSOR_SIZE/2,
		CURSOR_SIZE, CURSOR_SIZE, TFT_BLACK);
/*
		lcd_image_draw(&backImage, &tft,
				 oldX - CURSOR_SIZE/2,
			 	 oldY - CURSOR_SIZE/2,
			   oldX - CURSOR_SIZE/2, 
			   oldY - CURSOR_SIZE/2,
				 CURSOR_SIZE, CURSOR_SIZE);
*/
		redrawCursor(TFT_RED, x, y);
	}

		delay(20);

}



/*
	   Description: Updates information for the desktop square according to keyboard input from
	   				the desktop
	   Arguments: direciton: determines which way the square will move, will be w, a, s, or d
	   Returns: none
*/
void tank::desktopUpdate(char direction){
	int oldY = y;
	int oldX = x;
	if (direction == 'w'){
		y -= 5;
	}
	if (direction == 's'){
		y += 5;
	}
	
	if (direction == 'd'){
		x += 5;
	
	}
	if (direction == 'a'){
		x -= 5;
	}

	redrawCursor(TFT_BLACK, oldX, oldY); 

	/*
	lcd_image_draw(&backImage, &tft,
						 oldX - CURSOR_SIZE/2,
					 	 oldY - CURSOR_SIZE/2,
					   oldX - CURSOR_SIZE/2, 
					   oldY - CURSOR_SIZE/2,
						 CURSOR_SIZE, CURSOR_SIZE);
	tft.fillRect(oldX - CURSOR_SIZE/2, oldY - CURSOR_SIZE/2,
	CURSOR_SIZE, CURSOR_SIZE, TFT_BLACK); */

	redrawCursor(TFT_RED, x, y); 


}



bullet::bullet(int inputx, int inputy){
	this->x = inputx;
	this->y = inputy;
	velX = 3;
	active = 0;
	velY = 3;
	bounce = 0;
	startTime = 0;
	gracePeriod = 1;
}



void bullet::checkCollision(struct tank tankYou) {
			/*
		tft.fillRect(80, 100, CURSOR_SIZE, CURSOR_SIZE, TFT_BLACK);

		tft.setCursor(80, 100);
		tft.setTextColor(TFT_BLUE);
		tft.setTextSize(2);
		tft.print(gracePeriod);
		*/
	if ((x < tankYou.x +5) && (x > tankYou.x -5) && (y < tankYou.y +5) && (y > tankYou.y -5)) {

			tft.fillRect(tankYou.x - CURSOR_SIZE, tankYou.y - CURSOR_SIZE, 2*CURSOR_SIZE, 2*CURSOR_SIZE, TFT_BLUE);

			tankYou.deaths++;

			tft.setCursor(60, 260);
			tft.setTextColor(TFT_BLUE);
			tft.setTextSize(2);

			tft.print(tankYou.deaths);

	}
}

	/*
	   Description: Updates information about the bullet
	   Arguments: &numbullett: the number of bullets of a tank, may be subtracted from
	   			  if we end up destroying the bullet

	   Returns:
*/

int bullet::updateBullet(int &numBullets){
	//Serial.println(bounce);

	if (!active){ // bullet isn't actually active, so don't do anything
		return 1;
	}

	if ((millis() > startTime + 1000) && (gracePeriod)) {
		gracePeriod = false;
	}

	if (bounce > 2){ // bullet bounced 3 times and shoild be destroyed
		active = 0;
		bounce = 0;
		tft.fillCircle(x, y, BULLET_SIZE, TFT_BLACK);
		numBullets--; // this will go too whatever tank shot the bullet
		return 0;
	}

/*				lcd_image_draw(&backImage, &tft,
						 x - CURSOR_SIZE/2,
					 	 y - CURSOR_SIZE/2,
					   x - CURSOR_SIZE/2, 
					   y - CURSOR_SIZE/2,
						 CURSOR_SIZE, CURSOR_SIZE);*/

// draws over the old position of the bullet
	tft.fillCircle(x, y, BULLET_SIZE, TFT_BLACK); 


	if (x < 0){// we hit the right boundry
		x = 0;
		velX *= -1;
		bounce++;
	}
	if (x > DISPLAY_WIDTH){// we hit the left boundry
		x = DISPLAY_WIDTH;
		velX *= -1;
		bounce++;

	}
	if (y <= 0 ){// we hit the top boundry
		y = 0;
		velY *= -1;
		bounce++;
	}
	if (y >= DISPLAY_HEIGHT){// we hit the bottom boundry
		y = DISPLAY_HEIGHT;
		velY *= -1;
		bounce++;
	}


		
	// communicate with the desktop to see if we have hitn a rectangle
	char flag = check_xy(x, y);


	if (flag == 'S'){velX *= -1;}// we hit the left or right side
	if (flag == 'T'){velY *= -1;}// we hit the top or bottom


	// finally, update the position of the bullet and draw it
	x += velX;
	y += velY;
	tft.fillCircle(x, y, BULLET_SIZE, TFT_BLUE);
	return 1;
}



void bullet::fire(int tapX, int tapY){
	float vecX = tapX - x;
	float vecY = -1*(tapY - y);


	tft.fillRect(260, 160, 80, 80, TFT_BLACK);


	tft.setCursor(260, 160);
	tft.setTextColor(TFT_BLUE);
	tft.setTextSize(2);
	tft.print(vecX);
	tft.setCursor(260, 200);
	tft.print(vecY);



	float radAng = atan(vecY/vecX);


	tft.setCursor(260, 240);
	tft.print(radAng);



	if (   (vecY > 0 && vecX < 0) ){
		radAng += 3.14;
	}

	if (vecY < 0 && vecX < 0){
		radAng -= 3.14;
	}

	active = 1;


	velX = 5*cos(radAng);
	velY = -5*sin(radAng);
}
