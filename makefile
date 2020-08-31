CPPFLAGS = -g -Wall -pthread -D __LINUX__
LDFLAGS = -g -Wall -pthread
LDLIBS = -lThread

demo: demo.o AsyncProcManager.o AsyncProcThread.o
	g++ $(LDFLAGS) -o demo demo.o AsyncProcManager.o AsyncProcThread.o

demo.o: demo.cpp
	g++ $(CPPFLAGS) -c demo.cpp
	
AsyncProcManager.o: AsyncProcManager.cpp
	g++ $(CPPFLAGS) -c AsyncProcManager.cpp

AsyncProcThread.o: AsyncProcThread.cpp
	g++ $(CPPFLAGS) -c AsyncProcThread.cpp

clean:
	rm -f *.o
	rm -f demo

rebuild: clean demo
