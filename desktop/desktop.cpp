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

void sendMovement(){
    char c;
    read( fileno( stdin ), &c, 1 ); //THIS LINE WAS ALSO NOT MADE BY US
    cout << c << endl;
    Serial.writeline("M");
    if (c == 'w'){Serial.writeline("w");}
    else if (c == 'a'){Serial.writeline("a");}
    else if (c == 's'){Serial.writeline("s");}
    else if (c == 'd'){Serial.writeline("d");}
}

struct point
{
    uint16_t x, y;
    string face; // we need to know if this is on the side or top of rectangle

    point(uint16_t x_in = 0, uint16_t y_in = 0, string face_in = "NULL"){
        x = x_in;
        y = y_in;
    }


    
    bool operator==(const point & obj) const
    {
        if (x == obj.x && y == obj.y)
        {
            return true;
        }
        return false;
    }

};



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




void setup_rectangle(rectangle rect, unordered_set<point> &archive){


    //cout  << "R" << to_string(rect.xo) << ", " << to_string(rect.yo) << ", " 
    //<< to_string(rect.width) << ", " << to_string(rect.length) << "\n";
    // send rectangle information

    Serial.writeline("R");
    Serial.writeline(to_string(rect.xo) + ", " + to_string(rect.yo)
                    + ", " + to_string(rect.width) + ", " + to_string(rect.length) + "\n");
    point p;
    //top and bottom points
    for (int i = 0; i <= rect.width; i++){
        p.face = "T";
        p.y = rect.yo;
        p.x = rect.xo + i;
        archive.insert(p);
        cout << "TOP POINT: " << p.x << "," << p.y << endl;
        p.y = rect.yo + rect.length;
        archive.insert(p);
        cout << "BOTTOM POINT " << p.x << "," << p.y << endl;
    }
    // left and right points
    for (int i = 0; i < rect.length; i++){
        p.face = "S";
        p.y = rect.yo + i;
        p.x = rect.xo;
        archive.insert(p);
        cout  << "LEFT POINT "<< p.x << "," << p.y << endl;
        p.x = rect.xo + rect.width;
        archive.insert(p);
        cout << "RIGHT POINT " <<  p.x << "," << p.y << endl;



    }
     
}

void check_point(string arduinoPoint, unordered_set<point> &archive){

    arduinoPoint = arduinoPoint.substr(arduinoPoint.find(" ") + 1);  // cut off first space

    string xStr = arduinoPoint.substr(0, arduinoPoint.find(","));
    string yStr = arduinoPoint.substr(arduinoPoint.find(",") + 2);

    point pCheck(stoi(xStr), stoi(yStr));


    auto iter = archive.find(pCheck);
    if (iter != archive.end()){ // point is not valid
        pCheck = *iter;
        Serial.writeline("N" + pCheck.face);
    }
    else{
        Serial.writeline("V");
    }



/*    point p = *iter;
    cout << "endhere4" << endl;
    cout << p.x << ' ' << p.y << endl;
    if (archive.find(pCheck) == archive.end()){
        cout << "sending" << endl;
    }
    else{
        cout << "sending" << endl;
        Serial.writeline("V\n");// point was valid
    }*/
}

int main(){
    // only start once the arduino is ready to recive input
    while(1){
        string flag = Serial.readline();
        if (flag == "Y\n"){
            break;
        }
    }
    unordered_set<point> archive;
    //cout << "ASdasdsadasd" << endl;
    rectangle test1(200, 100, 20, 80);
  //  rectangle test2(100, 100, 20, 80);
    // let arduino kow we are done sending info
    //Serial.writeline("D");

    setup_rectangle(test1, archive);
  //  setup_rectangle(test2, archive);


    // letting arduino know we are done sending rectangle data
    Serial.writeline("F");

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


        string flag = Serial.readline();
        if (flag[0] == 'P'){
            check_point(flag, archive);
        }
        if( res > 0 )
        {
        
            sendMovement();
            
        }
        else if( res < 0 )
        {
            perror( "select error" );
            break;
        }
        else
        {
        }
    }

    return 0;
}