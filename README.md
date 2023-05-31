# Overview

This project is the Sleeping TA problem done via software. It uses semaphores and threads to simulate the experience. I would recommend changing the defined MINUTES_AS_SECONDS to a number smaller than 60 to have the program run faster, as a requirement for writing this program originally was to have it wait accurately to real time. The overall statistics will scale appropriately to that number being changed.

Below is how to run the code, if there are any issues please let me know. 

Sleeping Barber: https://en.wikipedia.org/wiki/Sleeping_barber_problem

The sleeping TA is the same problem, but in this case its just a teacher assistant helping students instead of a barber. 

If you want to create your own test data in a text document, the columns are identified as such:

Student ID, First Arrival Time, Time needed from TA

# Requirements

	* Make
	* g++ (GCC) 11.2.0 or newer
	* run on UNIX system (pthreads are used)

# Compilation

The code can be compiled with the provided makefile using the `make` command.

Include these flags if compiling the code manually:

```
CFLAGS = -g -std=c++17 -Wall -w -pthread

```

`make clean` to clear out object files and run `make` to remake them

# Sample Execution


./prodCon.exe testData1.txt
./prodCon.exe testData2.txt

# Your make may make these execution lines instead (aka the .exe removed):

./prodCon testData1.txt
./prodCon testData2.txt
