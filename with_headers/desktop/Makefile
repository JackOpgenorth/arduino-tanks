OBJS =serialport.o desktop.o
CFLAGS= -c -Wall -std=c++11

desktop:$(OBJS)
	g++ $(OBJS) -o desktop

serialport.o: serialport.h serialport.cpp
	g++ serialport.cpp -o serialport.o $(CFLAGS)

desktop.o:serialport.h desktop.cpp
	g++ desktop.cpp -o desktop.o $(CFLAGS)

clean:
	@rm -f $(OBJS) $(EXE) desktop
