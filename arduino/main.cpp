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

bool shot = false;

char check_xy(int x, int y); //forward declaration
bool check_boundries(int &x, int &y);

void testing(char message[]){
	tft.setCursor(160, 160);
	tft.setTextColor(TFT_BLUE);
	tft.setTextSize(2);
	tft.print(message);
}



struct tank
{

	int x, y, bullets = 0;

	tank(int inputx, int inputy){
		x = inputx;
		y = inputy;
	}


	void redrawCursor(uint16_t colour, int &x, int &y) {
  		tft.fillRect(x - CURSOR_SIZE/2, y - CURSOR_SIZE/2,
        CURSOR_SIZE, CURSOR_SIZE, colour);
	}


	void ardiUpdate(){
		//obtaining the velocity by scaling down the values obtained from the x and y pins
		//so we won't go too fast, then subtracting so that while the joystick is
		//centered we will always get a velocity of 0.


		int velX = abs(0.002 * analogRead(JOY_HORIZ) - 10);
		int velY = abs(0.002 * analogRead(JOY_VERT) - 10);
		// so we only redraw the screen when we are moving
		bool moving = false;

		int xVal = analogRead(JOY_HORIZ);
		int yVal = analogRead(JOY_VERT);
		int buttonVal = digitalRead(JOY_SEL);

		// for redrawing later
		int oldY = y;
		int oldX = x;


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

		check_boundries(x, y);

		// redrawing patch at old position.
		if (moving){
			//tft.fillRect(oldX - CURSOR_SIZE/2, oldY - CURSOR_SIZE/2,
			//CURSOR_SIZE, CURSOR_SIZE, TFT_BLACK);
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


		void desktopUpdate(char c){
			int oldY = y;
			int oldX = x;
			if (c == 'w'){
				y -= 5;
			}
			if (c == 's'){
				y += 5;
			}
			
			if (c == 'd'){
				x += 5;
			
			}
			if (c == 'a'){
				x -= 5;
			}

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



struct bullet
{

	bool active = 0;
	int x, y, velX = 5, velY = 5;
	int bounce = 0;

	bullet(int inputx = 0, int inputy = 0){
		x = inputx;
		y = inputy;
	}

	void checkCollision(struct tank tankYou) {

		if ((x < tankYou.x +5) && (x > tankYou.x -5) && (y < tankYou.y +5) && (y > tankYou.y -5)) {

			tft.setCursor(260, 160);
			tft.setTextColor(TFT_BLUE);
			tft.setTextSize(2);
			tft.print("TANK HIT!");

		}
	}
	

	int updateBullet(int &numBullets){
		//Serial.println(bounce);

		if (!active){
			//Serial.print("notactive");
			return 1;
		}

		if (bounce > 2){
			active = 0;
			bounce = 0;
			tft.fillCircle(x, y, BULLET_SIZE, TFT_BLACK);
			numBullets--;
			//Serial.print("boom");
			return 0;
		}

/*				lcd_image_draw(&backImage, &tft,
							 x - CURSOR_SIZE/2,
						 	 y - CURSOR_SIZE/2,
						   x - CURSOR_SIZE/2, 
						   y - CURSOR_SIZE/2,
							 CURSOR_SIZE, CURSOR_SIZE);*/


		tft.fillCircle(x, y, BULLET_SIZE, TFT_BLACK);
		x += velX;
		y += velY;

		if (x < 0){
			x = 0;
			velX *= -1;
			bounce++;
		}
		if (x > DISPLAY_WIDTH){
			x = DISPLAY_WIDTH;
			velX *= -1;
			bounce++;

		}
		if (y <=0 ){
			y = 0;
			velY *=-1;
			bounce++;
		}
		if (y >= DISPLAY_HEIGHT){
			y = DISPLAY_HEIGHT;
			velY *= -1;
			bounce++;
		}

		//for (int i = 0; i < 5; i++){
/*			char flag = check_xy(x, y);
			if (flag == 'S'){velX *= -1;}
			if (flag == 'T'){velY *= -1;}
			flag = check_xy(x, y);
			if (flag == 'S'){velX *= -1;}
			if (flag == 'T'){velY *= -1;}*/


		//}



		tft.fillCircle(x, y, BULLET_SIZE, TFT_BLUE);
		return 1;
	}


	void fire(int tapX, int tapY){
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
};




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
	//lcd_image_draw(&backImage, &tft, 0,0,0,0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
}

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



void readTouch(int &cooldown, bullet bullArray[], tank &Atank){

	TSPoint touch = ts.getPoint();
	pinMode(YP, OUTPUT);
	pinMode(XM, OUTPUT);
	if ( (touch.z > MINPRESSURE && touch.z < MAXPRESSURE ) && Atank.bullets <= 5 ){
		bullet bull = bullArray[Atank.bullets];

		bull.x = Atank.x;
		bull.y = Atank.y;

		int touchX = map(touch.y, TS_MINX, TS_MAXX, DISPLAY_WIDTH - 1, 0);
		int touchY = map(touch.x, TS_MINY, TS_MAXY, DISPLAY_HEIGHT - 1, 0);

		bull.fire(touchX, touchY);

		bullArray[Atank.bullets] = bull;

		Atank.bullets += 1;
		cooldown = millis();

	}
}


void readDesktop(tank& deskTank){
	if (Serial.available()){
		//testing("MESSAGE");
		char incoming = Serial.read();
		Serial.println(incoming);
		deskTank.desktopUpdate(incoming);
	}
}

void readRect(){
	char rectInfo[50];
	int i = 0;
	while(1){
		if (Serial.available()){
			char incoming = Serial.read();
			if (incoming == '\n'){
				break;
			}

			rectInfo[i] = incoming;
			i++;
			rectInfo[i] = 0;

		}
	}
	// parse
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
	
	//testing(str);	
	tft.fillRect(xStr.toInt(), yStr.toInt(), wStr.toInt(), lStr.toInt(), TFT_BLUE );

}




void wait_for_rectangles(){
	bool waiting = 1;
	char incoming;
	while (waiting){
		if (Serial.available()){
			incoming = Serial.read();
			Serial.println(incoming);
			if (incoming == 'R'){
				readRect();
			}
			if (incoming == 'F'){
				break;
			}

		}
	}


}

char check_xy(int x, int y){
	Serial.print("P ");
	Serial.print(x);
	Serial.print(", ");
	Serial.println(y);
	char incoming;
	while (1){
		if (Serial.available()){
			incoming = Serial.read();
			if (incoming == 'V'){
			
				
				return incoming;
			}
			else if (incoming == 'N'){

				// need to wait just a split second until we are ready to read
				while (!Serial.available()){
					continue;
				}
				incoming = Serial.read();
				
				return incoming;
			}
		}
	}

}



int main(){
	setup();
	// let the desktop know we are ready
	Serial.println("Y\n");

	wait_for_rectangles();
	tank thisTank(50,150);
	tank deskTank(200,150);


	
	int cooldown = 0;
 	bullet bullArray[5] =
 	{
 		bullet(20, 20),
 		bullet(20, 20),
 		bullet(20, 20),
 		bullet(20, 20),
 		bullet(20, 20)

 	};
 	check_xy(220, 178);
	while(1){
		thisTank.ardiUpdate();

		if (Serial.available()){
			char incoming = Serial.read();

			if (incoming == 'M'){
				
				readDesktop(deskTank);
			}	
		}



		
		if (millis() - cooldown > 1000){
			readTouch(cooldown, bullArray, thisTank);
		}

		for (int i = 0; i < 5; i++){
			int flag = bullArray[i].updateBullet(thisTank.bullets);
		}
	}
}

