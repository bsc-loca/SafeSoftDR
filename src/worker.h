/*
 * Copyright 2022 Barcelona Supercomputing Center
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef WORKER_H
#define WORKER_H

#include <string>
#include <iostream>
#include <cstring>
#include <vector>


#define HEAD_TRAIL_PRIORITY 0
#define MONITOR_PRIORITY -10

enum metrics { instructions, cycles };  
enum wtype { monitor_t, head_t , trail_t};

class worker {

	public:
		/*
		*	CONSTRUCTOR
		*/
		worker();
		worker(wtype);

		/*
		*	SETTER
		*/
		void setworkerPid(int pid);
		void setCPU(int cpu);
		void setSizeRank(int size);
		void setBindOnCPU();
		void setFD(int fd, int i);
		void setScheduler( int scheduler_policy);	
		/*
		*	GETTER
		*/
		int getCPU();
		int getworkerPid();
		wtype getType();
		
		/*
		*	get read values
		*/
		long long getHWInstruction(int fd);
		long long getCycles(int fd);

		/*
		*	get file descriptor
		*/
		int getHWInstruction_fd();
		int getCycles_fd();
		int getFD(int i);
		/*
		*	VOID
		*/
		void resetPMC_at(int fd);
		void enablePMC_at(int fd);
		void disablePMC_at(int fd);
		void schedulePriority(int priority);
		void lockCPU(int cpu);

	private: 
		wtype type;
		int cpu;
		int pid;
		int fd[4];
};

#endif
