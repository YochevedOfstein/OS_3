CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pg
AR = ar rcs

.PHONY: all clean

LIBS = libreactor.a

all: $(LIBS)

reactor.o: reactor.cpp reactor.hpp
	$(CXX) $(CXXFLAGS) -c $<

proactor.o: proactor.cpp proactor.hpp
	$(CXX) $(CXXFLAGS) -c $<

$(LIBS): reactor.o proactor.o
	$(AR) $@ $^

clean:
	rm -f  *.o $(LIBS)
