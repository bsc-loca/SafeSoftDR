/*
 * Copyright 2022 Barcelona Supercomputing Center
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// #include  <mpi.h>    PROVIDES THE BASIC MPI DEFINITION AND TYPES 
#include <iostream>
#include <unistd.h>
#include <signal.h>

#include <bits/stdc++.h>
#include <string>

//TIME
#include <time.h>

#include <sys/types.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>


//NANO SLEEP
#include <chrono>
#include <thread>

//PERF
#include <linux/perf_event.h> //perf_event_open
#include <linux/hw_breakpoint.h> //perf_event_open
#include <sys/syscall.h>

//SHARED MEMORY
#include <stdlib.h>
#include <sys/mman.h>

//ioctl
#include <sys/ioctl.h>

//CPU SET 
#include <sched.h>
#include <iomanip>

//vector
#include <vector>


#include "worker.h"
#include "monitor.h"

#define BILLION  1000000000

using namespace std;

#define call protect_real_waitpid

#ifdef demo
#define LOG_SIZE (1024*1024)
class DemoLog{
	public: 
	static const unsigned char STAGGER = 0;
	static const unsigned char CONTINUE = 1;
	static const unsigned char EXITED = 2;
	static const unsigned char DONE = 3;
	static bool add_entry(int t, long long one, long long two);
	static void init();
	static void dump();
	private:
	static unsigned log_head;
	static long long proc1[LOG_SIZE];
	static long long proc2[LOG_SIZE];
	static unsigned char type[LOG_SIZE];
};

	unsigned DemoLog::log_head;
	long long DemoLog::proc1[LOG_SIZE];
	long long DemoLog::proc2[LOG_SIZE];
	unsigned char DemoLog::type[LOG_SIZE];
	void DemoLog::init() {
		log_head = 0;
	}
	void DemoLog::dump() {
		string infostr;
		for (int i=0;i<log_head; i++) {
			if (type[i] == STAGGER)
				infostr = "staggering ";
			else if (type[i] == CONTINUE) 
				infostr = "continuing ";
			else if (type[i] == EXITED) 
				infostr = "task exited";
			else if (type[i] == DONE) 
				infostr = "all exited ";
			else
				infostr = "unknown    ";

			cout << infostr + ": " << setw(15) <<
			to_formatted((long long)(proc1[i] - proc2[i]))  << "    (" <<  
			to_formatted((long long)(proc1[i]))  << " - " <<  to_formatted((long long)(proc2[i])) << ")" << endl;
		}
	}
	bool DemoLog::add_entry(int t, long long one, long long two)
	{
		//assert(log_head < LOG_SIZE);
		if (log_head >= LOG_SIZE){
			return false;
		}
		else {
			proc1[log_head] = one;
			proc2[log_head] = two;
			type[log_head] = t;
			log_head++;
		}
		return true;
	}

#endif

pro_res protect_wrapper(void  (* function )(void * [] ,  void * [] ),void * argv_input_head[] ,void * argv_input_trail[] ,int * input_size[],void * argv_output_head[],void * argv_output_trail[],int * output_size[]){
	return call(function,argv_input_head,argv_input_trail,input_size,argv_output_head,argv_output_trail,output_size);
}

pro_res protect_real_waitpid(void  (* function )(void * [] ,  void * [] ),void * argv_input_head[] ,void * argv_input_trail[] ,int * input_size[] ,void * argv_output_head[] ,void * argv_output_trail[] ,int * output_size[]){
	
	#ifdef debug
		std::vector<struct timespec> info(0); 
		std::vector<int> state(0);
		bool flag = false;
	#endif

	struct timespec * time_shared= (struct timespec *)create_shared_memory(3 * sizeof(timespec));
	struct timespec time_actual;
	memset(shmem,0x0,128);
	/*
	*        PIDs
	*/

	pid_t monitor_pid = getpid(); 
	pid_t head_pid = 0 ;
	pid_t trail_pid = 0 ; 

	/*
	*   WORKER
	*/
	
	worker monitor(monitor_t);
	worker head(head_t);
	worker trail(trail_t);

	monitor.lockCPU(0);

	head_pid = fork();	
	if(head_pid == 0 ) work( &head,shmem,function,input_share_head,out_share_head);
	
	trail_pid = fork();
	if(trail_pid == 0 ) work( &trail,shmem,function,input_share_head,out_share_head);
	/*
	* MONITOR CODE SECTION
	*/
	int head_status = -1 ;
	int trail_status = -1 ;
	/*
	* HEAD [0]
	* TRAIL [1]
	*/
	long long instructions_head_trail[2];  memset((void * )instructions_head_trail , 0x0, sizeof(long long) * 2);
	long long instructions_head_add = 0;
	long long sub = 0;
	
	/*
	* hv_tv[0] HEAD
	* hv_tv[1] TRAIL
	*/
	bool hv_tv[2]; memset( (void * ) hv_tv, 0b0,sizeof(bool) * 2);
	unsigned short * hv_tv_ptr = (unsigned short *)&hv_tv[0];

	/*
	* pid, fd
	*/
	init_workers(&head,&trail,head_pid,trail_pid);
	/*
	*	Check error perf open
	*/
	check_perf_open(&head, &trail);
	/*
	* if head trail file with perf 
	*/

	/*
	*  If HEAD,TRAIL are ready to execute
	*/
	
	while(( ((unsigned short *)shmem)[0]) != 0x0101 );
	
	clock_gettime(CLOCK_MONOTONIC, &time_shared[0]);

	head.enablePMC_at(head.getFD(instructions));
	trail.enablePMC_at(trail.getFD(instructions));
	
	/*
	* The staggering is produced only in the monitoring section of the monitor.
	* There is not staggering at the start of the execution of head and trail process.
	* If you want add the while in the same portion of code, in protect_real_waitpid_selene().
	*/

	/*
	*  START HEAD
	*/
	((unsigned char *)shmem)[2]= 0x1;
	head.resetPMC_at(head.getFD(instructions)); 

	/*
	* START TRAIL
	*/
	((unsigned char *)shmem)[3]= 0x1;
	trail.resetPMC_at(trail.getFD(instructions));

	/*
	* TIMEOUT COUNTERS VARIABLES
	*/
	std::chrono::time_point<std::chrono::high_resolution_clock> HeadTimeoutCounter = std::chrono::system_clock::now();
	std::chrono::time_point<std::chrono::high_resolution_clock> TrailTimeoutCounter = std::chrono::system_clock::now();
	
	monitor.setScheduler(SCHEDULER_START);
	
	#ifdef demo
	DemoLog::init();
	#endif

	/*
	* counter of number of stall
	*/
	int c = 0 ;
	while(1){

			waitpid(trail_pid, &trail_status, WNOHANG);
			if ( not hv_tv[1] AND WIFEXITED(trail_status) ){
				endWorker(&time_shared[2] , &hv_tv[1]);
			}

			waitpid(head_pid, &head_status, WNOHANG);
			if ( not hv_tv[0] AND WIFEXITED(head_status)){			
				endWorker(&time_shared[1] , &hv_tv[0]);
			}
			
			#ifdef demo
//				usleep(DEMO_MS * 1000);
			#endif

			/*
			* both alive
			*/
			if( not hv_tv[0] AND not hv_tv[1] ){

				//CHECK IF PROGRESS FOR TIMEOUT
				std::chrono::time_point<std::chrono::high_resolution_clock> Now = std::chrono::system_clock::now();
				if (head.getHWInstruction(head.getFD(instructions)) == 0){
					//no progress check timeout:
					//if timeout bigger than threshold kill process message timeout and exit
					if (Now - HeadTimeoutCounter >= TIMEOUT_THRESHOLD){
						#ifdef debug
						cout << "TIMEOUT reached for HEAD process" << endl;
						#endif
						// TODO KILL Process
					}
					//else do nothing
				}
				else{//progress made, renew timeout counter
					HeadTimeoutCounter = std::chrono::system_clock::now(); 
				}
				if (trail.getHWInstruction(trail.getFD(instructions)) == 0){
					//no progress check timeout:
					//if timeout bigger than threshold kill process message timeout and exit
					if (Now - TrailTimeoutCounter >= TIMEOUT_THRESHOLD){
						#ifdef debug
						cout << "TIMEOUT reached for TRAIL process" << endl;
						#endif
						// TODO KILL Process
					}
					//else do nothing
				}
				else{//progress made, renew timeout counter
					TrailTimeoutCounter = std::chrono::system_clock::now(); 
				}
				read_add_reset(&instructions_head_trail[0] ,&head,&trail);
				sub = instructions_head_trail[0] - instructions_head_trail[1];
				
				#ifdef instr
					debug_instr(instructions_head_trail[0],instructions_head_trail[1]);  
				#endif

				#ifdef active
					debug_active(head.getworkerPid(),trail.getworkerPid(),false);
				#endif

                

				/*
				* create stag
				*/
				if(sub < WINDOWS_INSTRUCTION){
					
					#ifdef demo
					DemoLog::add_entry(DemoLog::STAGGER, instructions_head_trail[0], instructions_head_trail[1]);
					#ifdef demoprint
					cout << "[+] staggering : " << setw(15) <<
					to_formatted((long long)(instructions_head_trail[0] - instructions_head_trail[1]))  << " " <<  
					to_formatted((long long)(instructions_head_trail[0]))  << ":" <<  to_formatted((long long)(instructions_head_trail[1])) <<  endl;
					#endif
					#endif

					
					if(kill(trail.getworkerPid(),SIGSTOP) == KILL_SUCCESSFUL){
						char state = getState(trail.getworkerPid());
						while(state == 'R' AND ! WIFEXITED(trail_status)){
							waitpid(trail_pid, &trail_status, WNOHANG);
							state = getState(trail.getworkerPid());
						}
					}else{
						waitpid(trail_pid, &trail_status, WNOHANG);
						if (WIFEXITED(trail_status) AND not hv_tv[1]) endWorker(&time_shared[2] , &hv_tv[1]);
					}
					read_add_reset(&instructions_head_trail[0] ,&head,&trail);

					while((instructions_head_trail[0] - instructions_head_trail[1]) < WINDOWS_INSTRUCTION ){
						//CHECK IF PROGRESS FOR TIMEOUT
						std::chrono::time_point<std::chrono::high_resolution_clock> Now = std::chrono::system_clock::now();
						if (head.getHWInstruction(head.getFD(instructions)) == 0){
							//no progress check timeout:
							//if timeout bigger than threshold kill process message timeout and exit
							if (Now - HeadTimeoutCounter >= TIMEOUT_THRESHOLD){
								#ifdef debug
								cout << "TIMEOUT reached for HEAD process" << endl;
								#endif
								// TODO Kill process
							}
						//else do nothing
						}
						else{//progress made, renew timeout counter
							HeadTimeoutCounter = std::chrono::system_clock::now(); 
						}

						instructions_head_trail[0] +=  head.getHWInstruction(head.getFD(instructions));
						head.resetPMC_at(head.getFD(instructions));

						#ifdef instr
							debug_instr(instructions_head_trail[0],instructions_head_trail[1]);
						#endif

						#ifdef active
							debug_active(head.getworkerPid(),trail.getworkerPid(),true);
						#endif

						sub = instructions_head_trail[0] - instructions_head_trail[1];
						waitpid(head_pid, &head_status, WNOHANG);
						/*
						* head death
						*/
						if (not hv_tv[0] AND WIFEXITED(head_status) ){
							endWorker(&time_shared[1] , &hv_tv[0]);
							instructions_head_trail[0] += head.getHWInstruction(head.getFD(instructions));
							head.resetPMC_at(head.getFD(instructions)); 
							break;
						}
					}
					c++;
					
					if(kill(trail.getworkerPid(),SIGCONT) == KILL_SUCCESSFUL){
						char state = getState(trail.getworkerPid());
						while( state != 'R'){
							if(state == 'Z')break;
							state = getState(trail.getworkerPid());
						}
					}
					
					#ifdef demo
						if(WIFEXITED(trail_status) OR WIFEXITED(head_status)){
							DemoLog::add_entry(DemoLog::EXITED, instructions_head_trail[0], instructions_head_trail[1]);
							#ifdef demoprint
							cout << "[+] task exited: "  << setw(15) <<
							to_formatted((long long)(instructions_head_trail[0] - instructions_head_trail[1]))  << " " <<  
							to_formatted((long long)(instructions_head_trail[0]))  << ":" <<  to_formatted((long long)(instructions_head_trail[1])) <<  endl;
							#endif
						}else{
							DemoLog::add_entry(DemoLog::CONTINUE, instructions_head_trail[0], instructions_head_trail[1]);
							#ifdef demoprint
							cout << "[+] continuing : "  << setw(15) <<
							to_formatted((long long)(instructions_head_trail[0] - instructions_head_trail[1]))  << " " <<  
							to_formatted((long long)(instructions_head_trail[0]))  << ":" <<  to_formatted((long long)(instructions_head_trail[1])) <<  endl;
							#endif
						}
					
					#endif
				
				}
				
			}else{
				//HEAD Finished 
					
							waitpid(head_pid, &head_status, WNOHANG);
			if ( not hv_tv[0] AND WIFEXITED(head_status)){			
				endWorker(&time_shared[1] , &hv_tv[0]);
			}

				//HEAD Finished (hv_tv[0] == TRUE)
												#ifdef debug
								cout << "TIMEOUT reached for HEAD process" << endl;
								#endif
							
			}
			
	

			/*
			* both death
			*/
			if( hv_tv[0] AND hv_tv[1] ){
				read_add_reset(&instructions_head_trail[0] ,&head,&trail);
				#ifdef instr
					debug_instr(instructions_head_trail[0],instructions_head_trail[1]);
				#endif

				#ifdef active
					debug_active(head.getworkerPid(),trail.getworkerPid(),false);
				#endif

				#ifdef demo
					//DemoLog::add_entry(DemoLog::DONE, instructions_head_trail[0], instructions_head_trail[1]);
					#ifdef demoprint
					cout << "[+] all exited : " << setw(15) <<
					to_formatted((long long)(instructions_head_trail[0] - instructions_head_trail[1]))  << " " <<  
					to_formatted((long long)(instructions_head_trail[0]))  << ":" <<  to_formatted((long long)(instructions_head_trail[1])) <<  endl;
					#endif
				#endif

				break;
			}
			
		}
#ifdef demo
	DemoLog::dump();
#endif
	monitor.setScheduler(SCHEDULER_END);

	long long head_ns = (time_shared[1].tv_sec - time_shared[0].tv_sec ) * BILLION + 
												(time_shared[1].tv_nsec - time_shared[0].tv_nsec );

	long long trail_ns = (time_shared[2].tv_sec - time_shared[0].tv_sec ) * BILLION + 
												(time_shared[2].tv_nsec - time_shared[0].tv_nsec );

	bool good = false;
	
	head.disablePMC_at(head.getFD(instructions));
	trail.disablePMC_at(trail.getFD(instructions));

	close(head.getFD(instructions)) ;
	close(trail.getFD(instructions)) ;

	pro_res res;

	res.safe = good;
	res.head_ns = head_ns;
	res.trail_ns = trail_ns;
	res.head_instr = instructions_head_trail[0];
	res.trail_instr = instructions_head_trail[1];

	#ifdef demo
		//debug_execution(res);
	#endif

	return res;
}

bool protect_default(void  (* function )(void * [] ,  void * [] ),void * argv_input[],int * input_size[],void * argv_output[],int * output_size[]){

	clean_sh_memory();
	shmem = create_shared_memory(128);

	set_input(argv_input,input_size,NODUP);
	set_output(argv_output,output_size);

	pro_res safe = protect_wrapper(function,input_share_head,input_share_trail,input_size,out_share_head,out_share_trail, output_size);

	save_result(argv_output,output_size);

	free_shared_memory(shmem,128);
	free_input(argv_input,input_size,NODUP);
	free_output(argv_output,output_size);
	

	#ifdef debug
		debug_execution(safe);
	#endif

	return safe.safe ; 
}

bool protect_input(void  (* function )(void * [], void * [] ),void * argv_input[],int * input_size[],void * argv_output[],int * output_size[]){
	
	clean_sh_memory();
	shmem = create_shared_memory(128);

	set_input(argv_input,input_size,DUP);
	set_output(argv_output,output_size);

	pro_res safe = protect_wrapper(function,input_share_head,input_share_trail,input_size,out_share_head,out_share_trail, output_size);
	
	save_result(argv_output,output_size);

	free_shared_memory(shmem,128);
	free_input(argv_input,input_size,DUP);
	free_output(argv_output,output_size);

#ifdef debug
	debug_execution(safe);
#endif

	return safe.safe ; 
}

bool protect_output(void  (* function )(void * [], void * [] ),void * argv_input[],int * input_size[],void * argv_output[],int * output_size[]){

	clean_sh_memory();
	shmem = create_shared_memory(128);
	
	set_input(argv_input,input_size,NODUP);
	set_output(argv_output,output_size);

	pro_res safe = protect_wrapper(function,input_share_head,input_share_trail,input_size,out_share_head,out_share_trail, output_size);

	save_result(argv_output,output_size);
	
	if(safe.safe) safe.safe = isResultsEqual(argv_output,output_size);

	free_shared_memory(shmem,128);
	free_input(argv_input,input_size,NODUP);
	free_output(argv_output,output_size);

	#ifdef debug
		debug_execution(safe);
	#endif

	return safe.safe ; 
}

bool protect_input_output(void  (* function )(void * [], void * [] ),void * argv_input[],int * input_size[],void * argv_output[] ,int * output_size[]){

	clean_sh_memory();
	shmem = create_shared_memory(128);
	
	set_input(argv_input,input_size,DUP);
	set_output(argv_output,output_size);

	pro_res safe = protect_wrapper(function,input_share_head,input_share_trail,input_size,out_share_head,out_share_trail, output_size);
	
	save_result(argv_output,output_size);

	if(safe.safe) safe.safe = isResultsEqual(argv_output,output_size);

	free_shared_memory(shmem,128);
	free_input(argv_input,input_size,DUP);
	free_output(argv_output,output_size);


	#ifdef debug
		debug_execution(safe);
	#endif

	return safe.safe;
}

bool protect_def(void  (* function )(void * [], void * [] ), void * argv_input[] ,int * input_size[] , void * argv_output[] , int * output_size[]){

	/*protect_n*/
	if(i_stack == -1){
		i_stack = 0 ; 

		clean_sh_memory();
		shmem = create_shared_memory(128);

		/*create input*/
		set_input(argv_input,input_size,NODUP);

		/*create output*/
		set_output(argv_output,output_size);
	}

	pro_res res  = protect_wrapper(function,input_share_head,input_share_trail,input_size,out_share_head,out_share_trail, output_size);

	/* copy head results */
	save_result(argv_output,output_size);


	/*protect_n*/
	if(i_stack == 0){
		/*free input output */
		i_stack = -1 ;
		
		/* free shared memory*/
		free_shared_memory(shmem,128);
		/*free input*/
		free_input(argv_input,input_size,NODUP);
		/*free output*/
		free_output(argv_output,output_size);

	}

	#ifdef debug
		debug_execution(res);
	#endif

	return res.safe;
}

bool protect_def_out(void  (* function )(void * [], void * [] ), void * argv_input[] ,int * input_size[] , void * argv_output[] , int * output_size[]){

	/*protect_no*/
	if(i_stack == -1 ){
		
		i_stack = 1;

		clean_sh_memory();

		/*create general*/
		shmem = create_shared_memory(128);
		/*create input*/
		set_input(argv_input,input_size,NODUP);

		/*create output*/
		set_output(argv_output,output_size);
	}

	/*protect_noi*/
	else if ( i_stack == 2 ){/*caller do*/ //SA: To Check what is this
	}
	
	bool safe = protect_def(function,argv_input,input_size,argv_output,output_size);


	/*protect_no*/
	if( i_stack == 1){
		i_stack = -1;
		
		/*compare output*/
		if(safe) safe = isResultsEqual(argv_output,output_size);

		/*free general*/
		free_shared_memory(shmem,128);
		/*free input*/
		free_input(argv_input,input_size,NODUP);
		/*free output*/
		free_output(argv_output,output_size);

	/*protect_noi*/
	}else if( i_stack == 2){
		/* check output */
		if(safe) safe = isResultsEqual(argv_output,output_size);
	}

	return safe;
}
bool protect_def_inp(void  (* function )(void * [], void * [] ), void * argv_input[] ,int * input_size[] , void * argv_output[] , int * output_size[]){

	/*protect_ni*/
	if(i_stack == -1 ){
		i_stack = 1;

		clean_sh_memory();
		/*create general*/
		shmem = create_shared_memory(128);
		/*create input*/ /*duplicate*/ 
		set_input(argv_input,input_size,DUP);
		/*create output*/
		set_output(argv_output,output_size);

	/*protect_nio*/
	}else if ( i_stack == 2 ){
		/*create input*/ /*duplicate*/ 
		set_input(argv_input,input_size,DUP);
	} 
	
	bool safe = protect_def(function,argv_input,input_size,argv_output,output_size);

	/*protect_ni*/
	if( i_stack == 1){
		i_stack = -1;

		/*free general*/
		free_shared_memory(shmem,128);
		/*free input*/
		free_input(argv_input,input_size,DUP);
		/*free output*/
		free_output(argv_output,output_size);

	/*protect_nio*/
	}else if( i_stack == 2){
		/*free input*/
		
		/*free general*/
		free_shared_memory(shmem,128);
		/*free input*/
		free_input(argv_input,input_size,DUP);
	}

	return safe;
}
bool protect_def_out_inp(void  (* function )(void * [], void * [] ), void * argv_input[] ,int * input_size[] , void * argv_output[] , int * output_size[]){

	/*protect_noi*/
	if(i_stack == -1){
		i_stack = 2;

		clean_sh_memory();
		/*create general*/
		shmem = create_shared_memory(128);
		/*create input*/ /*duplicate*/
		set_input(argv_input,input_size,DUP);
		/*create output*/
		set_output(argv_output,output_size);
	}

	bool safe = protect_def_out(function,argv_input,input_size,argv_output,output_size);

	/*protect_noi*/
	if(i_stack == 2){
		i_stack = -1;

		/*free general*/
		free_shared_memory(shmem,128);
		/*free input*/
		free_input(argv_input,input_size,DUP);
		/*free output*/
		free_output(argv_output,output_size);
	}
	

	return safe ; 
}
bool protect_def_inp_out(void  (* function )(void * [], void * [] ), void * argv_input[] ,int * input_size[] , void * argv_output[] , int * output_size[]){
	/*protect_nio*/
	if( i_stack == -1){
		/*create general*/
        /*create output*/
		i_stack = 2;

		clean_sh_memory();
		/*sh mem*/
		shmem = create_shared_memory(128);
		/*create output*/
		set_output(argv_output,output_size);
	}

	bool safe = protect_def_inp(function,argv_input,input_size,argv_output,output_size);

	/*protect_nio*/
	if(i_stack == 2){
		i_stack = -1;

		/*compare output*/
		safe = isResultsEqual(argv_output,output_size);
		/*free general*/
		free_shared_memory(shmem,128);
		/*free output*/
		free_output(argv_output,output_size);
	}

	return safe;
}


string to_formatted(long long num){

	string test = std::to_string(num);
	string res = "";
	bool negative = false;

	if(test.size() >= 2){
		if(test[0] == '-') {
			test = test.substr(1,test.size()-1);
			negative=true;
		}
	}

	int i;
	int c=0;
	for(i=test.size()-1; i>=0;i--){
		res = test[i] + res ;
		c++;
		if(c==3){
			if(i>0)res = "." + res ;
			c=0;
		}
	}

	if(negative) res = "-" + res ; 

	return res ;
}
void debug_execution( pro_res result){

	int w = 5;
	char sign = ' ';
	char pass = ' ';
	if(result.head_ns < result.trail_ns){
		sign = '<';
		pass = 'V';
	}else{
		sign = '>';
		pass = 'F';
	}
	

	cout << 
	setw(w) <<
	"[+] Hnsec Tnsec Hinstr Tinstr " <<
	" "  << 
	setw(w) <<  to_formatted(result.head_ns) << 
	" " << sign  << " " << 
	setw(w) <<  to_formatted(result.trail_ns) << 
	" "  << 
	setw(w) <<  to_formatted(result.head_instr) << 
	" "  << 
	setw(w) << to_formatted(result.trail_instr) << 
	'\t' << pass <<
	endl;
}


void set_input(void * argv_input[], int * input_size[], in_mode in){
	int in_size = getArgvSize(argv_input);

	if( in == NODUP){
		for(int i=0;i<in_size && i<ARGV_SIZE;i++){
			input_share_head[i] = argv_input[i];
			input_share_trail[i] = argv_input[i];
		}
	}else
	if( in == DUP){
		for(int i=0;i<in_size && i<ARGV_SIZE;i++){
			input_share_head[i] = create_shared_memory(*input_size[i]);
			input_share_trail[i] = create_shared_memory(*input_size[i]);
			memcpy(input_share_head[i], argv_input[i], *input_size[i]);
			memcpy(input_share_trail[i], argv_input[i], *input_size[i]);
		}	
	}	
}
void set_output(void * argv_output[], int * output_size[]){

	int out_size = getArgvSize(argv_output);

	for(int i=0;i<out_size && i<ARGV_SIZE;i++){
		out_share_head[i] = create_shared_memory(*output_size[i]);
		out_share_trail[i] = create_shared_memory(*output_size[i]);
	}

}
void save_result(void * argv_output[], int * output_size[]){
	
	int out_size = getArgvSize(argv_output);
	
	/*as choice we copy the head results*/

	for(int i=0;i<out_size && i<ARGV_SIZE;i++){
			memcpy(argv_output[i], out_share_head[i],*output_size[i]);
		}

}
void free_input(void * argv_input[], int * input_size[], in_mode in){
	
	int in_size = getArgvSize(argv_input);

	if( in == DUP){
		for(int i=0;i<in_size && i<ARGV_SIZE;i++){
			if(input_share_head[i] != NULL) free_shared_memory(input_share_head[i], *input_size[i]);
			if(input_share_trail[i] != NULL) free_shared_memory(input_share_trail[i], *input_size[i]);
		}		
	}
	memset(&input_share_head[0],0x0,sizeof(void *) * ARGV_SIZE);
	memset(&input_share_trail[0],0x0,sizeof(void *) * ARGV_SIZE);
}

void free_output(void * argv_output[], int * output_size[]){
	int out_size = getArgvSize(argv_output);

	for(int i=0;i<out_size && i<ARGV_SIZE;i++){
		if(out_share_head[i] != NULL) free_shared_memory(out_share_head[i], *output_size[i]);
		if(out_share_trail[i] != NULL) free_shared_memory(out_share_trail[i], *output_size[i]);
	}
	memset(&out_share_head[0],0x0,sizeof(void *) * ARGV_SIZE);
	memset(&out_share_trail[0],0x0,sizeof(void *) * ARGV_SIZE);
}

char getState(pid_t pid){
	
	FILE *fp;
	char c = 'E' ;
	char path[255];
	sprintf(path, "/proc/%i/stat",pid);
	fp = fopen(path, "r");

	if(fp != NULL){
		c = fgetc(fp);
		while(fgetc(fp) != ' ');
		while(fgetc(fp) != ' ');
		c = fgetc(fp);

		fclose(fp);	
	}

	return c ;
}


void work( worker * w, void * shmem , void  (* function )(void * [], void * [] ), void * argv_input[]  , void * argv_output[] ){

	int cpu;
	int shemm_index[2];

	if( w->getType() == head_t){
		cpu = 1;
		shemm_index[0] = 0;
		shemm_index[1] = 2;
	}else if( w->getType() == trail_t){
		cpu = 2 ;		
		shemm_index[0] = 1;
		shemm_index[1] = 3;		
	}

	w->lockCPU(cpu);	
	w->setScheduler(SCHEDULER_START);
	((unsigned char *)shmem)[shemm_index[0]] = 0x1;
	while(( ((unsigned char *)shmem)[shemm_index[1]]) == 0x0 );
	(*function)(input_share_head,out_share_head);
	w->setScheduler(SCHEDULER_END);
	exit(0);

}

void read_add_reset( long long * instr ,  worker * head, worker * trail){

	instr[0] += head->getHWInstruction(head->getFD(instructions));
	instr[1] += trail->getHWInstruction(trail->getFD(instructions));
	head->resetPMC_at(head->getFD(instructions));
	trail->resetPMC_at(trail->getFD(instructions));
}

void init_workers(worker * head, worker * trail ,  pid_t head_pid , pid_t trail_pid){

	head->setworkerPid(head_pid);
	head->setFD(head->getHWInstruction_fd(), instructions );
	head->resetPMC_at(head->getFD(instructions));

	trail->setworkerPid(trail_pid);
	trail->setFD(trail->getHWInstruction_fd(), instructions );
	trail->resetPMC_at(trail->getFD(instructions));

}


#ifdef active
	void debug_active(pid_t head, pid_t trail, bool stag){

		struct timespec time_actual;
		clock_gettime(CLOCK_MONOTONIC, &time_actual);
		long long value = time_actual.tv_sec * 1000000000 +  time_actual.tv_nsec  ;
		// if(stag == false )std::cout << ":::" << value << "," << ((kill(head,0) == 0 ) ? HA : HD) << "," <<  ((kill(trail,0) == 0 ) ? TA : TD) << std::endl;
		// else std::cout << ":::" << value << "," << ((kill(head,0) == 0 ) ? HA : HD) << "," << TS << std::endl;
		std::cout << ":a::" << value << "," << ((getState(head) == 'R' ) ? HA : HD) << "," ;
		if(getState(trail) == 'R' ){
			std::cout << TA ;	
		}else if(getState(trail) == 'T'){
			std::cout << TS ; 
		}else{
			std::cout << TD;
		} 
		cout << std::endl;	
		
	}
#endif

#ifdef instr
	void debug_instr( long long head, long long trail){
		struct timespec time_actual;
		clock_gettime(CLOCK_MONOTONIC, &time_actual);
		long long value = time_actual.tv_sec * 1000000000 +  time_actual.tv_nsec  ;
		cout << ":i::" << value <<  "," << head << "," << trail << endl;
	}
#endif
