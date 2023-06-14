/*
 * Copyright 2022 Barcelona Supercomputing Center
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "lib.h"
#include <iostream>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

#include <iostream>
#include <iomanip>



using namespace std;


typedef struct result{
    int ms_average;
    int passed;
    int verified;
}result;

/*
*   matrix part
*/
void matrix_Init(void * matA , void * matB , void * matC , int rows, int cols);
void matrix_Mul(int * matA, int * matB, int * matC , int rows , int cols);
void vector_copy_add( int * src, int * dst, int size);
void vector_print(int * vector, int size);


bool is_matrix_mul(int * matrix_res, int rows, int cols ){

    bool ok = true;

    int * matA = (int *)malloc(rows * cols * sizeof(int));
    int * matB = (int *)malloc(rows * cols * sizeof(int));
    int * matC = (int *)malloc(rows * cols * sizeof(int));

    
    matrix_Init((void * )matA,(void * )matB,(void * )matC,rows, cols);
    matrix_Mul(matA, matB, matC , rows, cols);

    for(int i=0;i<rows && ok;i++){
        for(int j=0;j<cols && ok;j++){
            if(matC[ i * rows + j] != matrix_res[i * rows + j]) ok = false;
        }
    }

    free((void *)matC);
    free((void *)matB);
    free((void *)matA);

    return ok ; 

}
void matrix_Init(void * matA , void * matB , void * matC , int rows, int cols){

    int * pA = (int *)matA;
    int * pB = (int *)matB;
    int * pC = (int *)matC;

    for(int i = 0; i < rows; ++i)
        for(int j = 0; j < cols; ++j)
        {
            pA[i * rows + j] = i;
            pB[i * rows + j] = i;
            pC[i * rows + j] = 0;
        }
}
void matrix_Mul(int * matA, int * matB, int * matC , int rows , int cols){
    cout << "START : " << getpid() << endl;
    for(int i = 0; i < rows; ++i)
        for(int j = 0; j < cols; ++j)
            for(int k = 0; k < rows; ++k)
            {
                matC[i * rows + j] += matA[i * rows + k] * matB[k * rows + j];
            }

    cout << "END : " << getpid() << endl;
}
void matrix_Print( void * mat, int rows , int cols){
    int * p = (int *)mat;

    for(int i = 0; i < rows; ++i){
        for(int j = 0; j < cols; ++j)
        {
            cout << p[i * rows + j] << "," ;
        }
        cout << endl;
    }
}
void matrix_mul_wrapper(void * argv_input[] , void * argv_output[] ){
    
    int * matA = (int * )argv_input[0];
    int * matB = (int * )argv_input[1];
    int * matC = (int * )argv_output[0];
    
    int rows = *(int * )argv_input[2];
    int cols = *(int * )argv_input[3]; 

    matrix_Mul(matA, matB, matC ,rows,cols );
}
int matrix_average_execution(int iteration , int size){

    cpu_set_t  mask;
    CPU_ZERO(&mask);
    CPU_SET(0x4, &mask);
    sched_setaffinity(getpid(), sizeof(mask), &mask); 

    int rows = size ; 
    int cols = size ; 

    int rowsbytes = sizeof(rows);
    int colsbytes = sizeof(cols);

    int matAbytes = rows * cols * sizeof(int);
    int matBbytes = rows * cols * sizeof(int);
    int matCbytes = rows * cols * sizeof(int);

    int * matA = (int *) malloc( matAbytes );
    int * matB = (int *) malloc( matBbytes );
    int * matC = (int *) malloc( matCbytes );

    matrix_Init(matA, matB, matC, rows, cols);
 
    
    struct timespec start ; 
    struct timespec stop;
    long long accum;
    long long ns_average = 0 ;

    for(int i=0;i<iteration;i++){
            clock_gettime(CLOCK_MONOTONIC, &start);
            matrix_Mul(matA ,matB, matC, rows, cols) ; 
            clock_gettime(CLOCK_MONOTONIC, &stop);
            accum =(long long) ( stop.tv_sec - start.tv_sec ) * 1000000000 + (long long)( stop.tv_nsec - start.tv_nsec );
            ns_average += (long long)accum;
    }

    free((void * )matA);
    free((void * )matB);
    free((void * )matC);

    int ms_average = (int)(ns_average / iteration) / 1000000;

    return ms_average;
}
result matrix_test_call(int iteration, int size , int  mode , int type ){

    int rows = size ; 
    int cols = size ; 

    int rowsbytes = sizeof(rows);
    int colsbytes = sizeof(cols);

    int matAbytes = rows * cols * sizeof(int);
    int matBbytes = rows * cols * sizeof(int);
    int matCbytes = rows * cols * sizeof(int);

    int * matA = (int *) malloc( matAbytes );
    int * matB = (int *) malloc( matBbytes );
    int * matC = (int *) malloc( matCbytes );

    matrix_Init(matA, matB, matC, rows, cols);

    void (*ptr)( void * [] , void *[]) = &matrix_mul_wrapper;
    void * argv_input[] = { (void * )matA , (void *)matB, (void *)&rows, (void * )&cols, NULL };
    void * argv_output[] = { (void * )matC, NULL};
    int * input_size[] = { (int *)& matAbytes , (int *)&matBbytes , (int *)&rowsbytes , (int *)& colsbytes, NULL};
    int * output_size[] = { (int *)& matCbytes , NULL };
 
    int pass_counter = 0;
    bool pass_flag = false;
    struct timespec start ; 
    struct timespec stop;
    long long accum = 0;
    long long ns_average = 0 ;
    int verified = 0 ;

    for(int i=0;i<iteration;i++){
        /*clean matrix result*/
        memset(argv_output[0], 0x0,matCbytes);
        

        if(mode == 0){
            if(type == 0){
                clock_gettime(CLOCK_MONOTONIC, &start);
                pass_flag = protect_default(ptr , argv_input , input_size , argv_output , output_size ) ; 
                clock_gettime(CLOCK_MONOTONIC, &stop);
            }else if(type == 1){
                clock_gettime(CLOCK_MONOTONIC, &start);
                pass_flag = protect_input(ptr , argv_input , input_size , argv_output , output_size ) ; 
                clock_gettime(CLOCK_MONOTONIC, &stop);
            }else if(type == 2){
                clock_gettime(CLOCK_MONOTONIC, &start);
                pass_flag = protect_output(ptr , argv_input , input_size , argv_output , output_size ) ; 
                clock_gettime(CLOCK_MONOTONIC, &stop);
            }else if(type == 3){
                clock_gettime(CLOCK_MONOTONIC, &start);
                pass_flag = protect_input_output(ptr , argv_input , input_size , argv_output , output_size ) ; 
                clock_gettime(CLOCK_MONOTONIC, &stop);
            }
        }else if( mode == 1){
            if(type == 0){
                clock_gettime(CLOCK_MONOTONIC, &start);
                pass_flag = protect_def(ptr , argv_input , input_size , argv_output , output_size ) ; 
                clock_gettime(CLOCK_MONOTONIC, &stop);
            }else if( type == 1){
                clock_gettime(CLOCK_MONOTONIC, &start);
                pass_flag = protect_def_out(ptr , argv_input , input_size , argv_output , output_size ) ; 
                clock_gettime(CLOCK_MONOTONIC, &stop);
            }else if( type == 2 ){
                clock_gettime(CLOCK_MONOTONIC, &start);
                pass_flag = protect_def_inp(ptr , argv_input , input_size , argv_output , output_size ) ; 
                clock_gettime(CLOCK_MONOTONIC, &stop);
            }else if( type == 3 ){
                clock_gettime(CLOCK_MONOTONIC, &start);
                pass_flag = protect_def_out_inp(ptr , argv_input , input_size , argv_output , output_size ) ; 
                clock_gettime(CLOCK_MONOTONIC, &stop);
            }else if( type == 4 ){
                clock_gettime(CLOCK_MONOTONIC, &start);
                pass_flag = protect_def_inp_out(ptr , argv_input , input_size , argv_output , output_size ) ; 
                clock_gettime(CLOCK_MONOTONIC, &stop);
            }


        }
        
        if(pass_flag) pass_counter++; 
        if(is_matrix_mul((int *)argv_output[0],rows, cols)) verified++;
        accum = (long long)( stop.tv_sec - start.tv_sec ) * 1000000000 + (long long)( stop.tv_nsec - start.tv_nsec );
        ns_average += (long long)accum;
        
    }

    free((void * )matA);
    free((void * )matB);
    free((void * )matC);

    result res;
    res.passed = pass_counter;
    res.verified = verified;
    res.ms_average =(int) ((ns_average / iteration) / 1000000);

    return res;

}
void matrix_test(int it , int s, int b){

    int small =  s; 
    int big = b;
    int iteration = it;
    int size[2] = { small , big }; 


    int small_ms_average = matrix_average_execution(iteration, small);
    int big_ms_average = matrix_average_execution(iteration, big);
    int vector_ms_average[2] = {  small_ms_average,  big_ms_average };

    
    cout << "[+] small average size:" << small << " ms:" << small_ms_average << endl;
    cout << "[+] big average size:" << big << " ms:" << big_ms_average << endl;
    int mode ;
    
    
    cout << "[+++CHAINED matrix]" << endl;
    
    mode = 0;
    cout << "[+++DIRECT matrix]" << endl;
    /*DIRECT*/
    for(int i=0;i<2;i++){
        result no_protect_dir = matrix_test_call(iteration,size[i],mode,0);
        result i_protect_dir = matrix_test_call(iteration,size[i],mode,1);
        result o_protect_dir = matrix_test_call(iteration,size[i],mode,2);
        result io_protect_dir = matrix_test_call(iteration,size[i],mode,3);
    
        cout << 
        "[+] no_protect_dir size:" <<
        size[i] << 
        " passed:" <<
        no_protect_dir.passed <<
        "/" << iteration <<
        " ms_average:" <<
        no_protect_dir.ms_average <<
        " r: " <<
        100 * ( (double) no_protect_dir.ms_average / vector_ms_average[i]  ) <<
        "%" <<
        " ver:" <<
        no_protect_dir.verified << 
        endl;

        cout << 
        "[+] i_protect_dir size:" <<
        size[i] << 
        " passed:" <<
        i_protect_dir.passed <<
        "/" << iteration <<
        " ms_average:" <<
        i_protect_dir.ms_average <<
        " r: " <<
        100 * ( (double) i_protect_dir.ms_average / vector_ms_average[i]  ) <<
        "%" <<
        " ver:" <<
        i_protect_dir.verified << 
        endl;

        cout << 
        "[+] o_protect_dir size:" <<
        size[i] << 
        " passed:" <<
        o_protect_dir.passed <<
        "/" << iteration <<
        " ms_average:" <<
        o_protect_dir.ms_average <<
        " r: " <<
        100 * ( (double) o_protect_dir.ms_average / vector_ms_average[i]  ) <<
        "%" <<
        " ver:" <<
        o_protect_dir.verified << 
        endl;

        cout << 
        "[+] io_protect_dir size:" <<
        size[i] << 
        " passed:" <<
        io_protect_dir.passed <<
        "/" << iteration <<
        " ms_average:" <<
        io_protect_dir.ms_average <<
        " r: " <<
        100 * ( (double) io_protect_dir.ms_average / vector_ms_average[i]  ) <<
        "%" <<
        " ver:" <<
        io_protect_dir.verified << 
        endl;

    }
}


void nops(void * in[], void * out[] ){
    
    int *  iteration = (int * )in[0];

    for(int i=0; i< *iteration; i++){

        /*32*/

        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");

        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
    }
}

void test_nop(){

    for(int i=1;i<=64;i++){
    
        int iteration = 1024 * 1024  * i; 

        int iteration_bytes = sizeof(iteration);
        void (*ptr)( void * [] , void * []) = &nops;
        void * argv_input[] = { (void * )&iteration , NULL };
        void * argv_output[] = {  NULL};
        int * input_size[] = { (int *)&iteration_bytes , NULL};
        int * output_size[] = {  NULL };

        
        bool pass_flag = protect_default( ptr , argv_input , input_size , argv_output , output_size ) ; 
        cout << pass_flag << endl;
    }
    
}


void test_demo(){

    struct timespec start;
    struct timespec stop;

    int iteration = 10;
    int size = 400; //400

    int rows = size ; 
    int cols = size ; 

    int rowsbytes = sizeof(rows);
    int colsbytes = sizeof(cols);

    int matAbytes = rows * cols * sizeof(int);
    int matBbytes = rows * cols * sizeof(int);
    int matCbytes = rows * cols * sizeof(int);

    int * matA = (int *) malloc( matAbytes );
    int * matB = (int *) malloc( matBbytes );
    int * matC = (int *) malloc( matCbytes );

    matrix_Init(matA, matB, matC, rows, cols);

    void (*ptr)( void * [] , void *[]) = &matrix_mul_wrapper;
    void * argv_input[] = { (void * )matA , (void *)matB, (void *)&rows, (void * )&cols, NULL };
    void * argv_output[] = { (void * )matC, NULL};
    int * input_size[] = { (int *)& matAbytes , (int *)&matBbytes , (int *)&rowsbytes , (int *)& colsbytes, NULL};
    int * output_size[] = { (int *)& matCbytes , NULL };
 
    for(int i=1;i<=1;i++){    
        bool pass_flag = protect_default(ptr , argv_input , input_size , argv_output , output_size ) ; 

    }
    
    free((void * )matA);
    free((void * )matB);
    free((void * )matC);

}


void test_nop_isolated(){

    int cpu = 0x0;

    cpu_set_t  mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);
    sched_setaffinity(getpid(), sizeof(mask), &mask);  


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
	pe.pinned = 1;
	
	fd = syscall(__NR_perf_event_open, &pe, getpid(), cpu, -1, 0);
    if (fd == -1) {
        fprintf(stderr, "Error opening leader %llx\n", pe.config);
        exit(EXIT_FAILURE);
    }

    for(int i=1;i<=96;i++){
        long long count = 0;
        int iteration = 1024 * 1024 * i;
        void * argv_input[] = { (void * )&iteration , NULL };
        void * argv_output[] = {  NULL};
        
        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

        nops(argv_input, argv_output);

        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
        read(fd, &count, sizeof(count));
        ioctl(fd, PERF_EVENT_IOC_RESET, 0);

        cout << count <<  ":" << 1024 * 1024 * 32 *(long long)i << ":"  <<  std::setprecision(10)  <<  (long double)count / (1024 * 1024 * 32 * (long long)i) <<  endl;

    }

    close(fd);

}

/*
* vector part
*/
bool is_vector_ca(int * dst, int size){

    bool ok = true;

    int * v = (int *)malloc(size * sizeof(int));
    memset((void*)v,0x0,size * sizeof(int));

    vector_copy_add(v, v, size);
    
    for(int i=0;i<size && ok;i++){
        if(v[i] != dst[i]) ok = false;
    }

    free((void*)v);
    
    return ok ;
}
void vector_copy_add( int * src, int * dst, int size){
    for(int i=0;i<size;i++){
        dst[i] = src[i] + 1;
    }
}
void  vector_copy_add_wrapper(void * argv_input[] , void * argv_output[]){
    int * src = (int * )argv_input[0];
    int size =  (*(int * )argv_input[1]);
    int * dst = (int * )argv_output[0];
    
    vector_copy_add(src,dst,size);
}
int vector_average_execution(int iteration, int size){

    int sizebytes = sizeof(size);

    int src_size = size *  sizeof(int);
    int dst_size = size *  sizeof(int);

    int * src = (int *) malloc( src_size );
    int * dst = (int *) malloc( dst_size );
    
    struct timespec start ; 
    struct timespec stop;
    long long accum;
    int ns_average = 0 ;

    for(int i=0;i<iteration;i++){
            clock_gettime(CLOCK_MONOTONIC, &start);
            vector_copy_add(src,dst,size);
            clock_gettime(CLOCK_MONOTONIC, &stop);
            accum = (long long)( stop.tv_sec - start.tv_sec ) * 1000000000 + (long long )( stop.tv_nsec - start.tv_nsec );
            ns_average += (int)accum;
    }

    free((void *)dst);
    free((void *)src);

    int ms_average = (int)(ns_average / iteration) / 1000000;

    return ms_average;
}
result vector_test_call(int iteration, int size , int way ,  int type ){
    int elements = size;
    int elements_bsize = sizeof(elements);

    int src_size = size *  sizeof(int);
    int dst_size = size *  sizeof(int);

    int * src = (int *) malloc( src_size );
    int * dst = (int *) malloc( dst_size );

    memset((void *)src,0x0, src_size);
    memset((void *)dst,0x0, dst_size);

    void (*ptr)( void * [] , void *[]) = &vector_copy_add_wrapper;
    void * argv_input[] = { (void * )src , (void *)&elements , NULL };
    void * argv_output[] = { (void * )dst, NULL};
    int * input_size[] = { (int *)&src_size ,(int *)&elements_bsize , NULL};
    int * output_size[] = { (int *)&dst_size , NULL };
    
    struct timespec start ; 
    struct timespec stop;
    long long accum;
    long long ns_average = 0 ;
    int pass_counter = 0;
    bool pass_flag = false;
    int verified = 0 ;

    for(int i=0;i<iteration;i++){
        /*clean dst vector*/
        memset((void *)src,0x0, src_size);
        memset((void *)dst,0x0, dst_size);
        /*DIRECT*/
        if(way  == 0 ){
            if(type == 0){
                clock_gettime(CLOCK_MONOTONIC, &start);
                pass_flag = protect_default(ptr , argv_input , input_size , argv_output , output_size ) ; 
                clock_gettime(CLOCK_MONOTONIC, &stop);
            }else if(type == 1){
                clock_gettime(CLOCK_MONOTONIC, &start);
                pass_flag = protect_input(ptr , argv_input , input_size , argv_output , output_size ) ; 
                clock_gettime(CLOCK_MONOTONIC, &stop);
            }else if(type == 2){
                clock_gettime(CLOCK_MONOTONIC, &start);
                pass_flag = protect_output(ptr , argv_input , input_size , argv_output , output_size ) ; 
                clock_gettime(CLOCK_MONOTONIC, &stop);
            }else if(type == 3){
                clock_gettime(CLOCK_MONOTONIC, &start);
                pass_flag = protect_input_output(ptr , argv_input , input_size , argv_output , output_size ) ; 
                clock_gettime(CLOCK_MONOTONIC, &stop);
            }
        /*CHAINED*/
        }else 
        if( way == 1 ){
            if(type == 0){
                clock_gettime(CLOCK_MONOTONIC, &start);
                pass_flag = protect_def(ptr , argv_input , input_size , argv_output , output_size ) ; 
                clock_gettime(CLOCK_MONOTONIC, &stop);
            }else if(type == 1){
                clock_gettime(CLOCK_MONOTONIC, &start);
                pass_flag = protect_def_out(ptr , argv_input , input_size , argv_output , output_size ) ; 
                clock_gettime(CLOCK_MONOTONIC, &stop);
            }else if(type == 2){
                clock_gettime(CLOCK_MONOTONIC, &start);
                pass_flag = protect_def_inp(ptr , argv_input , input_size , argv_output , output_size ) ; 
                clock_gettime(CLOCK_MONOTONIC, &stop);
            }else if(type == 3){
                clock_gettime(CLOCK_MONOTONIC, &start);
                pass_flag = protect_def_out_inp(ptr , argv_input , input_size , argv_output , output_size ) ; 
                clock_gettime(CLOCK_MONOTONIC, &stop);
            }else if(type == 4){
                clock_gettime(CLOCK_MONOTONIC, &start);
                pass_flag = protect_def_inp_out(ptr , argv_input , input_size , argv_output , output_size ) ; 
                clock_gettime(CLOCK_MONOTONIC, &stop);
            }
        }
        
        accum = (long long)( stop.tv_sec - start.tv_sec ) * 1000000000 + (long long)( stop.tv_nsec - start.tv_nsec );
        ns_average += (long long)accum;
        if(pass_flag)pass_counter++;
        if(is_vector_ca(dst,size)) verified++;
        
    }

    free((void * )dst);
    free((void * )src);

    result res ; 
    res.passed = pass_counter;
    res.ms_average = (int)((ns_average / iteration) / 1000000);
    res.verified = verified;    

    return res;
}
void vector_test(int it , int s, int b){

    int iteration = it;
    int small = s;
    int big = b;
    int small_ms_average = vector_average_execution(iteration, small);
    int big_ms_average = vector_average_execution(iteration, big);
    int vector_ms_average[2] = {  small_ms_average,  big_ms_average };
    
    
    int size[2] = { small, big}; 
    
    cout << "[+] small average size:" << small << " ms:" << small_ms_average << endl;
    cout << "[+] big average size:" << big << " ms:" << big_ms_average << endl;


    int mode ;

    cout << "[+++DIRECT vector]" << endl;
    /*DIRECT*/
    mode = 0;
    for(int i=0;i<2;i++){
        result no_protect_dir = vector_test_call(iteration,size[i],mode,0);
        result i_protect_dir = vector_test_call(iteration,size[i],mode,1);
        result o_protect_dir = vector_test_call(iteration,size[i],mode,2);
        result io_protect_dir = vector_test_call(iteration,size[i],mode,3);
    
        cout << 
        "[+] no_protect_dir size:" <<
        size[i] << 
        " passed:" <<
        no_protect_dir .passed <<
        "/" << iteration <<
        " ms_average:" <<
        no_protect_dir.ms_average <<
        " r: " <<
        100 * ( (double) no_protect_dir.ms_average / vector_ms_average[i]  ) <<
        "%" <<
        " ver:" << 
        no_protect_dir.verified <<
        endl;

        cout << 
        "[+] i_protect_dir size:" <<
        size[i] << 
        " passed:" <<
        i_protect_dir.passed <<
        "/" << iteration <<
        " ms_average:" <<
        i_protect_dir.ms_average <<
        " r: " <<
        100 * ((double) i_protect_dir.ms_average / vector_ms_average[i] ) <<
        "%" <<
        " ver:" << 
        i_protect_dir.verified <<
        endl;

        cout << 
        "[+] o_protect_dir size:" <<
        size[i] << 
        " passed:" <<
        o_protect_dir.passed <<
        "/" << iteration <<
        " ms_average:" <<
        o_protect_dir.ms_average <<
        " r: " <<
        100 * ((double)o_protect_dir.ms_average / vector_ms_average[i] ) <<
        "%" <<
        " ver:" << 
        o_protect_dir.verified <<
        endl;

        cout << 
        "[+] io_protect_dir size:" <<
        size[i] << 
        " passed:" <<
        io_protect_dir.passed <<
        "/" << iteration <<
        " ms_average:" <<
        io_protect_dir.ms_average <<
        " r: " <<
        100 * ((double)io_protect_dir.ms_average /  vector_ms_average[i] ) <<
        "%" <<
        " ver:" << 
        io_protect_dir.verified <<
        endl;

    }

    cout << "[+++CHAINED vector]" << endl;
    /*CHAINED*/
    mode = 1;
    for(int i=0;i<2;i++){
        result n_protect_chain = vector_test_call(iteration,size[i],mode,0);
        result no_protect_chain = vector_test_call(iteration,size[i],mode,1);
        result ni_protect_chain = vector_test_call(iteration,size[i],mode,2);
        result noi_protect_chain = vector_test_call(iteration,size[i],mode,3);
        result nio_protect_chain = vector_test_call(iteration,size[i],mode,4);

        cout << 
        "[+] n_protect_chain size:" <<
        size[i] << 
        " passed:" <<
        n_protect_chain.passed <<
        "/" << iteration <<
        " ms_average:" <<
        n_protect_chain.ms_average <<
        " r: " <<
        100 * ( (double) n_protect_chain.ms_average / vector_ms_average[i]  ) <<
        "%" <<
        " ver:" << 
        n_protect_chain.verified <<
        endl;

        cout << 
        "[+] no_protect_chain size:" <<
        size[i] << 
        " passed:" <<
        no_protect_chain.passed <<
        "/" << iteration <<
        " ms_average:" <<
        no_protect_chain.ms_average <<
        " r: " <<
        100 * ((double) no_protect_chain.ms_average / vector_ms_average[i] ) <<
        "%" <<
        " ver:" << 
        no_protect_chain.verified <<
        endl;

        cout << 
        "[+] ni_protect_chain size:" <<
        size[i] << 
        " passed:" <<
        ni_protect_chain.passed <<
        "/" << iteration <<
        " ms_average:" <<
        ni_protect_chain.ms_average <<
        " r: " <<
        100 * ((double)ni_protect_chain.ms_average / vector_ms_average[i] ) <<
        "%" <<
        " ver:" << 
        ni_protect_chain.verified <<
        endl;

        cout << 
        "[+] noi_protect_chain size:" <<
        size[i] << 
        " passed:" <<
        noi_protect_chain.passed <<
        "/" << iteration <<
        " ms_average:" <<
        noi_protect_chain.ms_average <<
        " r: " <<
        100 * ((double)noi_protect_chain.ms_average /  vector_ms_average[i] ) <<
        "%" <<
        " ver:" << 
        noi_protect_chain.verified <<
        endl;


        cout << 
        "[+] nio_protect_chain size:" <<
        size[i] << 
        " passed:" <<
        nio_protect_chain.passed <<
        "/" << iteration <<
        " ms_average:" <<
        nio_protect_chain.ms_average <<
        " r: " <<
        100 * ((double)nio_protect_chain.ms_average /  vector_ms_average[i] ) <<
        "%" <<
        " ver:" << 
        nio_protect_chain.verified <<
        endl;

    }

}
void vector_print(int * vector, int size){
    for(int i=0;i<size;i++){
        cout << vector[i] << ",";
    }
    cout << endl;
}

int main(){
    
    matrix_test(20,32,64);
    vector_test(10,1024*1024,1024*1024*5);
    test_nop_isolated();
    
    test_demo();
    

return 0 ;
}
