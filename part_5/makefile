CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pg

.PHONY: all clean

SRCS = reactor.cpp
TARGETS = REACTOR

all: $(TARGETS)

$(TARGETS): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGETS) *.o
