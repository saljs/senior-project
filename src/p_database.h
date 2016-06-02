/* p_database.h
 * This file is a parallel version of
 * database.c
 */

#include "database.h"

#ifndef P_DATABASE_H_
#define P_DATABASE_H_

void p_compare();
void p_linkInput(input* pattern, int type, memory* database, int world_size);

#endif
