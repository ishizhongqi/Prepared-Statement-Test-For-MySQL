#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "log.h"
#include "pst_parse.h"

/* global variables */
static PstConnection* conn;
static PstPreparedStatements* prep_stmts;

/* declarations */
static char* ReadLine(FILE* file, char* buffer);
static int InitBuffer();

int pst_parse_Parse(const char* filename) {
    if (InitBuffer() != RET_OK) {
        return RET_ERR;
    }

    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        log_error(PST_FORMAT_MSG_ERR_FOPEN, filename);
        return RET_ERR;
    }

    char* buffer = NULL;
    char* str = NULL;
    unsigned long str_len = 0;
    while ((buffer = ReadLine(fp, buffer)) != NULL) {
        if (str) {
            str = realloc(str, str_len + strlen(buffer) + 1);
        } else {
            str = malloc(strlen(buffer) + 1);
        }
        if (!str) {
            log_error(PST_FORMAT_MSG_ERR_ALLOC, "string buffer");
            fclose(fp);
            return RET_ERR;
        }
        memset(str + str_len, 0, strlen(buffer) + 1);
        strcat(str, buffer);
        str_len += strlen(str);
    }

    log_debug("File Content: %s", str);
    fclose(fp);

    /* parse json string */
    cJSON* root = cJSON_Parse(str);
    if (root == NULL) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            log_error("Error before: %s\n", error_ptr);
        }
        cJSON_Delete(root);
        free(str);
        str = NULL;
        return RET_ERR;
    }

    cJSON* cjson_user = NULL;
    cJSON* cjson_password = NULL;
    cJSON* cjson_host = NULL;
    cJSON* cjson_port = NULL;
    cJSON* cjson_database = NULL;

    cjson_user = cJSON_GetObjectItemCaseSensitive(root, "user");
    if (!cJSON_IsString(cjson_user) && cjson_user->valuestring == NULL) {
        log_error("user is not found or is null");
        cJSON_Delete(root);
        free(str);
        str = NULL;
        return RET_ERR;
    }

    cjson_password = cJSON_GetObjectItemCaseSensitive(root, "password");
    if (!cJSON_IsString(cjson_password) && cjson_password->valuestring == NULL) {
        log_error("password is not found or is null");
        cJSON_Delete(root);
        free(str);
        str = NULL;
        return RET_ERR;
    }

    cjson_host = cJSON_GetObjectItemCaseSensitive(root, "host");
    if (!cJSON_IsString(cjson_host) && cjson_host->valuestring == NULL) {
        log_error("host is not found or is null");
        cJSON_Delete(root);
        free(str);
        str = NULL;
        return RET_ERR;
    }

    cjson_port = cJSON_GetObjectItemCaseSensitive(root, "port");
    if (!cJSON_IsNumber(cjson_port) && cjson_port->valueint == 0) {
        log_error("port is not found or is null");
        cJSON_Delete(root);
        free(str);
        str = NULL;
        return RET_ERR;
    }

    cjson_database = cJSON_GetObjectItemCaseSensitive(root, "database");
    if (!cJSON_IsString(cjson_database) && cjson_database->valuestring == NULL) {
        log_error("database is not found or is null");
        cJSON_Delete(root);
        free(str);
        str = NULL;
        return RET_ERR;
    }

    log_debug("user: %s, password: %s, host: %s, port: %u, database: %s",
        cjson_user->valuestring,
        cjson_password->valuestring,
        cjson_host->valuestring,
        (unsigned int)cjson_port->valuedouble,
        cjson_database->valuestring);

    strcpy(conn->user, cjson_user->valuestring);
    strcpy(conn->password, cjson_password->valuestring);
    strcpy(conn->host, cjson_host->valuestring);
    conn->port = (unsigned int)cjson_port->valuedouble;
    strcpy(conn->database, cjson_database->valuestring);

    cJSON* cjson_prepared_statements = NULL;
    int    cjson_prepared_statements_size = 0;

    cJSON* cjson_prepared_statement = NULL;
    cJSON* cjson_statement = NULL;

    cJSON* cjson_parameters = NULL;
    int    cjson_parameters_size = 0;

    cJSON* cjson_parameter = NULL;
    int    cjson_parameter_count = 0;

    cJSON* cjson_parameter_item = NULL;
    cJSON* cjson_parameter_item_type = NULL;
    cJSON* cjson_parameter_item_unsigned = NULL;
    cJSON* cjson_parameter_item_value = NULL;

    cjson_prepared_statements = cJSON_GetObjectItemCaseSensitive(root, "prepared_statement");
    cjson_prepared_statements_size = cJSON_GetArraySize(cjson_prepared_statements);
    if (!cJSON_IsArray(cjson_prepared_statements) && cjson_prepared_statements_size == 0) {
        log_error("prepared_statement is not found or is null");
        cJSON_Delete(root);
        free(str);
        str = NULL;
        return RET_ERR;
    }

    prep_stmts->prep_stmt_size = cjson_prepared_statements_size;
    prep_stmts->prep_stmt = (PstPreparedStatement*)malloc(prep_stmts->prep_stmt_size * sizeof(PstPreparedStatement));
    if (!prep_stmts->prep_stmt) {
        log_error(PST_FORMAT_MSG_ERR_ALLOC, "prepared statement");
        cJSON_Delete(root);
        free(str);
        str = NULL;
        return RET_ERR;
    }
    memset(prep_stmts->prep_stmt, 0, prep_stmts->prep_stmt_size * sizeof(PstPreparedStatement));

    for (int i = 0; i < cjson_prepared_statements_size; i++) {
        cjson_prepared_statement = cJSON_GetArrayItem(cjson_prepared_statements, i);
        if (!cJSON_IsObject(cjson_prepared_statement)) {
            log_error("prepared_statement is not an object");
            cJSON_Delete(root);
            free(str);
            str = NULL;
            return RET_ERR;
        }

        cjson_statement = cJSON_GetObjectItemCaseSensitive(cjson_prepared_statement, "statement");
        if (!cJSON_IsString(cjson_statement) && cjson_statement->valuestring == NULL) {
            log_error("statement is not found or is null");
            cJSON_Delete(root);
            free(str);
            str = NULL;
            return RET_ERR;
        }
        log_debug("statement: %s", cjson_statement->valuestring);

        prep_stmts->prep_stmt[i].stmt = (char*)malloc(strlen(cjson_statement->valuestring) + 1);
        if (!prep_stmts->prep_stmt[i].stmt) {
            log_error(PST_FORMAT_MSG_ERR_ALLOC, "prepared statement");
            cJSON_Delete(root);
            free(str);
            str = NULL;
            return RET_ERR;
        }
        memset(prep_stmts->prep_stmt[i].stmt, 0, strlen(cjson_statement->valuestring) + 1);
        memcpy(prep_stmts->prep_stmt[i].stmt, cjson_statement->valuestring, strlen(cjson_statement->valuestring));
        prep_stmts->prep_stmt[i].stmt_len = strlen(prep_stmts->prep_stmt[i].stmt);

        cjson_parameters = cJSON_GetObjectItemCaseSensitive(cjson_prepared_statement, "parameter");
        cjson_parameters_size = cJSON_GetArraySize(cjson_parameters);
        if (!cJSON_IsArray(cjson_parameters) && cjson_parameters_size == 0) {
            log_error("parameter is not found or is null");
            cJSON_Delete(root);
            free(str);
            str = NULL;
            return RET_ERR;
        }

        prep_stmts->prep_stmt[i].params_size = cjson_parameters_size;
        prep_stmts->prep_stmt[i].params = (PstParameter**)malloc(prep_stmts->prep_stmt[i].params_size * sizeof(PstParameter*));
        if (!prep_stmts->prep_stmt[i].params) {
            log_error(PST_FORMAT_MSG_ERR_ALLOC, "parameter");
            cJSON_Delete(root);
            free(str);
            str = NULL;
            return RET_ERR;
        }
        memset(prep_stmts->prep_stmt[i].params, 0, prep_stmts->prep_stmt[i].params_size * sizeof(PstParameter*));

        for (int j = 0; j < cjson_parameters_size; j++) {
            cjson_parameter = cJSON_GetArrayItem(cjson_parameters, j);
            cjson_parameter_count = cJSON_GetArraySize(cjson_parameter);
            if (!cJSON_IsArray(cjson_parameter) && cjson_parameter_count == 0) {
                log_error("parameter is not found or is null");
                cJSON_Delete(root);
                free(str);
                str = NULL;
                return RET_ERR;
            }

            prep_stmts->prep_stmt[i].param_markers_count = cjson_parameter_count;
            prep_stmts->prep_stmt[i].params[j] = (PstParameter*)malloc(prep_stmts->prep_stmt[i].param_markers_count * sizeof(PstParameter));
            if (!prep_stmts->prep_stmt[i].params[j]) {
                log_error(PST_FORMAT_MSG_ERR_ALLOC, "parameter");
                cJSON_Delete(root);
                free(str);
                str = NULL;
                return RET_ERR;
            }
            memset(prep_stmts->prep_stmt[i].params[j], 0, prep_stmts->prep_stmt[i].param_markers_count * sizeof(PstParameter));

            for (int k = 0; k < cjson_parameter_count; k++) {
                cjson_parameter_item = cJSON_GetArrayItem(cjson_parameter, k);
                if (!cJSON_IsObject(cjson_parameter_item)) {
                    log_error("parameter is not an object");
                    cJSON_Delete(root);
                    free(str);
                    str = NULL;
                    return RET_ERR;
                }

                cjson_parameter_item_type = cJSON_GetObjectItemCaseSensitive(cjson_parameter_item, "type");
                if (!cJSON_IsString(cjson_parameter_item_type) && cjson_parameter_item_type->valuestring == NULL) {
                    log_error("type is not found or is null");
                    cJSON_Delete(root);
                    free(str);
                    str = NULL;
                    return RET_ERR;
                }
                log_debug("type: %s", cjson_parameter_item_type->valuestring);

                memcpy(prep_stmts->prep_stmt[i].params[j][k].type, cjson_parameter_item_type->valuestring, strlen(cjson_parameter_item_type->valuestring));

                cjson_parameter_item_unsigned = cJSON_GetObjectItemCaseSensitive(cjson_parameter_item, "unsigned");
                if (cJSON_IsTrue(cjson_parameter_item_unsigned)) {
                    prep_stmts->prep_stmt[i].params[j][k].is_unsigned = true;
                } else {
                    prep_stmts->prep_stmt[i].params[j][k].is_unsigned = false;
                }
                log_debug("unsigned: %s", prep_stmts->prep_stmt[i].params[j][k].is_unsigned ? "true" : "false");

                cjson_parameter_item_value = cJSON_GetObjectItemCaseSensitive(cjson_parameter_item, "value");
                if (cJSON_IsString(cjson_parameter_item_value) && cjson_parameter_item_value->valuestring != NULL) {
                    log_debug("value: %s", cjson_parameter_item_value->valuestring);
                    prep_stmts->prep_stmt[i].params[j][k].valuestring = malloc(strlen(cjson_parameter_item_value->valuestring) + 1);
                    if (!prep_stmts->prep_stmt[i].params[j][k].valuestring) {
                        log_error(PST_FORMAT_MSG_ERR_ALLOC, "parameter value");
                        cJSON_Delete(root);
                        free(str);
                        str = NULL;
                        return RET_ERR;
                    }
                    memset(prep_stmts->prep_stmt[i].params[j][k].valuestring, 0, strlen(cjson_parameter_item_value->valuestring) + 1);
                    memcpy(prep_stmts->prep_stmt[i].params[j][k].valuestring, cjson_parameter_item_value->valuestring, strlen(cjson_parameter_item_value->valuestring));
                } else if (cJSON_IsNumber(cjson_parameter_item_value)) {
                    log_debug("value: %lf", cjson_parameter_item_value->valuedouble);
                    prep_stmts->prep_stmt[i].params[j][k].valuedouble = cjson_parameter_item_value->valuedouble;
                } else {
                    log_error("value is not found or is null");
                    cJSON_Delete(root);
                    free(str);
                    str = NULL;
                    return RET_ERR;
                }

            }
        }
    }

    cJSON_Delete(root);
    free(str);
    str = NULL;

    return RET_OK;
}

PstConnection* pst_parse_GetConnection() {
    return conn;
}


PstPreparedStatements* pst_parse_GetPreparedStatement() {
    return prep_stmts;
}

void pst_parse_Free() {
    /* free connection memory */
    if (conn) {
        free(conn);
        conn = NULL;
    }

    /* free prepared statement memory */
    if (prep_stmts) {
        if (prep_stmts->prep_stmt) {
            for (int i = 0; i < prep_stmts->prep_stmt_size; i++) {
                if (prep_stmts->prep_stmt[i].stmt) {
                    free(prep_stmts->prep_stmt[i].stmt);
                    prep_stmts->prep_stmt[i].stmt = NULL;
                }
                for (int j = 0; j < prep_stmts->prep_stmt[i].params_size; j++) {
                    if (prep_stmts->prep_stmt[i].params[j]) {
                        for (int k = 0; k < prep_stmts->prep_stmt[i].param_markers_count; k++) {
                            if (prep_stmts->prep_stmt[i].params[j][k].valuestring) {
                                free(prep_stmts->prep_stmt[i].params[j][k].valuestring);
                                prep_stmts->prep_stmt[i].params[j][k].valuestring = NULL;
                            }
                        }
                        free(prep_stmts->prep_stmt[i].params[j]);
                        prep_stmts->prep_stmt[i].params[j] = NULL;
                    }
                }
                free(prep_stmts->prep_stmt[i].params);
                prep_stmts->prep_stmt[i].params = NULL;
            }
            free(prep_stmts->prep_stmt);
            prep_stmts->prep_stmt = NULL;
        }
        free(prep_stmts);
        prep_stmts = NULL;
    }
}


/* static functions */
static char* ReadLine(FILE* file, char* buffer) {
    if (buffer) {
        free(buffer);
        buffer = NULL;
    }
    int buffer_size = 1024;
    buffer = (char*)malloc(buffer_size);
    if (!buffer) {
        log_error(PST_FORMAT_MSG_ERR_ALLOC, "file buffer");
        return NULL;
    }
    memset(buffer, 0, buffer_size);

    int position = 0;
    int c;

    while ((c = fgetc(file)) != EOF) {
        /* if c is a line break, break the loop */
        if (c == '\n') {
            break;
        }

        /* add c to buffer */
        buffer[position++] = c;

        /* check if buffer is full */
        if (position >= buffer_size) {
            buffer_size *= 2; /* add buffer size */
            char* new_buffer = (char*)realloc(buffer, buffer_size);
            if (!new_buffer) {
                free(buffer); /* free old buffer */
                log_error(PST_FORMAT_MSG_ERR_ALLOC, "file buffer");
                return NULL;
            }
            memset(new_buffer + position, 0, buffer_size - position);
            buffer = new_buffer;
        }
    }

    /* if buffer is empty and EOF, return NULL */
    if (position == 0 && c == EOF) {
        free(buffer);
        return NULL;
    }

    /* add terminator to buffer */
    buffer[position] = '\0';

    return buffer;
}

static int InitBuffer() {
    conn = (PstConnection*)malloc(sizeof(PstConnection));
    if (!conn) {
        log_error(PST_FORMAT_MSG_ERR_ALLOC, "connection");
        return RET_ERR;
    }
    memset(conn, 0, sizeof(PstConnection));

    prep_stmts = (PstPreparedStatements*)malloc(sizeof(PstPreparedStatements));
    if (!prep_stmts) {
        log_error(PST_FORMAT_MSG_ERR_ALLOC, "prepared statement");
        return RET_ERR;
    }
    memset(prep_stmts, 0, sizeof(PstPreparedStatements));

    return RET_OK;
}

