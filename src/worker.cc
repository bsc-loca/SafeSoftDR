/*
 * Copyright 2022 Barcelona Supercomputing Center
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "worker.h"
#include <stdio.h>
#include <unistd.h>

#include <iostream>

//MEMSET
#include <string.h>
#include <string>


//PERF
#include <linux/perf_event.h> //perf_event_open
#include <linux/hw_breakpoint.h> //perf_event_open
#include <sys/syscall.h>

//ioctl
#include <sys/ioctl.h>


//SCHEDULING
#include <sys/resource.h>


#include <vector>

using namespace std;

worker::worker(){
	this->pid = -1 ;
	this->cpu = -1;
}

worker::worker(wtype type){
	this->type = type;
	this->pid = -1 ;
	this->cpu = -1;
}

/*
* SETTER
*/

void worker::setworkerPid(int pid){
	this->pid = pid;
}
void worker::setCPU(int cpu){
	this->cpu = cpu;
}
void worker::setBindOnCPU(){
#ifdef binding
	cpu_set_t  mask;
	CPU_ZERO(&mask);
	CPU_SET(this->cpu, &mask);
	sched_setaffinity(getpid(), sizeof(mask), &mask);  
#endif
}
void worker::setFD(int fd, int i){
	this->fd[i] = fd ; 
}

void worker::setScheduler( int scheduler_policy){
#ifdef scheduler
	struct sched_param param; memset(&param, 0x0, sizeof(param));
	param.sched_priority = sched_get_priority_max(scheduler_policy);
	sched_setscheduler(0, scheduler_policy , &param);
#endif
}
/*
* GETTER
*/

wtype worker::getType(){
	return this->type;
}

int worker::getworkerPid(){
	return this->pid;
}

int worker::getHWInstruction_fd(){

	struct perf_event_attr pe;
	int fd; //file descriptor to be used

	memset(&pe, 0, sizeof(struct perf_event_attr));

	//Configure the options
	pe.type = PERF_TYPE_HARDWARE;
	pe.config = PERF_COUNT_HW_INSTRUCTIONS;
	pe.disabled = 1; //start disabled
	pe.size = sizeof(struct perf_event_attr);
	pe.exclude_hv = 1; //exclude hypervisor
	pe.exclude_kernel = 1; //exclude kernel
	pe.exclude_user = 0;
	pe.precise_ip = 0;
	pe.pinned = 0;
		
	fd = syscall(__NR_perf_event_open, &pe, this->pid, -1, -1, 0);
	
	return fd;
}

int worker::getCycles_fd(){
	struct perf_event_attr pe;
	int fd; //file descriptor to be used

	memset(&pe, 0, sizeof(struct perf_event_attr));

	//Configure the options
	pe.type = PERF_TYPE_HARDWARE;
	pe.config = PERF_COUNT_HW_CPU_CYCLES;
	pe.disabled = 1; //start disabled
	pe.size = sizeof(struct perf_event_attr);
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;
    pe.exclude_idle = 1 ;
    pe.exclude_user = 0  ;
    pe.pinned = 1 ;

	fd = syscall(__NR_perf_event_open, &pe, this->pid, this->cpu, -1, 0);

	return fd;

}


int worker::getFD(int i){
	return this->fd[i];
}

int worker::getCPU(){
	return this->cpu ;
}


long long worker::getHWInstruction(int fd){
  long long buffer;
  read(fd,&buffer, sizeof(buffer)); 
  return buffer;
}

long long worker::getCycles(int fd){
  long long buffer;
  read(fd,&buffer, sizeof(buffer)); 
  return buffer;
}
void  worker::resetPMC_at(int fd){
	ioctl(fd, PERF_EVENT_IOC_RESET, 0);
}
void  worker::enablePMC_at(int fd){
	ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
}

void  worker::disablePMC_at(int fd){
	ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
}

void worker::schedulePriority(int priority){
	int which = PRIO_PROCESS;
	int ret = setpriority(which, getpid(), priority);
}

void worker::lockCPU(int cpu){
	this->setCPU(cpu);
	this->setBindOnCPU();
}
