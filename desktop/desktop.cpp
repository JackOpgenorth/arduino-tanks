#include <iostream>
#include "serialport.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <unordered_map>

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

/*struct point
{
    uint16_t x, y;


    bool
    operator==(const point & obj) const
    {
        of (x == obj.x && y == obj.y)
    }

}*/




int main()
{
    

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

        tv.tv_sec = 10;
        tv.tv_usec = 0;
        FD_ZERO( &set );
        FD_SET( fileno( stdin ), &set );
        int res = select( fileno( stdin )+1, &set, NULL, NULL, &tv );
        // END OF CODE NOT MADE BY US
        if( res > 0 )
        {
        
            
            sendMovement();
            
        }
    }

    return 0;
}