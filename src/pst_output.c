#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "pst_output.h"
#include "pst_print.h"

static MYSQL_STMT* g_stmt = NULL;
static uint64_t rows;
static int ret;
static MYSQL_RES* result_metadata = NULL;
static PstResultSet* result_set = NULL;
static MYSQL_BIND* bind = NULL;
static PstResult* result = NULL;

static void GetRowsAffected();
static int InitResultSet();
static int FetchResultSet();
static void GetResultSet();

/* Output of SQL Syntax Permitted in Prepared Statements */
static void PrintAlterTable();
static void PrintAlterUser();
static void PrintAnalyzeTable();
static void PrintCacheIndex();
static void PrintCall();
static void PrintChange();
static void PrintCheckSum();
static void PrintCommit();
static void PrintCreateOrDropIndex();
static void PrintCreateOrRenameOrDropDatabase();
static void PrintCreateOrDropTable();
static void PrintCreateOrRenameOrDropUser();
static void PrintCreateOrDropView();
static void PrintDelete();
static void PrintDo();
static void PrintFlush();
static void PrintGrant();
static void PrintInsert();
static void PrintInsertSelect();
static void PrintInstallPlugin();
static void PrintKill();
static void PrintLoadIndexIntoCache();
static void PrintOptimizeTable();
static void PrintRenameTable();
static void PrintRepairTable();
static void PrintReplace();
static void PrintReset();
static void PrintRevoke();
static void PrintSelect();
static void PrintSet();
static void PrintShow();
static void PrintShowCreate();
static void PrintStartOrStopReplica();
static void PrintTruncate();
static void PrintUninstallPlugin();
static void PrintUpdate();

int pst_output_OutputResult(MYSQL_STMT* stmt, PstSyntax syntax) {
    g_stmt = stmt;
    rows = 0;
    ret = RET_OK;

    switch (syntax) {
    case PstSyntax_AlterTable: PrintAlterTable(); break;
    case PstSyntax_AlterUser: PrintAlterUser(); break;
    case PstSyntax_AnalyzeTable: PrintAnalyzeTable(); break;
    case PstSyntax_CacheIndex: PrintCacheIndex(); break;
    case PstSyntax_Call: PrintCall(); break;
    case PstSyntax_Change: PrintChange(); break;
    case PstSyntax_CheckSum: PrintCheckSum(); break;
    case PstSyntax_Commit: PrintCommit(); break;
    case PstSyntax_CreateOrDropIndex: PrintCreateOrDropIndex(); break;
    case PstSyntax_CreateOrRenameOrDropDatabase: PrintCreateOrRenameOrDropDatabase(); break;
    case PstSyntax_CreateOrDropTable: PrintCreateOrDropTable(); break;
    case PstSyntax_CreateOrRenameOrDropUser: PrintCreateOrRenameOrDropUser(); break;
    case PstSyntax_CreateOrDropView: PrintCreateOrDropView(); break;
    case PstSyntax_Delete: PrintDelete(); break;
    case PstSyntax_Do: PrintDo(); break;
    case PstSyntax_Flush: PrintFlush(); break;
    case PstSyntax_Grant: PrintGrant(); break;
    case PstSyntax_Insert: PrintInsert(); break;
    case PstSyntax_InsertSelect: PrintInsertSelect(); break;
    case PstSyntax_InstallPlugin: PrintInstallPlugin(); break;
    case PstSyntax_Kill: PrintKill(); break;
    case PstSyntax_LoadIndexIntoCache: PrintLoadIndexIntoCache(); break;
    case PstSyntax_OptimizeTable: PrintOptimizeTable(); break;
    case PstSyntax_RenameTable: PrintRenameTable(); break;
    case PstSyntax_RepairTable: PrintRepairTable(); break;
    case PstSyntax_Replace: PrintReplace(); break;
    case PstSyntax_Reset: PrintReset(); break;
    case PstSyntax_Revoke: PrintRevoke(); break;
    case PstSyntax_Select: PrintSelect(); break;
    case PstSyntax_Set: PrintSet(); break;
    case PstSyntax_Show: PrintShow(); break;
    case PstSyntax_ShowCreate: PrintShowCreate(); break;
    case PstSyntax_StartOrStopReplica: PrintStartOrStopReplica(); break;
    case PstSyntax_Truncate: PrintTruncate(); break;
    case PstSyntax_UninstallPlugin: PrintUninstallPlugin(); break;
    case PstSyntax_Update: PrintUpdate(); break;

    default: ret = RET_ERR; break;
    }

    return ret;
}

void pst_output_FreeResult() {
    if (result != NULL) {
        for (uint64_t col = 0; col < result_set->column_count; col++) {
            if (result[col].valuestring != NULL) {
                free(result[col].valuestring);
                result[col].valuestring = NULL;
            }
            if (result[col].value != NULL) {
                free(result[col].value);
                result[col].value = NULL;
            }
        }
        free(result);
        result = NULL;
    }

    if (result_set != NULL) {
        if (result_set->result != NULL) {
            for (uint64_t row = 0; row < result_set->row_count; row++) {
                if (result_set->result[row] != NULL) {
                    for (uint64_t col = 0; col < result_set->column_count; col++) {
                        if (result_set->result[row][col].valuestring != NULL) {
                            free(result_set->result[row][col].valuestring);
                            result_set->result[row][col].valuestring = NULL;
                        }
                        if (result_set->result[row][col].value != NULL) {
                            free(result_set->result[row][col].value);
                            result_set->result[row][col].value = NULL;
                        }
                    }
                    free(result_set->result[row]);
                    result_set->result[row] = NULL;
                }
            }
            free(result_set->result);
            result_set->result = NULL;
        }
        free(result_set);
        result_set = NULL;
    }

    if (bind != NULL) {
        free(bind);
        bind = NULL;
    }

    mysql_free_result(result_metadata);
    mysql_stmt_free_result(g_stmt);
}

static void GetRowsAffected() {
    rows = mysql_stmt_affected_rows(g_stmt);
}

static int InitResultSet() {
    result_set = (PstResultSet*)malloc(sizeof(PstResultSet));
    if (result_set == NULL) {
        log_error(PST_FORMAT_MSG_ERR_ALLOC, "result_set");
        return RET_ERR;
    }
    memset(result_set, 0, sizeof(PstResultSet));

    bind = NULL;
    result = NULL;

    return RET_OK;
}

static int FetchResultSet() {
    /* Get metadata */
    result_metadata = mysql_stmt_result_metadata(g_stmt);
    if (result_metadata == NULL) {
        log_error(PST_FORMAT_MSG_ERR_MYSQL, mysql_stmt_errno(g_stmt), mysql_stmt_sqlstate(g_stmt), mysql_stmt_error(g_stmt));
        return RET_ERR;
    }

    /* Get fields and max length */
    unsigned int field_count = mysql_num_fields(result_metadata);
    MYSQL_FIELD* fields = mysql_fetch_fields(result_metadata);

    int update_max_length = 1;
    if (mysql_stmt_attr_set(g_stmt, STMT_ATTR_UPDATE_MAX_LENGTH, &update_max_length)) {
        log_error(PST_FORMAT_MSG_ERR_MYSQL, mysql_stmt_errno(g_stmt), mysql_stmt_sqlstate(g_stmt), mysql_stmt_error(g_stmt));
        return RET_ERR;
    }

    /* Store result */
    if (mysql_stmt_store_result(g_stmt)) {
        log_error(PST_FORMAT_MSG_ERR_MYSQL, mysql_stmt_errno(g_stmt), mysql_stmt_sqlstate(g_stmt), mysql_stmt_error(g_stmt));
        return RET_ERR;
    }

    /* Get columns and rows */
    result_set->column_count = field_count;
    rows = mysql_stmt_num_rows(g_stmt);
    if (rows == 0) {
        return RET_OK;
    }
    result_set->row_count = rows + 1;

    result_set->result = (PstResult**)malloc(sizeof(PstResult*) * result_set->row_count);
    if (result_set->result == NULL) {
        log_error(PST_FORMAT_MSG_ERR_ALLOC, "result");
        return RET_ERR;
    }
    memset(result_set->result, 0, sizeof(PstResult*) * result_set->row_count);

    for (uint64_t row = 0; row < result_set->row_count; row++) {
        result_set->result[row] = (PstResult*)malloc(sizeof(PstResult) * result_set->column_count);
        if (result_set->result[row] == NULL) {
            log_error(PST_FORMAT_MSG_ERR_ALLOC, "result");
            return RET_ERR;
        }
        memset(result_set->result[row], 0, sizeof(PstResult) * result_set->column_count);
    }

    /* Bind results */
    bind = (MYSQL_BIND*)malloc(sizeof(MYSQL_BIND) * result_set->column_count);
    if (bind == NULL) {
        log_error(PST_FORMAT_MSG_ERR_ALLOC, "bind");
        return RET_ERR;
    }
    memset(bind, 0, sizeof(MYSQL_BIND) * result_set->column_count);

    result = (PstResult*)malloc(sizeof(PstResult) * result_set->column_count);
    if (result == NULL) {
        log_error(PST_FORMAT_MSG_ERR_ALLOC, "result");
        return RET_ERR;
    }
    memset(result, 0, sizeof(PstResult) * result_set->column_count);

    for (uint64_t col = 0; col < result_set->column_count; col++) {
        result[col].type = fields[col].type;
        result[col].max_length = fields[col].max_length + 1;
        result[col].value = malloc(result[col].max_length + 63);
        if (result[col].value == NULL) {
            log_error(PST_FORMAT_MSG_ERR_ALLOC, "result->value");
            return RET_ERR;
        }
        memset(result[col].value, 0, result[col].max_length + 63);
        memset(&bind[col], 0, sizeof(MYSQL_BIND));
        bind[col].buffer_type = result[col].type;
        bind[col].buffer = result[col].value;
        bind[col].buffer_length = result[col].max_length;
        bind[col].length = &result[col].length;
        bind[col].is_null = &result[col].is_null;
        bind[col].error = &result[col].error;
    }

    if (mysql_stmt_bind_result(g_stmt, bind)) {
        log_error(PST_FORMAT_MSG_ERR_MYSQL, mysql_stmt_errno(g_stmt), mysql_stmt_sqlstate(g_stmt), mysql_stmt_error(g_stmt));
        return RET_ERR;
    }


    /* Store fields */
    /* Metadata */
    for (uint64_t col = 0; col < result_set->column_count; col++) {
        result_set->result[0][col].type = fields[col].type;
        result_set->result[0][col].length = 56;
        result_set->result[0][col].max_length = fields[col].max_length + 1;
        result_set->result[0][col].field_length = fields[col].name_length;
        result_set->result[0][col].valuestring = malloc(result_set->result[0][col].field_length + 63);
        if (result_set->result[0][col].valuestring == NULL) {
            log_error(PST_FORMAT_MSG_ERR_ALLOC, "result->value");
            return RET_ERR;
        }
        memset(result_set->result[0][col].valuestring, 0, result_set->result[0][col].field_length + 63);
        strcpy(result_set->result[0][col].valuestring, fields[col].name);
        result_set->result[0][col].value = malloc(result_set->result[0][col].field_length + 63);
        if (result_set->result[0][col].value == NULL) {
            log_error(PST_FORMAT_MSG_ERR_ALLOC, "result->value");
            return RET_ERR;
        }
        memset(result_set->result[0][col].value, 0, result_set->result[0][col].field_length + 63);
        memcpy(result_set->result[0][col].value, result_set->result[0][col].valuestring, result_set->result[0][col].field_length + 63);
        result_set->result[0][col].is_null = 0;
        result_set->result[0][col].error = 0;
    }

    /* Data */
    int row = 0;
    while (1) {
        int ret = mysql_stmt_fetch(g_stmt);
        if (ret == 1 || ret == MYSQL_NO_DATA) {
            break;
        }

        if (row > result_set->row_count) {
            log_error("The number of rows obtained using mysql_stmt_fetch does not match the number of rows obtained using mysql_stmt_num_rows.");
            return RET_ERR;
        }

        for (uint64_t col = 0; col < result_set->column_count; col++) {

            result[col].valuestring = malloc(result_set->result[0][col].max_length + 63);
            if (result[col].valuestring == NULL) {
                log_error(PST_FORMAT_MSG_ERR_ALLOC, "result->valuestring");
                return RET_ERR;
            }
            memset(result[col].valuestring, 0, result_set->result[0][col].max_length + 63);

            switch (result[col].type) {
            case MYSQL_TYPE_TINY:
                sprintf(result[col].valuestring, "%c", *(unsigned char*)result[col].value);
                break;
            case MYSQL_TYPE_SHORT:
                sprintf(result[col].valuestring, "%hd", *(short*)result[col].value);
                break;
            case MYSQL_TYPE_INT24:
            case MYSQL_TYPE_LONG:
                sprintf(result[col].valuestring, "%d", *(int*)result[col].value);
                break;
            case MYSQL_TYPE_LONGLONG:
                sprintf(result[col].valuestring, "%lld", *(long long*)result[col].value);
                break;
            case MYSQL_TYPE_FLOAT:
                sprintf(result[col].valuestring, "%.2f", *(float*)result[col].value);
                break;
            case MYSQL_TYPE_DOUBLE:
                sprintf(result[col].valuestring, "%.2lf", *(double*)result[col].value);
                break;
            case MYSQL_TYPE_NEWDECIMAL:
                sprintf(result[col].valuestring, "%-*s", (int)result[col].max_length, (char*)result[col].value);
                break;
            case MYSQL_TYPE_YEAR:
                sprintf(result[col].valuestring, "%hd", *(short*)result[col].value);
                break;
            case MYSQL_TYPE_TIME:
                sprintf(result[col].valuestring, "%02d:%02d:%02d",
                    ((MYSQL_TIME*)result[col].value)->hour,
                    ((MYSQL_TIME*)result[col].value)->minute,
                    ((MYSQL_TIME*)result[col].value)->second);
                break;
            case MYSQL_TYPE_DATE:
                sprintf(result[col].valuestring, "%04d-%02d-%02d",
                    ((MYSQL_TIME*)result[col].value)->year,
                    ((MYSQL_TIME*)result[col].value)->month,
                    ((MYSQL_TIME*)result[col].value)->day);
                break;
            case MYSQL_TYPE_DATETIME:
            case MYSQL_TYPE_TIMESTAMP:
                sprintf(result[col].valuestring, "%04d-%02d-%02d %02d:%02d:%02d",
                    ((MYSQL_TIME*)result[col].value)->year,
                    ((MYSQL_TIME*)result[col].value)->month,
                    ((MYSQL_TIME*)result[col].value)->day,
                    ((MYSQL_TIME*)result[col].value)->hour,
                    ((MYSQL_TIME*)result[col].value)->minute,
                    ((MYSQL_TIME*)result[col].value)->second);
                break;
            case MYSQL_TYPE_STRING:
            case MYSQL_TYPE_VAR_STRING:
            case MYSQL_TYPE_TINY_BLOB:
            case MYSQL_TYPE_BLOB:
            case MYSQL_TYPE_MEDIUM_BLOB:
            case MYSQL_TYPE_LONG_BLOB:
            case MYSQL_TYPE_BIT:
                sprintf(result[col].valuestring, "%-*s", (int)result[col].max_length, (char*)result[col].value);
                break;
            default:
                sprintf(result[col].valuestring, "(Unknown type: %d)", result[col].type);
                break;
            }

            /* Trim space in the end of valuestring */
            for (int i = strlen(result[col].valuestring) - 1; i >= 0; i--) {
                if (result[col].valuestring[i] == ' ') {
                    result[col].valuestring[i] = '\0';
                } else {
                    break;
                }
            }

            if (strlen(result[col].valuestring) > result_set->result[0][col].field_length) {
                result_set->result[0][col].field_length = strlen(result[col].valuestring);
            }

            /* Result set */
            result_set->result[row + 1][col].type = result->type;
            result_set->result[row + 1][col].value = malloc(result[col].max_length + 63);
            if (result_set->result[row + 1][col].value == NULL) {
                log_error(PST_FORMAT_MSG_ERR_ALLOC, "result_set->result->value");
                return RET_ERR;
            }
            memset(result_set->result[row + 1][col].value, 0, result[col].max_length + 63);
            memcpy(result_set->result[row + 1][col].value, result[col].value, result[col].max_length + 63);
            result_set->result[row + 1][col].max_length = result[col].max_length;
            result_set->result[row + 1][col].length = result[col].length;
            result_set->result[row + 1][col].is_null = result[col].is_null;
            result_set->result[row + 1][col].error = result[col].error;
            result_set->result[row + 1][col].valuestring = malloc(result_set->result[0][col].max_length + 63);
            if (result_set->result[row + 1][col].valuestring == NULL) {
                log_error(PST_FORMAT_MSG_ERR_ALLOC, "result_set->result->valuestring");
                return RET_ERR;
            }
            memset(result_set->result[row + 1][col].valuestring, 0, result_set->result[0][col].max_length + 63);
            strcpy(result_set->result[row + 1][col].valuestring, result[col].valuestring);

            free(result[col].valuestring);
            result[col].valuestring = NULL;

        }

        row++;
    }

    return RET_OK;

}

static void GetResultSet() {
    if (InitResultSet() != RET_OK) {
        ret = RET_ERR;
        return;
    }

    if (FetchResultSet() != RET_OK) {
        ret = RET_ERR;
        return;
    }
}

static void PrintAlterTable() {
    GetRowsAffected();
    pst_print_PrintRowsAffectedAndDuplicate(rows);
}

static void PrintAlterUser() {
    GetRowsAffected();
    pst_print_PrintRowsAffected(rows);
}

static void PrintAnalyzeTable() {
    GetResultSet();
    if (ret == RET_ERR) return;
    if (rows == 0) {
        pst_print_PrintEmptySet();
    } else {
        pst_print_PrintResultSet(result_set);
        pst_print_PrintRowsInSet(rows);
    }
}

static void PrintCacheIndex() {
    GetResultSet();
    if (ret == RET_ERR) return;
    if (rows == 0) {
        pst_print_PrintEmptySet();
    } else {
        pst_print_PrintResultSet(result_set);
        pst_print_PrintRowsInSet(rows);
    }
}

static void PrintCall() {
    GetRowsAffected();
    pst_print_PrintRowsAffected(rows);
}

static void PrintChange() {
    GetRowsAffected();
    pst_print_PrintRowsAffectedIncludeWarnings(rows);
}

static void PrintCheckSum() {
    GetResultSet();
    if (ret == RET_ERR) return;
    if (rows == 0) {
        pst_print_PrintEmptySet();
    } else {
        pst_print_PrintResultSet(result_set);
        pst_print_PrintRowsInSet(rows);
    }
}

static void PrintCommit() {
    GetRowsAffected();
    pst_print_PrintRowsAffected(rows);
}

static void PrintCreateOrDropIndex() {
    GetRowsAffected();
    pst_print_PrintRowsAffectedAndDuplicate(rows);
}

static void PrintCreateOrRenameOrDropDatabase() {
    GetRowsAffected();
    pst_print_PrintRowsAffected(rows);
}

static void PrintCreateOrDropTable() {
    GetRowsAffected();
    pst_print_PrintRowsAffected(rows);
}

static void PrintCreateOrRenameOrDropUser() {
    GetRowsAffected();
    pst_print_PrintRowsAffected(rows);
}

static void PrintCreateOrDropView() {
    GetRowsAffected();
    pst_print_PrintRowsAffected(rows);
}

static void PrintDelete() {
    GetRowsAffected();
    pst_print_PrintRowsAffected(rows);
}

static void PrintDo() {
    GetRowsAffected();
    pst_print_PrintRowsAffected(rows);
}

static void PrintFlush() {
    GetRowsAffected();
    pst_print_PrintRowsAffected(rows);
}

static void PrintGrant() {
    GetRowsAffected();
    pst_print_PrintRowsAffected(rows);
}

static void PrintInsert() {
    GetRowsAffected();
    pst_print_PrintRowsAffected(rows);
}

static void PrintInsertSelect() {
    GetRowsAffected();
    pst_print_PrintRowsAffectedAndDuplicate(rows);
}

static void PrintInstallPlugin() {
    GetRowsAffected();
    pst_print_PrintRowsAffected(rows);
}

static void PrintKill() {
    GetRowsAffected();
    pst_print_PrintRowsAffected(rows);
}

static void PrintLoadIndexIntoCache() {
    GetResultSet();
    if (ret == RET_ERR) return;
    if (rows == 0) {
        pst_print_PrintEmptySet();
    } else {
        pst_print_PrintResultSet(result_set);
        pst_print_PrintRowsInSet(rows);
    }
}
static void PrintOptimizeTable() {
    GetResultSet();
    if (ret == RET_ERR) return;
    if (rows == 0) {
        pst_print_PrintEmptySet();
    } else {
        pst_print_PrintResultSet(result_set);
        pst_print_PrintRowsInSet(rows);
    }
}

static void PrintRenameTable() {
    GetRowsAffected();
    pst_print_PrintRowsAffected(rows);
}

static void PrintRepairTable() {
    GetResultSet();
    if (ret == RET_ERR) return;
    if (rows == 0) {
        pst_print_PrintEmptySet();
    } else {
        pst_print_PrintResultSet(result_set);
        pst_print_PrintRowsInSet(rows);
    }
}

static void PrintReplace() {
    GetRowsAffected();
    pst_print_PrintRowsAffected(rows);
}

static void PrintReset() {
    GetRowsAffected();
    pst_print_PrintRowsAffected(rows);
}

static void PrintRevoke() {
    GetRowsAffected();
    pst_print_PrintRowsAffected(rows);
}

static void PrintSelect() {
    GetResultSet();
    if (ret == RET_ERR) return;
    if (rows == 0) {
        pst_print_PrintEmptySet();
    } else {
        pst_print_PrintResultSet(result_set);
        pst_print_PrintRowsInSet(rows);
    }
}

static void PrintSet() {
    GetRowsAffected();
    pst_print_PrintRowsAffectedIncludeWarnings(rows);
}

static void PrintShow() {
    GetResultSet();
    if (ret == RET_ERR) return;
    if (rows == 0) {
        pst_print_PrintEmptySet();
    } else {
        pst_print_PrintResultSet(result_set);
        pst_print_PrintRowsInSet(rows);
    }
}

static void PrintShowCreate() {
    GetResultSet();
    if (ret == RET_ERR) return;
    if (rows == 0) {
        pst_print_PrintEmptySet();
    } else {
        pst_print_PrintResultSet(result_set);
        pst_print_PrintRowsInSet(rows);
    }
}
static void PrintStartOrStopReplica() {
    GetRowsAffected();
    pst_print_PrintRowsAffected(rows);
}

static void PrintTruncate() {
    GetRowsAffected();
    pst_print_PrintRowsAffected(rows);
}

static void PrintUninstallPlugin() {
    GetRowsAffected();
    pst_print_PrintRowsAffected(rows);
}

static void PrintUpdate() {
    GetRowsAffected();
    pst_print_PrintRowsAffectedAndChanged(rows);
}
