/* vars.h
 * This file contains constants that need to be
 * fine tuned depending on the application the
 * code is being used for.
 */

#ifndef VARS_H_
#define VARS_H_

//The number of inputs being used in the program
#define NUMINPUTS 6
//The threshold for branching in the search function. Higher numbers will lead to a more accurate but constrained and possibly longer search. Max is one (branches only on perfect match) and minimum is 0 (always branches)
#define BRANCH_LIMIT 0.85
//The stopping threshold for the search function. Higher numbers will cause the function to search for longer
#define STOP_LIMIT 1.5
//The limit for chunking memories into discrete collections. Higher will split more often and lower less often.
#define SPLIT_LIMIT 0.85

#endif
