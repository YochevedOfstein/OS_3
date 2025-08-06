CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pg

.PHONY: all clean

SRCS = server.cpp Graph.cpp 
TARGETS = CH_PROGRAM

all: $(TARGETS)

$(TARGETS): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGETS) *.o gmon.out

