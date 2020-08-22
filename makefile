CPPFLAGS = -g -Wall -pthread -D __LINUX__
LDFLAGS = -g -Wall -pthread
LDLIBS = -lThread

demo: demo.o AsyncProcManager.o
	g++ $(LDFLAGS) -o demo demo.o AsyncProcManager.o

demo.o: demo.c
	g++ $(CPPFLAGS) -c demo.c
	
AsyncProcManager.o: AsyncProcManager.cpp
	g++ $(CPPFLAGS) -c AsyncProcManager.cpp