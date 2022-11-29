# SafeSoftDR (a Safe Software Diverse Redundancy library)

Applications with safety requirements have become ubiquitous nowadays and can be found in edge devices of all kinds. However, microcontrollers in those devices, despite offering moderate performance by implementing multicores and cache hierarchies, may fail to offer adequate support to implement some safety measures needed for the highest integrity levels, such as lockstepped execution to avoid so-called common cause failures (i.e., a fault affecting redundant components causing the same error in all of them). To respond to this limitation, an approach based on a software monitor enforcing some sort of software-based lockstepped execution across cores has been proposed recently, providing a proof of concept. This paper presents SafeSoftDR, a library providing a standard interface to deploy software-based lockstepped execution across non-natively lockstepped cores relieving end-users from having to manage the burden to create redundant processes, copying input/output data, and performing result comparison. Our library has been tested on x86-based Linux and is currently being integrated on top of an open-source RISC-V platform targeting safety-related applications, hence offering a convenient environment for safety-critical applications.

"SafeSoftDR: A Library to Enable Software-based Diverse Redundancy for Safety-Critical Tasks" (to appear)
Fabio Mazzocchetti, Sergi Alcaide, Francisco Bas, Pedro Benedicte, Guillem Cabo, Feng Chang, Francisco Fuentes, Jaume Abella
FORECAST 2022 Functional Properties and Dependability in Cyber-Physical Systems Workshop (held jointly with HiPEAC Conference)
Budapest (Hungary), June 21 2022

```
@misc{https://doi.org/10.48550/arxiv.2210.00833,
  doi = {10.48550/ARXIV.2210.00833},
  url = {https://arxiv.org/abs/2210.00833},
  author = {Mazzocchetti, Fabio and Alcaide, Sergi and Bas, Francisco and Benedicte, Pedro and Cabo, Guillem and Chang, Feng and Fuentes, Francisco and Abella, Jaume},
  keywords = {Hardware Architecture (cs.AR), FOS: Computer and information sciences, FOS: Computer and information sciences},
  title = {SafeSoftDR: A Library to Enable Software-based Diverse Redundancy for Safety-Critical Tasks},
  publisher = {arXiv},
  year = {2022},
  copyright = {Creative Commons Attribution Non Commercial No Derivatives 4.0 International}
}

```

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
