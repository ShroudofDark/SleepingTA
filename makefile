MAINPROG=prodCon # Replace this with your desired program name

SOURCES:=$(wildcard *.cpp)
OBJECTS=$(SOURCES:.cpp=.o)
# compiler
CC = g++

# compiler flags
# -g adds debugging info to exe
# -Wall turns off most compiler warnings
CFLAGS = -g -std=c++17 -Wall -w -pthread

# the build target executable:
TARGET = prodCon

all: $(SOURCES) $(MAINPROG)

$(MAINPROG): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@
	
.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm *.o $(MAINPROG)
