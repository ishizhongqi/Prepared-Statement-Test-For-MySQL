#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "pst_input.h"

/* global variable for parameter binding */
static MYSQL_BIND* bind = NULL;
static unsigned long g_count;

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <mysql/mysql.h>

static int BindParameters(PstParameter* param, unsigned long count) {
    /* free previous parameter binding */
    pst_input_FreeParameters();

    bind = (MYSQL_BIND*)malloc(count * sizeof(MYSQL_BIND));
    if (bind == NULL) {
        log_error(PST_FORMAT_MSG_ERR_ALLOC, "bind");
        return RET_ERR;
    }
    memset(bind, 0, count * sizeof(MYSQL_BIND));

    for (int i = 0; i < count; i++) {
        bind[i].length = 0;
        bind[i].is_null = (bool*)false;

        bind[i].is_unsigned = param[i].is_unsigned;
        bind[i].buffer_type = pst_ToMySQLFieldType(param[i].type);
        switch (bind[i].buffer_type) {
        case MYSQL_TYPE_TINY:
            bind[i].buffer = malloc(sizeof(signed char));
            if (bind[i].buffer == NULL) {
                log_error(PST_FORMAT_MSG_ERR_ALLOC, "bind->buffer");
                return RET_ERR;
            }
            memset(bind[i].buffer, 0, sizeof(signed char));
            *(signed char*)bind[i].buffer = (signed char)param[i].valuedouble;
            break;
        case MYSQL_TYPE_SHORT:
            bind[i].buffer = malloc(sizeof(short));
            if (bind[i].buffer == NULL) {
                log_error(PST_FORMAT_MSG_ERR_ALLOC, "bind->buffer");
                return RET_ERR;
            }
            memset(bind[i].buffer, 0, sizeof(short));
            *(short*)bind[i].buffer = (short)param[i].valuedouble;
            break;
        case MYSQL_TYPE_LONG:
            bind[i].buffer = malloc(sizeof(int));
            if (bind[i].buffer == NULL) {
                log_error(PST_FORMAT_MSG_ERR_ALLOC, "bind->buffer");
                return RET_ERR;
            }
            memset(bind[i].buffer, 0, sizeof(int));
            *(int*)bind[i].buffer = (int)param[i].valuedouble;
            break;
        case MYSQL_TYPE_LONGLONG:
            bind[i].buffer = malloc(sizeof(long long));
            if (bind[i].buffer == NULL) {
                log_error(PST_FORMAT_MSG_ERR_ALLOC, "bind->buffer");
                return RET_ERR;
            }
            memset(bind[i].buffer, 0, sizeof(long long));
            *(long long*)bind[i].buffer = (long long)param[i].valuedouble;
            break;
        case MYSQL_TYPE_FLOAT:
            bind[i].buffer = malloc(sizeof(float));
            if (bind[i].buffer == NULL) {
                log_error(PST_FORMAT_MSG_ERR_ALLOC, "bind->buffer");
                return RET_ERR;
            }
            memset(bind[i].buffer, 0, sizeof(float));
            *(float*)bind[i].buffer = (float)param[i].valuedouble;
            break;
        case MYSQL_TYPE_DOUBLE:
            bind[i].buffer = malloc(sizeof(double));
            if (bind[i].buffer == NULL) {
                log_error(PST_FORMAT_MSG_ERR_ALLOC, "bind->buffer");
                return RET_ERR;
            }
            memset(bind[i].buffer, 0, sizeof(double));
            *(double*)bind[i].buffer = (double)param[i].valuedouble;
            break;
        case MYSQL_TYPE_TIME:
        case MYSQL_TYPE_DATE:
        case MYSQL_TYPE_DATETIME:
        case MYSQL_TYPE_TIMESTAMP:
            bind[i].buffer = malloc(sizeof(MYSQL_TIME));
            if (bind[i].buffer == NULL) {
                log_error(PST_FORMAT_MSG_ERR_ALLOC, "bind->buffer");
                return RET_ERR;
            }
            memset(bind[i].buffer, 0, sizeof(MYSQL_TIME));
            *(MYSQL_TIME*)bind[i].buffer = pst_ToMySQLTime(param[i].valuestring);
            break;
        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_BLOB:
            bind[i].buffer_length = strlen(param[i].valuestring) + 1;
            bind[i].length = &bind[i].buffer_length;
            bind[i].buffer = malloc(bind[i].buffer_length);
            if (bind[i].buffer == NULL) {
                log_error(PST_FORMAT_MSG_ERR_ALLOC, "bind->buffer");
                return RET_ERR;
            }
            memset(bind[i].buffer, 0, bind[i].buffer_length);
            memcpy(bind[i].buffer, param[i].valuestring, bind[i].buffer_length);
            break;
        case MYSQL_TYPE_NULL:
            bind[i].is_null = (bool*)true;
        default:
            break;
        }
    }

    return RET_OK;
}

int pst_input_InputParameters(MYSQL_STMT* stmt, PstParameter* param, unsigned long count) {
    g_count = count;
    if (g_count != mysql_stmt_param_count(stmt)) {
        log_error("Param count not match, statement param count is %lu, input parameter count is %lu",
            mysql_stmt_param_count(stmt), g_count);
        return RET_ERR;
    }

    if (g_count == 0) {
        return RET_OK;
    }

    if (BindParameters(param, g_count) != RET_OK) {
        return RET_ERR;
    }

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        log_error(PST_FORMAT_MSG_ERR_MYSQL, mysql_stmt_errno(stmt), mysql_stmt_sqlstate(stmt), mysql_stmt_error(stmt));
        return RET_ERR;
    }

    return RET_OK;

}

void pst_input_FreeParameters() {
    if (bind) {
        for (unsigned long i = 0; i < g_count; i++) {
            if (bind[i].buffer) {
                free(bind[i].buffer);
                bind[i].buffer = NULL;
            }
        }
        free(bind);
        bind = NULL;
    }
}
