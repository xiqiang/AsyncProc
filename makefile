CPPFLAGS = -g -Wall -pthread -D __LINUX__
LDFLAGS = -g -Wall -pthread
LDLIBS = -lThread
SRCS := $(wildcard *.cpp) $(wildcard base/*.cpp) $(wildcard statistic/*.cpp)
OBJS := $(patsubst %.cpp, %.o, $(SRCS))
EXECUTABLE :=demo

demo: $(OBJS)
	g++ $(LDFLAGS) -o $(EXECUTABLE) $(OBJS)

clean:
	rm -f *.o
	rm -f base/*.o
	rm -f statistic/*.o
	rm -f $(EXECUTABLE)

rebuild: clean demo
