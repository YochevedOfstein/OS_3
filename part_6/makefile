CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pg

.PHONY: all clean

SRCS_SERVER = server.cpp Graph.cpp
TARGETS_SERVER = server

SRCS_CLIENT = client.cpp
TARGETS_CLIENT = client

LIBDIR = ../part_5
LIBREACT = $(LIBDIR)/libreactor.a

all: lib $(TARGETS_SERVER) $(TARGETS_CLIENT)

lib: 
	$(MAKE) -C $(LIBDIR)

$(TARGETS_SERVER): $(SRCS_SERVER) $(LIBREACT)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBREACT)

$(TARGETS_CLIENT): $(SRCS_CLIENT)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGETS_SERVER) $(TARGETS_CLIENT) *.o gmon.out
	$(MAKE) -C $(LIBDIR) clean

