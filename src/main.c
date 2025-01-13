#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mysql/mysql.h>

#include "log.h"
#include "pst.h"
#include "pst_parse.h"
#include "pst_print.h"
#include "pst_input.h"
#include "pst_output.h"

static void FreeResources(FILE* log_file, MYSQL* mysql, MYSQL_STMT* stmt) {
    pst_input_FreeParameters();
    pst_output_FreeResult();

    if (stmt) {
        mysql_stmt_close(stmt);
        stmt = NULL;
    }

    if (mysql) {
        mysql_close(mysql);
        mysql = NULL;
    }

    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }

    pst_parse_Free();
}

int main(int argc, char* argv[]) {
    /* Check arguments */
    char file_json[256];
    memset(file_json, 0, sizeof(file_json));
    if (argc < 2) {
        strcpy(file_json, argv[0]);
        char* p = strrchr(file_json, '/');
        p[0] = 0;
        strcat(file_json, "/statement.json");
        fprintf(stdout, "No arguments is provided, the default file '%s' will be input.\n", file_json);
    } else if (argc == 2) {
        strcpy(file_json, argv[1]);
    } else {
        fprintf(stderr, "Too many arguments is provided.\n");
        return RET_ERR;
    }

    if (access(file_json, R_OK) != 0) {
        fprintf(stderr, "Can not access file '%s'.\n", file_json);
        return RET_ERR;
    }

    /* Log file */
    FILE* file_log = fopen("pst.log", "a+");
    if (file_log == NULL) {
        fprintf(stderr, "Can not open log file.\n");
        return RET_ERR;
    }

    log_set_level(LOG_ERROR);
    log_add_fp(file_log, LOG_TRACE);

    /* Set stream for output */
    pst_print_SetStream(stdout);

    /* Parse statement json */
    if (pst_parse_Parse(file_json) != RET_OK) {
        FreeResources(file_log, NULL, NULL);
        pst_print_PrintExceptionMessage();
        return RET_ERR;
    }

    PstConnection* connection = pst_parse_GetConnection();
    if (connection == NULL) {
        FreeResources(file_log, NULL, NULL);
        pst_print_PrintExceptionMessage();
        return RET_ERR;
    }
    log_info("Get connection information successfully.");
    pst_print_PrintConnection(connection);

    PstPreparedStatements* prepared_statements = pst_parse_GetPreparedStatement();
    if (prepared_statements == NULL) {
        FreeResources(file_log, NULL, NULL);
        pst_print_PrintExceptionMessage();
        return RET_ERR;
    }
    log_info("Get prepared statements successfully.");

    /* Connect MySQL database */
    MYSQL* mysql = NULL;

    mysql = mysql_init(NULL);
    if (mysql == NULL) {
        log_error("Failed to initialize MySQL client");
        FreeResources(file_log, mysql, NULL);
        pst_print_PrintExceptionMessage();
        return RET_ERR;
    }
    log_info("Successfully initialized MySQL client.");

    if (mysql_real_connect(mysql,
        connection->host, connection->user, connection->password, connection->database, connection->port,
        connection->unix_socket, connection->client_flag) == NULL) {
        log_error(PST_FORMAT_MSG_ERR_MYSQL, mysql_errno(mysql), mysql_sqlstate(mysql), mysql_error(mysql));
        FreeResources(file_log, mysql, NULL);
        pst_print_PrintExceptionMessage();
        return RET_ERR;
    }
    log_info("Successfully connected to database.");

    /*  Prepare statements */
    MYSQL_STMT* stmt = mysql_stmt_init(mysql);
    if (stmt == NULL) {
        log_error(PST_FORMAT_MSG_ERR_MYSQL, mysql_errno(mysql), mysql_sqlstate(mysql), mysql_error(mysql));
        FreeResources(file_log, mysql, stmt);
        pst_print_PrintExceptionMessage();
        return RET_ERR;
    }
    log_info("MySQL statement initialized");

    for (unsigned long i = 0; i < prepared_statements->prep_stmt_size; i++) {

        if (mysql_stmt_prepare(stmt, prepared_statements->prep_stmt[i].stmt, prepared_statements->prep_stmt[i].stmt_len) != 0) {
            log_error(PST_FORMAT_MSG_ERR_MYSQL, mysql_stmt_errno(stmt), mysql_stmt_sqlstate(stmt), mysql_stmt_error(stmt));
            FreeResources(file_log, mysql, stmt);
            pst_print_PrintExceptionMessage();
            return RET_ERR;
        }
        log_info("Statement prepared");

        pst_print_PrintStatement(&prepared_statements->prep_stmt[i], i);

        PstSyntax syntax = pst_GetSyntax(prepared_statements->prep_stmt[i].stmt);
        log_info("Syntax : %d", syntax);

        if (prepared_statements->prep_stmt[i].params_size == 0) {
            if (mysql_stmt_execute(stmt) != 0) {
                log_error(PST_FORMAT_MSG_ERR_MYSQL, mysql_stmt_errno(stmt), mysql_stmt_sqlstate(stmt), mysql_stmt_error(stmt));
                FreeResources(file_log, mysql, stmt);
                pst_print_PrintExceptionMessage();
                return RET_ERR;
            }

            if (pst_output_OutputResult(stmt, syntax) != RET_OK) {
                FreeResources(file_log, mysql, stmt);
                pst_print_PrintExceptionMessage();
                return RET_ERR;
            }

            if (mysql_stmt_reset(stmt) != 0) {
                FreeResources(file_log, mysql, stmt);
                pst_print_PrintExceptionMessage();
                return RET_ERR;
            }
        }

        for (unsigned long j = 0; j < prepared_statements->prep_stmt[i].params_size; j++) {
            if (pst_input_InputParameters(stmt, prepared_statements->prep_stmt[i].params[j], prepared_statements->prep_stmt[i].param_markers_count) != RET_OK) {
                FreeResources(file_log, mysql, stmt);
                pst_print_PrintExceptionMessage();
                return RET_ERR;
            }
            pst_print_PrintParameter(prepared_statements->prep_stmt[i].params[j], prepared_statements->prep_stmt[i].param_markers_count, j);

            if (mysql_stmt_execute(stmt) != 0) {
                log_error(PST_FORMAT_MSG_ERR_MYSQL, mysql_stmt_errno(stmt), mysql_stmt_sqlstate(stmt), mysql_stmt_error(stmt));
                FreeResources(file_log, mysql, stmt);
                pst_print_PrintExceptionMessage();
                return RET_ERR;
            }

            pst_input_FreeParameters();

            if (pst_output_OutputResult(stmt, syntax) != RET_OK) {
                FreeResources(file_log, mysql, stmt);
                pst_print_PrintExceptionMessage();
                return RET_ERR;
            }

            pst_output_FreeResult();

            if (mysql_stmt_reset(stmt) != 0) {
                FreeResources(file_log, mysql, stmt);
                pst_print_PrintExceptionMessage();
                return RET_ERR;
            }

        }
    }

    FreeResources(file_log, mysql, stmt);

    log_info("MySQL client closed.");

    return 0;
}
