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

## Prequisites

For testing in multicore on the board, change the scheduler in monitor.h defines.
Just the start_scheduler, the end_scheduler should remain SCHED_OTHER (default of the system).

The defines of binding and scheduling put into effect in worker.cc functions.

Perf configuration bit, mask ecc is in worker.cc .
