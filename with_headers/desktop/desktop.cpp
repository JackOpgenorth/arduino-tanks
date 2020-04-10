

/* 
This file contains all code that is being run on the desktop.
*/



#include <iostream>
#include "serialport.h"
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <unordered_set>

SerialPort Serial("/dev/ttyACM0");




// structure for a point on the edge of a rectangle
struct point
{
    uint16_t x, y;
    string face; // we need to know if this is on the side or top of rectangle

    // constructor
    point(uint16_t x_in = 0, uint16_t y_in = 0, string face_in = "NULL"){
        x = x_in;
        y = y_in;
    }


    // needed to overload == operator
    bool operator==(const point & obj) const
    {
        if (x == obj.x && y == obj.y)
        {
            return true;
        }
        return false;
    }

};


// hashing
namespace std
{
template<>
    struct hash<point>
    {
        
        size_t operator()(const point & obj) const
        {
            // hashes the two 16 bit integers by putting the bits of one
            // in the upper 16 bits of a 32 bit integer then adding them
            // so we get one unique 32 bit integer
            uint32_t upper_bits = obj.y << 16;
            uint32_t lower_bits = obj.x;
            uint32_t pointHash = upper_bits + lower_bits;
            return hash<int>()(pointHash);
        }
    };

}


struct pointHasher
{
    size_t
    operator()(const point & obj) const
    {   uint32_t upper_bits = obj.y << 16;
        uint32_t lower_bits = obj.x;
        uint32_t pointHash = upper_bits + lower_bits;
        return hash<int>()(pointHash);
    }
};

// needed to overload == operator
struct pcompare
{
    bool
    operator()(const point & obj1, const point & obj2)const
    {
        if (obj1.x == obj2.x && obj1.y == obj2.y){
            return true;
        }
        return false;
    }
};

// contains infomation about a rectangle that will be drawn by the arduino
struct rectangle
{
    uint32_t xo, yo, width, length;
    rectangle(int x, int y, int w, int l) {
        xo = x,
        yo = y;
        width = w;
        length = l;
    }
};




/*
       Description: tells the arduino how we want to move the desktop square
       Arguments: none

       Returns: none
*/
int sendMovement(){
    char c;
    read( fileno( stdin ), &c, 1 ); //THIS LINE WAS NOT MADE BY US, it reads in input
                                    // and stores it in c
    cout << c << endl;
    Serial.writeline("M");
    // send movement to arduino
    if (c == 'w'){Serial.writeline("w");}
    else if (c == 'a'){Serial.writeline("a");}
    else if (c == 's'){Serial.writeline("s");}
    else if (c == 'd'){Serial.writeline("d");}

    return 0;
}




/*
       Description: tells the arduino to draw a rectangle
       Arguments: rect: contains information about the rectangle we want to draw
                  archive: the set that will contain all invalid points, i.e points on the
                           edge of teh rectangle

       Returns: nothing
*/

void setup_rectangle(rectangle rect, unordered_set<point> &archive){


    // send rectangle information to arduino

    Serial.writeline("R");
    Serial.writeline(to_string(rect.xo) + ", " + to_string(rect.yo)
                    + ", " + to_string(rect.width) + ", " + to_string(rect.length) + "\n");

    
    // now we need to put the points on the bountry of the rectangle into a set so we can
    // tell the arduino if a bullet has hit the rectangle

    point p; // will hold info about a point we wanna store in the set

    //top and bottom points
    for (int i = 0; i <= rect.width; i++){
        // inner loop is due to the fact that the bullets jump 5 pixels
        // each time they move, se we must account for cases when the bullet jumps inside
        // the rectangles
        for (int j = -3; j <= 20; j++){
            p.face = "T"; 

            // add points on the top
            p.y = rect.yo + j;
            p.x = rect.xo + i;
            archive.insert(p);
            //add points on the bottom
            p.y = rect.yo + rect.length - j;
            archive.insert(p);
        }
    }
    // left and right points
    for (int i = 0; i <= rect.length; i++){

        for (int j = -3; j <= 20; j++){

            p.face = "S";

            // add point on left side
            p.y = rect.yo + i;
            p.x = rect.xo + j;
            archive.insert(p);

            // add point on left side
            p.x = rect.xo + rect.width - j;
            archive.insert(p);
        }



    }

    // before we exit the function  we wait for a message from the
    // arduino telling us it has drawn the rectangle. This is so we do
    // not overflow the serial buffer
    while(1){
        string msg = Serial.readline();
        if (msg[0] == 'D'){ // the message we are looking for
            break;
        }
    }
     
}


/*
       Description: tells the arduino if it has hit a rectangle or not
       Arguments: arduinoPoint: the message sent the the arduino containingthe point
                  archive: set containging all invalid points

       Returns: nothing
*/


void check_point(string arduinoPoint, unordered_set<point> &archive){

    // need to parse message
    arduinoPoint = arduinoPoint.substr(arduinoPoint.find(" ") + 1);  // cut off first space

    string xStr = arduinoPoint.substr(0, arduinoPoint.find(",")); // get x
    string yStr = arduinoPoint.substr(arduinoPoint.find(",") + 2); // get y

    point pCheck(stoi(xStr), stoi(yStr)); // will be used to check the set

    // checking set for point
    auto iter = archive.find(pCheck);
    if (iter != archive.end()){ // point is not valid
        pCheck = *iter;
        Serial.writeline("N" + pCheck.face);
    }
    else{// point is valid
        Serial.writeline("V");
    }
}


int main(){
    // only start once the arduino has told us it is ready to receive input
    while(1){
        string flag = Serial.readline();
        if (flag == "Y\n"){
            break;
        }
    }


    unordered_set<point> archive; // will to store any invalid points

    // setting up rectangles
    rectangle rect1(100, 20, 20, 80);
    rectangle rect2(360, 20, 20, 80);
    rectangle rect3(120, 200, 20, 80);
    rectangle rect4(360, 200, 20, 80);

    setup_rectangle(rect1, archive);
    setup_rectangle(rect2, archive);
    setup_rectangle(rect3, archive);
    setup_rectangle(rect4, archive);


    
    // telling adruino we are done setting up rectangles
    Serial.writeline("F");
    cout << "F" << endl;

    // BEGINNING OF CODE NOT MADE BY US
    struct termios oldSettings, newSettings;
    tcgetattr( fileno( stdin ), &oldSettings );
    newSettings = oldSettings;
    newSettings.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr( fileno( stdin ), TCSANOW, &newSettings );  

    while ( 1 )
    {
        fd_set set;
        struct timeval tv;

        tv.tv_sec = 0.1;
        tv.tv_usec = 0;
        FD_ZERO( &set );
        FD_SET( fileno( stdin ), &set );
        int res = select( fileno( stdin )+1, &set, NULL, NULL, &tv );
        // END OF CODE NOT MADE BY US

        
        string flag = Serial.readline();// see if the arduino has sent us anything
        if (flag[0] == 'P'){ // arduino has sent us a point to check
            check_point(flag, archive);
        }
        if( res > 0 )// if we enter somthing in the keyboard, tell the arduino how
                     // to move its square
        {
            
            sendMovement();
            
        }
    }

    return 0;
}
