/**
 * @file ndomod.c Nagios Data Output Event Broker Module
 */
/*
 * Copyright 2009-2019 Nagios Core Development Team and Community Contributors
 * Copyright 2005-2009 Ethan Galstad
 *
 * This file is part of NDOUtils.
 *
 * First Written: 2005-05-19
 * Last Modified: 2019-01-20
 *
 *****************************************************************************
 * NDOUtils is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NDOUtils is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NDOUtils. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @todo Add service parents
 * @todo hourly value (hosts / services)
 * @todo minimum value (contacts)
 */

/* include our project's header files */
#include "../include/common.h"
#include "../include/io.h"
#include "../include/utils.h"
#include "../include/protoapi.h"
#include "../include/ndomod.h"

#include <pthread.h>

/* include (minimum required) event broker header files */
#include "../include/nagios/nebstructs.h"
#include "../include/nagios/nebmodules.h"
#include "../include/nagios/nebcallbacks.h"
#include "../include/nagios/broker.h"

/* include other Nagios header files for access to functions, data structs, etc. */
#include "../include/nagios/common.h"
#include "../include/nagios/nagios.h"
#include "../include/nagios/downtime.h"
#include "../include/nagios/comments.h"
#include "../include/nagios/macros.h"

/* specify event broker API version (required) */
NEB_API_VERSION(CURRENT_NEB_API_VERSION)


#define NDOMOD_VERSION "3.0.0"
#define NDOMOD_NAME "NDOMOD"
#define NDOMOD_DATE "2019-01-20"


/******************************************************************************/
/* Database related */
/******************************************************************************/


/* wrapper for handler function definitions
   @todo convert these to what they should be - this is confusing */
#define NDOMOD_HANDLER_FUNCTION(type) \
void ndomod_handle_##type(nebstruct_##type * data)


#define MAX_SQL_BUFFER 4096
#define MAX_SQL_BINDINGS 64
#define MAX_SQL_RESULT_BINDINGS 16
#define MAX_BIND_BUFFER 256


/* prepare our sql query */
#define PREPARE_SQL() \
do { \
    ndomod_mysql_return = mysql_stmt_prepare(ndomod_mysql_stmt, ndomod_mysql_query, strlen(ndomod_mysql_query)); \
    if (ndomod_mysql_return > 0) { \
        /* handle errors */ \
    } \
} while (0)


/* note: the variadic arguments here are a trick
   see: https://stackoverflow.com/questions/797318/how-to-split-a-string-literal-across-multiple-lines-in-c-objective-c/17996915#17996915 

    when debugging, you need to pretend there are **NO** newlines at all or any extraneous whitespace
*/
#define SET_SQL(...) \
do { \
    RESET_QUERY(); \
    strncpy(ndomod_mysql_query, #__VA_ARGS__, MAX_SQL_BUFFER - 1); \
    PREPARE_SQL(); \
} while (0)

/* this is a straight query, with no parameters for binding. it is 
   slightly more efficient than preparing when isn't necessary */
#define SET_SQL_AND_QUERY(...) \
todo()


/* there is likely a more efficient way to reset the query string @todo */

/* reset the string containing the query */
#define RESET_QUERY() \
memset(ndomod_mysql_query, 0, sizeof(ndomod_mysql_query))



/*
resets would likely be better suited to:
not sure if memset all with constant sizes is quicker
or if detecting the actual appropriate size and then memsetting is quicker
will need benchmarked @todo
    memset(ndomod_mysql_bind, 0, (sizeof(ndomod_mysql_bind[0]) * ndomod_mysql_i) + 1);
    memset(ndomod_mysql_result, 0, (sizeof(ndomod_mysql_result) * ndomod_mysql_result_i) + 1);
*/

/* reset bindings */
#define RESET_BIND() \
do { \
    memset(ndomod_mysql_bind, 0, sizeof(ndomod_mysql_bind)); \
    ndomod_mysql_i = 0; \
} while (0)


/* reset result bindings */
#define RESET_RESULT_BIND() \
do { \
    memset(ndomod_mysql_result, 0, sizeof(ndomod_mysql_result)); \
    ndomod_mysql_result_i = 0; \
} while (0)


/* parameter bindings */
#define SET_BIND_TINY(_buffer) \
do { \
    ndomod_mysql_bind[ndomod_mysql_i].buffer_type = MYSQL_TYPE_TINY; \
    ndomod_mysql_bind[ndomod_mysql_i].buffer      = &(_buffer); \
    ndomod_mysql_i++; \
} while (0)

#define SET_BIND_SHORT(_buffer) \
do { \
    ndomod_mysql_bind[ndomod_mysql_i].buffer_type = MYSQL_TYPE_SHORT; \
    ndomod_mysql_bind[ndomod_mysql_i].buffer      = &(_buffer); \
    ndomod_mysql_i++; \
} while (0)

#define SET_BIND_INT(_buffer) \
do { \
    ndomod_mysql_bind[ndomod_mysql_i].buffer_type = MYSQL_TYPE_LONG; \
    ndomod_mysql_bind[ndomod_mysql_i].buffer      = &(_buffer); \
    ndomod_mysql_i++; \
} while (0)

#define SET_BIND_LONG(_buffer) SET_BIND_INT(i, _buffer)

#define SET_BIND_LONGLONG(_buffer) \
do { \
    ndomod_mysql_bind[ndomod_mysql_i].buffer_type = MYSQL_TYPE_LONGLONG; \
    ndomod_mysql_bind[ndomod_mysql_i].buffer      = &(_buffer); \
    ndomod_mysql_i++; \
} while (0)

#define SET_BIND_FLOAT(_buffer) \
do { \
    ndomod_mysql_bind[ndomod_mysql_i].buffer_type = MYSQL_TYPE_FLOAT; \
    ndomod_mysql_bind[ndomod_mysql_i].buffer      = &(_buffer); \
    ndomod_mysql_i++; \
} while (0)

#define SET_BIND_DOUBLE(_buffer) \
do { \
    ndomod_mysql_bind[ndomod_mysql_i].buffer_type = MYSQL_TYPE_DOUBLE; \
    ndomod_mysql_bind[ndomod_mysql_i].buffer      = &(_buffer); \
    ndomod_mysql_i++; \
} while (0)

#define SET_BIND_STR(_buffer) \
do { \
    ndomod_mysql_bind[ndomod_mysql_i].buffer_type   = MYSQL_TYPE_STRING; \
    ndomod_mysql_bind[ndomod_mysql_i].buffer_length = MAX_BIND_BUFFER; \
    ndomod_mysql_bind[ndomod_mysql_i].buffer        = &(_buffer); \
    ndomod_mysql_bind[ndomod_mysql_i].length        = &(ndomod_mysql_tmp_str_len[ndomod_mysql_i]); \
    \
    ndomod_mysql_tmp_str_len[ndomod_mysql_i]        = strlen(_buffer); \
    ndomod_mysql_i++; \
} while (0)


/* result bindings */
#define SET_RESULT_TINY(_buffer) \
do { \
    ndomod_mysql_result[ndomod_mysql_result_i].buffer_type = MYSQL_TYPE_TINY; \
    ndomod_mysql_result[ndomod_mysql_result_i].buffer      = &(_buffer); \
    ndomod_mysql_result_i++; \
} while (0)

#define SET_RESULT_SHORT(_buffer) \
do { \
    ndomod_mysql_result[ndomod_mysql_result_i].buffer_type = MYSQL_TYPE_SHORT; \
    ndomod_mysql_result[ndomod_mysql_result_i].buffer      = &(_buffer); \
    ndomod_mysql_result_i++; \
} while (0)

#define SET_RESULT_INT(_buffer) \
do { \
    ndomod_mysql_result[ndomod_mysql_result_i].buffer_type = MYSQL_TYPE_LONG; \
    ndomod_mysql_result[ndomod_mysql_result_i].buffer      = &(_buffer); \
    ndomod_mysql_result_i++; \
} while (0)

#define SET_RESULT_LONG(_buffer) SET_RESULT_INT(_buffer)

#define SET_RESULT_LONGLONG(_buffer) \
do { \
    ndomod_mysql_result[ndomod_mysql_result_i].buffer_type = MYSQL_TYPE_LONGLONG; \
    ndomod_mysql_result[ndomod_mysql_result_i].buffer      = &(_buffer); \
    ndomod_mysql_result_i++; \
} while (0)

#define SET_RESULT_FLOAT(_buffer) \
do { \
    ndomod_mysql_result[ndomod_mysql_result_i].buffer_type = MYSQL_TYPE_FLOAT; \
    ndomod_mysql_result[ndomod_mysql_result_i].buffer      = &(_buffer); \
    ndomod_mysql_result_i++; \
} while (0)

#define SET_RESULT_DOUBLE(_buffer) \
do { \
    ndomod_mysql_result[ndomod_mysql_result_i].buffer_type = MYSQL_TYPE_DOUBLE; \
    ndomod_mysql_result[ndomod_mysql_result_i].buffer      = &(_buffer); \
    ndomod_mysql_result_i++; \
} while (0)

#define SET_RESULT_STR(_buffer) \
do { \
    ndomod_mysql_result[ndomod_mysql_result_i].buffer_type   = MYSQL_TYPE_STRING; \
    ndomod_mysql_result[ndomod_mysql_result_i].buffer_length = MAX_BIND_BUFFER; \
    ndomod_mysql_result[ndomod_mysql_result_i].buffer        = &(_buffer); \
    ndomod_mysql_result[ndomod_mysql_result_i].length        = &(ndomod_mysql_tmp_str_len[ndomod_mysql_result_i]); \
    \
    ndomod_mysql_tmp_str_len[ndomod_mysql_result_i]          = strlen(_buffer); \
    ndomod_mysql_result_i++; \
} while (0)



/* bind for parameters */
#define BIND() \
do { \
    ndomod_mysql_return = mysql_stmt_bind_param(ndomod_mysql_stmt, ndomod_mysql_bind); \
    if (ndomod_mysql_return > 0) { \
        /* handle errors */ \
    } \
} while (0)


/* bind for results */
#define RESULT_BIND() \
do { \
    ndomod_mysql_return = mysql_stmt_bind_result(ndomod_mysql_stmt, ndomod_mysql_result); \
    if (ndomod_mysql_return > 0) { \
        /* handle errors */ \
    } \
} while (0)


/* execute the prepared statement */
#define QUERY() \
do { \
    ndomod_mysql_return = mysql_stmt_execute(ndomod_mysql_stmt); \
    if (ndomod_mysql_return > 0) { \
        /* handle errors */ \
    } \
} while (0)


/* fetch next result,
   usage:

    ```
    while (FETCH()) {
        do_stuff_with_result_binds();
    }
    ```
*/
#define FETCH() (!mysql_stmt_fetch(ndomod_mysql_stmt))


/* store mysql results */
#define STORE() \
do {\
    ndomod_mysql_return = mysql_stmt_store_result(ndomod_mysql_stmt); \
    if (ndomod_mysql_return > 0) { \
        /* handle errors */ \
    } \
} while (0)


/* why use the mysql api when we can just make arbitrary macros? */
#define NUM_ROWS_INT (int) NUM_ROWS
#define NUM_ROWS mysql_stmt_num_rows(ndomod_mysql_stmt)


/* useful for quickly changing the query text and not having to completely
   overwrite it 
   specifically: when there are multiple queries per handler, and they
   are exactly the same, except for the table name
   e.g.: scheduleddowntime vs. downtimehistory */
#define HACK_SQL_QUERY(_overwrite_pos, _overwrite_str, _start_pos, _end_pos) \
do { \
    int overwrite_i = 0; \
    strcpy(ndomod_mysql_query + _overwrite_pos, _overwrite_str); \
    for (overwrite_i = _start_pos; overwrite_i <= _end_pos; overwrite_i++) { \
        ndomod_mysql_query[overwrite_i] = ' '; \
    } \
} while (0)


/* macros for ugly get_object_id checks based on variable detection, etc. */
#define CHECK_2SVC_AND_GET_OBJECT_ID(_check, _svc_check1, _svc_check2, _var, _insert, _hst, _svc) \
do { \
    if (_check == _svc_check1 || _check == _svc_check2) { \
        _var = ndomod_get_object_id(_insert, NDO2DB_OBJECTTYPE_SERVICE, \
                                    _hst, _svc); \
    } \
    else { _ELSE_CHECK_GET_OBJECT_ID(_insert, _var, _hst); } \
} while (0)

#define CHECK_AND_GET_OBJECT_ID(_check, _svc_check, _var, _insert, _hst, _svc) \
do { \
    if (_check == _svc_check) { \
        _var = ndomod_get_object_id(_insert, NDO2DB_OBJECTTYPE_SERVICE, \
                                    _hst, _svc); \
    } \
    else { _ELSE_CHECK_GET_OBJECT_ID(_insert, _var, _hst); } \
} while (0)


/* helper macro for the get_object_id wrappers */
#define _ELSE_CHECK_GET_OBJECT_ID(_insert, _var, _hst) \
_var = ndomod_get_object_id(_insert, NDO2DB_OBJECTTYPE_HOST, _hst)


/* helper function for debugging during development */
#define MYSQL_DEBUG() do { mysql_dump_debug_info(ndomod_mysql) } while (0)
/******************************************************************************/


void *ndomod_module_handle = NULL;
unsigned long ndomod_process_options = 0;
int ndomod_config_output_options = NDOMOD_CONFIG_DUMP_ALL;
int has_ver403_long_output = (CURRENT_OBJECT_STRUCTURE_VERSION >= 403);

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

int ndomod_mysql_i = 0;
int ndomod_mysql_result_i = 0;


extern int errno;

/**** NAGIOS VARIABLES ****/
extern command *command_list;
extern timeperiod *timeperiod_list;
extern contact *contact_list;
extern contactgroup *contactgroup_list;
extern host *host_list;
extern hostgroup *hostgroup_list;
extern service *service_list;
extern servicegroup *servicegroup_list;
extern hostescalation *hostescalation_list;
extern serviceescalation *serviceescalation_list;
extern hostdependency *hostdependency_list;
extern servicedependency *servicedependency_list;

extern char *config_file;
extern sched_info scheduling_info;
extern char *global_host_event_handler;
extern char *global_service_event_handler;

extern int __nagios_object_structure_version;

extern int use_ssl;


/* this function gets called when the module is loaded by the event broker */
int nebmodule_init(int flags, char *args, void *handle)
{
    char temp_buffer[NDOMOD_MAX_BUFLEN] = { 0 };

    /* save our handle */
    ndomod_module_handle = handle;

    /* log module info to the Nagios log file */
    snprintf(temp_buffer, sizeof(temp_buffer) - 1, 
        "ndo: %s %s (%s) "
        "Copyright (c) 2009 "
        "Nagios Core Development Team and Community Contributors", 
        NDOMOD_NAME, NDOMOD_VERSION, NDOMOD_DATE);
    ndomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);

    /* check Nagios object structure version */
    if (ndomod_check_nagios_object_version() == NDO_ERROR) {
        return NDO_ERROR;
    }

    /* process arguments */
    if (ndomod_process_module_args(args) == NDO_ERROR) {
        ndomod_write_to_logs(
            "ndo: An error occurred attempting to process module arguments.",
            NSLOG_INFO_MESSAGE);
        return NDO_ERROR;
    }

    /* do some initialization stuff... */
    if (ndomod_init() == NDO_ERROR) {
        ndomod_write_to_logs(
            "ndo: An error occurred while attempting to initialize.",
            NSLOG_INFO_MESSAGE);
        return NDO_ERROR;
    }

    /* connect to the database */
    if (ndomod_db_connect() == NDO_ERROR) {

        memset(temp_buffer, 0, sizeof(temp_buffer));
        snprintf(temp_buffer, sizeof(temp_buffer) - 1,
            "ndo: An error occured while attempting to connect to database: %s",
            mysql_error(ndomod_mysql));
        ndomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);
        return NDO_ERROR;
    }

    return NDO_OK;
}


/* Shutdown and release our resources when the module is unloaded. */
int nebmodule_deinit(int flags, int reason)
{
    ndomod_deinit();
    ndomod_write_to_logs("ndo: Shutdown complete.", NSLOG_INFO_MESSAGE);
    return NDO_OK;
}



/****************************************************************************/
/* INIT/DEINIT FUNCTIONS                                                    */
/****************************************************************************/

/* checks to make sure Nagios object version matches what we know about */
int ndomod_check_nagios_object_version(void)
{
    char temp_buffer[NDOMOD_MAX_BUFLEN];
    int compiled_version = __nagios_object_structure_version;
    int included_version = CURRENT_OBJECT_STRUCTURE_VERSION;

    if (compiled_version == included_version) {
        return NDO_OK;
    }

    /* Temporary special case so newer ndomod can be used with slightly
       older nagios in order to get longoutput on state changes */
    if (included_version >= 403 && compiled_version == 402) {
        has_ver403_long_output = NDO_FALSE;
        return NDO_OK;
    }

    snprintf(temp_buffer, sizeof(temp_buffer) - 1, 
        "ndo: I've been compiled with support for revision %d "
        "of the internal Nagios object structures, "
        "but the Nagios daemon is currently using revision %d. "
        "I'm going to unload so I don't cause any problems...\n",
        included_version, compiled_version);
    temp_buffer[sizeof(temp_buffer) - 1] = '\x0';
    ndomod_write_to_logs(temp_buffer, NSLOG_INFO_MESSAGE);

    return NDO_ERROR;
}


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



int ndomod_init(void)
{
    if (ndomod_register_callbacks() == NDO_ERROR) {
        return NDO_ERROR;
    }

    return NDO_OK;
}



int ndomod_deinit(void)
{
    ndomod_deregister_callbacks();
    ndomod_free_config_memory();

    return NDO_OK;
}




/****************************************************************************/
/* CONFIG FUNCTIONS                                                         */
/****************************************************************************/

/* process arguments that were passed to the module at startup */
int ndomod_process_module_args(char *args)
{
    char *ptr = NULL;
    char **arglist = NULL;
    char **newarglist = NULL;
    int argcount = 0;
    int memblocks = 64;
    int arg = 0;

    if (args == NULL) {
        return NDO_OK;
    }

    /* get all the var/val argument pairs */

    /* allocate some memory */
    arglist = (char **)malloc(memblocks*sizeof(char **));
    if (arglist == NULL) {
        return NDO_ERROR;
    }

    /* process all args */
    ptr = strtok(args, ",");
    while (ptr) {

        /* save the argument */
        arglist[argcount++] = strdup(ptr);

        /* allocate more memory if needed */
        if (!(argcount % memblocks)) {

            newarglist = (char **)realloc(arglist, (argcount+memblocks)*sizeof(char **));
            if (newarglist == NULL) {

                for (arg = 0; arg < argcount; arg++) {
                    free(arglist[argcount]);
                }

                free(arglist);
                return NDO_ERROR;
            }
            else {
                arglist = newarglist;
            }
        }

        ptr = strtok(NULL, ",");
    }

    /* terminate the arg list */
    arglist[argcount] = '\x0';

    /* process each argument */
    for (arg = 0; arg < argcount; arg++) {

        if (ndomod_process_config_var(arglist[arg]) == NDO_ERROR) {

            for (arg = 0; arg < argcount; arg++) {
                free(arglist[arg]);
            }

            free(arglist);
            return NDO_ERROR;
        }
    }

    /* free allocated memory */
    for (arg = 0; arg < argcount; arg++) {
        free(arglist[arg]);
    }

    free(arglist);

    return NDO_OK;
}


/* process all config vars in a file */
int ndomod_process_config_file(char *filename)
{
    ndo_mmapfile *thefile = NULL;
    char *buf             = NULL;
    int result            = NDO_OK;

    /* open the file */
    thefile = ndo_mmap_fopen(filename);
    if (thefile == NULL) {
        return NDO_ERROR;
    }

    /* process each line of the file */
    while (buf = ndo_mmap_fgets(thefile)) {

        /* skip comments */
        if (buf[0] == '#') {
            free(buf);
            continue;
        }

        /* skip blank lines */
        if (!strcmp(buf, "")) {
            free(buf);
            continue;
        }

        /* process the variable */
        result = ndomod_process_config_var(buf);

        /* free memory */
        free(buf);

        if (result != NDO_OK)
            break;
        }

    /* close the file */
    ndo_mmap_fclose(thefile);

    return result;
}


/* process a single module config variable */
int ndomod_process_config_var(char *arg)
{
    char *var = NULL;
    char *val = NULL;

    /* split var/val */
    var = strtok(arg, " = ");
    val = strtok(NULL, "\n");

    /* skip incomplete var/val pairs */
    if (var == NULL || val == NULL) {
        return NDO_OK;
    }

    /* strip var/val */
    ndomod_strip(var);
    ndomod_strip(val);

    /* process the variable... */

    if (!strcmp(var, "config_file")) {
        ndomod_process_config_file(val);
    }

    else if (!strcmp(var, "db_host")) {
        ndomod_db_host = strdup(val);
    }
    else if (!strcmp(var, "db_port")) {
        ndomod_db_port = atoi(val);
    }
    else if (!strcmp(var, "db_socket")) {
        ndomod_db_socket = strdup(val);
    }
    else if (!strcmp(var, "db_user")) {
        ndomod_db_user = strdup(val);
    }
    else if (!strcmp(var, "db_password") || !strcmp(var, "db_pass")) {
        ndomod_db_pass = strdup(val);
    }
    else if (!strcmp(var, "db_name")) {
        ndomod_db_name = strdup(val);
    }

    /* add bitwise processing opts */
    else if (!strcmp(var, "process_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_PROCESS_DATA;
    }
    else if (!strcmp(var, "timed_event_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_TIMED_EVENT_DATA;
    }
    else if (!strcmp(var, "log_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_LOG_DATA;
    }
    else if (!strcmp(var, "system_command_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_SYSTEM_COMMAND_DATA;
    }
    else if (!strcmp(var, "event_handler_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_EVENT_HANDLER_DATA;
    }
    else if (!strcmp(var, "notification_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_NOTIFICATION_DATA;
    }
    else if (!strcmp(var, "service_check_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_SERVICE_CHECK_DATA ;
    }
    else if (!strcmp(var, "host_check_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_HOST_CHECK_DATA;
    }
    else if (!strcmp(var, "comment_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_COMMENT_DATA;
    }
    else if (!strcmp(var, "downtime_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_DOWNTIME_DATA;
    }
    else if (!strcmp(var, "flapping_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_FLAPPING_DATA;
    }
    else if (!strcmp(var, "program_status_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_PROGRAM_STATUS_DATA;
    }
    else if (!strcmp(var, "host_status_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_HOST_STATUS_DATA;
    }
    else if (!strcmp(var, "service_status_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_SERVICE_STATUS_DATA;
    }
    else if (!strcmp(var, "external_command_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_EXTERNAL_COMMAND_DATA;
    }
    else if (!strcmp(var, "object_config_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_OBJECT_CONFIG_DATA;
    }
    else if (!strcmp(var, "main_config_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_MAIN_CONFIG_DATA;
    }
    else if (!strcmp(var, "acknowledgement_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_ACKNOWLEDGEMENT_DATA;
    }
    else if (!strcmp(var, "statechange_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_STATECHANGE_DATA ;
    }
    else if (!strcmp(var, "state_change_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_STATECHANGE_DATA ;
    }
    else if (!strcmp(var, "contact_status_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_CONTACT_STATUS_DATA;
    }

    /* we don't do anything with these:
    else if (!strcmp(var, "aggregated_status_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_AGGREGATED_STATUS_DATA;
    }
    else if (!strcmp(var, "adaptive_program_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_ADAPTIVE_PROGRAM_DATA;
    }
    else if (!strcmp(var, "adaptive_host_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_ADAPTIVE_HOST_DATA;
    }
    else if (!strcmp(var, "adaptive_service_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_ADAPTIVE_SERVICE_DATA;
    }
    else if (!strcmp(var, "retention_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_RETENTION_DATA;
    }
    else if (!strcmp(var, "adaptive_contact_data") && atoi(val) == 1) {
        ndomod_process_options |= NDOMOD_PROCESS_ADAPTIVE_CONTACT_DATA ;
    }
    */

    /* data_processing_options will override individual values if set */
    else if (!strcmp(var, "data_processing_options")) {
        if (!strcmp(val, " - 1")) {
            ndomod_process_options = NDOMOD_PROCESS_EVERYTHING;
        }
        else {
            ndomod_process_options = strtoul(val, NULL, 0);
        }
    }

    else if (!strcmp(var, "config_output_options")) {
        ndomod_config_output_options = atoi(val);
    }

    /* new processing options will be skipped if they're set to 0
    else {
        printf("Invalid ndomod config option: %s\n", var);
        return NDO_ERROR;
    }
    */

    return NDO_OK;
}



/****************************************************************************/
/* UTILITY FUNCTIONS                                                        */
/****************************************************************************/

/* writes a string to Nagios logs */
int ndomod_write_to_logs(char * buf, int flags)
{
    if (buf == NULL) {
        return NDO_ERROR;
    }

    return write_to_all_logs(buf, flags);
}


/****************************************************************************/
/* CALLBACK FUNCTIONS                                                       */
/****************************************************************************/

#define NDO_REGISTER_CALLBACK(opt, type, msg) \
    do { \
        if ((result == NDO_OK) && (ndomod_process_options & NDOMOD_PROCESS_##opt)) { \
            result = neb_register_callback(NEBCALLBACK_##type, ndomod_module_handle, 0, ndomod_broker_data); \
            if (result == NDO_OK) { \
                ndomod_write_to_logs("ndo registered for " msg " data", NSLOG_INFO_MESSAGE); \
            } \
            else { \
                ndomod_write_to_logs("error registering ndo for " msg " data", NSLOG_INFO_MESSAGE); \
            } \
        } \
    } while (0)


/* registers for callbacks */
int ndomod_register_callbacks(void)
{
    int result = NDO_OK;

    NDO_REGISTER_CALLBACK(PROCESS_DATA, PROCESS_DATA, "process");
    NDO_REGISTER_CALLBACK(TIMED_EVENT_DATA, TIMED_EVENT_DATA, "timed event");
    NDO_REGISTER_CALLBACK(LOG_DATA, LOG_DATA, "log");
    NDO_REGISTER_CALLBACK(SYSTEM_COMMAND_DATA, SYSTEM_COMMAND_DATA, "system command");
    NDO_REGISTER_CALLBACK(EVENT_HANDLER_DATA, EVENT_HANDLER_DATA, "event handler");
    NDO_REGISTER_CALLBACK(NOTIFICATION_DATA, NOTIFICATION_DATA, "notification");
    NDO_REGISTER_CALLBACK(SERVICE_CHECK_DATA, SERVICE_CHECK_DATA, "service check");
    NDO_REGISTER_CALLBACK(HOST_CHECK_DATA, HOST_CHECK_DATA, "host check");
    NDO_REGISTER_CALLBACK(COMMENT_DATA, COMMENT_DATA, "comment");
    NDO_REGISTER_CALLBACK(DOWNTIME_DATA, DOWNTIME_DATA, "downtime");
    NDO_REGISTER_CALLBACK(FLAPPING_DATA, FLAPPING_DATA, "flapping");
    NDO_REGISTER_CALLBACK(PROGRAM_STATUS_DATA, PROGRAM_STATUS_DATA, "program status");
    NDO_REGISTER_CALLBACK(HOST_STATUS_DATA, HOST_STATUS_DATA, "host status");
    NDO_REGISTER_CALLBACK(SERVICE_STATUS_DATA, SERVICE_STATUS_DATA, "service status");
    NDO_REGISTER_CALLBACK(EXTERNAL_COMMAND_DATA, EXTERNAL_COMMAND_DATA, "external command");
    NDO_REGISTER_CALLBACK(ACKNOWLEDGEMENT_DATA, ACKNOWLEDGEMENT_DATA, "acknowledgement");
    NDO_REGISTER_CALLBACK(STATECHANGE_DATA, STATE_CHANGE_DATA, "state change");
    NDO_REGISTER_CALLBACK(CONTACT_STATUS_DATA, CONTACT_STATUS_DATA, "contact status");
    NDO_REGISTER_CALLBACK(CONTACT_NOTIFICATION_DATA, CONTACT_NOTIFICATION_DATA, "contact");
    NDO_REGISTER_CALLBACK(CONTACT_NOTIFICATION_METHOD_DATA, CONTACT_NOTIFICATION_METHOD_DATA, "contact notification");

    /* we don't do anything with these, so no need to register callbacks:
    NDO_REGISTER_CALLBACK(ADAPTIVE_CONTACT_DATA, ADAPTIVE_CONTACT_DATA, "adaptive contact");
    NDO_REGISTER_CALLBACK(AGGREGATED_STATUS_DATA, AGGREGATED_STATUS_DATA, "aggregated status");
    NDO_REGISTER_CALLBACK(RETENTION_DATA, RETENTION_DATA, "retention");
    NDO_REGISTER_CALLBACK(ADAPTIVE_PROGRAM_DATA, ADAPTIVE_PROGRAM_DATA, "adaptive program");
    NDO_REGISTER_CALLBACK(ADAPTIVE_HOST_DATA, ADAPTIVE_HOST_DATA, "adaptive host");
    NDO_REGISTER_CALLBACK(ADAPTIVE_SERVICE_DATA, ADAPTIVE_SERVICE_DATA, "adaptive service");
    */

    return result;
}


/* deregisters callbacks */
int ndomod_deregister_callbacks(void)
{
    neb_deregister_callback(NEBCALLBACK_PROCESS_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_TIMED_EVENT_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_LOG_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_SYSTEM_COMMAND_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_EVENT_HANDLER_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_NOTIFICATION_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_SERVICE_CHECK_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_HOST_CHECK_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_COMMENT_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_DOWNTIME_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_FLAPPING_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_PROGRAM_STATUS_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_HOST_STATUS_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_SERVICE_STATUS_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_EXTERNAL_COMMAND_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_CONTACT_NOTIFICATION_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_ACKNOWLEDGEMENT_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_STATE_CHANGE_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_CONTACT_STATUS_DATA, ndomod_broker_data);

    /* we don't register these:
    neb_deregister_callback(NEBCALLBACK_ADAPTIVE_CONTACT_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_ADAPTIVE_PROGRAM_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_ADAPTIVE_HOST_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_ADAPTIVE_SERVICE_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_AGGREGATED_STATUS_DATA, ndomod_broker_data);
    neb_deregister_callback(NEBCALLBACK_RETENTION_DATA, ndomod_broker_data);
    */

    return NDO_OK;
}


/* handles brokered event data */
int ndomod_broker_data(int event_type, void * data)
{
    if (data == NULL) {
        return NDO_ERROR;
    }

    switch(event_type) {


    case NEBCALLBACK_PROCESS_DATA:

        ndomod_handle_process_data(data);
        break;


    case NEBCALLBACK_TIMED_EVENT_DATA:

        ndomod_handle_timed_event_data(data);
        break;


    case NEBCALLBACK_LOG_DATA:

        ndomod_handle_log_data(data);
        break;


    case NEBCALLBACK_SYSTEM_COMMAND_DATA:

        ndomod_handle_system_command_data(data);
        break;


    case NEBCALLBACK_EVENT_HANDLER_DATA:

        ndomod_handle_event_handler_data(data);
        break;


    case NEBCALLBACK_NOTIFICATION_DATA:

        ndomod_handle_notification_data(data);
        break;


    case NEBCALLBACK_SERVICE_CHECK_DATA:

        ndomod_handle_service_check_data(data);
        break;


    case NEBCALLBACK_HOST_CHECK_DATA:

        ndomod_handle_host_check_data(data);
        break;


    case NEBCALLBACK_COMMENT_DATA:

        ndomod_handle_comment_data(data);
        break;


    case NEBCALLBACK_DOWNTIME_DATA:

        ndomod_handle_downtime_data(data);
        break;


    case NEBCALLBACK_FLAPPING_DATA:

        ndomod_handle_flapping_data(data);
        break;


    case NEBCALLBACK_PROGRAM_STATUS_DATA:

        ndomod_handle_program_status_data(data);
        break;


    case NEBCALLBACK_HOST_STATUS_DATA:

        ndomod_handle_host_status_data(data);
        break;


    case NEBCALLBACK_SERVICE_STATUS_DATA:

        ndomod_handle_service_status_data(data);
        break;


    case NEBCALLBACK_CONTACT_STATUS_DATA:

        ndomod_handle_contact_status_data(data);
        break;


    case NEBCALLBACK_EXTERNAL_COMMAND_DATA:

        ndomod_handle_external_command_data(data);
        break;


    case NEBCALLBACK_CONTACT_NOTIFICATION_DATA:

        ndomod_handle_contact_notification_data(data);
        break;


    case NEBCALLBACK_CONTACT_NOTIFICATION_METHOD_DATA:

        ndomod_handle_contact_notification_method_data(data);
        break;


    case NEBCALLBACK_ACKNOWLEDGEMENT_DATA:

        ndomod_handle_acknowledgement_data(data);
        break;


    case NEBCALLBACK_STATE_CHANGE_DATA:

        ndomod_handle_statechange_data(data);
        break;


    /* these are all ignored:
    case NEBCALLBACK_ADAPTIVE_PROGRAM_DATA:
    case NEBCALLBACK_ADAPTIVE_HOST_DATA:
    case NEBCALLBACK_ADAPTIVE_SERVICE_DATA:
    case NEBCALLBACK_ADAPTIVE_CONTACT_DATA:
    case NEBCALLBACK_AGGREGATED_STATUS_DATA:
    case NEBCALLBACK_RETENTION_DATA:
    */
    }

    /* POST PROCESSING... */

    switch(event_type) {

    case NEBCALLBACK_PROCESS_DATA:

        ndomod_handle_process_data(data);

        /* process has passed pre-launch config verification, so dump original config */
        if (procdata->type == NEBTYPE_PROCESS_START) {

            ndomod_write_config_files();
            ndomod_write_config(NDOMOD_CONFIG_DUMP_ORIGINAL);
        }

        /* process is starting the event loop, so dump runtime vars */
        if (procdata->type == NEBTYPE_PROCESS_EVENTLOOPSTART) {

            ndomod_write_runtime_variables();
        }

        break;

    case NEBCALLBACK_RETENTION_DATA:

        ndomod_handle_retention_data(data);

        /* retained config was just read, so dump it */
        if (rdata->type == NEBTYPE_RETENTIONDATA_ENDLOAD) {

            ndomod_write_config(NDOMOD_CONFIG_DUMP_RETAINED);
        }

        break;

    default:
        break;
    }

    return 0;
}



/****************************************************************************/
/* CONFIG OUTPUT FUNCTIONS                                                  */
/****************************************************************************/

/* dumps all configuration data to sink */
int ndomod_write_config(int config_type)
{
    char temp_buffer[NDOMOD_MAX_BUFLEN];
    struct timeval now;
    int result;

    if (!(ndomod_config_output_options & config_type)) {
        return NDO_OK;
    }

    gettimeofday(&now, NULL);

    /* record start of config dump */
    snprintf(temp_buffer, NDOMOD_MAX_BUFLEN,
        "\n\n%d:\n%d = %s\n%d = %ld.%06ld\n%d\n\n",
        NDO_API_STARTCONFIGDUMP,
        NDO_DATA_CONFIGDUMPTYPE,
        (config_type == NDOMOD_CONFIG_DUMP_ORIGINAL) ? NDO_API_CONFIGDUMP_ORIGINAL : NDO_API_CONFIGDUMP_RETAINED,
        NDO_DATA_TIMESTAMP,
        now.tv_sec,
        now.tv_usec,
        NDO_API_ENDDATA);

    ndomod_write_to_sink(temp_buffer, NDO_TRUE, NDO_TRUE);

/*  ndomod_write_active_objects(); */

    /* dump object config info */
    result = ndomod_write_object_config(config_type);
    if (result != NDO_OK) {
        return result;
    }

    /* record end of config dump */
    snprintf(temp_buffer, NDOMOD_MAX_BUFLEN,
        "\n\n%d:\n%d = %ld.%06ld\n%d\n\n",
        NDO_API_ENDCONFIGDUMP,
        NDO_DATA_TIMESTAMP,
        now.tv_sec,
        now.tv_usec,
        NDO_API_ENDDATA);

    result = ndomod_write_to_sink(temp_buffer, NDO_TRUE, NDO_TRUE);

    return result;
}


/*************************************************************
 * Get a list of all active objects, so the "is_active" flag *
 * can be set on them in batches, instead of one at a time.  *
 *************************************************************/
void ndomod_write_active_objects()
{
    ndo_dbuf dbuf;
    struct ndo_broker_data active_objects[256];
    struct timeval now;
    command *temp_command           = NULL;
    timeperiod *temp_timeperiod     = NULL;
    contact *temp_contact           = NULL;
    contactgroup *temp_contactgroup = NULL;
    host *temp_host                 = NULL;
    hostgroup *temp_hostgroup       = NULL;
    service *temp_service           = NULL;
    servicegroup *temp_servicegroup = NULL;
    char *name1                     = NULL;
    char *name2                     = NULL;
    int i                           = 0;
    int obj_count                   = 0;

    /* get current time */
    gettimeofday(&now, NULL);

    /* initialize dynamic buffer (2KB chunk size) */
    ndo_dbuf_init(&dbuf, 2048);


    /********************************************************************************
    command definitions
    ********************************************************************************/
    active_objects[0].key = NDO_DATA_ACTIVEOBJECTSTYPE;
    active_objects[0].datatype = BD_INT;
    active_objects[0].value.integer = NDO_API_COMMANDDEFINITION;

    obj_count = 1;

    for (temp_command = command_list; temp_command != NULL; temp_command = temp_command->next) {

        name1 = ndo_escape_buffer(temp_command->name);
        if (name1 == NULL) {
            continue;
        }

        active_objects[obj_count].key = obj_count;
        active_objects[obj_count].datatype = BD_STRING;
        active_objects[obj_count].value.string = name1;

        if (++obj_count > 250) {

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_ACTIVEOBJECTSLIST,
                active_objects,
                obj_count,
                TRUE);

            ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
            ndo_dbuf_free(&dbuf);

            while (--obj_count) {
                free(active_objects[obj_count].value.string);
            }

            obj_count = 1;
        }
    }

    if (obj_count > 1) {

        ndomod_broker_data_serialize(&dbuf,
            NDO_API_ACTIVEOBJECTSLIST,
            active_objects, 
            obj_count, 
            TRUE);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
        ndo_dbuf_free(&dbuf);

        while (--obj_count) {
            free(active_objects[obj_count].value.string);
        }
    }


    /********************************************************************************
    time period definitions
    ********************************************************************************/
    active_objects[0].value.integer = NDO_API_TIMEPERIODDEFINITION;
    obj_count = 1;

    for (temp_timeperiod = timeperiod_list; temp_timeperiod != NULL; temp_timeperiod = temp_timeperiod->next) {

        name1 = ndo_escape_buffer(temp_timeperiod->name);
        if (name1 == NULL) {
            continue;
        }

        active_objects[obj_count].key = obj_count;
        active_objects[obj_count].datatype = BD_STRING;
        active_objects[obj_count].value.string = name1;

        if (++obj_count > 250) {

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_ACTIVEOBJECTSLIST,
                active_objects, 
                obj_count, 
                TRUE);

            ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
            ndo_dbuf_free(&dbuf);

            while (--obj_count) {
                free(active_objects[obj_count].value.string);
            }

            obj_count = 1;
        }
    }

    if (obj_count > 1) {

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_ACTIVEOBJECTSLIST,
            active_objects, 
            obj_count, 
            TRUE);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
        ndo_dbuf_free(&dbuf);

        while (--obj_count) {
            free(active_objects[obj_count].value.string);
        }
    }


    /********************************************************************************
    contact definitions
    ********************************************************************************/
    active_objects[0].value.integer = NDO_API_CONTACTDEFINITION;
    obj_count = 1;

    for (temp_contact = contact_list; temp_contact != NULL; temp_contact = temp_contact->next) {
        name1 = ndo_escape_buffer(temp_contact->name);
        if (name1 == NULL) {
            continue;
        }

        active_objects[obj_count].key = obj_count;
        active_objects[obj_count].datatype = BD_STRING;
        active_objects[obj_count].value.string = name1;

        if (++obj_count > 250) {

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_ACTIVEOBJECTSLIST,
                active_objects, 
                obj_count, 
                TRUE);

            ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
            ndo_dbuf_free(&dbuf);

            while (--obj_count) {
                free(active_objects[obj_count].value.string);
            }

            obj_count = 1;
        }
    }

    if (obj_count > 1) {

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_ACTIVEOBJECTSLIST,
            active_objects, 
            obj_count, 
            TRUE);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
        ndo_dbuf_free(&dbuf);

        while (--obj_count) {
            free(active_objects[obj_count].value.string);
        }
    }


    /********************************************************************************
    contact group definitions
    ********************************************************************************/
    active_objects[0].value.integer = NDO_API_CONTACTGROUPDEFINITION;
    obj_count = 1;

    for (temp_contactgroup = contactgroup_list; temp_contactgroup != NULL; temp_contactgroup = temp_contactgroup->next) {

        name1 = ndo_escape_buffer(temp_contactgroup->group_name);
        if (name1 == NULL) {
            continue;
        }

        active_objects[obj_count].key = obj_count;
        active_objects[obj_count].datatype = BD_STRING;
        active_objects[obj_count].value.string = name1;

        if (++obj_count > 250) {

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_ACTIVEOBJECTSLIST,
                active_objects, 
                obj_count, 
                TRUE);

            ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
            ndo_dbuf_free(&dbuf);

            while (--obj_count) {
                free(active_objects[obj_count].value.string);
            }

            obj_count = 1;
        }
    }

    if (obj_count > 1) {

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_ACTIVEOBJECTSLIST,
            active_objects, 
            obj_count, 
            TRUE);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
        ndo_dbuf_free(&dbuf);

        while (--obj_count) {
            free(active_objects[obj_count].value.string);
        }
    }


    /********************************************************************************
    host definitions
    ********************************************************************************/
    active_objects[0].value.integer = NDO_API_HOSTDEFINITION;
    obj_count = 1;

    for (temp_host = host_list; temp_host != NULL; temp_host = temp_host->next) {

        name1 = ndo_escape_buffer(temp_host->name);
        if (name1 == NULL) {
            continue;
        }

        active_objects[obj_count].key = obj_count;
        active_objects[obj_count].datatype = BD_STRING;
        active_objects[obj_count].value.string = name1;

        if (++obj_count > 250) {

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_ACTIVEOBJECTSLIST,
                active_objects, 
                obj_count, 
                TRUE);

            ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
            ndo_dbuf_free(&dbuf);

            while (--obj_count) {
                free(active_objects[obj_count].value.string);
            }

            obj_count = 1;
        }
    }

    if (obj_count > 1) {

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_ACTIVEOBJECTSLIST,
            active_objects, 
            obj_count, 
            TRUE);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
        ndo_dbuf_free(&dbuf);

        while (--obj_count) {
            free(active_objects[obj_count].value.string);
        }
    }


    /********************************************************************************
    host group definitions
    ********************************************************************************/
    active_objects[0].value.integer = NDO_API_HOSTGROUPDEFINITION;
    obj_count = 1;

    for (temp_hostgroup = hostgroup_list; temp_hostgroup != NULL; temp_hostgroup = temp_hostgroup->next) {

        name1 = ndo_escape_buffer(temp_hostgroup->group_name);
        if (name1 == NULL) {
            continue;
        }

        active_objects[obj_count].key = obj_count;
        active_objects[obj_count].datatype = BD_STRING;
        active_objects[obj_count].value.string = name1;

        if (++obj_count > 250) {

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_ACTIVEOBJECTSLIST,
                active_objects, 
                obj_count, 
                TRUE);

            ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
            ndo_dbuf_free(&dbuf);

            while (--obj_count) {
                free(active_objects[obj_count].value.string);
            }

            obj_count = 1;
        }
    }

    if (obj_count > 1) {

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_ACTIVEOBJECTSLIST,
            active_objects, 
            obj_count, 
            TRUE);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
        ndo_dbuf_free(&dbuf);

        while (--obj_count) {
            free(active_objects[obj_count].value.string);
        }
    }


    /********************************************************************************
    service definitions
    ********************************************************************************/
    active_objects[0].value.integer = NDO_API_SERVICEDEFINITION;
    obj_count = 1;

    for (temp_service = service_list; temp_service != NULL; temp_service = temp_service->next) {

        name1 = ndo_escape_buffer(temp_service->host_name);
        if (name1 == NULL) {
            continue;
        }

        name2 = ndo_escape_buffer(temp_service->description);
        if (name2 == NULL) {
            free(name1);
            continue;
        }

        active_objects[obj_count].key = obj_count;
        active_objects[obj_count].datatype = BD_STRING;
        active_objects[obj_count].value.string = name1;

        ++obj_count;

        active_objects[obj_count].key = obj_count;
        active_objects[obj_count].datatype = BD_STRING;
        active_objects[obj_count].value.string = name2;

        if (++obj_count > 250) {

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_ACTIVEOBJECTSLIST,
                active_objects, 
                obj_count, 
                TRUE);

            ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
            ndo_dbuf_free(&dbuf);

            while (--obj_count) {
                free(active_objects[obj_count].value.string);
            }

            obj_count = 1;
        }
    }

    if (obj_count > 1) {

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_ACTIVEOBJECTSLIST,
            active_objects, 
            obj_count, 
            TRUE);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
        ndo_dbuf_free(&dbuf);

        while (--obj_count) {
            free(active_objects[obj_count].value.string);
        }
    }


    /********************************************************************************
    service group definitions
    ********************************************************************************/
    active_objects[0].value.integer = NDO_API_SERVICEGROUPDEFINITION;
    obj_count = 1;

    for (temp_servicegroup = servicegroup_list; temp_servicegroup != NULL ; temp_servicegroup = temp_servicegroup->next) {

        name1 = ndo_escape_buffer(temp_servicegroup->group_name);
        if (name1 == NULL) {
            continue;
        }

        active_objects[obj_count].key = obj_count;
        active_objects[obj_count].datatype = BD_STRING;
        active_objects[obj_count].value.string = name1;

        if (++obj_count > 250) {

            ndomod_broker_data_serialize(&dbuf, 
                NDO_API_ACTIVEOBJECTSLIST,
                active_objects, 
                obj_count, 
                TRUE);

            ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
            ndo_dbuf_free(&dbuf);

            while (--obj_count) {
                free(active_objects[obj_count].value.string);
            }

            obj_count = 1;
        }
    }

    if (obj_count > 1) {

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_ACTIVEOBJECTSLIST,
            active_objects, 
            obj_count, 
            TRUE);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);
        ndo_dbuf_free(&dbuf);

        while (--obj_count) {
            free(active_objects[obj_count].value.string);
        }
    }
}


#define OBJECTCONFIG_ES_ITEMS 16

/* dumps object configuration data to sink */
int ndomod_write_object_config(int config_type)
{
    char temp_buffer[NDOMOD_MAX_BUFLEN] = { 0 };
    char *es[OBJECTCONFIG_ES_ITEMS] = { NULL };
    ndo_dbuf dbuf;
    struct timeval now;
    int x                                         = 0;
    command *temp_command                         = NULL;
    timeperiod *temp_timeperiod                   = NULL;
    timerange *temp_timerange                     = NULL;
    contact *temp_contact                         = NULL;
    commandsmember *temp_commandsmember           = NULL;
    contactgroup *temp_contactgroup               = NULL;
    host *temp_host                               = NULL;
    hostsmember *temp_hostsmember                 = NULL;
    contactgroupsmember *temp_contactgroupsmember = NULL;
    hostgroup *temp_hostgroup                     = NULL;
    service *temp_service                         = NULL;
    servicegroup *temp_servicegroup               = NULL;
    hostescalation *temp_hostescalation           = NULL;
    serviceescalation *temp_serviceescalation     = NULL;
    hostdependency *temp_hostdependency           = NULL;
    servicedependency *temp_servicedependency     = NULL;


    int have_2d_coords                            = FALSE;
    int x_2d                                      = 0;
    int y_2d                                      = 0;
    int have_3d_coords                            = FALSE;
    double x_3d                                   = 0.0;
    double y_3d                                   = 0.0;
    double z_3d                                   = 0.0;
    double first_notification_delay               = 0.0;
    double retry_interval                         = 0.0;
    int notify_on_host_downtime                   = 0;
    int notify_on_service_downtime                = 0;
    int host_notifications_enabled                = 0;
    int service_notifications_enabled             = 0;
    int can_submit_commands                       = 0;
    int flap_detection_on_up                      = 0;
    int flap_detection_on_down                    = 0;
    int flap_detection_on_unreachable             = 0;
    int flap_detection_on_ok                      = 0;
    int flap_detection_on_warning                 = 0;
    int flap_detection_on_unknown                 = 0;
    int flap_detection_on_critical                = 0;

    customvariablesmember *temp_customvar         = NULL;
    contactsmember *temp_contactsmember           = NULL;
    servicesmember *temp_servicesmember           = NULL;

    if (!(ndomod_process_options & NDOMOD_PROCESS_OBJECT_CONFIG_DATA)) {
        return NDO_OK;
    }

    if (!(ndomod_config_output_options & config_type)) {
        return NDO_OK;
    }

    /* get current time */
    gettimeofday(&now, NULL);

    /* initialize dynamic buffer (2KB chunk size) */
    ndo_dbuf_init(&dbuf, 2048);


    /****** dump command config ******/
    for (temp_command = command_list; temp_command != NULL; temp_command = temp_command->next) {

        es[0] = ndo_escape_buffer(temp_command->name);
        es[1] = ndo_escape_buffer(temp_command->command_line);

        struct ndo_broker_data command_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_COMMANDNAME, es[0]),
            NDODATA_STRING(NDO_DATA_COMMANDLINE, es[1]),
        };

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_COMMANDDEFINITION, 
            command_definition,
            ARRAY_SIZE(command_definition), 
            TRUE);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndomod_free_esc_buffers(es, 2);
        ndo_dbuf_free(&dbuf);
    }


    /****** dump timeperiod config ******/
    for (temp_timeperiod = timeperiod_list; temp_timeperiod != NULL; temp_timeperiod = temp_timeperiod->next) {

        es[0] = ndo_escape_buffer(temp_timeperiod->name);
        es[1] = ndo_escape_buffer(temp_timeperiod->alias);

        struct ndo_broker_data timeperiod_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_TIMEPERIODNAME, es[0]),
            NDODATA_STRING(NDO_DATA_TIMEPERIODALIAS, es[1]),
        };

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_TIMEPERIODDEFINITION, 
            timeperiod_definition, 
            ARRAY_SIZE(timeperiod_definition), 
            FALSE);

        /* dump timeranges for each day */
        for (x = 0; x < 7; x++) {
            for (temp_timerange = temp_timeperiod->days[x]; temp_timerange != NULL; temp_timerange = temp_timerange->next) {

                snprintf(temp_buffer, NDOMOD_MAX_BUFLEN,
                    "\n%d = %d:%lu-%lu",
                    NDO_DATA_TIMERANGE,
                    x,
                    temp_timerange->range_start,
                    temp_timerange->range_end);
                
                ndo_dbuf_strcat(&dbuf, temp_buffer);
            }
        }

        ndomod_enddata_serialize(&dbuf);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndomod_free_esc_buffers(es, 2);
        ndo_dbuf_free(&dbuf);
    }


    /****** dump contact config ******/
    for (temp_contact = contact_list; temp_contact != NULL; temp_contact = temp_contact->next) {

        es[0] = ndo_escape_buffer(temp_contact->name);
        es[1] = ndo_escape_buffer(temp_contact->alias);
        es[2] = ndo_escape_buffer(temp_contact->email);
        es[3] = ndo_escape_buffer(temp_contact->pager);
        es[4] = ndo_escape_buffer(temp_contact->host_notification_period);
        es[5] = ndo_escape_buffer(temp_contact->service_notification_period);

        struct ndo_broker_data contact_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_CONTACTNAME, es[0]),
            NDODATA_STRING(NDO_DATA_CONTACTALIAS, es[1]),
            NDODATA_STRING(NDO_DATA_EMAILADDRESS, es[2]),
            NDODATA_STRING(NDO_DATA_PAGERADDRESS, es[3]),
            NDODATA_STRING(NDO_DATA_HOSTNOTIFICATIONPERIOD, es[4]),
            NDODATA_STRING(NDO_DATA_SERVICENOTIFICATIONPERIOD, es[5]),
            NDODATA_INTEGER(NDO_DATA_SERVICENOTIFICATIONSENABLED, temp_contact->service_notifications_enabled),
            NDODATA_INTEGER(NDO_DATA_HOSTNOTIFICATIONSENABLED, temp_contact->host_notifications_enabled),
            NDODATA_INTEGER(NDO_DATA_CANSUBMITCOMMANDS, temp_contact->can_submit_commands),

            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICEUNKNOWN, flag_isset(temp_contact->service_notification_options, OPT_UNKNOWN)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICEWARNING, flag_isset(temp_contact->service_notification_options, OPT_WARNING)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICECRITICAL, flag_isset(temp_contact->service_notification_options, OPT_CRITICAL)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICERECOVERY, flag_isset(temp_contact->service_notification_options, OPT_RECOVERY)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICEFLAPPING, flag_isset(temp_contact->service_notification_options, OPT_FLAPPING)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICEDOWNTIME, flag_isset(temp_contact->service_notification_options, OPT_DOWNTIME)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYHOSTDOWN, flag_isset(temp_contact->host_notification_options, OPT_DOWN)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYHOSTUNREACHABLE, flag_isset(temp_contact->host_notification_options, OPT_UNREACHABLE)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYHOSTRECOVERY, flag_isset(temp_contact->host_notification_options, OPT_RECOVERY)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYHOSTFLAPPING, flag_isset(temp_contact->host_notification_options, OPT_FLAPPING)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYHOSTDOWNTIME, flag_isset(temp_contact->host_notification_options, OPT_DOWNTIME)),
            NDODATA_INTEGER(NDO_DATA_MINIMUMIMPORTANCE, temp_contact->minimum_value),
        };

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_CONTACTDEFINITION, 
            contact_definition, 
            ARRAY_SIZE(contact_definition), 
            FALSE);

        ndomod_free_esc_buffers(es, 6);

        /* dump addresses for each contact */
        for (x = 0; x < MAX_CONTACT_ADDRESSES; x++) {

            es[0] = ndo_escape_buffer(temp_contact->address[x]);

            snprintf(temp_buffer, NDOMOD_MAX_BUFLEN,
                "\n%d = %d:%s",
                NDO_DATA_CONTACTADDRESS,
                x + 1,
                (es[0] == NULL) ? "" : es[0]);

            ndo_dbuf_strcat(&dbuf, temp_buffer);

            free(es[0]);
        }

        /* dump host notification commands for each contact */
        ndomod_commands_serialize(temp_contact->host_notification_commands,
                &dbuf, NDO_DATA_HOSTNOTIFICATIONCOMMAND);

        /* dump service notification commands for each contact */
        ndomod_commands_serialize(temp_contact->service_notification_commands,
                &dbuf, NDO_DATA_SERVICENOTIFICATIONCOMMAND);

        /* dump customvars */
        ndomod_customvars_serialize(temp_contact->custom_variables, &dbuf);

        ndomod_enddata_serialize(&dbuf);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndo_dbuf_free(&dbuf);
    }


    /****** dump contactgroup config ******/
    for (temp_contactgroup = contactgroup_list; temp_contactgroup != NULL; temp_contactgroup = temp_contactgroup->next) {

        es[0] = ndo_escape_buffer(temp_contactgroup->group_name);
        es[1] = ndo_escape_buffer(temp_contactgroup->alias);

        struct ndo_broker_data contactgroup_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_CONTACTGROUPNAME, es[0]),
            NDODATA_STRING(NDO_DATA_CONTACTGROUPALIAS, es[1]),
        };

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_CONTACTGROUPDEFINITION, 
            contactgroup_definition, 
            ARRAY_SIZE(contactgroup_definition), 
            FALSE);

        /* dump members for each contactgroup */
        ndomod_contacts_serialize(temp_contactgroup->members, &dbuf,
                NDO_DATA_CONTACTGROUPMEMBER);

        ndomod_enddata_serialize(&dbuf);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndomod_free_esc_buffers(es, 2);
        ndo_dbuf_free(&dbuf);
    }


    /****** dump host config ******/
    for (temp_host = host_list; temp_host != NULL; temp_host = temp_host->next) {

        es[0] = ndo_escape_buffer(temp_host->name);
        es[1] = ndo_escape_buffer(temp_host->alias);
        es[2] = ndo_escape_buffer(temp_host->address);
        es[3] = ndo_escape_buffer(temp_host->check_command);
        es[4] = ndo_escape_buffer(temp_host->event_handler);
        es[5] = ndo_escape_buffer(temp_host->notification_period);
        es[6] = ndo_escape_buffer(temp_host->check_period);
        es[7] = ndo_escape_buffer("");

        es[8] = ndo_escape_buffer(temp_host->notes);
        es[9] = ndo_escape_buffer(temp_host->notes_url);
        es[10] = ndo_escape_buffer(temp_host->action_url);
        es[11] = ndo_escape_buffer(temp_host->icon_image);
        es[12] = ndo_escape_buffer(temp_host->icon_image_alt);
        es[13] = ndo_escape_buffer(temp_host->vrml_image);
        es[14] = ndo_escape_buffer(temp_host->statusmap_image);
        es[15] = ndo_escape_buffer(temp_host->display_name);


        struct ndo_broker_data host_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_HOSTNAME, es[0]),
            NDODATA_STRING(NDO_DATA_DISPLAYNAME, es[15]),
            NDODATA_STRING(NDO_DATA_HOSTALIAS, es[1]),
            NDODATA_STRING(NDO_DATA_HOSTADDRESS, es[2]),
            NDODATA_STRING(NDO_DATA_HOSTCHECKCOMMAND, es[3]),
            NDODATA_STRING(NDO_DATA_HOSTEVENTHANDLER, es[4]),
            NDODATA_STRING(NDO_DATA_HOSTNOTIFICATIONPERIOD, es[5]),
            NDODATA_STRING(NDO_DATA_HOSTFAILUREPREDICTIONOPTIONS, es[7]),
            NDODATA_FLOATING_POINT(NDO_DATA_HOSTCHECKINTERVAL, (double)temp_host->check_interval),
            NDODATA_FLOATING_POINT(NDO_DATA_HOSTRETRYINTERVAL, (double)temp_host->retry_interval),
            NDODATA_INTEGER(NDO_DATA_HOSTMAXCHECKATTEMPTS, temp_host->max_attempts),
            NDODATA_FLOATING_POINT(NDO_DATA_FIRSTNOTIFICATIONDELAY, temp_host->first_notification_delay),
            NDODATA_FLOATING_POINT(NDO_DATA_HOSTNOTIFICATIONINTERVAL, (double)temp_host->notification_interval),
            NDODATA_INTEGER(NDO_DATA_NOTIFYHOSTDOWN, flag_isset(temp_host->notification_options, OPT_DOWN)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYHOSTUNREACHABLE, flag_isset(temp_host->notification_options, OPT_UNREACHABLE)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYHOSTRECOVERY, flag_isset(temp_host->notification_options, OPT_RECOVERY)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYHOSTFLAPPING, flag_isset(temp_host->notification_options, OPT_FLAPPING)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYHOSTDOWNTIME, flag_isset(temp_host->notification_options, OPT_DOWNTIME)),
            NDODATA_INTEGER(NDO_DATA_HOSTFLAPDETECTIONENABLED, temp_host->flap_detection_enabled),
            NDODATA_INTEGER(NDO_DATA_FLAPDETECTIONONUP, flag_isset(temp_host->flap_detection_options, OPT_UP)),
            NDODATA_INTEGER(NDO_DATA_FLAPDETECTIONONDOWN, flag_isset(temp_host->flap_detection_options, OPT_DOWN)),
            NDODATA_INTEGER(NDO_DATA_FLAPDETECTIONONUNREACHABLE, flag_isset(temp_host->flap_detection_options, OPT_UNREACHABLE)),
            NDODATA_FLOATING_POINT(NDO_DATA_LOWHOSTFLAPTHRESHOLD, temp_host->low_flap_threshold),
            NDODATA_FLOATING_POINT(NDO_DATA_HIGHHOSTFLAPTHRESHOLD, temp_host->high_flap_threshold),
            NDODATA_INTEGER(NDO_DATA_STALKHOSTONUP, flag_isset(temp_host->stalking_options, OPT_UP)),
            NDODATA_INTEGER(NDO_DATA_STALKHOSTONDOWN, flag_isset(temp_host->stalking_options, OPT_DOWN)),
            NDODATA_INTEGER(NDO_DATA_STALKHOSTONUNREACHABLE, flag_isset(temp_host->stalking_options, OPT_UNREACHABLE)),
            NDODATA_INTEGER(NDO_DATA_HOSTFRESHNESSCHECKSENABLED, temp_host->check_freshness),
            NDODATA_INTEGER(NDO_DATA_HOSTFRESHNESSTHRESHOLD, temp_host->freshness_threshold),
            NDODATA_INTEGER(NDO_DATA_PROCESSHOSTPERFORMANCEDATA, temp_host->process_performance_data),
            NDODATA_INTEGER(NDO_DATA_ACTIVEHOSTCHECKSENABLED, temp_host->checks_enabled),
            NDODATA_INTEGER(NDO_DATA_PASSIVEHOSTCHECKSENABLED, temp_host->accept_passive_checks),
            NDODATA_INTEGER(NDO_DATA_HOSTEVENTHANDLERENABLED, temp_host->event_handler_enabled),
            NDODATA_INTEGER(NDO_DATA_RETAINHOSTSTATUSINFORMATION, temp_host->retain_status_information),
            NDODATA_INTEGER(NDO_DATA_RETAINHOSTNONSTATUSINFORMATION, temp_host->retain_nonstatus_information),
            NDODATA_INTEGER(NDO_DATA_HOSTNOTIFICATIONSENABLED, temp_host->notifications_enabled),
            NDODATA_INTEGER(NDO_DATA_HOSTFAILUREPREDICTIONENABLED, 0),
            NDODATA_INTEGER(NDO_DATA_OBSESSOVERHOST, temp_host->obsess),
            NDODATA_STRING(NDO_DATA_NOTES, es[8]),
            NDODATA_STRING(NDO_DATA_NOTESURL, es[9]),
            NDODATA_STRING(NDO_DATA_ACTIONURL, es[10]),
            NDODATA_STRING(NDO_DATA_ICONIMAGE, es[11]),
            NDODATA_STRING(NDO_DATA_ICONIMAGEALT, es[12]),
            NDODATA_STRING(NDO_DATA_VRMLIMAGE, es[13]),
            NDODATA_STRING(NDO_DATA_STATUSMAPIMAGE, es[14]),
            NDODATA_STRING(NDO_DATA_CONTACTGROUPALIAS, es[1]),
            NDODATA_INTEGER(NDO_DATA_HAVE2DCOORDS, temp_host->have_2d_coords),
            NDODATA_INTEGER(NDO_DATA_X2D, temp_host->x_2d),
            NDODATA_INTEGER(NDO_DATA_Y2D, temp_host->y_2d),
            NDODATA_INTEGER(NDO_DATA_HAVE3DCOORDS, temp_host->have_3d_coords),
            NDODATA_FLOATING_POINT(NDO_DATA_X3D, temp_host->y_3d),
            NDODATA_FLOATING_POINT(NDO_DATA_Y3D, temp_host->y_3d),
            NDODATA_FLOATING_POINT(NDO_DATA_Z3D, temp_host->z_3d),
            NDODATA_INTEGER(NDO_DATA_IMPORTANCE, temp_host->hourly_value),
        };

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_HOSTDEFINITION, 
            host_definition, 
            ARRAY_SIZE(host_definition), 
            FALSE);

        /* dump parent hosts */
        ndomod_hosts_serialize(temp_host->parent_hosts, &dbuf,
                NDO_DATA_PARENTHOST);

        /* dump contactgroups */
        ndomod_contactgroups_serialize(temp_host->contact_groups, &dbuf);

        /* dump individual contacts (not supported in Nagios 2.x) */
        ndomod_contacts_serialize(temp_host->contacts, &dbuf, NDO_DATA_CONTACT);

        /* dump customvars */
        ndomod_customvars_serialize(temp_host->custom_variables, &dbuf);

        ndomod_enddata_serialize(&dbuf);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndomod_free_esc_buffers(es, OBJECTCONFIG_ES_ITEMS);
        ndo_dbuf_free(&dbuf);
    }


    /****** dump hostgroup config ******/
    for (temp_hostgroup = hostgroup_list; temp_hostgroup != NULL; temp_hostgroup = temp_hostgroup->next) {

        es[0] = ndo_escape_buffer(temp_hostgroup->group_name);
        es[1] = ndo_escape_buffer(temp_hostgroup->alias);

        struct ndo_broker_data hostgroup_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_HOSTGROUPNAME, es[0]),
            NDODATA_STRING(NDO_DATA_HOSTGROUPALIAS, es[1]),
        };

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_HOSTGROUPDEFINITION,
            hostgroup_definition, 
            ARRAY_SIZE(hostgroup_definition), 
            FALSE);

        /* dump members for each hostgroup */
        ndomod_hosts_serialize(temp_hostgroup->members, &dbuf,
                NDO_DATA_HOSTGROUPMEMBER);

        ndomod_enddata_serialize(&dbuf);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndomod_free_esc_buffers(es, 2);
        ndo_dbuf_free(&dbuf);
    }


    /****** dump service config ******/
    for (temp_service = service_list; temp_service != NULL; temp_service = temp_service->next) {

        es[0] = ndo_escape_buffer(temp_service->host_name);
        es[1] = ndo_escape_buffer(temp_service->description);
        es[2] = ndo_escape_buffer(temp_service->check_command);
        es[3] = ndo_escape_buffer(temp_service->event_handler);
        es[4] = ndo_escape_buffer(temp_service->notification_period);
        es[5] = ndo_escape_buffer(temp_service->check_period);
        es[6] = ndo_escape_buffer("");
        es[7] = ndo_escape_buffer(temp_service->notes);
        es[8] = ndo_escape_buffer(temp_service->notes_url);
        es[9] = ndo_escape_buffer(temp_service->action_url);
        es[10] = ndo_escape_buffer(temp_service->icon_image);
        es[11] = ndo_escape_buffer(temp_service->icon_image_alt);
        es[12] = ndo_escape_buffer(temp_service->display_name);

        struct ndo_broker_data service_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_HOSTNAME, es[0]),
            NDODATA_STRING(NDO_DATA_DISPLAYNAME, es[12]),
            NDODATA_STRING(NDO_DATA_SERVICEDESCRIPTION, es[1]),
            NDODATA_STRING(NDO_DATA_SERVICECHECKCOMMAND, es[2]),
            NDODATA_STRING(NDO_DATA_SERVICEEVENTHANDLER, es[3]),
            NDODATA_STRING(NDO_DATA_SERVICENOTIFICATIONPERIOD, es[4]),
            NDODATA_STRING(NDO_DATA_SERVICECHECKPERIOD, es[5]),
            NDODATA_STRING(NDO_DATA_SERVICEFAILUREPREDICTIONOPTIONS, es[6]),
            NDODATA_FLOATING_POINT(NDO_DATA_SERVICECHECKINTERVAL, (double)temp_service->check_interval),
            NDODATA_FLOATING_POINT(NDO_DATA_SERVICERETRYINTERVAL, (double)temp_service->retry_interval),
            NDODATA_INTEGER(NDO_DATA_MAXSERVICECHECKATTEMPTS, temp_service->max_attempts),
            NDODATA_FLOATING_POINT(NDO_DATA_FIRSTNOTIFICATIONDELAY, temp_service->first_notification_delay),
            NDODATA_FLOATING_POINT(NDO_DATA_SERVICENOTIFICATIONINTERVAL, (double)temp_service->notification_interval),
            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICEUNKNOWN, flag_isset(temp_service->notification_options, OPT_UNKNOWN)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICEWARNING, flag_isset(temp_service->notification_options, OPT_WARNING)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICECRITICAL, flag_isset(temp_service->notification_options, OPT_CRITICAL)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICERECOVERY, flag_isset(temp_service->notification_options, OPT_RECOVERY)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICEFLAPPING, flag_isset(temp_service->notification_options, OPT_FLAPPING)),
            NDODATA_INTEGER(NDO_DATA_NOTIFYSERVICEDOWNTIME, flag_isset(temp_service->notification_options, OPT_DOWNTIME)),
            NDODATA_INTEGER(NDO_DATA_STALKSERVICEONOK, flag_isset(temp_service->stalking_options, OPT_OK)),
            NDODATA_INTEGER(NDO_DATA_STALKSERVICEONWARNING, flag_isset(temp_service->stalking_options, OPT_WARNING)),
            NDODATA_INTEGER(NDO_DATA_STALKSERVICEONUNKNOWN, flag_isset(temp_service->stalking_options, OPT_UNKNOWN)),
            NDODATA_INTEGER(NDO_DATA_STALKSERVICEONCRITICAL, flag_isset(temp_service->stalking_options, OPT_CRITICAL)),
            NDODATA_INTEGER(NDO_DATA_SERVICEISVOLATILE, temp_service->is_volatile),
            NDODATA_INTEGER(NDO_DATA_SERVICEFLAPDETECTIONENABLED, temp_service->flap_detection_enabled),
            NDODATA_INTEGER(NDO_DATA_FLAPDETECTIONONOK, flag_isset(temp_service->flap_detection_options, OPT_OK)),
            NDODATA_INTEGER(NDO_DATA_FLAPDETECTIONONWARNING, flag_isset(temp_service->flap_detection_options, OPT_WARNING)),
            NDODATA_INTEGER(NDO_DATA_FLAPDETECTIONONUNKNOWN, flag_isset(temp_service->flap_detection_options, OPT_UNKNOWN)),
            NDODATA_INTEGER(NDO_DATA_FLAPDETECTIONONCRITICAL, flag_isset(temp_service->flap_detection_options, OPT_CRITICAL)),
            NDODATA_FLOATING_POINT(NDO_DATA_LOWSERVICEFLAPTHRESHOLD, temp_service->low_flap_threshold),
            NDODATA_FLOATING_POINT(NDO_DATA_HIGHSERVICEFLAPTHRESHOLD, temp_service->high_flap_threshold),
            NDODATA_INTEGER(NDO_DATA_PROCESSSERVICEPERFORMANCEDATA, temp_service->process_performance_data),
            NDODATA_INTEGER(NDO_DATA_SERVICEFRESHNESSCHECKSENABLED, temp_service->check_freshness),
            NDODATA_INTEGER(NDO_DATA_SERVICEFRESHNESSTHRESHOLD, temp_service->freshness_threshold),
            NDODATA_INTEGER(NDO_DATA_PASSIVESERVICECHECKSENABLED, temp_service->accept_passive_checks),
            NDODATA_INTEGER(NDO_DATA_SERVICEEVENTHANDLERENABLED, temp_service->event_handler_enabled),
            NDODATA_INTEGER(NDO_DATA_ACTIVESERVICECHECKSENABLED, temp_service->checks_enabled),
            NDODATA_INTEGER(NDO_DATA_RETAINSERVICESTATUSINFORMATION, temp_service->retain_status_information),
            NDODATA_INTEGER(NDO_DATA_RETAINSERVICENONSTATUSINFORMATION, temp_service->retain_nonstatus_information),
            NDODATA_INTEGER(NDO_DATA_SERVICENOTIFICATIONSENABLED, temp_service->notifications_enabled),
            NDODATA_INTEGER(NDO_DATA_OBSESSOVERSERVICE, temp_service->obsess),
            NDODATA_INTEGER(NDO_DATA_FAILUREPREDICTIONENABLED, 0),
            NDODATA_STRING(NDO_DATA_NOTES, es[7]),
            NDODATA_STRING(NDO_DATA_NOTESURL, es[8]),
            NDODATA_STRING(NDO_DATA_ACTIONURL, es[9]),
            NDODATA_STRING(NDO_DATA_ICONIMAGE, es[10]),
            NDODATA_STRING(NDO_DATA_ICONIMAGEALT, es[11]),
            NDODATA_INTEGER(NDO_DATA_IMPORTANCE, temp_service->hourly_value),
        };

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_SERVICEDEFINITION,
            service_definition, 
            ARRAY_SIZE(service_definition), 
            FALSE);

        /* dump parent services */
        ndomod_services_serialize(temp_service->parents, &dbuf,
                NDO_DATA_PARENTSERVICE);

        /* dump contactgroups */
        ndomod_contactgroups_serialize(temp_service->contact_groups, &dbuf);

        /* dump individual contacts (not supported in Nagios 2.x) */
        ndomod_contacts_serialize(temp_service->contacts, &dbuf,
                NDO_DATA_CONTACT);

        /* dump customvars */
        ndomod_customvars_serialize(temp_service->custom_variables, &dbuf);

        ndomod_enddata_serialize(&dbuf);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndomod_free_esc_buffers(es, 13);
        ndo_dbuf_free(&dbuf);
    }


    /****** dump servicegroup config ******/
    for (temp_servicegroup = servicegroup_list; temp_servicegroup != NULL; temp_servicegroup = temp_servicegroup->next) {

        es[0] = ndo_escape_buffer(temp_servicegroup->group_name);
        es[1] = ndo_escape_buffer(temp_servicegroup->alias);

        struct ndo_broker_data servicegroup_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_SERVICEGROUPNAME, es[0]),
            NDODATA_STRING(NDO_DATA_SERVICEGROUPALIAS, es[1]),
        };

        ndomod_broker_data_serialize(&dbuf, 
            NDO_API_SERVICEGROUPDEFINITION,
            servicegroup_definition, 
            ARRAY_SIZE(servicegroup_definition), 
            FALSE);

        /* dump members for each servicegroup */
        ndomod_services_serialize(temp_servicegroup->members, &dbuf,
                NDO_DATA_SERVICEGROUPMEMBER);

        ndomod_enddata_serialize(&dbuf);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndomod_free_esc_buffers(es, 2);
        ndo_dbuf_free(&dbuf);
    }


    /****** dump host escalation config ******/
    for (x = 0; x < num_objects.hostescalations; x++) {
        temp_hostescalation = hostescalation_ary[ x];
        es[0] = ndo_escape_buffer(temp_hostescalation->host_name);
        es[1] = ndo_escape_buffer(temp_hostescalation->escalation_period);

        struct ndo_broker_data hostescalation_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_HOSTNAME, es[0]),
            NDODATA_STRING(NDO_DATA_ESCALATIONPERIOD, es[1]),
            NDODATA_INTEGER(NDO_DATA_FIRSTNOTIFICATION, temp_hostescalation->first_notification),
            NDODATA_INTEGER(NDO_DATA_LASTNOTIFICATION, temp_hostescalation->last_notification),
            NDODATA_FLOATING_POINT(NDO_DATA_NOTIFICATIONINTERVAL, (double)temp_hostescalation->notification_interval),
            NDODATA_INTEGER(NDO_DATA_ESCALATEONRECOVERY, flag_isset(temp_hostescalation->escalation_options, OPT_RECOVERY)),
            NDODATA_INTEGER(NDO_DATA_ESCALATEONDOWN, flag_isset(temp_hostescalation->escalation_options, OPT_DOWN)),
            NDODATA_INTEGER(NDO_DATA_ESCALATEONUNREACHABLE, flag_isset(temp_hostescalation->escalation_options, OPT_UNREACHABLE)),
        };

        ndomod_broker_data_serialize(&dbuf,
            NDO_API_HOSTESCALATIONDEFINITION,
            hostescalation_definition,
            ARRAY_SIZE(hostescalation_definition), 
            FALSE);

        /* dump contactgroups */
        ndomod_contactgroups_serialize(temp_hostescalation->contact_groups,
                &dbuf);

        /* dump individual contacts (not supported in Nagios 2.x) */
        ndomod_contacts_serialize(temp_hostescalation->contacts, &dbuf,
                NDO_DATA_CONTACT);

        ndomod_enddata_serialize(&dbuf);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndomod_free_esc_buffers(es, 2);
        ndo_dbuf_free(&dbuf);
    }


    /****** dump service escalation config ******/
    for (x = 0; x < num_objects.serviceescalations; x++) {
        temp_serviceescalation = serviceescalation_ary[ x];

        es[0] = ndo_escape_buffer(temp_serviceescalation->host_name);
        es[1] = ndo_escape_buffer(temp_serviceescalation->description);
        es[2] = ndo_escape_buffer(temp_serviceescalation->escalation_period);

        struct ndo_broker_data serviceescalation_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_HOSTNAME, es[0]),
            NDODATA_STRING(NDO_DATA_SERVICEDESCRIPTION, es[1]),
            NDODATA_STRING(NDO_DATA_ESCALATIONPERIOD, es[2]),
            NDODATA_INTEGER(NDO_DATA_FIRSTNOTIFICATION, temp_serviceescalation->first_notification),
            NDODATA_INTEGER(NDO_DATA_LASTNOTIFICATION, temp_serviceescalation->last_notification),
            NDODATA_FLOATING_POINT(NDO_DATA_NOTIFICATIONINTERVAL, (double)temp_serviceescalation->notification_interval),
            NDODATA_INTEGER(NDO_DATA_ESCALATEONRECOVERY, flag_isset(temp_serviceescalation->escalation_options, OPT_RECOVERY)),
            NDODATA_INTEGER(NDO_DATA_ESCALATEONWARNING, flag_isset(temp_serviceescalation->escalation_options, OPT_WARNING)),
            NDODATA_INTEGER(NDO_DATA_ESCALATEONUNKNOWN, flag_isset(temp_serviceescalation->escalation_options, OPT_UNKNOWN)),
            NDODATA_INTEGER(NDO_DATA_ESCALATEONCRITICAL, flag_isset(temp_serviceescalation->escalation_options, OPT_CRITICAL)),
        };

        ndomod_broker_data_serialize(&dbuf,
            NDO_API_SERVICEESCALATIONDEFINITION,
            serviceescalation_definition,
            ARRAY_SIZE(serviceescalation_definition), 
            FALSE);

        /* dump contactgroups */
        ndomod_contactgroups_serialize(temp_serviceescalation->contact_groups,
                &dbuf);

        /* dump individual contacts (not supported in Nagios 2.x) */
        ndomod_contacts_serialize(temp_serviceescalation->contacts, &dbuf,
                NDO_DATA_CONTACT);

        ndomod_enddata_serialize(&dbuf);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndomod_free_esc_buffers(es, 3);
        ndo_dbuf_free(&dbuf);
    }


    /****** dump host dependency config ******/
    for (x = 0; x < num_objects.hostdependencies; x++) {
        temp_hostdependency = hostdependency_ary[ x];

        es[0] = ndo_escape_buffer(temp_hostdependency->host_name);
        es[1] = ndo_escape_buffer(temp_hostdependency->dependent_host_name);
        es[2] = ndo_escape_buffer(temp_hostdependency->dependency_period);

        struct ndo_broker_data hostdependency_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_HOSTNAME, es[0]),
            NDODATA_STRING(NDO_DATA_DEPENDENTHOSTNAME, es[1]),
            NDODATA_INTEGER(NDO_DATA_DEPENDENCYTYPE, temp_hostdependency->dependency_type),
            NDODATA_INTEGER(NDO_DATA_INHERITSPARENT, temp_hostdependency->inherits_parent),
            NDODATA_STRING(NDO_DATA_DEPENDENCYPERIOD, es[2]),
            NDODATA_INTEGER(NDO_DATA_FAILONUP, flag_isset(temp_hostdependency->failure_options, OPT_UP)),
            NDODATA_INTEGER(NDO_DATA_FAILONDOWN, flag_isset(temp_hostdependency->failure_options, OPT_DOWN)),
            NDODATA_INTEGER(NDO_DATA_FAILONUNREACHABLE, flag_isset(temp_hostdependency->failure_options, OPT_UNREACHABLE)),
        };

        ndomod_broker_data_serialize(&dbuf,
            NDO_API_HOSTDEPENDENCYDEFINITION,
            hostdependency_definition,
            ARRAY_SIZE(hostdependency_definition), 
            TRUE);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndomod_free_esc_buffers(es, 3);
        ndo_dbuf_free(&dbuf);
    }


    /****** dump service dependency config ******/
    for (x = 0; x < num_objects.servicedependencies; x++) {
        temp_servicedependency = servicedependency_ary[ x];

        es[0] = ndo_escape_buffer(temp_servicedependency->host_name);
        es[1] = ndo_escape_buffer(temp_servicedependency->service_description);
        es[2] = ndo_escape_buffer(temp_servicedependency->dependent_host_name);
        es[3] = ndo_escape_buffer(temp_servicedependency->dependent_service_description);
        es[4] = ndo_escape_buffer(temp_servicedependency->dependency_period);

        struct ndo_broker_data servicedependency_definition[] = {
            NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, now),
            NDODATA_STRING(NDO_DATA_HOSTNAME, es[0]),
            NDODATA_STRING(NDO_DATA_SERVICEDESCRIPTION, es[1]),
            NDODATA_STRING(NDO_DATA_DEPENDENTHOSTNAME, es[2]),
            NDODATA_STRING(NDO_DATA_DEPENDENTSERVICEDESCRIPTION, es[3]),
            NDODATA_INTEGER(NDO_DATA_DEPENDENCYTYPE, temp_servicedependency->dependency_type),
            NDODATA_INTEGER(NDO_DATA_INHERITSPARENT, temp_servicedependency->inherits_parent),
            NDODATA_STRING(NDO_DATA_DEPENDENCYPERIOD, es[4]),
            NDODATA_INTEGER(NDO_DATA_FAILONOK, flag_isset(temp_servicedependency->failure_options, OPT_OK)),
            NDODATA_INTEGER(NDO_DATA_FAILONWARNING, flag_isset(temp_servicedependency->failure_options, OPT_WARNING)),
            NDODATA_INTEGER(NDO_DATA_FAILONUNKNOWN, flag_isset(temp_servicedependency->failure_options, OPT_UNKNOWN)),
            NDODATA_INTEGER(NDO_DATA_FAILONCRITICAL, flag_isset(temp_servicedependency->failure_options, OPT_CRITICAL)),
        };

        ndomod_broker_data_serialize(&dbuf,
            NDO_API_SERVICEDEPENDENCYDEFINITION, 
            servicedependency_definition,
            ARRAY_SIZE(servicedependency_definition), 
            TRUE);

        ndomod_write_to_sink(dbuf.buf, NDO_TRUE, NDO_TRUE);

        ndomod_free_esc_buffers(es, 5);
        ndo_dbuf_free(&dbuf);
    }

    return NDO_OK;
}



/* Dumps config files to the data sink. */
int ndomod_write_config_files(void)
{
    return ndomod_write_main_config_file();
}


/* dumps main config file data to sink */
int ndomod_write_main_config_file(void)
{
    struct timeval now;
    char fbuf[NDOMOD_MAX_BUFLEN] = { 0 };
    char *temp_buffer            = NULL;
    FILE *fp                     = NULL;
    char *var                    = NULL;
    char *val                    = NULL;

    /* get current time */
    gettimeofday(&now, NULL);

    asprintf(&temp_buffer,
        "\n%d:\n%d = %ld.%06ld\n"
        "%d = %s\n",
        NDO_API_MAINCONFIGFILEVARIABLES,
        NDO_DATA_TIMESTAMP,
        now.tv_sec,
        now.tv_usec,
        NDO_DATA_CONFIGFILENAME,
        config_file);

    ndomod_write_to_sink(temp_buffer, NDO_TRUE, NDO_TRUE);
    free(temp_buffer);
    temp_buffer = NULL;

    /* write each var/val pair from config file */
    fp = fopen(config_file, "r");
    if (fp != NULL) {

        while (fgets(fbuf, sizeof(fbuf), fp)) {

            /* skip blank lines */
            if (fbuf[0] == '\x0' || fbuf[0] == '\n' || fbuf[0] == '\r') {
                continue;
            }

            strip(fbuf);

            /* skip comments */
            if (fbuf[0] == '#' || fbuf[0] == ';') {
                continue;
            }

            var = strtok(fbuf, " = ");
            if (var == NULL) {
                continue;
            }

            val = strtok(NULL, "\n");

            asprintf(&temp_buffer,
                 "%d = %s = %s\n",
                 NDO_DATA_CONFIGFILEVARIABLE,
                 var,
                 (val == NULL) ? "" : val);

            ndomod_write_to_sink(temp_buffer, NDO_TRUE, NDO_TRUE);

            free(temp_buffer);
            temp_buffer = NULL;
        }

        fclose(fp);
    }

    asprintf(&temp_buffer,
        "%d\n\n",
        NDO_API_ENDDATA);

    ndomod_write_to_sink(temp_buffer, NDO_TRUE, NDO_TRUE);
    free(temp_buffer);
    temp_buffer = NULL;

    return NDO_OK;
}

/* dumps runtime variables to sink */
int ndomod_write_runtime_variables(void)
{
    char *temp_buffer = NULL;
    struct timeval now;

    /* get current time */
    gettimeofday(&now, NULL);

    asprintf(&temp_buffer,
        "\n%d:\n%d = %ld.%06ld\n",
        NDO_API_RUNTIMEVARIABLES,
        NDO_DATA_TIMESTAMP,
        now.tv_sec,
        now.tv_usec);

    ndomod_write_to_sink(temp_buffer, NDO_TRUE, NDO_TRUE);
    free(temp_buffer);
    temp_buffer = NULL;

    /* write out main config file name */
    asprintf(&temp_buffer,
        "%d = %s = %s\n",
        NDO_DATA_RUNTIMEVARIABLE,
        "config_file",
        config_file);

    ndomod_write_to_sink(temp_buffer, NDO_TRUE, NDO_TRUE);
    free(temp_buffer);
    temp_buffer = NULL;

    /* write out vars determined after startup */
    asprintf(&temp_buffer,
        "%d = %s = %d\n"
        "%d = %s = %d\n"
        "%d = %s = %d\n"
        "%d = %s = %d\n"
        "%d = %s = %lf\n"
        "%d = %s = %lf\n"
        "%d = %s = %lu\n"
        "%d = %s = %lu\n"
        "%d = %s = %lf\n"
        "%d = %s = %lf\n"
        "%d = %s = %lf\n"
        "%d = %s = %lf\n"
        "%d = %s = %lf\n"
        "%d = %s = %lf\n"
        "%d = %s = %d\n"
        "%d = %s = %d\n"
        "%d = %s = %d\n",
        NDO_DATA_RUNTIMEVARIABLE,
            "total_services",
            scheduling_info.total_services,
        NDO_DATA_RUNTIMEVARIABLE,
            "total_scheduled_services",
            scheduling_info.total_scheduled_services,
        NDO_DATA_RUNTIMEVARIABLE,
            "total_hosts",
            scheduling_info.total_hosts,
        NDO_DATA_RUNTIMEVARIABLE,
            "total_scheduled_hosts",
            scheduling_info.total_scheduled_hosts,
        NDO_DATA_RUNTIMEVARIABLE,
            "average_services_per_host",
            scheduling_info.average_services_per_host,
        NDO_DATA_RUNTIMEVARIABLE,
            "average_scheduled_services_per_host",
            scheduling_info.average_scheduled_services_per_host,
        NDO_DATA_RUNTIMEVARIABLE,
            "service_check_interval_total",
            scheduling_info.service_check_interval_total,
        NDO_DATA_RUNTIMEVARIABLE,
            "host_check_interval_total",
            scheduling_info.host_check_interval_total,
        NDO_DATA_RUNTIMEVARIABLE,
            "average_service_check_interval",
            scheduling_info.average_service_check_interval,
        NDO_DATA_RUNTIMEVARIABLE,
            "average_host_check_interval",
            scheduling_info.average_host_check_interval,
        NDO_DATA_RUNTIMEVARIABLE,
            "average_service_inter_check_delay",
            scheduling_info.average_service_inter_check_delay,
        NDO_DATA_RUNTIMEVARIABLE,
            "average_host_inter_check_delay",
            scheduling_info.average_host_inter_check_delay,
        NDO_DATA_RUNTIMEVARIABLE,
            "service_inter_check_delay",
            scheduling_info.service_inter_check_delay,
        NDO_DATA_RUNTIMEVARIABLE,
            "host_inter_check_delay",
            scheduling_info.host_inter_check_delay,
        NDO_DATA_RUNTIMEVARIABLE,
            "service_interleave_factor",
            scheduling_info.service_interleave_factor,
        NDO_DATA_RUNTIMEVARIABLE,
            "max_service_check_spread",
            scheduling_info.max_service_check_spread,
        NDO_DATA_RUNTIMEVARIABLE,
            "max_host_check_spread",
            scheduling_info.max_host_check_spread);

    ndomod_write_to_sink(temp_buffer, NDO_TRUE, NDO_TRUE);
    free(temp_buffer);
    temp_buffer = NULL;

    asprintf(&temp_buffer,
        "%d\n\n",
        NDO_API_ENDDATA);

    ndomod_write_to_sink(temp_buffer, NDO_TRUE, NDO_TRUE);
    free(temp_buffer);
    temp_buffer = NULL;

    return NDO_OK;
}
