Prepared Statement Test For MySQL (PSTest For MySQL)  

This project is used to test MySQL prepared statements (e.g. POC).
You need to start the program under the Linux system.  
Command: `./PSTest [JSON PATH] `

JSON example:
```json
{
    "user": "user",
    "password": "passwd",
    "host": "127.0.0.1",
    "port": 3306,
    "database": "employees",
    "prepared_statement": [
        {
            "statement": "SELECT * FROM employees WHERE emp_no = ?",
            "parameter": [
                [
                    {
                        "type": "int",
                        "unsigned": false,
                        "value": 10001
                    }
                ]
            ]
        },
        {
            "statement": "SELECT * FROM employees WHERE gender = ? LIMIT ?",
            "parameter": [
                [
                    {
                        "type": "varchar",
                        "value": "F"
                    },
                    {
                        "type": "int",
                        "unsigned": true,
                        "value": 10
                    }
                ],
                [
                    {
                        "type": "varchar",
                        "value": "M"
                    },
                    {
                        "type": "int",
                        "unsigned": true,
                        "value": 20
                    }
                ]
            ]
        },
        {
            "statement": "SHOW CREATE TABLE employees",
            "parameter": []
        }
    ]
}
```
user, password, host, port, database : Database connection information  
prepared_statement : array of prepared statements  
statement : statement you want to test  
parameter : array of parameters, if no parameters, you need to add an empty array  
type : type of parameter  
unsigned : if parameter is number or unsigned type, you need to set it to true or false  
value : value of parameter  
  
SQL Syntax Permitted in Prepared Statements ⇒ ([SQL Syntax Permitted in Prepared Statements](https://dev.mysql.com/doc/refman/8.0/en/sql-prepared-statements.html))

Permissible Input Data Types for MYSQL_BIND Structures ⇒ ([MySQL 8.0 C API Developer Guide](https://dev.mysql.com/doc/refman/8.0/en/)):
| Input Variable C Type | buffer_type Value    | SQL Type of Destination Value |
| :-------------------- | :------------------- | :---------------------------- |
| signed char           | MYSQL_TYPE_TINY      | TINYINT                       |
| short int             | MYSQL_TYPE_SHORT     | SMALLINT                      |
| int                   | MYSQL_TYPE_LONG      | INT                           |
| long long int         | MYSQL_TYPE_LONGLONG  | BIGINT                        |
| float                 | MYSQL_TYPE_FLOAT     | FLOAT                         |
| double                | MYSQL_TYPE_DOUBLE    | DOUBLE                        |
| MYSQL_TIME            | MYSQL_TYPE_TIME      | TIME                          |
| MYSQL_TIME            | MYSQL_TYPE_DATE      | DATE                          |
| MYSQL_TIME            | MYSQL_TYPE_DATETIME  | DATETIME                      |
| MYSQL_TIME            | MYSQL_TYPE_TIMESTAMP | TIMESTAMP                     |
| char[]                | MYSQL_TYPE_STRING    | TEXT,CHAR,VARCHAR             |
| char[]                | MYSQL_TYPE_BLOB      | BLOB, BINARY, VARBINARY       |
