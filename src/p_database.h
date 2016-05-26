/* p_database.h
 * This file is a parrelell version of
 * database.c
 */

#include "database.h"

#ifndef P_DATABASE_H_
#define P_DATABASE_H_

//memory* AddtoMem(input* newInput, int index, memory* database, bool* newTrigger);
void p_compare();
void p_linkInput(input* pattern, int type, memory* database, int world_size);

#endif
