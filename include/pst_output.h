#ifndef PST_OUTPUT_H
#define PST_OUTPUT_H

#include "pst.h"

int pst_output_OutputResult(MYSQL_STMT* stmt, PstSyntax syntax);
void pst_output_FreeResult();

#endif /* PST_OUTPUT_H */
