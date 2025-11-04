# Makefile for ipasirup-graph2

CXX      ?= g++
CXXFLAGS ?= -O2 -std=c++17
CADICAL  ?= $(HOME)/github/cadical

TARGET := ipasirup-graph
SRC    := ipasirup-graph.cpp

INCLUDES := -I"$(CADICAL)/src"
LIBS     := "$(CADICAL)/build/ipasir.o" "$(CADICAL)/build/libcadical.a"

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(INCLUDES) $(LIBS)

clean:
	rm -f $(TARGET)
