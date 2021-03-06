/**
 * @file ndo2db.h Nagios Data Output to Database Daemon declarations
 */
/*
 * Copyright 2009-2014 Nagios Core Development Team and Community Contributors
 * Copyright 2005-2009 Ethan Galstad
 *
 * This file is part of NDOUtils.
 *
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

#ifndef NDO2DB_NDO2DB_H_INCLUDED
#define NDO2DB_NDO2DB_H_INCLUDED

#include "config.h"
#include "utils.h"


/*************** mbuf definitions *************/
#define NDO2DB_MBUF_CONTACTGROUP                        0
#define NDO2DB_MBUF_CONTACTGROUPMEMBER                  1
#define NDO2DB_MBUF_SERVICEGROUPMEMBER                  2
#define NDO2DB_MBUF_HOSTGROUPMEMBER                     3
#define NDO2DB_MBUF_SERVICENOTIFICATIONCOMMAND          4
#define NDO2DB_MBUF_HOSTNOTIFICATIONCOMMAND             5
#define NDO2DB_MBUF_CONTACTADDRESS                      6
#define NDO2DB_MBUF_TIMERANGE                           7
#define NDO2DB_MBUF_PARENTHOST                          8
#define NDO2DB_MBUF_CONFIGFILEVARIABLE                  9
#define NDO2DB_MBUF_CONFIGVARIABLE                      10
#define NDO2DB_MBUF_RUNTIMEVARIABLE                     11
#define NDO2DB_MBUF_CUSTOMVARIABLE                      12
#define NDO2DB_MBUF_CONTACT                             13
#define NDO2DB_MBUF_PARENTSERVICE                       14

#define NDO2DB_MAX_MBUF_ITEMS                           15


/***************** structures *****************/

typedef struct ndo2db_mbuf_struct{
	int used_lines;
	int allocated_lines;
	char **buffer;
        }ndo2db_mbuf;


typedef struct ndo2db_dbobject_struct{
	char *name1;
	char *name2;
	int object_type;
	unsigned long object_id;
	struct ndo2db_dbobject_struct *nexthash;
        }ndo2db_dbobject;


typedef struct ndo2db_dbconninfo_struct{
	int server_type;
	int connected;
	int error;
#ifdef USE_MYSQL
	MYSQL mysql_conn;
	MYSQL_RES *mysql_result;
	MYSQL_ROW mysql_row;
#endif
	unsigned long instance_id;
	unsigned long conninfo_id;
	time_t latest_program_status_time;
	time_t latest_host_status_time;
	time_t latest_service_status_time;
	time_t latest_contact_status_time;
	time_t latest_queued_event_time;
	time_t latest_realtime_data_time;
	time_t latest_comment_time;
	int clean_event_queue;
	unsigned long last_notification_id;
	unsigned long last_contact_notification_id;
	unsigned long max_timedevents_age;
	unsigned long max_systemcommands_age;
	unsigned long max_servicechecks_age;
	unsigned long max_hostchecks_age;
	unsigned long max_eventhandlers_age;
	unsigned long max_externalcommands_age;
	unsigned long max_notifications_age;
	unsigned long max_contactnotifications_age;
	unsigned long max_contactnotificationmethods_age;
	unsigned long max_logentries_age;
	unsigned long max_acknowledgements_age;
	time_t last_table_trim_time;
	time_t last_logentry_time;
	char *last_logentry_data;
	ndo2db_dbobject **object_hashlist;
        }ndo2db_dbconninfo;


typedef struct ndo2db_input_data_info_struct{
	int protocol_version;
	int disconnect_client;
	int ignore_client_data;
	char *instance_name;
	char *agent_name;
	char *agent_version;
	char *disposition;
	char *connect_source;
	char *connect_type;
	int current_input_section;
	int current_input_data;
	unsigned long bytes_processed;
	unsigned long lines_processed;
	unsigned long entries_processed;
	unsigned long data_start_time;
	unsigned long data_end_time;
	int current_object_config_type;
	char **buffered_input;
	ndo2db_mbuf mbuf[NDO2DB_MAX_MBUF_ITEMS];
	ndo2db_dbconninfo dbinfo;
        }ndo2db_idi;



/*************** DB server types ***************/
#define NDO2DB_DBSERVER_NONE                            0
#define NDO2DB_DBSERVER_MYSQL                           1


/*************** misc definitions **************/
#define NDO2DB_INPUT_BUFFER                             1024
#define NDO2DB_OBJECT_HASHSLOTS                         1024


/*********** types of input sections ***********/
#define NDO2DB_INPUT_SECTION_NONE                       0
#define NDO2DB_INPUT_SECTION_HEADER                     1
#define NDO2DB_INPUT_SECTION_FOOTER                     2
#define NDO2DB_INPUT_SECTION_DATA                       3


/************* types of input data *************/
#define NDO2DB_INPUT_DATA_NONE                          0

#define NDO2DB_INPUT_DATA_CONFIGDUMPSTART               1
#define NDO2DB_INPUT_DATA_CONFIGDUMPEND                 2

#define NDO2DB_INPUT_DATA_LOGENTRY                      10

#define NDO2DB_INPUT_DATA_PROCESSDATA                   20
#define NDO2DB_INPUT_DATA_TIMEDEVENTDATA                21
#define NDO2DB_INPUT_DATA_LOGDATA                       22
#define NDO2DB_INPUT_DATA_SYSTEMCOMMANDDATA             23
#define NDO2DB_INPUT_DATA_EVENTHANDLERDATA              24
#define NDO2DB_INPUT_DATA_NOTIFICATIONDATA              25
#define NDO2DB_INPUT_DATA_SERVICECHECKDATA              26
#define NDO2DB_INPUT_DATA_HOSTCHECKDATA                 27
#define NDO2DB_INPUT_DATA_COMMENTDATA                   28
#define NDO2DB_INPUT_DATA_DOWNTIMEDATA                  29
#define NDO2DB_INPUT_DATA_FLAPPINGDATA                  30
#define NDO2DB_INPUT_DATA_PROGRAMSTATUSDATA             31
#define NDO2DB_INPUT_DATA_HOSTSTATUSDATA                32
#define NDO2DB_INPUT_DATA_SERVICESTATUSDATA             33
#define NDO2DB_INPUT_DATA_ADAPTIVEPROGRAMDATA           34
#define NDO2DB_INPUT_DATA_ADAPTIVEHOSTDATA              35
#define NDO2DB_INPUT_DATA_ADAPTIVESERVICEDATA           36
#define NDO2DB_INPUT_DATA_EXTERNALCOMMANDDATA           37
#define NDO2DB_INPUT_DATA_AGGREGATEDSTATUSDATA          38
#define NDO2DB_INPUT_DATA_RETENTIONDATA                 39
#define NDO2DB_INPUT_DATA_CONTACTNOTIFICATIONDATA       40
#define NDO2DB_INPUT_DATA_CONTACTNOTIFICATIONMETHODDATA 41
#define NDO2DB_INPUT_DATA_ACKNOWLEDGEMENTDATA           42
#define NDO2DB_INPUT_DATA_STATECHANGEDATA               43
#define NDO2DB_INPUT_DATA_CONTACTSTATUSDATA             44
#define NDO2DB_INPUT_DATA_ADAPTIVECONTACTDATA           45

#define NDO2DB_INPUT_DATA_MAINCONFIGFILEVARIABLES       50
#define NDO2DB_INPUT_DATA_RESOURCECONFIGFILEVARIABLES   51
#define NDO2DB_INPUT_DATA_CONFIGVARIABLES               52
#define NDO2DB_INPUT_DATA_RUNTIMEVARIABLES              53

#define NDO2DB_INPUT_DATA_HOSTDEFINITION                61
#define NDO2DB_INPUT_DATA_HOSTGROUPDEFINITION           62
#define NDO2DB_INPUT_DATA_SERVICEDEFINITION             63
#define NDO2DB_INPUT_DATA_SERVICEGROUPDEFINITION        64
#define NDO2DB_INPUT_DATA_HOSTDEPENDENCYDEFINITION      65
#define NDO2DB_INPUT_DATA_SERVICEDEPENDENCYDEFINITION   66
#define NDO2DB_INPUT_DATA_HOSTESCALATIONDEFINITION      67
#define NDO2DB_INPUT_DATA_SERVICEESCALATIONDEFINITION   68
#define NDO2DB_INPUT_DATA_COMMANDDEFINITION             69
#define NDO2DB_INPUT_DATA_TIMEPERIODDEFINITION          70
#define NDO2DB_INPUT_DATA_CONTACTDEFINITION             71
#define NDO2DB_INPUT_DATA_CONTACTGROUPDEFINITION        72
#define NDO2DB_INPUT_DATA_HOSTEXTINFODEFINITION         73
#define NDO2DB_INPUT_DATA_SERVICEEXTINFODEFINITION      74
#define NDO2DB_INPUT_DATA_ACTIVEOBJECTSLIST             75


/************* types of config data *************/
#define NDO2DB_CONFIGTYPE_ORIGINAL                      0
#define NDO2DB_CONFIGTYPE_RETAINED                      1



/***************** debugging levels *******************/

#define NDO2DB_DEBUGL_ALL                      -1
#define NDO2DB_DEBUGL_NONE                     0
#define NDO2DB_DEBUGL_PROCESSINFO              1
#define NDO2DB_DEBUGL_SQL                      2

#define NDO2DB_DEBUGV_BASIC                    0
#define NDO2DB_DEBUGV_MORE		       1
#define NDO2DB_DEBUGV_MOST                     2




/***************** functions *******************/

int ndo2db_process_arguments(int,char **);

int ndo2db_process_config_var(char *);
int ndo2db_process_config_file(char *);

int ndo2db_initialize_variables(void);

int ndo2db_check_init_reqs(void);

int ndo2db_drop_privileges(char *,char *);
int ndo2db_daemonize(void);
int ndo2db_cleanup_socket(void);
void ndo2db_parent_sighandler(int);
void ndo2db_child_sighandler(int);

int ndo2db_free_program_memory(void);
int ndo2db_free_input_memory(ndo2db_idi *);
int ndo2db_free_connection_memory(ndo2db_idi *);

int ndo2db_wait_for_connections(void);
int ndo2db_handle_client_connection(int);
int ndo2db_idi_init(ndo2db_idi *);
int ndo2db_check_for_client_input(ndo2db_idi *,ndo_dbuf *);
int ndo2db_handle_client_input(ndo2db_idi *,char *);

int ndo2db_start_input_data(ndo2db_idi *);
int ndo2db_end_input_data(ndo2db_idi *);
int ndo2db_add_input_data_item(ndo2db_idi *,int,char *);
int ndo2db_add_input_data_mbuf(ndo2db_idi *,int,int,char *);

int ndo2db_convert_standard_data_elements(ndo2db_idi *,int *,int *,int *,struct timeval *);
int ndo2db_convert_string_to_int(char *,int *);
int ndo2db_convert_string_to_float(char *,float *);
int ndo2db_convert_string_to_double(char *,double *);
int ndo2db_convert_string_to_long(char *,long *);
int ndo2db_convert_string_to_unsignedlong(char *,unsigned long *);
int ndo2db_convert_string_to_timeval(char *,struct timeval *);

int ndo2db_open_debug_log(void);
int ndo2db_close_debug_log(void);

void ndo2db_async_client_handle();
#endif
