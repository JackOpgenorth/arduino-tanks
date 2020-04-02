#include <iostream>
#include "serialport.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

int main()
{
	SerialPort Serial("/dev/ttyACM0");
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

        if( res > 0 )
        {
            char c;
            read( fileno( stdin ), &c, 1 );
            cout << c << endl;
           // Serial.writeline("I");
            
            if (c == 'w'){Serial.writeline("w");}
            else if (c == 'a'){Serial.writeline("a");}
            else if (c == 's'){Serial.writeline("s");}
            else if (c == 'd'){Serial.writeline("d");}



            
        }
        else if( res < 0 )
        {
            break;
        }
        else
        {
        }
    }

    tcsetattr( fileno( stdin ), TCSANOW, &oldSettings );
    return 0;
}