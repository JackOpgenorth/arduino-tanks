#ifndef _BUllTANK_H
#define _BULLTANK_H
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>

#define YP A3 // must be an analog pin, use "An" notation!
#define XM A2 // must be an analog pin, use "An" notation!
#define YM 9  // can be a digital pin
#define XP 8  // can be a digital pin




struct tank
{

	int x, y, bullets, deaths, old_deaths; // bullets refers to the number of bullets


	tank(int inputx, int inputy);


	void redrawCursor(uint16_t colour, int &x, int &y);


	void ardiUpdate();


	void desktopUpdate(char direction);


};



struct bullet
{
	bool active; // 0 means the bullet is not supposed to be on the screen
	int x, y, velX, velY, startTime;
	int bounce; // if a bullet bounces twice, we get rid of it
	bool gracePeriod;

	bullet(int inputx, int inputy);

	void checkCollision(struct tank &tankYou);

	int updateBullet(int &numBullets);

	void fire(int tapX, int tapY);
};


/*

				(x1, y1)					(x2, y2)


				(x3, y3)					(x4, y4)

*/
struct rectCoords
{

	int x1, y1, x2, y2, x3, y3, x4, y4;

	rectCoords();


};


#endif