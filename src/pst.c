#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "pst.h"

MYSQL_TIME pst_ToMySQLTime(const char* str) {
    MYSQL_TIME time;
    char format[30];
    memset(&time, 0, sizeof(MYSQL_TIME));
    memset(format, 0, sizeof(format));

    if (strlen(str) == 10) {
        /* DATE FORMAT: YYYY-MM-DD */
        strcpy(format, "%04d-%02d-%02d");
    } else if (strlen(str) == 19) {
        /* TIME FORMAT: HH:MM:SS */
        strcpy(format, "%02d:%02d:%02d");
    } else {
        /* DATETIME FORMAT: YYYY-MM-DD HH:MM:SS */
        strcpy(format, "%04d-%02d-%02d %02d:%02d:%02d");
    }

    sscanf(str, format,
        &time.year,
        &time.month,
        &time.day,
        &time.hour,
        &time.minute,
        &time.second);
    return time;
}

char* pst_Upper(const char* str) {
    static char buffer[256];
    int i = 0;

    while (str[i] != '\0' && i < sizeof(buffer) - 1) {
        buffer[i] = toupper((unsigned char)str[i]);
        i++;
    }
    buffer[i] = '\0';

    return buffer;
}


PstFieldTypes pst_ToMySQLFieldType(const char* type) {
    if (!type) return MYSQL_TYPE_NULL;
    const char* upperType = pst_Upper(type);
    if (strcmp(upperType, "TINYINT") == 0) return MYSQL_TYPE_TINY;
    if (strcmp(upperType, "SMALLINT") == 0) return MYSQL_TYPE_SHORT;
    if (strcmp(upperType, "INT") == 0) return MYSQL_TYPE_LONG;
    if (strcmp(upperType, "BIGINT") == 0)  return MYSQL_TYPE_LONGLONG;
    if (strcmp(upperType, "FLOAT") == 0)   return MYSQL_TYPE_FLOAT;
    if (strcmp(upperType, "DOUBLE") == 0)   return MYSQL_TYPE_DOUBLE;
    if (strcmp(upperType, "TIME") == 0)  return MYSQL_TYPE_TIME;
    if (strcmp(upperType, "DATE") == 0) return MYSQL_TYPE_DATE;
    if (strcmp(upperType, "DATETIME") == 0) return MYSQL_TYPE_DATETIME;
    if (strcmp(upperType, "TIMESTAMP") == 0)  return MYSQL_TYPE_TIMESTAMP;
    if (strcmp(upperType, "TEXT") == 0)  return MYSQL_TYPE_STRING;
    if (strcmp(upperType, "CHAR") == 0) return MYSQL_TYPE_STRING;
    if (strcmp(upperType, "VARCHAR") == 0) return MYSQL_TYPE_STRING;
    if (strcmp(upperType, "BLOB") == 0) return MYSQL_TYPE_BLOB;
    if (strcmp(upperType, "BINARY") == 0) return MYSQL_TYPE_BLOB;
    if (strcmp(upperType, "VARBINARY") == 0) return MYSQL_TYPE_BLOB;
    if (strcmp(upperType, "NULL") == 0)  return MYSQL_TYPE_NULL;

    return MYSQL_TYPE_NULL;
}

PstSyntax pst_GetSyntax(const char* stmt) {
    PstSyntax syntax = PstSyntax_Unkown;
    char* str = malloc(strlen(stmt) + 1);
    if (str == NULL) {
        return syntax;
    }
    memset(str, 0, strlen(stmt) + 1);

    int flag_quotes = 0, flag_space = 0;
    int j = 0;
    for (int i = 0; i < strlen(stmt); i++) {
        if (flag_quotes == 1) {
            if (stmt[i] == '\'') {
                flag_quotes = 0;
            }
            continue;
        }

        if (flag_quotes == 0 && stmt[i] == '\'') {
            flag_quotes = 1;
            continue;
        }

        if (flag_space == 1) {
            if (stmt[i] == ' ') {
                continue;
            } else {
                flag_space = 0;
            }
        }

        if (stmt[i] == ' ') {
            flag_space = 1;
        }

        str[j] = toupper(stmt[i]);
        j++;
    }

    if (strncmp(str, "ALTER TABLE", 11) == 0) {
        syntax = PstSyntax_AlterTable;
    } else if (strncmp(str, "ALTER USER", 10) == 0) {
        syntax = PstSyntax_AlterUser;
    } else if (strncmp(str, "ANALYZE TABLE", 13) == 0) {
        syntax = PstSyntax_AnalyzeTable;
    } else if (strncmp(str, "CALL", 4) == 0) {
        syntax = PstSyntax_Call;
    } else if (strncmp(str, "CHECKSUM", 8) == 0) {
        syntax = PstSyntax_CheckSum;
    } else if (strncmp(str, "COMMIT", 6) == 0) {
        syntax = PstSyntax_Commit;
    } else if (strncmp(str, "CREATE INDEX", 12) == 0 || strncmp(str, "DROP INDEX", 10) == 0) {
        syntax = PstSyntax_CreateOrDropIndex;
    } else if (strncmp(str, "CREATE DATABASE", 16) == 0 || strncmp(str, "RENAME DATABASE", 15) == 0 || strncmp(str, "DROP DATABASE", 13) == 0) {
        syntax = PstSyntax_CreateOrRenameOrDropDatabase;
    } else if (strncmp(str, "CREATE TABLE", 12) == 0 || strncmp(str, "DROP TABLE", 10) == 0) {
        syntax = PstSyntax_CreateOrDropTable;
    } else if (strncmp(str, "CREATE USER", 11) == 0 || strncmp(str, "RENAME USER", 11) == 0 || strncmp(str, "DROP USER", 9) == 0) {
        syntax = PstSyntax_CreateOrRenameOrDropUser;
    } else if (strncmp(str, "CREATE VIEW", 11) == 0 || strncmp(str, "DROP VIEW", 9) == 0) {
        syntax = PstSyntax_CreateOrDropTable;
    } else if (strncmp(str, "DELETE", 6) == 0) {
        syntax = PstSyntax_Delete;
    } else if (strncmp(str, "DO", 2) == 0) {
        syntax = PstSyntax_Do;
    } else if (strncmp(str, "FLUSH", 5) == 0) {
        syntax = PstSyntax_Flush;
    } else if (strncmp(str, "GRANT", 5) == 0) {
        syntax = PstSyntax_Grant;
    } else if (strncmp(str, "INSERT", 6) == 0) {
        if (strstr(str, "SELECT")) {
            syntax = PstSyntax_InsertSelect;
        } else {
            syntax = PstSyntax_Insert;
        }
    } else if (strncmp(str, "INSTALL PLUGIN", 14) == 0) {
        syntax = PstSyntax_InstallPlugin;
    } else if (strncmp(str, "KILL", 4) == 0) {
        syntax = PstSyntax_Kill;
    } else if (strncmp(str, "LOAD INDEX INTO CACHE", 21) == 0) {
        syntax = PstSyntax_LoadIndexIntoCache;
    } else if (strncmp(str, "OPTIMIZE TABLE", 14) == 0) {
        syntax = PstSyntax_OptimizeTable;
    } else if (strncmp(str, "RENAME TABLE", 12) == 0) {
        syntax = PstSyntax_RenameTable;
    } else if (strncmp(str, "REPAIR TABLE", 12) == 0) {
        syntax = PstSyntax_RepairTable;
    } else if (strncmp(str, "REPLACE", 7) == 0) {
        syntax = PstSyntax_Replace;
    } else if (strncmp(str, "RESET", 5) == 0) {
        syntax = PstSyntax_Reset;
    } else if (strncmp(str, "REVOKE", 6) == 0) {
        syntax = PstSyntax_Revoke;
    } else if (strncmp(str, "SELECT", 6) == 0) {
        syntax = PstSyntax_Select;
    } else if (strncmp(str, "SET", 3) == 0) {
        syntax = PstSyntax_Set;
    } else if (strncmp(str, "SHOW", 4) == 0) {
        if (strstr(str, "CREATE")) {
            syntax = PstSyntax_ShowCreate;
        } else {
            syntax = PstSyntax_Show;
        }
    } else if (strncmp(str, "START REPLICA", 13) == 0 || strncmp(str, "STOP REPLICA", 12) == 0) {
        syntax = PstSyntax_StartOrStopReplica;
    } else if (strncmp(str, "TRUNCATE", 8) == 0) {
        syntax = PstSyntax_Truncate;
    } else if (strncmp(str, "UNINSTALL PLUGIN", 16) == 0) {
        syntax = PstSyntax_UninstallPlugin;
    } else if (strncmp(str, "UPDATE", 6) == 0) {
        syntax = PstSyntax_UninstallPlugin;
    } else {
        syntax = PstSyntax_Unkown;
    }

    free(str);
    str = NULL;
    return syntax;
}
