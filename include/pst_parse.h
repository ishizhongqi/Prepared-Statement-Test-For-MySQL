#ifndef PST_PARSE_H
#define PST_PARSE_H

#include "pst.h"

int pst_parse_Parse(const char* filename);
PstConnection* pst_parse_GetConnection();
PstPreparedStatements* pst_parse_GetPreparedStatement();
void pst_parse_Free();

#endif /* PST_PARSE_H */