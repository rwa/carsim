
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 `sdl2-config --cflags`
LDLIBS = `sdl2-config --libs`

all: carsim.cpp
	$(CXX) $(CXXFLAGS) -o main carsim.cpp $(LDLIBS)

clean:
	rm -f main

