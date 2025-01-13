#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "pst_print.h"

static void* g_stream;

void pst_print_SetStream(void* stream) {
    g_stream = stream;
}

void pst_print_PrintExceptionMessage() {
    fprintf(g_stream, "An exception occurred, please check input content and log file.\n");
}

void pst_print_PrintConnection(const PstConnection* conn) {
    fprintf(g_stream, "User     : %s\n", conn->user);
    fprintf(g_stream, "Password : %s\n", conn->password);
    fprintf(g_stream, "Host     : %s\n", conn->host);
    fprintf(g_stream, "Port     : %u\n", conn->port);
    fprintf(g_stream, "Database : %s\n", conn->database);
    fprintf(g_stream, "\n");
}

void pst_print_PrintStatement(const PstPreparedStatement* prep_stmt, const unsigned long  prep_stmt_index) {
    fprintf(g_stream, "Statement[%ld]: %s\n", prep_stmt_index, prep_stmt->stmt);
}

void pst_print_PrintParameter(const PstParameter* param, const unsigned long param_markers_count, const unsigned long params_index) {
    fprintf(g_stream, "Parameter[%ld]: ", params_index);
    for (unsigned long i = 0; i < param_markers_count; i++) {
        PstFieldTypes type = pst_ToMySQLFieldType(param[i].type);
        switch (type) {
        case MYSQL_TYPE_TINY:
            fprintf(g_stream, "(%ld)%c ", i, (signed char)param[i].valuedouble);
            break;
        case MYSQL_TYPE_SHORT:
            fprintf(g_stream, "(%ld)%hd ", i, (short)param[i].valuedouble);
            break;
        case MYSQL_TYPE_LONG:
            fprintf(g_stream, "(%ld)%d ", i, (int)param[i].valuedouble);
            break;
        case MYSQL_TYPE_LONGLONG:
            fprintf(g_stream, "(%ld)%lld ", i, (long long)param[i].valuedouble);
            break;
        case MYSQL_TYPE_FLOAT:
            fprintf(g_stream, "(%ld)%f ", i, (float)param[i].valuedouble);
            break;
        case MYSQL_TYPE_DOUBLE:
            fprintf(g_stream, "(%ld)%lf ", i, (double)param[i].valuedouble);
            break;
        case MYSQL_TYPE_TIME:
        case MYSQL_TYPE_DATE:
        case MYSQL_TYPE_DATETIME:
        case MYSQL_TYPE_TIMESTAMP:
        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_BLOB:
            fprintf(g_stream, "(%ld)%s ", i, param[i].valuestring);
            break;
        case MYSQL_TYPE_NULL:
        default:
            break;
        }
    }
    fprintf(g_stream, "\n");
}

void pst_print_PrintResultSet(const PstResultSet* result_set) {
    /*
    example:
     ResultSet:
     +---------------------+---------+----------+----------+
     | Table               | Op      | Msg_type | Msg_text |
     +---------------------+---------+----------+----------+
     | employees.employees | analyze | status   | OK       |
     +---------------------+---------+----------+----------+
    */

    /* Top dashed */
    fprintf(g_stream, "+");
    for (uint64_t col = 0; col < result_set->column_count; col++) {
        fprintf(g_stream, "-");
        for (unsigned long i = 0; i < result_set->result[0][col].field_length; i++) {
            fprintf(g_stream, "-");
        }
        fprintf(g_stream, "-+");
    }
    fprintf(g_stream, "\n");

    /* Title */
    fprintf(g_stream, "|");
    for (uint64_t col = 0; col < result_set->column_count; col++) {
        fprintf(g_stream, " %-*s ", (int)(result_set->result[0][col]).field_length, (result_set->result[0][col]).valuestring);
        fprintf(g_stream, "|");
    }
    fprintf(g_stream, "\n");

    /* Title dashed */
    fprintf(g_stream, "+");
    for (uint64_t col = 0; col < result_set->column_count; col++) {
        fprintf(g_stream, "-");
        for (unsigned long i = 0; i < result_set->result[0][col].field_length; i++) {
            fprintf(g_stream, "-");
        }
        fprintf(g_stream, "-+");
    }
    fprintf(g_stream, "\n");


    /* Result set */
    for (uint64_t row = 1; row < result_set->row_count; row++) {
        fprintf(g_stream, "|");
        for (uint64_t col = 0; col < result_set->column_count; col++) {
            fprintf(g_stream, " %-*s ", (int)(result_set->result[0][col]).field_length, (result_set->result[row][col]).valuestring);
            fprintf(g_stream, "|");
        }
        fprintf(g_stream, "\n");
    }

    /* Bottom dashed */
    fprintf(g_stream, "+");
    for (uint64_t col = 0; col < result_set->column_count; col++) {
        fprintf(g_stream, "-");
        for (unsigned long i = 0; i < result_set->result[0][col].field_length; i++) {
            fprintf(g_stream, "-");
        }
        fprintf(g_stream, "-+");
    }
    fprintf(g_stream, "\n");

}

void pst_print_PrintExecutionMessage(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(g_stream, fmt, args);
    va_end(args);
    fprintf(g_stream, "\n");
    fprintf(g_stream, "\n");
}
