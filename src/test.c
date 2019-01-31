#include <stdio.h>
#include <string.h>
#include <mysql.h>
#include <errmsg.h>


#define MAX_SQL_BUFFER 4096
#define MAX_SQL_BINDINGS 64
#define MAX_SQL_RESULT_BINDINGS 16
#define MAX_BIND_BUFFER 256













#define PREPARE_SQL(which) \
do { \
    ndomod_mysql_return = mysql_stmt_prepare(which##_mysql_stmt, which##_mysql_query, strlen(which##_mysql_query)); \
    if (ndomod_mysql_return > 0) { \
        /* handle errors */ \
    } \
} while (0)

/* note: the variadic arguments here are a trick
   see: https://stackoverflow.com/questions/797318/how-to-split-a-string-literal-across-multiple-lines-in-c-objective-c/17996915#17996915 

    when debugging, you need to pretend there are **NO** newlines at all or any extraneous whitespace
*/
#define SET_QUERY(which, ...) strncpy(which##_mysql_query, __VA_ARGS__, MAX_SQL_BUFFER - 1);
#define SET_SQL(which, ...) \
do { \
    RESET_QUERY(which); \
    SET_QUERY(which, #__VA_ARGS__); \
    /* strncpy(ndomod_mysql_query, __VA_ARGS__, MAX_SQL_BUFFER - 1); */ \
    PREPARE_SQL(which); \
} while (0)

/* this is a straight query, with no parameters for binding. it is 
   slightly more efficient than preparing when isn't necessary */
#define SET_SQL_AND_QUERY(which, ...) \
todo()

#define RESET_QUERY(which) \
memset(which##_mysql_query, 0, sizeof(which##_mysql_query))

#define RESET_BIND(which) \
do { \
    memset(which##_mysql_bind, 0, sizeof(which##_mysql_bind)); \
    which##_mysql_i = 0; \
} while (0)

#define RESET_RESULT_BIND(which) \
do { \
    memset(which##_mysql_result, 0, sizeof(which##_mysql_result)); \
    which##_mysql_result_i = 0; \
} while (0)


#define SET_BIND_INT(which, _buffer) \
do { \
    which##_mysql_bind[which##_mysql_i].buffer_type = MYSQL_TYPE_LONG; \
    which##_mysql_bind[which##_mysql_i].buffer      = &(_buffer); \
    which##_mysql_i++; \
} while(0)

#define SET_BIND_LONG(which, _buffer) SET_BIND_INT(which, _buffer)

#define SET_BIND_TINY(which, _buffer) \
do { \
    which##_mysql_bind[which##_mysql_i].buffer_type = MYSQL_TYPE_TINY; \
    which##_mysql_bind[which##_mysql_i].buffer      = &(_buffer); \
    which##_mysql_i++; \
} while(0)

#define SET_BIND_SHORT(which, _buffer) \
do { \
    which##_mysql_bind[which##_mysql_i].buffer_type = MYSQL_TYPE_SHORT; \
    which##_mysql_bind[which##_mysql_i].buffer      = &(_buffer); \
    which##_mysql_i++; \
} while(0)

#define SET_BIND_LONGLONG(which, _buffer) \
do { \
    which##_mysql_bind[which##_mysql_i].buffer_type = MYSQL_TYPE_LONGLONG; \
    which##_mysql_bind[which##_mysql_i].buffer      = &(_buffer); \
    which##_mysql_i++; \
} while(0)

#define SET_BIND_FLOAT(which, _buffer) \
do { \
    which##_mysql_bind[which##_mysql_i].buffer_type = MYSQL_TYPE_FLOAT; \
    which##_mysql_bind[which##_mysql_i].buffer      = &(_buffer); \
    which##_mysql_i++; \
} while(0)

#define SET_BIND_DOUBLE(which, _buffer) \
do { \
    which##_mysql_bind[which##_mysql_i].buffer_type = MYSQL_TYPE_DOUBLE; \
    which##_mysql_bind[which##_mysql_i].buffer      = &(_buffer); \
    which##_mysql_i++; \
} while(0)

#define SET_BIND_STR(which, _buffer) \
do { \
    which##_mysql_bind[which##_mysql_i].buffer_type   = MYSQL_TYPE_STRING; \
    which##_mysql_bind[which##_mysql_i].buffer_length = MAX_BIND_BUFFER; \
    which##_mysql_bind[which##_mysql_i].buffer        = _buffer; \
    which##_mysql_bind[which##_mysql_i].length        = &(which##_mysql_tmp_str_len[which##_mysql_i]); \
    \
    which##_mysql_tmp_str_len[which##_mysql_i]        = strlen(_buffer); \
    which##_mysql_i++; \
} while(0)



#define SET_RESULT_INT(which, _buffer) \
do { \
    which##_mysql_result[which##_mysql_result_i].buffer_type = MYSQL_TYPE_LONG; \
    which##_mysql_result[which##_mysql_result_i].buffer      = &(_buffer); \
    which##_mysql_result_i++; \
} while(0)

#define SET_RESULT_LONG(which, _buffer) SET_RESULT_INT(which, _buffer)

#define SET_RESULT_TINY(which, _buffer) \
do { \
    which##_mysql_result[which##_mysql_result_i].buffer_type = MYSQL_TYPE_TINY; \
    which##_mysql_result[which##_mysql_result_i].buffer      = &(_buffer); \
    which##_mysql_result_i++; \
} while(0)

#define SET_RESULT_SHORT(which, _buffer) \
do { \
    which##_mysql_result[which##_mysql_result_i].buffer_type = MYSQL_TYPE_SHORT; \
    which##_mysql_result[which##_mysql_result_i].buffer      = &(_buffer); \
    which##_mysql_result_i++; \
} while(0)

#define SET_RESULT_LONGLONG(which, _buffer) \
do { \
    which##_mysql_result[which##_mysql_result_i].buffer_type = MYSQL_TYPE_LONGLONG; \
    which##_mysql_result[which##_mysql_result_i].buffer      = &(_buffer); \
    which##_mysql_result_i++; \
} while(0)

#define SET_RESULT_FLOAT(which, _buffer) \
do { \
    which##_mysql_result[which##_mysql_result_i].buffer_type = MYSQL_TYPE_FLOAT; \
    which##_mysql_result[which##_mysql_result_i].buffer      = &(_buffer); \
    which##_mysql_result_i++; \
} while(0)

#define SET_RESULT_DOUBLE(which, _buffer) \
do { \
    which##_mysql_result[which##_mysql_result_i].buffer_type = MYSQL_TYPE_DOUBLE; \
    which##_mysql_result[which##_mysql_result_i].buffer      = &(_buffer); \
    which##_mysql_result_i++; \
} while(0)

#define SET_RESULT_STR(which, _buffer) \
do { \
    which##_mysql_result[which##_mysql_result_i].buffer_type   = MYSQL_TYPE_STRING; \
    which##_mysql_result[which##_mysql_result_i].buffer_length = MAX_BIND_BUFFER; \
    which##_mysql_result[which##_mysql_result_i].buffer        = _buffer; \
    which##_mysql_result[which##_mysql_result_i].length        = &(which##_mysql_tmp_str_len[which##_mysql_result_i]); \
    \
    which##_mysql_result_i++; \
} while(0)



#define BIND(which) \
do { \
    ndomod_mysql_return = mysql_stmt_bind_param(which##_mysql_stmt, which##_mysql_bind); \
    if (ndomod_mysql_return > 0) { \
        /* handle errors */ \
    } \
} while(0)

#define RESULT_BIND(which) \
do { \
    ndomod_mysql_return = mysql_stmt_bind_result(which##_mysql_stmt, which##_mysql_result); \
    if (ndomod_mysql_return > 0) { \
        /* handle errors */ \
    } \
} while(0)



#define QUERY(which) \
do { \
    ndomod_mysql_return = mysql_stmt_execute(which##_mysql_stmt); \
    if (ndomod_mysql_return > 0) { \
        /* handle errors */ \
    } \
} while(0)


#define FETCH(which) (!mysql_stmt_fetch(which##_mysql_stmt))

#define STORE(which) \
do {\
    ndomod_mysql_return = mysql_stmt_store_result(which##_mysql_stmt); \
    if (ndomod_mysql_return > 0) { \
        /* handle errors */ \
    } \
} while (0)



#define NUM_ROWS_INT(which) (int) NUM_ROWS(which)
#define NUM_ROWS(which) mysql_stmt_num_rows(which##_mysql_stmt)

#define NDO_TRUE      1
#define NDO_FALSE     0

#define NDO_ERROR     -1
#define NDO_OK        0

int ndomod_database_connected = NDO_FALSE;
char * ndomod_db_host = NULL;
int ndomod_db_port = 3306;
char * ndomod_db_socket = NULL;
char * ndomod_db_user = NULL;
char * ndomod_db_pass = NULL;
char * ndomod_db_name = NULL;

MYSQL * ndomod_mysql = NULL;
MYSQL_STMT * ndomod_mysql_stmt = NULL;
MYSQL_BIND ndomod_mysql_bind[MAX_SQL_BINDINGS];
MYSQL_BIND ndomod_mysql_result[MAX_SQL_RESULT_BINDINGS];
int ndomod_mysql_return = 0;
char ndomod_mysql_query[MAX_SQL_BUFFER] = { 0 };
long ndomod_mysql_tmp_str_len[MAX_SQL_BINDINGS] = { 0 };

MYSQL_STMT * second_mysql_stmt = NULL;
MYSQL_BIND second_mysql_bind[MAX_SQL_BINDINGS];
MYSQL_BIND second_mysql_result[MAX_SQL_RESULT_BINDINGS];
char second_mysql_query[MAX_SQL_BUFFER] = { 0 };
long second_mysql_tmp_str_len[MAX_SQL_BINDINGS] = { 0 };

int ndomod_mysql_i = 0;
int ndomod_mysql_result_i = 0;


int second_mysql_i = 0;
int second_mysql_result_i = 0;


/* connects to the database and initializes some other 
   miscellaneous database related memory */
int ndomod_db_connect(void)
{
    MYSQL * connected = NULL;
    ndomod_mysql = mysql_init(NULL);

    if (ndomod_mysql == NULL) {
        return NDO_ERROR;
    }

    if (ndomod_db_host == NULL) {
        ndomod_db_host = strdup("localhost");
    }

    ndomod_db_user = "root";
    ndomod_db_pass = "welcome";
    ndomod_db_name = "ndo";

    connected = mysql_real_connect(
            ndomod_mysql,
            ndomod_db_host,
            ndomod_db_user,
            ndomod_db_pass,
            ndomod_db_name,
            ndomod_db_port,
            ndomod_db_socket,
            CLIENT_REMEMBER_OPTIONS);

    if (connected == NULL) {
        return NDO_ERROR;
    }

    ndomod_mysql_stmt = mysql_stmt_init(ndomod_mysql);
    second_mysql_stmt = mysql_stmt_init(ndomod_mysql);

    if (ndomod_mysql_stmt == NULL) {
        return NDO_ERROR;
    }

    return NDO_OK;
}



int ndomod_db_disconnect(void)
{
    mysql_close(ndomod_mysql);
    mysql_library_end();
}


int main(int argc, char const *argv[])
{
    int result = 0;
    char * h_display_name = strdup(" localhost");
    char * s_display_name = strdup(" Swap Usage");

    ndomod_db_connect();

    mysql_dump_debug_info(ndomod_mysql);

    SET_SQL(ndomod, SELECT host_id FROM nagios_hosts WHERE display_name = ? LIMIT 1);
    SET_SQL(second, SELECT service_id FROM nagios_services WHERE display_name = ? LIMIT 1);

    SET_BIND_STR(ndomod, h_display_name);
    SET_BIND_STR(second, s_display_name);

    BIND(ndomod);
    BIND(second);

    SET_RESULT_INT(ndomod, result);
    RESULT_BIND(ndomod);
    SET_RESULT_INT(second, result);
    RESULT_BIND(second);

    QUERY(ndomod);
    STORE(ndomod);
    QUERY(second);
    STORE(second);

    if (NUM_ROWS_INT(ndomod) == 1) {
        while (FETCH(ndomod)) {
            printf("fetching ndomod... %d\n", result);
        }
    }
    if (NUM_ROWS_INT(second) == 1) {
        while (FETCH(second)) {
            printf("fetching second... %d\n", result);
        }
    }

    QUERY(ndomod);
    STORE(ndomod);
    QUERY(second);
    STORE(second);

    if (NUM_ROWS_INT(ndomod) == 1) {
        while (FETCH(ndomod)) {
            printf("fetching ndomod... %d\n", result);
        }
    }
    if (NUM_ROWS_INT(second) == 1) {
        while (FETCH(second)) {
            printf("fetching second... %d\n", result);
        }
    }

    ndomod_db_disconnect();

    return 0;
}