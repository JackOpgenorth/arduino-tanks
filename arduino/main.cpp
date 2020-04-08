#include <Arduino.h>

// core graphics library (written by Adafruit)
#include <Adafruit_GFX.h>

// Hardware-specific graphics library for MCU Friend 3.5" TFT LCD shield
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
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


MCUFRIEND_kbv tft;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

lcd_image_t backImage = {"yeg-big.lcd", BACK_WIDTH, BACK_HEIGHT};

/*void redrawCursor(uint16_t colour, int x, int y);*/


//forward declaration of a few functions is needed
char check_xy(int x, int y); 
bool check_boundries(int &x, int &y);

void testing(char message[]){
	tft.setCursor(160, 160);
	tft.setTextColor(TFT_BLUE);
	tft.setTextSize(2);
	tft.print(message);
}













// hold almost all information about the "tanks"

struct tank
{

	int x, y, bullets = 0, deaths = 0, old_deaths = 0; // bullets refers to the number of bullets


	// constructer
	tank(int inputx, int inputy){
		x = inputx;
		y = inputy;
	}

	/*
		   Description: redraws the square at the specified posititon,
		   				taken entirely from major assignment 1

		   Arguments: colour: what colour we want the square
		   			  x: x position of where we will be drawing
		   			  y: y position of where we will bed drawing
		   Returns: nothing
	*/
	void redrawCursor(uint16_t colour, int &x, int &y) {
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
	void ardiUpdate(){


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
	void desktopUpdate(char direction){
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


};












// keeps track of most of the information related to the bullets
struct bullet
{

	bool active = 0; // 0 means the bullet is not supposed to be on the screen
	int x, y, velX = 3, velY = 3, startTime = 0;
	int bounce = 0; // if a bullet bounces twice, we get rid of it
	bool gracePeriod = true;

	// constructor
	bullet(int inputx = 0, int inputy = 0){
		x = inputx;
		y = inputy;
	}

	void checkCollision(struct tank tankYou) {

		/*
		tft.fillRect(80, 100, CURSOR_SIZE, CURSOR_SIZE, TFT_BLACK);

		tft.setCursor(80, 100);
		tft.setTextColor(TFT_BLUE);
		tft.setTextSize(2);
		tft.print(gracePeriod);
		*/


		if (((x < tankYou.x +5) && (x > tankYou.x -5) && (y < tankYou.y +5) && (y > tankYou.y -5)) && !(gracePeriod)) {

			//tft.setCursor(260, 160);
			//tft.setTextColor(TFT_BLUE);
			//tft.setTextSize(2);
			//tft.print("TANK HIT!");
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

	int updateBullet(int &numBullets){
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


	void fire(int tapX, int tapY){
		float vecX = tapX - x;
		float vecY = -1*(tapY - y);

		// for grace period
		startTime = millis();


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
};







/*

	Displays current scores to top of arduino screen

*/
void displayScores(struct tank ard_tank, struct tank desk_tank) {

	tft.setTextColor(TFT_BLACK);
	tft.setTextSize(3);

	if (ard_tank.deaths != ard_tank.old_deaths) {

		tft.fillRect(210, 0, 30, 25, TFT_RED);

		//arduino score
		tft.setCursor(212, 3);
		tft.print(ard_tank.deaths);

	}

	if (desk_tank.deaths != desk_tank.old_deaths) {

		tft.fillRect(240, 0, 30, 25, TFT_RED);

		//desktop score
		tft.setCursor(248, 3);
		tft.print(desk_tank.deaths);
	}
}





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

	//arduino score
	tft.setCursor(212, 3);
	tft.print(0);

	//desktop score
	tft.setCursor(248, 3);
	tft.print(0);
	

	//lcd_image_draw(&backImage, &tft, 0,0,0,0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
}

	/*
	   Description: very simple function to check if a square hit the boundry of the screen
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





// NEEDS TO BE CHANGED TO ACCOMIDATE THE DESKTOP
void process_shot(int touchX, int touchY, tank &Atank, bullet bulls[], int &cooldown){
	if (Atank.bullets >= 2 ||  millis() - cooldown < 1000){return;}
	

	bullet bull = bulls[Atank.bullets];

	bull.x = Atank.x;
	bull.y = Atank.y;

	bull.fire(touchX, touchY);


	bulls[Atank.bullets] = bull;

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
	Serial.println(rectStr);
	Serial.println(wStr);
	rectStr = rectStr.substring(rectStr.indexOf(" ", 1));

	String lStr = rectStr.substring(1, rectStr.indexOf(','));
	

	// draw rectangle
	tft.fillRect(xStr.toInt(), yStr.toInt(), wStr.toInt(), lStr.toInt(), TFT_BLUE );

}


	/*
	   Description: waits for the desktop to signal it is about to send a rectangle;
	   Arguments: none


	   Returns: none
*/

void wait_for_rectangles(){
	bool waiting = 1;
	char incoming;
	while (waiting){ // keep waiting until we get a signal from the desktop
		if (Serial.available()){

			incoming = Serial.read();
			Serial.println(incoming);
			if (incoming == 'R'){// desktop says its sending a rectangle
				readRect();
			}
			if (incoming == 'F'){// desktop says its done sending reactangles
				break;
			}

		}
	}


}


	/*
	   Description: sends info about a point to the desktop, which then tells the arduino 
	   				wheather or not this is a "valid" point. (i.e have we just crashed into a 
	   				rectangle)
	   Arguments: x: x position we want to check
	   			  y: y position we want to check


	   Returns: incoming: will tell us wheather the point is valid or not	*/			
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
			else if (incoming == 'N'){// point is nbot valid

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

	// get squares ready
	tank thisTank(50,150);
	tank deskTank(300, 150);


	// get bullets ready
	int ardiCooldown = 0, deskCooldown = 0;
 	bullet ardiBull[5] =
 	{
 		bullet(20, 20), // the starting position of the bullets don't actually matter
 		bullet(20, 20),
 		bullet(20, 20),
 		bullet(20, 20),
 		bullet(20, 20)

 	};
 	bullet deskBull[5] = 
 	{
  	bullet(20, 20),
 		bullet(20, 20),
 		bullet(20, 20),
 		bullet(20, 20),
 		bullet(20, 20)	
 	};

 	// main loop
	while(1){
		thisTank.ardiUpdate();

		if (Serial.available()){// check if desktop is sending somthing
			char incoming = Serial.read();

			if (incoming == 'M'){ //check if desktop is telling us to move its square
				readDesktop(deskTank);
			}	
		}

		TSPoint touch = ts.getPoint();
		pinMode(YP, OUTPUT);
		pinMode(XM, OUTPUT);
		
		if ( (touch.z > MINPRESSURE && touch.z < MAXPRESSURE )){
			int touchX = map(touch.y, TS_MINX, TS_MAXX, DISPLAY_WIDTH - 1, 0);
			int touchY = map(touch.x, TS_MINY, TS_MAXY, DISPLAY_HEIGHT - 1, 0);

			if (touchX > DISPLAY_WIDTH/2){
				process_shot(touchX, touchY, thisTank, ardiBull, ardiCooldown);
			}

			else if (touchX < DISPLAY_WIDTH/2){
				process_shot(touchX, touchY, deskTank, deskBull, deskCooldown);
			}

		}

		



		for (int i = 0; i < 5; i++){//update all the bullets

			ardiBull[i].updateBullet(thisTank.bullets);
			deskBull[i].updateBullet(thisTank.bullets);

			displayScores(thisTank, deskTank);

			//arduino tank
			ardiBull[i].checkCollision(thisTank);
			deskBull[i].checkCollision(thisTank);

			//desktop tank
			ardiBull[i].checkCollision(deskTank);
			deskBull[i].checkCollision(deskTank);
		}
	}
}

