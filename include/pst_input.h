#ifndef PST_INPUT_H
#define PST_INPUT_H

#include "pst.h"

int pst_input_InputParameters(MYSQL_STMT* stmt, PstParameter* param, unsigned long count);
void pst_input_FreeParameters();

#endif /* PST_INPUT_H */
