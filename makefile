CPPFLAGS = -g -Wall -pthread -D __LINUX__
LDFLAGS = -g -Wall -pthread
LDLIBS = -lThread

demo: demo.o AsyncProcThread.o
	g++ $(LDFLAGS) -o demo demo.o AsyncProcThread.o

demo.o: demo.c
	g++ $(CPPFLAGS) -c demo.c
	
AsyncProcThread.o: AsyncProcThread.cpp
	g++ $(CPPFLAGS) -c AsyncProcThread.cpp

clean:
	rm -f *.o
	rm -f demo