#ifndef PST_H
#define PST_H

#include <mysql/mysql.h>

typedef enum enum_field_types PstFieldTypes;

typedef enum enum_statement_syntax {
    PstSyntax_AlterTable,
    PstSyntax_AlterUser,
    PstSyntax_AnalyzeTable,
    PstSyntax_CacheIndex,
    PstSyntax_Call,
    PstSyntax_Change,
    PstSyntax_CheckSum,
    PstSyntax_Commit,
    PstSyntax_CreateOrDropIndex,
    PstSyntax_CreateOrRenameOrDropDatabase,
    PstSyntax_CreateOrDropTable,
    PstSyntax_CreateOrRenameOrDropUser,
    PstSyntax_CreateOrDropView,
    PstSyntax_Delete,
    PstSyntax_Do,
    PstSyntax_Flush,
    PstSyntax_Grant,
    PstSyntax_Insert,
    PstSyntax_InsertSelect,
    PstSyntax_InstallPlugin,
    PstSyntax_Kill,
    PstSyntax_LoadIndexIntoCache,
    PstSyntax_OptimizeTable,
    PstSyntax_RenameTable,
    PstSyntax_RepairTable,
    PstSyntax_Replace,
    PstSyntax_Reset,
    PstSyntax_Revoke,
    PstSyntax_Select,
    PstSyntax_Set,
    PstSyntax_Show,
    PstSyntax_ShowCreate,
    PstSyntax_StartOrStopReplica,
    PstSyntax_Truncate,
    PstSyntax_UninstallPlugin,
    PstSyntax_Update,

    PstSyntax_Unkown
} PstSyntax;

typedef struct PstPreparedStatementParameter {
    char type[16];
    bool is_unsigned;
    char* valuestring;
    double valuedouble;
} PstParameter;

typedef struct PstPreparedStatement {
    char* stmt;
    unsigned long stmt_len;
    PstSyntax syntax;
    PstParameter** params;
    unsigned long param_markers_count;
    unsigned long params_size;
} PstPreparedStatement;

typedef struct PstPreparedStatements {
    PstPreparedStatement* prep_stmt;
    unsigned long prep_stmt_size;
} PstPreparedStatements;

typedef struct PstConnection {
    char user[16];
    char password[42];
    char host[16];
    unsigned int port;
    char database[64];
    char unix_socket[256];
    unsigned long client_flag;
} PstConnection;

typedef struct PstResult {
    PstFieldTypes type;
    void* value;
    unsigned long max_length;
    unsigned long length;
    bool is_null;
    bool error;
    char* valuestring;
    unsigned long field_length;
} PstResult;

typedef struct PstResultSet {
    PstResult** result;
    uint64_t column_count;
    uint64_t row_count;
} PstResultSet;

#define RET_OK 0
#define RET_ERR -1

/* CJSON's string type is char*, if SQL type is TIME or DATE or DATETIME or TIMESTAMP, */
/* we need to convert it to MYSQL_TIME before using it */
MYSQL_TIME pst_ToMySQLTime(const char* str);

#define PST_FORMAT_MSG_ERR_MYSQL "ERROR %d (%s): %s"
#define PST_FORMAT_MSG_ERR_ALLOC "Insufficient memory available, variable '%s' was not allocated"
#define PST_FORMAT_MSG_ERR_FOPEN "Can not open file '%s'"

char* pst_Upper(const char* str);

PstFieldTypes pst_ToMySQLFieldType(const char* type_str);
PstSyntax pst_GetSyntax(const char* stmt);

#endif /* PST_H */
