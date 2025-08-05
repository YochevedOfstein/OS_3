CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pg

.PHONY: all clean

all: CH

CH: main.o
	$(CXX) $(CXXFLAGS) -o CH main.o

main.o: main.cpp Point.hpp
	$(CXX) $(CXXFLAGS) -c main.cpp

clean:
	rm -f CH *.o gmon.out profile.txt

