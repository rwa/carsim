
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 `sdl2-config --cflags`
LDLIBS = `sdl2-config --libs` -lSDL2_ttf

# define the executable file 
MAIN = simcar

# define the C++ source files
SRCS = linesensor.cpp maze.cpp carsim.cpp

# define the C++ object files 
OBJS = $(SRCS:.cpp=.o)

.PHONY: depend clean

all: $(MAIN)
	@echo  Program has been compiled

$(MAIN): $(OBJS) 
	$(CXX) $(CXXFLAGS) -o $(MAIN) $(OBJS) $(LDLIBS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

depend: $(SRCS)
	makedepend $^

# DO NOT DELETE THIS LINE -- make depend needs it
