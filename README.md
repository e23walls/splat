# splat
====

Splat is a CHIP-8 emulator.
For more information about CHIP-8, see http://en.wikipedia.org/wiki/CHIP-8

####Compiling and running the code:

`$ g++ stateData.cpp opcodeProcessing.cpp main.cpp -o SPLAT -Wall -pedantic -lm -O2 -L/usr/X11R6/lib -std=c++11 -pthread -lX11`

`$ ./SPLAT path_to_CHIP-8_ROM_file > output.txt  # Redirect the output to a text file so you don't have to see it while running the program. Also, path_to_CHIP-8_ROM_file can be relative from the current directory, or absolute.`

*End program using **Ctrl+c or Ctrl+z***

Wiki (for collaborators): http://chip8emulator.pbworks.com/w/page/82128005/And%20now%20it's%20time%20for…%20the%20Front%20Page

####Background

This project was created out of a desire to learn more about emulation. After doing some research, it was discovered that one of the easiest things to emulate is CHIP-8. So, we began this project. Note that all CHIP-8 ROMs are public domain, and can be found by simple Google queries.
