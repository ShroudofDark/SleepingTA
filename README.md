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