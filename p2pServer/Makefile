MUDUO_DIRECTORY ?= ./muduo
#MUDUO_DIRECTORY ?= $(HOME)/build/install
MUDUO_INCLUDE = $(MUDUO_DIRECTORY)/include
MUDUO_LIBRARY = $(MUDUO_DIRECTORY)/lib
SRC = .
app = p2pServer

CXXFLAGS = -Wall -Wextra -Werror \
	   -Wconversion -Wno-unused-parameter \
	   -Wold-style-cast -Woverloaded-virtual \
	   -Wpointer-arith -Wshadow -Wwrite-strings \
	   -march=native -rdynamic \
	   -I$(MUDUO_INCLUDE)

LDFLAGS = -L$(MUDUO_LIBRARY) -lmuduo_net -lmuduo_base -lpthread -lrt

debug: p2pServerDebug

release: p2pServerRelease

clean:
	rm -f p2pServer core

p2pServerDebug: main.cc p2pServer.cc
	g++ $(CXXFLAGS) -g -O0 -o $(app) $^ $(LDFLAGS)

p2pServerRelease: main.cc p2pServer.cc
	g++ $(CXXFLAGS) -O2 -o $(app) $^ $(LDFLAGS)

.PHONY: debug release clean
