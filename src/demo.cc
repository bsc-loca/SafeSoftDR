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
#include <stdio.h>


using namespace std;

/*
*   target
*/
void matrix_multiply(int * matA, int * matB, int * matC , int rows , int cols){

    for(int i = 0; i < rows; ++i)
        for(int j = 0; j < cols; ++j)
            for(int k = 0; k < rows; ++k)
            {
                matC[i * rows + j] += matA[i * rows + k] * matB[k * rows + j];
            }
}
/*
*   wrapper
*/
void matrix_multiply_wrapper(void * argv_input[] , void * argv_output[] ){
    
    int * matA = (int * )argv_input[0];
    int * matB = (int * )argv_input[1];
    int * matC = (int * )argv_output[0];
    
    int rows = *(int * )argv_input[2];
    int cols = *(int * )argv_input[3]; 

    matrix_multiply(matA, matB, matC ,rows,cols );
}

int main(){
    int size =  10; 

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

    /*
    * init
    */
    for(int i=0;i<rows;i++){
        for(int j=0;j<cols;j++){
            matA[i*cols + j] = i+j;
            matB[i*cols + j] = j-i;
            matC[i*cols + j] = 0;
        }
    }

    void (*ptr)( void * [] , void *[]) = &matrix_multiply_wrapper;
    void * argv_input[] = { (void * )matA , (void *)matB, (void *)&rows, (void * )&cols, NULL };
    void * argv_output[] = { (void * )matC, NULL};
    int * input_size[] = { (int *)& matAbytes , (int *)&matBbytes , (int *)&rowsbytes , (int *)& colsbytes, NULL};
    int * output_size[] = { (int *)& matCbytes , NULL };
    
    bool pass_flag = protect_def_inp_out(ptr , argv_input , input_size , argv_output , output_size ) ; 

    if (!pass_flag)
        printf("Comparison gave an incorrect result. \n");
    else   
        printf("Comparison made and results are the same.\n");
    

    free((void * )matA);
    free((void * )matB);
    free((void * )matC);

    return 0;
}
