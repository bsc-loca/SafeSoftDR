# SafeSoftDR (a Safe Software Diverse Redundancy library)

Applications with safety requirements have become ubiquitous nowadays and can be found in edge devices of all kinds. However, microcontrollers in those devices, despite offering moderate performance by implementing multicores and cache hierarchies, may fail to offer adequate support to implement some safety measures needed for the highest integrity levels, such as lockstepped execution to avoid so-called common cause failures (i.e., a fault affecting redundant components causing the same error in all of them). To respond to this limitation, an approach based on a software monitor enforcing some sort of software-based lockstepped execution across cores has been proposed recently, providing a proof of concept. This paper presents SafeSoftDR, a library providing a standard interface to deploy software-based lockstepped execution across non-natively lockstepped cores relieving end-users from having to manage the burden to create redundant processes, copying input/output data, and performing result comparison. Our library has been tested on x86-based Linux and is currently being integrated on top of an open-source RISC-V platform targeting safety-related applications, hence offering a convenient environment for safety-critical applications.

## Reference

If you use the SafeSoftDR for an academic purpose, please cite the following paper:

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
# How to use

This library is intended to protect a function by executing it in a staggered manner between two cores of a multicore system. The general usage is to compile the library in a static way [see Compiling](#compiling) and the call their functions.

In order to protect the function a wrapper of the function needs to be created with only two arguments, one array of void pointers for inputs and another for the outputs. To illustrate the process we will use an example.

Imagine we want to protect a simple matrix multiplication, the original function has 5 arguments, a pointer for each of the two input matrices (matA, matB), a pointer to the output matrix (matC), the number of rows and columns:

```
void matrix_multiply(int * matA, int * matB, int * matC , int rows , int cols){
  for(int i = 0; i < rows; ++i)
    for(int j = 0; j < cols; ++j)
      for(int k = 0; k < rows; ++k)
          matC[i * rows + j] += matA[i * rows + k] * matB[k * rows + j];
}
```
We would create a wrapper just like this one:
```
void matrix_multiply_wrapper(void * argv_input[], void * argv_output[] ){
  int *matA = (int * )argv_input[0];
  int *matB = (int * )argv_input[1];
  int *matC = (int * )argv_output[0];
  int rows = *(int *)argv_input[2];
  int cols = *(int *)argv_input[3];
  matrix_mutilply(matA, matB, matC, rows, cols);
}
```
The wrapper is the function that the library will call in order to execute the function.

## Types of functions
We have two set of functions that provide an equivalent functionality. The first set has a function for each type of protection (which we call ***direct***) and the second one has internal calls to reduce the amount of code compiled ((named ***chained***)). In our experiments we have seen similar performance for both of them.

The ***direct*** calls have the entire word in their name:
- protect_default
- protect_input
- protect_output
- ...

Whereas the ***chained*** version have an abreviation:
- protect_def
- protect_def_out
- protect_def_inp
- ...

## Levels of protection:

Different levels of protection are offered by the library based on the ```protect``` call you use (here we show the example for the ***chained*** version, but the same applies for the ***direct*** version)
 - **protect_def**: Executes a staggered execution in two cores, saves the result of one of them.
 - **protect_def_out**: Executes a staggered execution in two cores, compares the outputs and saves the result of one of them if fault free.
 - **protect_def_inp**: Duplicates the input of the calling functions and executes a staggered execution in two cores, saves the result of one of them.
 - **protect_def_inp_out**: Duplicates the input of the calling functions and executes a staggered execution in two cores, compares the results and saves the result of one of them if fault free.

## Calling the library functions

All the protection functions of the library have the same interface (group of arguments):
- A void pointer to the wrapper function
- An array of void pointers (containing the pointers to the input variables)
- An array of integer pointers (each integer identifies the size of the input variables)
- An array of void pointers (containing the pointers to the output variables)
- An array of integer pointers (each integer identifies the size of the outputs variables)

Following the example of the matrix multiplication:
```
void (*ptr)( void *[], void *[]) = &matrix_multiply_wrapper;
void * argv_input[] = { (void *)matA, (void *)matB, (void *)&rows, (void *)&cols, NULL};
void * argv_output[] = { (void *)matC, NULL};
int * input_size[] = {(int *)matAbytes, (int *)matBbytes, (int *)&rowsbytes, (int *)&colsbytes, NULL};
int * output_size[] = {(int *)matCbytes, NULL};
```

Once we have them, now we only need to decide which typo of protection we want based on [Levels of protection](#levels-of-protection).
For instance:
```
bool pass_flag = protect_default(ptr, argv_input, input_size, argv_output, output_size) ;
```

## Return value

Each function can return two different structs, either a ***boolean*** or a struct named ***pro_res***, defined in [monitor.h](https://gitlab.bsc.es/caos_hw/software-diverse-redundancy-library/-/blob/main/src/monitor.h#L72-L78) that we use to debug since it has stats of the executions:
```
typedef struct pro_res{
	bool safe; /** 0 if correct and compared, 1 otherwise */
	long long head_ns; /** Head execution time (in ns) */
	long long trail_ns; /** Trail execution time (in ns) */
	long long head_instr; /** Number of instructions commited during the activation by head core */
	long long trail_instr; /** Number of instructions commited during the activation by trail core */
}pro_res;
```
The boolean result is equivalent to the safe field in the struct.


# Compiling

As explained above the compilation of the library is made statically by calling the default g++. 

### Compiling the library

Compile the library using

	make

In order to cross-compile the library for risc-v (for example in an x86 computer) use:

	TARGET=cross make

This will call the compilers defined in the [Makefile (lines 2 and 3)](https://gitlab.bsc.es/caos_hw/software-diverse-redundancy-library/-/blob/main/Makefile#L2-L3)

### Compiling the example

Compile the library using

	make example

In order to cross-compile the library (for example in an x86 computer)

	TARGET=cross make example

## Running the example
To run the example simply execute bin/example on a RISC-V Linux machine with perf support. To enable perf support, add the following line to your kernel configuration:

	CONFIG_PERF_EVENTS=y


## Prequisites

For testing in multicore on the board, change the scheduler in monitor.h defines.
Just the start_scheduler, the end_scheduler should remain SCHED_OTHER (default of the system).

The defines of binding and scheduling put into effect in worker.cc functions.

Perf configuration bit, mask ecc is in worker.cc.

For some machines performance counter access may require to call ```sudo``` in front of the execution command
