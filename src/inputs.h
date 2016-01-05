/* inputs.h
 * This file should not be modified except for NUMINPUTS and threshold
 * constants. Make changes to the actual functions in inputs.cpp, that
 *  way the function declarations and usage will remain constant across
 * implementations. 
*/

#ifndef INPUTS_H_
#define INPUTS_H_

//The number of inputs being used in the program
#define NUMINPUTS 2
//The threshold for branching in the search function. Higher numbers will lead to a more accurate but constrained and possibly longer search. Max is one (branches only on perfect match) and minimum is 0 (always branches)
#define BRANCH_LIMIT 0.85
//The stopping threshold for the search function. Higher numbers will cause the function to search for longer
#define STOP_LIMIT 1.5

//Takes two inputs as arguments and returns the percent similarity between them.
float compareInputs(input input1, input input2, int type);

//Receives a input of type from whatever source. Can be blocking or nonblocking depending on agent setup.
input getInput(int type);

#endif
