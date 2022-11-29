# Software Diverse Redundancy Library

## Compiling

### Compiling the library

Compile the library using

	make

In order to cross-compile the library (for example in an x86 computer)

	TARGET=cross make

### Compiling the demo

Compile the library using

	make demo

In order to cross-compile the library (for example in an x86 computer)

	TARGET=cross make demo

## Running
To run the demo simply execute bin/demo on a RISC-V Linux machine with perf support. To enable perf support, add the following line to your kernel configuration:

	CONFIG_PERF_EVENTS=y

