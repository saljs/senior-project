/* inputs.h
 * This file should not be modified. Make changes to the actual 
 * functions in inputs.c, that  way the function declarations
 * and usage will remain constant across implementations. 
*/
#include "database.h"

#ifndef INPUTS_H_
#define INPUTS_H_

//Takes two inputs as arguments and returns the percent similarity between them.
float compareInputs(input* input1, input* input2, int type);

//Receives a input of type from whatever source. Can be blocking or nonblocking depending on agent setup.
input* getInput(int type, memory* database);
#endif
