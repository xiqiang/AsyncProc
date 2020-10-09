CPPFLAGS = -g -Wall -pthread -D __LINUX__
LDFLAGS = -g -Wall -pthread
LDLIBS = -lThread
SRCS := $(wildcard *.cpp) $(wildcard base/*.cpp) $(wildcard statistic/*.cpp)
OBJS := $(patsubst %.cpp, %.o, $(SRCS))

demo: $(OBJS)
	g++ $(LDFLAGS) -o demo $(OBJS)

clean:
	rm -f *.o
	rm -f demo

rebuild: clean demo
