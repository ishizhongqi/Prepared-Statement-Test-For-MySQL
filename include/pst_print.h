#ifndef PST_PRINT_H
#define PST_PRINT_H

#include <stdarg.h>

#include "pst.h"

void pst_print_SetStream(void* stream);
void pst_print_PrintExceptionMessage();
void pst_print_PrintConnection(const PstConnection* conn);
void pst_print_PrintStatement(const PstPreparedStatement* prep_stmt, const unsigned long  prep_stmt_index);
void pst_print_PrintParameter(const PstParameter* param, const unsigned long param_markers_count, const unsigned long params_index);
void pst_print_PrintResultSet(const PstResultSet* result_set);
void pst_print_PrintExecutionMessage(const char* fmt, ...);

/**
 *  MySQL messages will be printed
 *  after the prepared statements are successfully executed.
 *
 *  All message format:
 */
 /*
 Msg1 RowsAffected:
 Query OK, 0 rows affected (0.02 sec)

 Msg2 RowsAffectedAndDuplicate:
 Query OK, 0 rows affected (4.27 sec)
 Records: 0  Duplicates: 0  Warnings: 0

 Msg3 RowsAffectedIncludeWarnings:
 Query OK, 0 rows affected, 2 warnings (0.02 sec)

 Msg4 RowsAffectedAndChanged:
 Query OK, 1 row affected (0.02 sec)
 Rows matched: 1  Changed: 1  Warnings: 0

 Msg5 RowsInSet:
 1 row in set (0.02 sec)

 Msg6 EmptySet:
 Empty set (0.02 sec)


 Result set will be printed as follows:
 ResultSet:
 +---------------------+---------+----------+----------+
 | Table               | Op      | Msg_type | Msg_text |
 +---------------------+---------+----------+----------+
 | employees.employees | analyze | status   | OK       |
 +---------------------+---------+----------+----------+

 */

 /* Msg1~4 all format Msg1 now */

#define pst_print_PrintRowsAffected(rows) pst_print_PrintExecutionMessage("Query OK, %d %s affected", rows, rows == 1 ? "row" : "rows")
#define pst_print_PrintRowsAffectedAndDuplicate(rows) pst_print_PrintRowsAffected(rows)
#define pst_print_PrintRowsAffectedIncludeWarnings(rows) pst_print_PrintRowsAffected(rows)
#define pst_print_PrintRowsAffectedAndChanged(rows) pst_print_PrintRowsAffected(rows)
#define pst_print_PrintRowsInSet(rows) pst_print_PrintExecutionMessage("%d %s in set", rows, rows == 1 ? "row" : "rows")
#define pst_print_PrintEmptySet() pst_print_PrintExecutionMessage("Empty set")

#endif  /* PST_PRINT_H */
