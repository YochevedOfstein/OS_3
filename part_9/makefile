CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pg

.PHONY: all clean

SRCS_SERVER = server.cpp Graph.cpp reactor.cpp
TARGETS_SERVER = server

SRCS_CLIENT = client.cpp
TARGETS_CLIENT = client

all: $(TARGETS_SERVER) $(TARGETS_CLIENT)

$(TARGETS_SERVER): $(SRCS_SERVER)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TARGETS_CLIENT): $(SRCS_CLIENT)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGETS_SERVER) $(TARGETS_CLIENT) *.o gmon.out

