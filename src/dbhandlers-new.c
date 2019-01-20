

int ndomod_get_object_id(int insert, int object_type, char * name1, char * name2)
{
    int result = NDO_ERROR;

    /* no blanks */
    if (name1 != NULL && !strcmp(name1, "")) {
        name1 = NULL;
    }

    if (name2 != NULL && !strcmp(name2, "")) {
        name2 = NULL;
    }

    if (name1 == NULL && name2 == NULL) {
        return NDO_ERROR;
    }

    RESET_QUERY();
    snprintf(ndomod_mysql_query, MAX_SQL_BUFFER,
        "SELECT object_id FROM nagios_objects WHERE objecttype_id = %d "
        "AND name1 = ? AND name2 ",
        object_type);

    RESET_BIND();
    SET_BIND_STR(name1);

    if (name2 == NULL) {
        strcat(ndomod_mysql_query, "IS NULL");
    }
    else {
        strcat(ndomod_mysql_query, "= ?");
        SET_BIND_STR(name2);
    }

    strcat(ndomod_mysql_query, " LIMIT 1");

    BIND();
    QUERY();

    /* todo: this? */
    if (there_is_object) {
        return result;
    }

    if (insert == NDO_FALSE) {
        return NDO_ERROR;
    }

    /* we don't have to reset bind, since they will already be set properly */
    RESET_QUERY();

    if (name2 == NULL) {
        snprintf(ndomod_mysql_query, MAX_SQL_BUFFER
            "INSERT INTO nagios_objects (objecttype_id, name1) VALUES (%d, ?)",
            object_type);
    }
    else {
        snprintf(ndomod_mysql_query, MAX_SQL_BUFFER
            "INSERT INTO nagios_objects (objecttype_id, name1, name2) VALUES (%d, ?, ?)",
            object_type);
    }

    BIND();
    QUERY();

    return mysql_insert_id(ndomod_mysql);
}


void ndomod_clear_tables()
{
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_programstatus);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_hoststatus);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_servicestatus);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_contactstatus);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_timedeventqueue);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_comments);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_scheduleddowntime);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_runtimevariables);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_customvariablestatus);

    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_configfiles);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_configfilevariables);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_customvariables);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_commands);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_timeperiods);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_timeperiodtimeranges);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_contactgroups);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_contactgroupmembers);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_hostgroups);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_servicegroups);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_servicegroupmembers);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_hostescalations);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_hostescalationcontacts);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_serviceescalations);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_serviceescalationcontacts);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_hostdependencies);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_servicedependencies);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_contacts);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_contactaddresses);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_contactnotificationcommands);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_hosts);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_hostparenthosts);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_hostcontacts);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_services);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_serviceparentservices);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_servicecontacts);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_servicecontactgroups);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_hostcontactgroups);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_hostescalationcontactgroups);
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_serviceescalationcontactgroups);
}

void ndomod_set_all_objects_as_inactive()
{
    SET_SQL_AND_QUERY(UPDATE nagios_objects SET is_active = 0);
}


NDOMOD_HANDLER_FUNCTION(process_data)
{
    char * program_name              = strdup("Nagios");
    char * program_version           = strdup(get_program_version());
    char * program_modification_date = strdup(get_program_modification_date());
    int program_pid                  = (int) getpid();

    if (name == NULL || version == NULL || modification_date == NULL) {
        ndomod_write_to_logs("Unable to allocate memory for process data", NSLOG_INFO_MESSAGE);
        return;
    }

    if (data->type == NEBTYPE_PROCESS_START) {
        ndomod_write_active_objects();
    }

    RESET_BIND();

    SET_SQL(
            INSERT INTO
                nagios_processevents
            SET
                instance_id     = 1,
                event_type      = ?
                event_time      = FROM_UNIXTIME(?),
                event_time_usec = ?,
                process_id      = ?
                program_name    = ?
                program_version = ?
                program_date    = ?
            );

    SET_BIND_INT(data->type);
    SET_BIND_INT(data->timestamp.tv_sec);
    SET_BIND_INT(data->timestamp.tv_usec);
    SET_BIND_INT(program_pid);
    SET_BIND_STR(program_name);
    SET_BIND_STR(program_version);
    SET_BIND_STR(program_modification_date);

    BIND();
    QUERY();

    if (data->type == NEBTYPE_PROCESS_PRELAUNCH) {
        ndomod_clear_tables();
        ndomod_set_all_objects_as_inactive();
    }

    if (data->type == NEBTYPE_PROCESS_SHUTDOWN || data->type == NEBTYPE_PROCESS_RESTART) {

        RESET_BIND();

        SET_SQL(
                UPDATE
                    nagios_programstatus
                SET
                    program_end_time     = FROM_UNIXTIME(?),
                    is_currently_running = 0
            );

        SET_BIND_INT(data->timestamp.tv_sec);

        BIND();
        QUERY();
    }
}



NDOMOD_HANDLER_FUNCTION(log_data)
{
    /* this particular function is a bit weird because it starts passing
       logs to the neb module before the database initialization has occured,
       so if the db hasn't been initialized, we just return */
    if (ndomod_mysql == NULL) {
        return;
    }

    RESET_BIND();

    SET_SQL(
            INSERT INTO
                nagios_logentries
            SET
                instance_id             = 1,
                logentry_time           = FROM_UNIXTIME(?),
                entry_time              = FROM_UNIXTIME(?),
                entry_time_usec         = ?,
                logentry_type           = ?,
                logentry_data           = ?,
                realtime_data           = 1,
                inferred_data_extracted = 1
            );

    SET_BIND_INT(data->entry_time);
    SET_BIND_INT(data->timestamp.tv_sec);
    SET_BIND_INT(data->timestamp.tv_usec);
    SET_BIND_INT(data->data_type);
    SET_BIND_STR(data->data);

    BIND();
    QUERY();
}



// 1980
NDOMOD_HANDLER_FUNCTION(system_command_data)
{
    RESET_BIND();

    SET_SQL(
            INSERT INTO
                nagios_systemcommands
            SET
                instance_id     = 1,
                start_time      = FROM_UNIXTIME(?),
                start_time_usec = ?,
                end_time        = FROM_UNIXTIME(?),
                end_time_usec   = ?,
                command_line    = ?,
                timeout         = ?,
                early_timeout   = ?,
                execution_time  = ?,
                return_code     = ?,
                output          = ?,
                long_output     = ?
            ON DUPLICATE KEY UPDATE
                instance_id     = 1,
                start_time      = FROM_UNIXTIME(?),
                start_time_usec = ?,
                end_time        = FROM_UNIXTIME(?),
                end_time_usec   = ?,
                command_line    = ?,
                timeout         = ?,
                early_timeout   = ?,
                execution_time  = ?,
                return_code     = ?,
                output          = ?,
                long_output     = ?
            );

    SET_BIND_INT(data->start_time.tv_sec);
    SET_BIND_INT(data->start_time.tv_usec);
    SET_BIND_INT(data->end_time.tv_sec);
    SET_BIND_INT(data->end_time.tv_usec);
    SET_BIND_STR(data->command_line);
    SET_BIND_INT(data->timeout);
    SET_BIND_INT(data->early_timeout);
    SET_BIND_INT(data->expiration_time);
    SET_BIND_INT(data->return_code);
    SET_BIND_INT(data->output);
    SET_BIND_INT(data->long_output);

    SET_BIND_INT(data->start_time.tv_sec);
    SET_BIND_INT(data->start_time.tv_usec);
    SET_BIND_INT(data->end_time.tv_sec);
    SET_BIND_INT(data->end_time.tv_usec);
    SET_BIND_STR(data->command_line);
    SET_BIND_INT(data->timeout);
    SET_BIND_INT(data->early_timeout);
    SET_BIND_INT(data->expiration_time);
    SET_BIND_INT(data->return_code);
    SET_BIND_INT(data->output);
    SET_BIND_INT(data->long_output);

    BIND();
    QUERY();
}

NDOMOD_HANDLER_FUNCTION(event_handler_data)
{
    int object_id = 0;
    
    CHECK_2SVC_AND_GET_OBJECT_ID(data->eventhandler_type,
                                 SERVICE_EVENTHANDLER, 
                                 GLOBAL_SERVICE_EVENTHANDLER,
                                 object_id, NDO_TRUE,
                                 data->host_name, data->service_description);

    RESET_BIND();

    SET_SQL(
            INSERT INTO
                nagios_eventhandlers
            SET
                instance_id       = 1,
                start_time        = FROM_UNIXTIME(?),
                start_time_usec   = ?,
                end_time          = FROM_UNIXTIME(?),
                end_time_usec     = ?,
                eventhandler_type = ?,
                object_id         = ?,
                state             = ?,
                state_type        = ?,
                command_object_id = (SELECT object_id FROM nagios_objects WHERE name1 = ? and objecttype_id = 12 LIMIT 1),
                command_args      = ?,
                command_line      = ?
                timeout           = ?,
                early_timeout     = ?,
                execution_time    = ?,
                return_code       = ?,
                output            = ?,
                long_output       = ?
            ON DUPLICATE KEY UPDATE
                instance_id       = 1,
                start_time        = FROM_UNIXTIME(?),
                start_time_usec   = ?,
                end_time          = FROM_UNIXTIME(?),
                end_time_usec     = ?,
                eventhandler_type = ?,
                object_id         = ?,
                state             = ?,
                state_type        = ?,
                command_object_id = (SELECT object_id FROM nagios_objects WHERE name1 = ? and objecttype_id = 12 LIMIT 1),
                command_args      = ?,
                command_line      = ?
                timeout           = ?,
                early_timeout     = ?,
                execution_time    = ?,
                return_code       = ?,
                output            = ?,
                long_output       = ?
            );

    SET_BIND_INT(data->start_time.tv_sec);
    SET_BIND_INT(data->start_time.tv_usec);
    SET_BIND_INT(data->end_time.tv_sec);
    SET_BIND_INT(data->end_time.tv_usec);
    SET_BIND_INT(data->eventhandler_type);
    SET_BIND_INT(object_id);
    SET_BIND_INT(data->state);
    SET_BIND_INT(data->state_type);
    SET_BIND_STR(data->command_name);
    SET_BIND_STR(data->command_args);
    SET_BIND_STR(data->command_line);
    SET_BIND_INT(data->timeout);
    SET_BIND_INT(data->early_timeout);
    SET_BIND_INT(data->execution_time);
    SET_BIND_INT(data->return_code);
    SET_BIND_STR(data->output);
    SET_BIND_STR(data->long_output);

    SET_BIND_INT(data->start_time.tv_sec);
    SET_BIND_INT(data->start_time.tv_usec);
    SET_BIND_INT(data->end_time.tv_sec);
    SET_BIND_INT(data->end_time.tv_usec);
    SET_BIND_INT(data->eventhandler_type);
    SET_BIND_INT(object_id);
    SET_BIND_INT(data->state);
    SET_BIND_INT(data->state_type);
    SET_BIND_STR(data->command_name);
    SET_BIND_STR(data->command_args);
    SET_BIND_STR(data->command_line);
    SET_BIND_INT(data->timeout);
    SET_BIND_INT(data->early_timeout);
    SET_BIND_INT(data->execution_time);
    SET_BIND_INT(data->return_code);
    SET_BIND_STR(data->output);
    SET_BIND_STR(data->long_output);

    BIND();
    QUERY();
}

NDOMOD_HANDLER_FUNCTION(notification_data)
{
    int object_id = 0;

    CHECK_AND_GET_OBJECT_ID(data->notification_type, SERVICE_NOTIFICATION,
                            object_id, NDO_TRUE,
                            data->host_name, data->service_description);

    RESET_BIND();

    SET_SQL(
            INSERT INTO
                nagios_notifications
            SET
                instance_id         = 1,
                start_time          = FROM_UNIXTIME(?),
                start_time_usec     = ?,
                end_time            = FROM_UNIXTIME(?),
                end_time_usec       = ?,
                notification_type   = ?,
                notification_reason = ?,
                object_id           = ?,
                state               = ?,
                output              = ?,
                long_output         = ?,
                escalated           = ?,
                contacts_notified   = ?
            ON DUPLICATE KEY UPDATE
                instance_id         = 1,
                start_time          = FROM_UNIXTIME(?),
                start_time_usec     = ?,
                end_time            = FROM_UNIXTIME(?),
                end_time_usec       = ?,
                notification_type   = ?,
                notification_reason = ?,
                object_id           = ?,
                state               = ?,
                output              = ?,
                long_output         = ?,
                escalated           = ?,
                contacts_notified   = ?
        );

    SET_BIND_INT(data->start_time.tv_sec);
    SET_BIND_INT(data->start_time.tv_usec);
    SET_BIND_INT(data->end_time.tv_sec);
    SET_BIND_INT(data->end_time.tv_usec);
    SET_BIND_INT(data->notification_type);
    SET_BIND_INT(data->reason_type);
    SET_BIND_INT(object_id);
    SET_BIND_INT(data->state);
    SET_BIND_STR(data->output);
    SET_BIND_STR(data->long_output);
    SET_BIND_INT(data->escalated);
    SET_BIND_INT(data->contacts_notified);

    SET_BIND_INT(data->start_time.tv_sec);
    SET_BIND_INT(data->start_time.tv_usec);
    SET_BIND_INT(data->end_time.tv_sec);
    SET_BIND_INT(data->end_time.tv_usec);
    SET_BIND_INT(data->notification_type);
    SET_BIND_INT(data->reason_type);
    SET_BIND_INT(object_id);
    SET_BIND_INT(data->state);
    SET_BIND_STR(data->output);
    SET_BIND_STR(data->long_output);
    SET_BIND_INT(data->escalated);
    SET_BIND_INT(data->contacts_notified);

    BIND();
    QUERY();
}

NDOMOD_HANDLER_FUNCTION(service_check_data)
{
    int object_id = 0;

    /* this is really the only data we care about */
    if (type != NEBTYPE_SERVICECHECK_PROCESSED) {
        return;
    }

    RESET_BIND();

    SET_SQL(
            INSERT INTO
                nagios_servicechecks
            SET
                instance_id           = 1,
                start_time            = FROM_UNIXTIME(?),
                start_time_usec       = ?,
                end_time              = FROM_UNIXTIME(?),
                end_time_usec         = ?,
                service_object_id     = ?,
                check_type            = ?,
                current_check_attempt = ?,
                max_check_attempts    = ?,
                state                 = ?,
                state_type            = ?,
                timeout               = ?,
                early_timeout         = ?,
                execution_time        = ?,
                latency               = ?,
                return_code           = ?,
                output                = ?,
                long_output           = ?,
                perfdata              = ?,
                command_object_id     = (SELECT object_id FROM nagios_objects WHERE name1 = ? and objecttype_id = 12 LIMIT 1),
                command_args          = ?,
                command_line          = ?
            ON DUPLICATE KEY UPDATE
                instance_id           = 1,
                start_time            = FROM_UNIXTIME(?),
                start_time_usec       = ?,
                end_time              = FROM_UNIXTIME(?),
                end_time_usec         = ?,
                service_object_id     = ?,
                check_type            = ?,
                current_check_attempt = ?,
                max_check_attempts    = ?,
                state                 = ?,
                state_type            = ?,
                timeout               = ?,
                early_timeout         = ?,
                execution_time        = ?,
                latency               = ?,
                return_code           = ?,
                output                = ?,
                long_output           = ?,
                perfdata              = ?
        );

    object_id = ndomod_get_object_id(NDO_TRUE, NDO2DB_OBJECTTYPE_SERVICE,
                                     data->host_name,
                                     data->service_description);

    SET_BIND_INT(data->start_time.tv_sec);
    SET_BIND_INT(data->start_time.tv_usec);
    SET_BIND_INT(data->end_time.tv_sec);
    SET_BIND_INT(data->end_time.tv_usec);
    SET_BIND_INT(object_id);
    SET_BIND_INT(data->check_type);
    SET_BIND_INT(data->current_attempt);
    SET_BIND_INT(data->max_attempts);
    SET_BIND_INT(data->state);
    SET_BIND_INT(data->state_type);
    SET_BIND_INT(data->timeout);
    SET_BIND_INT(data->early_timeout);
    SET_BIND_DOUBLE(data->execution_time);
    SET_BIND_DOUBLE(data->latency);
    SET_BIND_INT(data->return_code);
    SET_BIND_STR(data->output);
    SET_BIND_STR(data->long_output);
    SET_BIND_STR(data->perfdata);
    SET_BIND_STR(data->command_name);
    SET_BIND_STR(data->command_args);
    SET_BIND_STR(data->command_line);

    SET_BIND_INT(data->start_time.tv_sec);
    SET_BIND_INT(data->start_time.tv_usec);
    SET_BIND_INT(data->end_time.tv_sec);
    SET_BIND_INT(data->end_time.tv_usec);
    SET_BIND_INT(object_id);
    SET_BIND_INT(data->check_type);
    SET_BIND_INT(data->current_attempt);
    SET_BIND_INT(data->max_attempts);
    SET_BIND_INT(data->state);
    SET_BIND_INT(data->state_type);
    SET_BIND_INT(data->timeout);
    SET_BIND_INT(data->early_timeout);
    SET_BIND_DOUBLE(data->execution_time);
    SET_BIND_DOUBLE(data->latency);
    SET_BIND_INT(data->return_code);
    SET_BIND_STR(data->output);
    SET_BIND_STR(data->long_output);
    SET_BIND_STR(data->perfdata);

    BIND();
    QUERY();
}


NDOMOD_HANDLER_FUNCTION(host_check_data)
{
    int object_id = 0;

    /* this is really the only data we care about */
    if (type != NEBTYPE_HOSTCHECK_PROCESSED) {
        return;
    }

    RESET_BIND();

    SET_SQL(
            INSERT INTO
                nagios_hostchecks
            SET
                instance_id           = 1,
                start_time            = FROM_UNIXTIME(?),
                start_time_usec       = ?,
                end_time              = FROM_UNIXTIME(?),
                end_time_usec         = ?,
                host_object_id        = ?,
                check_type            = ?,
                current_check_attempt = ?,
                max_check_attempts    = ?,
                state                 = ?,
                state_type            = ?,
                timeout               = ?,
                early_timeout         = ?,
                execution_time        = ?,
                latency               = ?,
                return_code           = ?,
                output                = ?,
                long_output           = ?,
                perfdata              = ?,
                command_object_id     = (SELECT object_id FROM nagios_objects WHERE name1 = ? and objecttype_id = 12 LIMIT 1),
                command_args          = ?,
                command_line          = ?
            ON DUPLICATE KEY UPDATE
                instance_id           = 1,
                start_time            = FROM_UNIXTIME(?),
                start_time_usec       = ?,
                end_time              = FROM_UNIXTIME(?),
                end_time_usec         = ?,
                host_object_id        = ?,
                check_type            = ?,
                current_check_attempt = ?,
                max_check_attempts    = ?,
                state                 = ?,
                state_type            = ?,
                timeout               = ?,
                early_timeout         = ?,
                execution_time        = ?,
                latency               = ?,
                return_code           = ?,
                output                = ?,
                long_output           = ?,
                perfdata              = ?
        );

    object_id = ndomod_get_object_id(NDO_TRUE, NDO2DB_OBJECTTYPE_HOST,
                                     data->host_name);

    SET_BIND_INT(data->start_time.tv_sec);
    SET_BIND_INT(data->start_time.tv_usec);
    SET_BIND_INT(data->end_time.tv_sec);
    SET_BIND_INT(data->end_time.tv_usec);
    SET_BIND_INT(object_id);
    SET_BIND_INT(data->check_type);
    SET_BIND_INT(data->current_attempt);
    SET_BIND_INT(data->max_attempts);
    SET_BIND_INT(data->state);
    SET_BIND_INT(data->state_type);
    SET_BIND_INT(data->timeout);
    SET_BIND_INT(data->early_timeout);
    SET_BIND_DOUBLE(data->execution_time);
    SET_BIND_DOUBLE(data->latency);
    SET_BIND_INT(data->return_code);
    SET_BIND_STR(data->output);
    SET_BIND_STR(data->long_output);
    SET_BIND_STR(data->perfdata);
    SET_BIND_STR(data->command_name);
    SET_BIND_STR(data->command_args);
    SET_BIND_STR(data->command_line);

    SET_BIND_INT(data->start_time.tv_sec);
    SET_BIND_INT(data->start_time.tv_usec);
    SET_BIND_INT(data->end_time.tv_sec);
    SET_BIND_INT(data->end_time.tv_usec);
    SET_BIND_INT(object_id);
    SET_BIND_INT(data->check_type);
    SET_BIND_INT(data->current_attempt);
    SET_BIND_INT(data->max_attempts);
    SET_BIND_INT(data->state);
    SET_BIND_INT(data->state_type);
    SET_BIND_INT(data->timeout);
    SET_BIND_INT(data->early_timeout);
    SET_BIND_DOUBLE(data->execution_time);
    SET_BIND_DOUBLE(data->latency);
    SET_BIND_INT(data->return_code);
    SET_BIND_STR(data->output);
    SET_BIND_STR(data->long_output);
    SET_BIND_STR(data->perfdata);

    BIND();
    QUERY();
}

NDOMOD_HANDLER_FUNCTION(comment_data)
{
    int object_id = 0;
    int i         = 0;

    CHECK_AND_GET_OBJECT_ID(data->comment_type, SERVICE_COMMENT,
                            object_id, NDO_TRUE,
                            data->host_name, data->service_description);

    switch (data->type) {

    case NEBTYPE_COMMENT_ADD:
    case NEBTYPE_COMMENT_LOAD:

        RESET_BIND();

        /* IMPORTANT: if you change this query, you likely will
                      have to change the magic numbers in the hack section
                      below */
        SET_SQL(
                INSERT INTO
                    nagios_commenthistory
                SET
                    instance_id         = 1,
                    comment_type        = ?,
                    entry_type          = ?,
                    object_id           = ?,
                    comment_time        = FROM_UNIXTIME(?),
                    internal_comment_id = ?,
                    author_name         = ?,
                    comment_data        = ?,
                    is_persistent       = ?,
                    comment_source      = ?,
                    expires             = ?,
                    expiration_time     = FROM_UNIXTIME(?),
                    entry_time          = FROM_UNIXTIME(?),
                    entry_time_usec     = ?
                ON DUPLICATE KEY UPDATE
                    instance_id         = 1,
                    comment_type        = ?,
                    entry_type          = ?,
                    object_id           = ?,
                    comment_time        = FROM_UNIXTIME(?),
                    internal_comment_id = ?,
                    author_name         = ?,
                    comment_data        = ?,
                    is_persistent       = ?,
                    comment_source      = ?,
                    expires             = ?,
                    expiration_time     = FROM_UNIXTIME(?)
            );

        SET_BIND_INT(data->comment_type);
        SET_BIND_INT(data->entry_type);
        SET_BIND_INT(object_id);
        SET_BIND_INT(data->comment_time);
        SET_BIND_INT(data->comment_id);
        SET_BIND_STR(data->author_name);
        SET_BIND_STR(data->comment_data);
        SET_BIND_INT(data->persistent);
        SET_BIND_INT(data->source);
        SET_BIND_INT(data->expires);
        SET_BIND_INT(data->expire_time);
        SET_BIND_INT(data->timestamp.tv_sec);
        SET_BIND_INT(data->timestamp.tv_usec);

        SET_BIND_INT(data->comment_type);
        SET_BIND_INT(data->entry_type);
        SET_BIND_INT(object_id);
        SET_BIND_INT(data->comment_time);
        SET_BIND_INT(data->comment_id);
        SET_BIND_STR(data->author_name);
        SET_BIND_STR(data->comment_data);
        SET_BIND_INT(data->persistent);
        SET_BIND_INT(data->source);
        SET_BIND_INT(data->expires);
        SET_BIND_INT(data->expire_time);

        BIND();
        QUERY();

        /* here we need to do all this again, but instead of for
           comment history, we need it in the comment table. really the 
           *ONLY* thing we need to do is remove the word "history" from the 
           table name (and replace it with an "s"),
           so what follows is a quick hack that does that

           "history" is characters 26-32 once this string passes the
           pre-processor

           note: there are more elegant and "future-proof" ways of solving
                 this, but since we're looking for performance here, don't
                 touch it :)
        */
        ndomod_mysql_query[26] = 's';
        for (i = 27; i <= 32; i++) {
            ndomod_mysql_query[i] = ' ';
        }

        PREPARE_SQL();
        BIND();
        QUERY();

        break;

    case NEBTYPE_COMMENT_DELETE:

        RESET_BIND();

        SET_SQL(
                UPDATE
                    nagios_commenthistory
                SET
                    deletion_time       = FROM_UNIXTIME(?),
                    deletion_time_usec  = ?
                WHERE
                    comment_time        = FROM_UNIXTIME(?)
                AND
                    internal_comment_id = ?
                );

        SET_BIND_INT(data->timestamp.tv_sec);
        SET_BIND_INT(data->timestamp.tv_usec);
        SET_BIND_INT(data->comment_time);
        SET_BIND_INT(data->comment_id);

        BIND();
        QUERY();

        RESET_BIND();

        SET_SQL(
                DELETE FROM
                    nagios_comments
                WHERE
                    comment_time        = FROM_UNIXTIME(?)
                AND
                    internal_comment_id = ?
                );

        SET_BIND_INT(data->comment_time);
        SET_BIND_INT(data->comment_id);

        BIND();
        QUERY();

        break;

    default:
        break;
    }
}

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

#define _ELSE_CHECK_GET_OBJECT_ID(_insert, _var, _hst) \
_var = ndomod_get_object_id(_insert, NDO2DB_OBJECTTYPE_HOST, _hst)

NDOMOD_HANDLER_FUNCTION(downtime_data)
{
    int object_id = 0;
    int i         = 0;
    int cancelled = 0;

    CHECK_AND_GET_OBJECT_ID(data->downtime_type, SERVICE_DOWNTIME,
                            object_id, NDO_TRUE,
                            data->host_name, data->service_description);

    switch (data->type) {


    case NEBTYPE_DOWNTIME_ADD:
    case NEBTYPE_DOWNTIME_LOAD:

        RESET_BIND();

        /* IMPORTANT: if you change this query, you MUST change the hacky
                      section below (much like in comments handler) */
        SET_SQL(
                INSERT INTO
                    nagios_scheduleddowntime
                SET
                    instance_id          = 1,
                    downtime_type        = ?,
                    object_id            = ?,
                    entry_time           = FROM_UNIXTIME(?),
                    author_name          = ?,
                    comment_data         = ?,
                    internal_downtime_id = ?,
                    triggered_by_id      = ?,
                    is_fixed             = ?,
                    duration             = ?,
                    scheduled_start_time = ?,
                    scheduled_end_time   = ?
                ON DUPLICATE KEY UPDATE
                    instance_id          = 1,
                    downtime_type        = ?,
                    object_id            = ?,
                    entry_time           = FROM_UNIXTIME(?),
                    author_name          = ?,
                    comment_data         = ?,
                    internal_downtime_id = ?,
                    triggered_by_id      = ?,
                    is_fixed             = ?,
                    duration             = ?,
                    scheduled_start_time = ?,
                    scheduled_end_time   = ?
                );

        SET_BIND_INT(data->downtime_type);
        SET_BIND_INT(object_id);
        SET_BIND_INT(data->entry_time);
        SET_BIND_STR(data->author_name);
        SET_BIND_STR(data->comment_data);
        SET_BIND_INT(data->downtime_id);
        SET_BIND_INT(data->triggered_by);
        SET_BIND_INT(data->fixed);
        SET_BIND_INT(data->duration);
        SET_BIND_INT(data->start_time);
        SET_BIND_INT(data->end_time);

        SET_BIND_INT(data->downtime_type);
        SET_BIND_INT(object_id);
        SET_BIND_INT(data->entry_time);
        SET_BIND_STR(data->author_name);
        SET_BIND_STR(data->comment_data);
        SET_BIND_INT(data->downtime_id);
        SET_BIND_INT(data->triggered_by);
        SET_BIND_INT(data->fixed);
        SET_BIND_INT(data->duration);
        SET_BIND_INT(data->start_time);
        SET_BIND_INT(data->end_time);

        BIND();
        QUERY();

        /* we must do the same thing we did in the comment handler
           we just have to rename "scheduleddowntime" to "downtimehistory"

           "scheduleddowntime" is characters 19-35 once this string passes the
           pre-processor

           note: much like the comment handler, yes this is a hack, and yes
                 we're more concerned with performance. don't touch it unless
                 you know what you're doing :)
        */
        strcpy(ndomod_mysql_query + 19, "downtimehistory");
        ndomod_mysql_query[34] = ' ';
        ndomod_mysql_query[35] = ' ';

        PREPARE_SQL();
        BIND();
        QUERY();

        break;


    case NEBTYPE_DOWNTIME_START:

        RESET_BIND();

        /* IMPORTANT: if you change this query, you MUST change the hacky
                      section below (much like in comments handler) */
        SET_SQL(
                UPDATE
                    nagios_scheduleddowntime
                SET
                    actual_start_time      = ?,
                    actual_start_time_usec = ?,
                    was_started            = 1
                WHERE
                    object_id              = ?
                AND
                    downtime_type          = ?
                AND
                    entry_time             = FROM_UNIXTIME(?)
                AND
                    scheduled_start_time   = ?
                AND
                    scheduled_end_time     = ?
                );

        SET_BIND_INT(data->timestamp.tv_sec);
        SET_BIND_INT(data->timestamp.tv_usec);
        SET_BIND_INT(object_id);
        SET_BIND_INT(data->downtime_type);
        SET_BIND_INT(data->entry_time);
        SET_BIND_INT(data->start_time);
        SET_BIND_INT(data->end_time);

        BIND();
        QUERY();

        /* we must do the same thing we did in the last switch case
           we just have to rename "scheduleddowntime" to "downtimehistory"

           "scheduleddowntime" is characters 14-30 once this string passes the
           pre-processor

           note: much like the comment handler, yes this is a hack, and yes
                 we're more concerned with performance. don't touch it unless
                 you know what you're doing :)
        */
        strcpy(ndomod_mysql_query + 14, "downtimehistory");
        ndomod_mysql_query[29] = ' ';
        ndomod_mysql_query[30] = ' ';

        PREPARE_SQL();
        BIND();
        QUERY();

        break;


    case NEBTYPE_DOWNTIME_STOP:

        if (data->attr == NEBATTR_DOWNTIME_STOP_CANCELLED) {
            cancelled = 1;
        }

        RESET_BIND();

        SET_SQL(
                UPDATE
                    nagios_downtimehistory
                SET
                    actual_end_time      = ?,
                    actual_end_time_usec = ?,
                    was_cancelled      = ?
                WHERE
                    object_id              = ?
                AND
                    downtime_type          = ?
                AND
                    entry_time             = FROM_UNIXTIME(?)
                AND
                    scheduled_start_time   = ?
                AND
                    scheduled_end_time     = ?
                );

        SET_BIND_INT(data->timestamp.tv_sec);
        SET_BIND_INT(data->timestamp.tv_usec);
        SET_BIND_INT(cancelled);
        SET_BIND_INT(object_id);
        SET_BIND_INT(data->downtime_type);
        SET_BIND_INT(data->entry_time);
        SET_BIND_INT(data->start_time);
        SET_BIND_INT(data->end_time);

        BIND();
        QUERY();

        /* no break */ 


    case NEBTYPE_DOWNTIME_DELETE:

        RESET_BIND();

        SET_SQL(
                DELETE FROM
                    nagios_scheduleddowntime
                WHERE
                    downtime_type        = ?
                AND
                    object_id            = ?
                AND
                    entry_time           = FROM_UNIXTIME(?)
                AND
                    scheduled_start_time = FROM_UNIXTIME(?)
                AND
                    scheduled_end_time   = FROM_UNIXTIME(?)
                );

        SET_BIND_INT(data->downtime_type);
        SET_BIND_INT(object_id);
        SET_BIND_INT(data->entry_time);
        SET_BIND_INT(data->start_time);
        SET_BIND_INT(data->end_time);

        BIND();
        QUERY();

        break;

    default:
        break;
    }
}
