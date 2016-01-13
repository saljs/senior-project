/* inputs.h
 * This file should not be modified except for NUMINPUTS and threshold
 * constants. Make changes to the actual functions in inputs.cpp, that
 *  way the function declarations and usage will remain constant across
 * implementations. 
*/
#include "database.h"

#ifndef INPUTS_H_
#define INPUTS_H_

//Takes two inputs as arguments and returns the percent similarity between them.
float compareInputs(input* input1, input* input2, int type);

//Receives a input of type from whatever source. Can be blocking or nonblocking depending on agent setup.
input* getInput(int type, memList* database);

#endif
