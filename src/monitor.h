/*
 * Copyright 2022 Barcelona Supercomputing Center
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE	 OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef MONITOR_H
#define MONITOR_H

#define ONON 0x0000
#define OFFOFF 0x0101
#define ONOFF 0x0001
#define OFFON 0x0100

#define HA 2
#define HD 0
#define TA 5
#define TS 4
#define TD 3

#define RUN 0
#define STALL 1


//#define SCHEDULER_START SCHED_FIFO
#define SCHEDULER_START SCHED_OTHER
#define SCHEDULER_END SCHED_OTHER

#define KILL_SUCCESSFUL 0

#define AND &&
#define OR ||

#define ARGV_SIZE 64
/*
*	Input and Output data structure
*/
void * input_share_head[ARGV_SIZE];
void * input_share_trail[ARGV_SIZE];
void * out_share_head[ARGV_SIZE] ;
void * out_share_trail[ARGV_SIZE] ;

// Defines how apart (in number of instructions executed) each copy of the program should be running
long long WINDOWS_INSTRUCTION = 50000;

//std::chrono::milliseconds TIMEOUT_THRESHOLD = 10ms;
std::chrono::milliseconds TIMEOUT_THRESHOLD = std::chrono::milliseconds(20);

typedef enum { NODUP, DUP } in_mode;
int i_stack = -1;

/*
	shmen
	shmem[0] ready head
	shmem[1] ready trail
	
	shmem[2] active waiting before execute head
	shmem[3] active waiting before execute trail

	shmem[4] finish execution head
	shmem[5] finish execution trail
*/

void * shmem ; 

typedef struct pro_res{

	bool safe;
	long long head_ns;
	long long trail_ns;
	long long head_instr;
	long long trail_instr;

}pro_res;

pro_res protect_real_waitpid_selene(void  (* function )(void * [] ,  void * [] ),void * argv_input_head[] ,void * argv_input_trail[] ,int * input_size[] ,void * argv_output_head[] ,void * argv_output_trail[] ,int * output_size[]);
pro_res protect_real_waitpid(void  (* function )(void * [] ,  void * [] ),void * argv_input_head[], void * argv_input_trail[] , int * input_size[] , void * argv_output_head[] ,	void * argv_output_trail[] ,int * output_size[]);
pro_res protect_real_sh(void  (* function )(void * [] ,  void * [] ),void * argv_input_head[], void * argv_input_trail[] , int * input_size[] , void * argv_output_head[] ,	void * argv_output_trail[] ,int * output_size[]);


bool protect_default( void  (* function )(void * [] ,  void * [] ), void * argv_input[] ,int * input_size[] , void * argv_output[] ,int * output_size[]);
bool protect_input(void  (* function )(void * [], void * [] ), void * argv_input[] ,int * input_size[] , void * argv_output[] , int * output_size[]	);
bool protect_output(void  (* function )(void * [], void * [] ),void * argv_input[] ,int * input_size[] , void * argv_output[] , int * output_size[]	);
bool protect_input_output(void  (* function )(void * [], void * [] ), void * argv_input[] ,int * input_size[] , void * argv_output[] , int * output_size[]);



bool protect_def(void(* function )(void * [], void * [] ), void * argv_input[] ,int * input_size[] , void * argv_output[] , int * output_size[]);
bool protect_def_out(void(* function )(void * [], void * [] ), void * argv_input[] ,int * input_size[] , void * argv_output[] , int * output_size[]);
bool protect_def_inp(void(* function )(void * [], void * [] ), void * argv_input[] ,int * input_size[] , void * argv_output[] , int * output_size[]);
bool protect_def_out_inp(void(* function )(void * [], void * [] ), void * argv_input[] ,int * input_size[] , void * argv_output[] , int * output_size[]);
bool protect_def_inp_out(void(* function )(void * [], void * [] ), void * argv_input[] ,int * input_size[] , void * argv_output[] , int * output_size[]);




/*
*	shared memory function
*/
/*h*/
void* create_shared_memory(size_t size);
void free_shared_memory(void * address , size_t size);
void clean_sh_memory();
bool isMemoryEqual(void * ptr1 , void * ptr2, int bytes );
int getArgvSize( void * argv[]);
void debug_execution( pro_res result);
inline void endWorker(struct timespec * time , bool * flag);
bool isResultsEqual( int argv_size);
char getState(pid_t pid);
void work( worker * w, void * shmem , void  (* function )(void * [], void * [] ), void * argv_input[] , void * argv_output[] );
void init_workers(worker * head, worker * trail ,  pid_t head_pid , pid_t trail_pid);


#ifdef active
void debug_active(pid_t head, pid_t trail, bool stag);
#endif
#ifdef instr
	void debug_instr( long long head, long long trail);
#endif


void set_input(void * argv_input[], int * input_size[], in_mode in);
void set_output(void * argv_output[], int * output_size[]);
void save_result(void * argv_output[], int * output_size[]);
void free_input(void * argv_input[], int * input_size[], in_mode in);
void free_output(void * argv_output[], int * output_size[]);
void check_perf_open(worker * head, worker * trail);
void read_add_reset(long long * instr, worker * head, worker * trail);


/*impl*/
void* create_shared_memory(size_t size) {
	// Our memory buffer will be readable and writable:
	int protection = PROT_READ | PROT_WRITE;

	// The buffer will be shared (meaning other processes can access it), but
	// anonymous (meaning third-party processes cannot obtain an address for it),
	// so only this process and its children will be able to use it:
	int visibility = MAP_SHARED | MAP_ANONYMOUS;

	// The remaining parameters to `mmap()` are not important for this use case,
	// but the manpage for `mmap` explains their purpose.
	return mmap(NULL, size, protection, visibility, -1, 0);
}
void free_shared_memory(void * address , size_t size){
	munmap(address, size);
}
void clean_sh_memory(){
	memset(&input_share_head[0],0x0, sizeof(void * ) * ARGV_SIZE);
	memset(&input_share_trail[0],0x0, sizeof(void * ) * ARGV_SIZE);
	memset(&out_share_head[0],0x0, sizeof(void * ) * ARGV_SIZE);
	memset(&out_share_trail[0],0x0, sizeof(void * ) * ARGV_SIZE);
}
inline void endWorker(struct timespec * time , bool * flag){
		clock_gettime(CLOCK_MONOTONIC, time);
		(*flag) = true ; 
}
bool isMemoryEqual(void * ptr1 , void * ptr2, int bytes ){
	bool equal = true;

	unsigned char * ptr1_local = (unsigned char *)ptr1;
	unsigned char * ptr2_local = (unsigned char *)ptr2;

	for(int i=0; i<bytes && equal;i++){
		if( ptr1_local[i] != ptr2_local[i] ) equal = false;
	}
	return equal;
}
int getArgvSize( void * argv[]){
	int size = 0;

	int i=0;
	while(argv[i] != NULL)i++;
	
	size = i ;
	return size;
}
bool isResultsEqual(void * argv_output[],int * output_size[]){
	
	int argv_size = getArgvSize(argv_output);
	bool flag = true ;


	for(int i=0;i<argv_size && i<ARGV_SIZE && flag;i++){
		flag = isMemoryEqual(out_share_head[i], out_share_trail[i], *output_size[i]);
	}

	return flag ; 
}

void check_perf_open(worker * head, worker * trail){

	if( (*head).getFD(instructions) == -1 OR (*trail).getFD(instructions) == -1){
		fprintf(stderr,"Error perf_open\n");
		kill((*head).getworkerPid(),SIGKILL);
		kill((*trail).getworkerPid(),SIGKILL);
		exit(EXIT_FAILURE);
	}
}

std::string to_formatted(long long num);

#endif
