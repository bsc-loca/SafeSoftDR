/*
 * Copyright 2022 Barcelona Supercomputing Center
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef LIB_H
#define LIB_H

/*direct
	
	the output result is the head output result as default
    
	default : staggering based protection
    input : default + duplicate input memory for head and trail
    output : default + compare head and trail output
    intput_output : default + input + output

*/
bool protect_default( void  (* function )(void * [] ,  void * [] ), void * argv_input[] ,int * input_size[] , void * argv_output[] ,int * output_size[]);
bool protect_input(void  (* function )(void * [], void * [] ), void * argv_input[] ,int * input_size[] , void * argv_output[] , int * output_size[]	);
bool protect_output(void  (* function )(void * [], void * [] ),void * argv_input[] ,int * input_size[] , void * argv_output[] , int * output_size[]	);
bool protect_input_output(void  (* function )(void * [], void * [] ), void * argv_input[] ,int * input_size[] , void * argv_output[] , int * output_size[]);


/*chained version

	the output result is the head output result as default

	def : staggering based protection
	out : compare head and trail output
	inp : duplicate input memory for head and trail
	
	def : def
	def_out : def + out
	def_inp : def + inp
	def_out_inp : def + out + inp
	def_inp_out : def + inp + out

*/
bool protect_def(void(* function )(void * [], void * [] ), void * argv_input[] ,int * input_size[] , void * argv_output[] , int * output_size[]);
bool protect_def_out(void(* function )(void * [], void * [] ), void * argv_input[] ,int * input_size[] , void * argv_output[] , int * output_size[]);
bool protect_def_inp(void(* function )(void * [], void * [] ), void * argv_input[] ,int * input_size[] , void * argv_output[] , int * output_size[]);
bool protect_def_out_inp(void(* function )(void * [], void * [] ), void * argv_input[] ,int * input_size[] , void * argv_output[] , int * output_size[]);
bool protect_def_inp_out(void(* function )(void * [], void * [] ), void * argv_input[] ,int * input_size[] , void * argv_output[] , int * output_size[]);



#endif
