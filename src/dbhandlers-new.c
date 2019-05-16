

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
        printf("return error, bro\n");
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

    PREPARE_SQL();

    BIND();
    SET_RESULT_INT(result);
    RESULT_BIND();

    QUERY();
    STORE();

    if (NUM_ROWS_INT == 1) {
        while (FETCH()) {
            return result;
        }
    }

    /* if we shouldn't try and insert, we need to return an error now */
    if (insert == NDO_FALSE) {
        return NDO_ERROR;
    }

    /* we don't have to reset bind, since they will already be set properly */
    RESET_QUERY();

    if (name2 == NULL) {
        snprintf(ndomod_mysql_query, MAX_SQL_BUFFER,
            "INSERT INTO nagios_objects (objecttype_id, name1) VALUES (%d, ?)",
            object_type);
    }
    else {
        snprintf(ndomod_mysql_query, MAX_SQL_BUFFER,
            "INSERT INTO nagios_objects (objecttype_id, name1, name2) VALUES (%d, ?, ?)",
            object_type);
    }

    PREPARE_SQL();
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
    SET_SQL_AND_QUERY(TRUNCATE TABLE nagios_contactgroup_members);
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



void ndomod_write_config_file()
{
    char line_buffer[NDO_MAX_BUFLEN] = { 0 };
    FILE * fp = NULL;
    char * key = NULL;
    char * value = NULL;
    int free_value = NDO_FALSE;

    fp = fopen(config_file, "r");

    /* @todo: this function should *really* return a value of some kind... */
    if (fp == NULL) {
        return;
    }

    RESET_BIND();

    /* configfile_type = 0 is a relic - not sure it is required
       to be honest, this function should do more than it does. */

    SET_SQL(
            INSERT INTO
                nagios_configfiles
            SET
                instance_id     = 1,
                configfile_type = 0,
                configfile_path = ?
            ON DUPLICATE KEY UPDATE
                instance_id     = 1,
                configfile_type = 0,
                configfile_path = ?
            );

    SET_BIND_STR(config_file);
    SET_BIND_STR(config_file);

    BIND();
    QUERY();

    snprintf(ndomod_mysql_query, MAX_SQL_BUFFER,
        "INSERT INTO nagios_configfilevariables"
        " (instance_id, configfile_id, varname, varvalue) "
        "VALUES (1, %d, ?, ?)",
        config_file_id);

    PREPARE_SQL();

    while (fgets(line_buffer, sizeof(NDO_MAX_BUFLEN), fp) != NULL) {

        switch (line_buffer[0]) {

        /* skip null */
        case 0:

        /* skip newlines */
        case '\n':
        case '\r':

        /* skip comments */
        /* original ndomod stripped the text before checking for these
           but that is just as bad as checking them as the first char, since
           they can occur at any point in a string... not only that it didn't
           work properly */
        case '#':
        case ';':
            continue;
        }

        key = strtok(line_buffer, "=");
        if (key == NULL) {
            continue;
        }

        value = strtok(NULL, "\n");
        if (value == NULL) {
            value = strdup("");
            free_value = NDO_TRUE;
        }

        /* @todo - we could optimize this to only do the steps necessary
                   instead of all the other mysql stuff going on */
        SET_BIND_STR(key);
        SET_BIND_STR(value);

        BIND();
        QUERY();

        /* skip a reset_bind in favor of not memsetting() bind vars */
        ndomod_mysql_i = 0;
    }
}


NDOMOD_HANDLER_FUNCTION(process_data)
{
    char * program_name              = "Nagios";
    char * program_version           = get_program_version();
    char * program_modification_date = get_program_modification_date();
    int program_pid                  = (int) getpid();


    if (data->type == NEBTYPE_PROCESS_PRELAUNCH) {
        ndomod_clear_tables();
        ndomod_set_all_objects_as_inactive();
    }

    else if (data->type == NEBTYPE_PROCESS_START) {
        ndomod_write_active_objects();
        ndomod_write_config_file();
        ndomod_write_object_config(NDOMOD_CONFIG_DUMP_ORIGINAL);
    }

    else if (data->type == NEBTYPE_PROCESS_EVENTLOOPSTART) {
        ndomod_write_runtime_variables();
    }

    else if (data->type == NEBTYPE_PROCESS_SHUTDOWN || data->type == NEBTYPE_PROCESS_RESTART) {

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
                program_name    = 'Nagios'
                program_version = ?
                program_date    = ?
            );

    SET_BIND_INT(data->type);
    SET_BIND_INT(data->timestamp.tv_sec);
    SET_BIND_INT(data->timestamp.tv_usec);
    SET_BIND_INT(program_pid);
    SET_BIND_STR(program_version);
    SET_BIND_STR(program_modification_date);

    BIND();
    QUERY();
}


/* @todo move this where it belongs */
#define MAX_ROW_BUFFER_ACTIVE 11


NDOMOD_HANDLER_FUNCTION(retention_data)
{
    if (data->type == NEBTYPE_RETENTIONDATA_ENDLOAD) {
        ndomod_write_object_config(NDOMOD_CONFIG_DUMP_RETAINED);
    }
}

void ndomod_write_active_objects()
{
    ndomod_write_objects(NDOMOD_OBJECT_ACTIVE, NDOMOD_CONFIG_DUMP_ALL);
}

void ndomod_write_object_config(int config_type)
{
    ndomod_write_objects(NDOMOD_OBJECT_CONFIG, config_type);
}


#define SET_SQL_ACTIVE_OBJECT_NAME1(obj_type_id) \
    snprintf(ndomod_mysql_query, MAX_SQL_BUFFER - 1, \
                     "INSERT INTO nagios_objects" \
                     "(objecttype_id,name1)VALUES(%d,?)", \
                     obj_type_id); \
    /* length of ndomod_mysql_query now */ \
    ndomod_mysql_query[59] = 0; \
    PREPARE_SQL(); \


#define INSERT_ACTIVE_OBJECT_NAME1(name1) \
        ndomod_mysql_i = 0; \
        SET_BIND_STR(name1); \
        BIND(); \
        QUERY();


#define SET_SQL_ACTIVE_OBJECT_NAME2(obj_type_id) \
    snprintf(ndomod_mysql_query, MAX_SQL_BUFFER - 1, \
                     "INSERT INTO nagios_objects" \
                     "(objecttype_id,name1,name2)VALUES(%d,?,?)", \
                     obj_type_id); \
    /* length of ndomod_mysql_query now */ \
    ndomod_mysql_query[67] = 0; \
    PREPARE_SQL(); \


#define INSERT_ACTIVE_OBJECT_NAME2(name1, name2) \
        ndomod_mysql_i = 0; \
        SET_BIND_STR(name1); \
        SET_BIND_STR(name2); \
        BIND(); \
        QUERY();


#define WRITE_ACTIVE_OBJECT_TYPE(type) \
    type * tmp = type##_list;


#define WRITE_ACTIVE_OBJECT_START_LOOP() \
\
    while (tmp != NULL) {


#define WRITE_ACTIVE_OBJECT_END_LOOP() \
\
        tmp = tmp->next; \
    }


#define WRITE_ACTIVE_OBJECT_NAME1(type, obj_type_id, name1) \
do { \
    WRITE_ACTIVE_OBJECT_TYPE(type) \
    SET_SQL_ACTIVE_OBJECT_NAME1(object_type) \
    WRITE_ACTIVE_OBJECT_START_LOOP() \
        INSERT_ACTIVE_OBJECT_NAME1(tmp->name1) \
    WRITE_ACTIVE_OBJECT_END_LOOP() \
} while (0)


#define WRITE_ACTIVE_OBJECT_NAME2(type, obj_type_id, name1, name2) \
do { \
    WRITE_ACTIVE_OBJECT_TYPE(type) \
    SET_SQL_ACTIVE_OBJECT_NAME1(object_type) \
    WRITE_ACTIVE_OBJECT_START_LOOP() \
        INSERT_ACTIVE_OBJECT_NAME2(tmp->name1, tmp->name2) \
    WRITE_ACTIVE_OBJECT_END_LOOP() \
} while (0)


void ndomod_write_objects(int write_type, int config_type)
{
    int i = 0;
    int object_id = 0;

    /* screw the optimization for now.
       i'll come back to it

       see: https://github.com/NagiosEnterprises/ndoutils/commit/87aa66e
    */

    if (write_type == NDOMOD_OBJECT_ACTIVE) {

        WRITE_ACTIVE_OBJECT_NAME1(command, object_type, name);
        WRITE_ACTIVE_OBJECT_NAME1(timeperiod, object_type, name);
        WRITE_ACTIVE_OBJECT_NAME1(contact, object_type, name);
        WRITE_ACTIVE_OBJECT_NAME1(contactgroup, object_type, group_name);
        WRITE_ACTIVE_OBJECT_NAME1(host, object_type, name);
        WRITE_ACTIVE_OBJECT_NAME1(hostgroup, object_type, group_name);
        WRITE_ACTIVE_OBJECT_NAME2(service, object_type, host_name, service_description);
        WRITE_ACTIVE_OBJECT_NAME1(servicegroup, object_type, group_name);
    }

    else if (write_type != NDOMOD_OBJECT_CONFIG) {
        return;
    }

    /* everything for the rest of the function is NDOMOD_OBJECT_CONFIG */

    if (!(ndomod_config_output_options & config_type)) {
        return;
    }

    /********************************************************************
        commands
    ********************************************************************/
    {
        command * tmp = command_list;
        object_id = 0;

        RESET_BIND();
        SET_SQL(INSERT INTO 
                    nagios_commands
                SET
                    instance_id  = 1,
                    object_id    = ?,
                    config_type  = ?,
                    command_line = ?
                ON DUPLICATE KEY UPDATE
                    instance_id  = 1,
                    object_id    = ?,
                    config_type  = ?,
                    command_line = ?
                );

        while (tmp != NULL) {

            /* we don't need to memset(0) since it's the same amount
               of arguments each time */
            ndomod_mysql_i = 0;

            /* @todo - it may be worth it to see if there is a better way
                       to do this, it will require some benchmarking,
                       but it might be nicer to just put a subquery in
                       and let sql handle the processing aspect */
            object_id = ndomod_get_object_id(NDO_TRUE, 
                                             NDO2DB_OBJECTTYPE_COMMAND,
                                             tmp->name,
                                             NULL);

            SET_BIND_INT(object_id);
            SET_BIND_INT(config_type);
            SET_BIND_STR(tmp->command_line);

            SET_BIND_INT(object_id);
            SET_BIND_INT(config_type);
            SET_BIND_STR(tmp->command_line);

            BIND();
            QUERY();

            tmp = tmp->next;
        }
    }

    /********************************************************************
        timeperiods and ranges
    ********************************************************************/
    {
        timeperiod * tmp  = timeperiod_list;
        timerange * range = NULL;

        object_id = 0;
        int day = 0;

        /* if you have more than 2k timeperiods, you're doing it wrong.. */
        int timeperiod_ids[2048];
        int timeperiod_i = 0;

        RESET_BIND();
        SET_SQL(INSERT INTO 
                    nagios_timeperiods
                SET
                    instance_id          = 1,
                    timeperiod_object_id = ?,
                    config_type          = ?,
                    alias                = ?
                ON DUPLICATE KEY UPDATE
                    instance_id          = 1,
                    timeperiod_object_id = ?,
                    config_type          = ?,
                    alias                = ?
                );

        while (tmp != NULL) {

            ndomod_mysql_i = 0;

            object_id = ndomod_get_object_id(NDO_TRUE, 
                                             NDO2DB_OBJECTTYPE_TIMEPERIOD,
                                             tmp->name,
                                             NULL);

            /* store this for later lookup */
            timeperiod_ids[timeperiod_i] = object_id;
            timeperiod_i++;

            SET_BIND_INT(object_id);
            SET_BIND_INT(config_type);
            SET_BIND_STR(tmp->alias);

            SET_BIND_INT(object_id);
            SET_BIND_INT(config_type);
            SET_BIND_STR(tmp->alias);

            BIND();
            QUERY();

            tmp = tmp->next;
        }

        /* loop back over for time ranges */
        tmp = timeperiod_list;
        timeperiod_i   = 0;

        RESET_BIND();
        SET_SQL(INSERT INTO 
                    nagios_timeperiod_timeranges
                SET
                    instance_id   = 1,
                    timeperiod_id = ?,
                    day           = ?,
                    start_sec     = ?,
                    end_sec       = ?
                ON DUPLICATE KEY UPDATE
                    instance_id   = 1,
                    timeperiod_id = ?,
                    day           = ?,
                    start_sec     = ?,
                    end_sec       = ?
                );

        while (tmp != NULL) {

            object_id = timeperiod_ids[timeperiod_i];

            for (day = 0; day < 7; day++) {

                range = tmp->days[day];
            
                ndomod_mysql_i = 0;

                SET_BIND_INT(object_id);
                SET_BIND_INT(day);
                SET_BIND_INT(range->range_start);
                SET_BIND_INT(range->range_end);

                SET_BIND_INT(object_id);
                SET_BIND_INT(day);
                SET_BIND_INT(range->range_start);
                SET_BIND_INT(range->range_end);

                BIND();
                QUERY();
            }

            timeperiod_i++;
            tmp = tmp->next;
        }
    }

    /********************************************************************
        contacts and contact addresses
    ********************************************************************/
    {
        contact * tmp = contact_list;
        object_id = 0;

        /* @todo - i'm only putting it here because this is where i was when
                   i thought about it

                   this artifical/arbitrary limit on objects is going to
                   byte us in the bits eventually - and the only reason i'm
                   doing it this way is for convenience of skipping another
                   prepared statement to look up an object id when looping
                   over the object (contacts) again

                   this could be solved a few different ways:

                        1) implement a cache (like the old ndo had, but
                           hopefully a teensy bit more efficient)

                        2) use multiple prepared statements simultaneously -
                           but i'm not sure if this is possible, so it
                           requires some extensive testing
                            - specifically - if it is truly using each
                              prepared statements bind() and not "resetting"

                        3) ...? profit!

        */

        int contact_ids[2048];
        int contact_i = 0;

        /* there are eleven of them */
        int notify_options[11] = { 0 };
        int notify_options_i = 0;

        RESET_BIND();
        SET_SQL(
                INSERT INTO
                    nagios_contacts
                SET
                    instance_id                   = 1,
                    config_type                   = ?,
                    contact_object_id             = ?,
                    alias                         = ?,
                    email_address                 = ?,
                    pager_address                 = ?,
                    host_timeperiod_object_id     = (SELECT object_id FROM nagios_objects WHERE name1 = ? and objecttype_id = 9 LIMIT 1),
                    service_timeperiod_object_id  = (SELECT object_id FROM nagios_objects WHERE name1 = ? and objecttype_id = 9 LIMIT 1),
                    host_notifications_enabled    = ?,
                    service_notifications_enabled = ?,
                    can_submit_commands           = ?,
                    notify_service_recovery       = ?,
                    notify_service_warning        = ?,
                    notify_service_unknown        = ?,
                    notify_service_critical       = ?,
                    notify_service_flapping       = ?,
                    notify_service_downtime       = ?,
                    notify_host_recovery          = ?,
                    notify_host_down              = ?,
                    notify_host_unreachable       = ?,
                    notify_host_flapping          = ?,
                    notify_host_downtime          = ?,
                    minimum_importance            = ?
                ON DUPLICATE KEY UPDATE
                    instance_id                   = 1,
                    config_type                   = ?,
                    contact_object_id             = ?,
                    alias                         = ?,
                    email_address                 = ?,
                    pager_address                 = ?,
                    host_timeperiod_object_id     = (SELECT object_id FROM nagios_objects WHERE name1 = ? and objecttype_id = 9 LIMIT 1),
                    service_timeperiod_object_id  = (SELECT object_id FROM nagios_objects WHERE name1 = ? and objecttype_id = 9 LIMIT 1),
                    host_notifications_enabled    = ?,
                    service_notifications_enabled = ?,
                    can_submit_commands           = ?,
                    notify_service_recovery       = ?,
                    notify_service_warning        = ?,
                    notify_service_unknown        = ?,
                    notify_service_critical       = ?,
                    notify_service_flapping       = ?,
                    notify_service_downtime       = ?,
                    notify_host_recovery          = ?,
                    notify_host_down              = ?,
                    notify_host_unreachable       = ?,
                    notify_host_flapping          = ?,
                    notify_host_downtime          = ?,
                    minimum_importance            = ?
                );

        while (tmp != NULL) {

            ndomod_mysql_i = 0;

            object_id = ndomod_get_object_id(NDO_TRUE, 
                                             NDO2DB_OBJECTTYPE_CONTACT,
                                             tmp->name,
                                             NULL);

            contact_ids[contacts_i] = object_id;
            contacts_i++;

            notify_options_i = 0;

            notify_options[notify_options_i++] = 
                flag_isset(tmp->service_notification_options, OPT_UNKNOWN);
            notify_options[notify_options_i++] = 
                flag_isset(tmp->service_notification_options, OPT_WARNING);
            notify_options[notify_options_i++] = 
                flag_isset(tmp->service_notification_options, OPT_CRITICAL);
            notify_options[notify_options_i++] = 
                flag_isset(tmp->service_notification_options, OPT_RECOVERY);
            notify_options[notify_options_i++] = 
                flag_isset(tmp->service_notification_options, OPT_FLAPPING);
            notify_options[notify_options_i++] = 
                flag_isset(tmp->service_notification_options, OPT_DOWNTIME);

            notify_options[notify_options_i++] = 
                flag_isset(tmp->host_notification_options, OPT_RECOVERY);
            notify_options[notify_options_i++] = 
                flag_isset(tmp->host_notification_options, OPT_DOWN);
            notify_options[notify_options_i++] = 
                flag_isset(tmp->host_notification_options, OPT_UNREACHABLE);
            notify_options[notify_options_i++] = 
                flag_isset(tmp->host_notification_options, OPT_FLAPPING);
            notify_options[notify_options_i++] = 
                flag_isset(tmp->host_notification_options, OPT_DOWNTIME);


            SET_BIND_INT(config_type);
            SET_BIND_INT(object_id);
            SET_BIND_STR(tmp->alias);
            SET_BIND_STR(tmp->email);
            SET_BIND_STR(tmp->pager);
            SET_BIND_STR(tmp->email);
            SET_BIND_STR(tmp->host_notification_period);
            SET_BIND_STR(tmp->service_notification_period);
            SET_BIND_INT(tmp->host_notifications_enabled);
            SET_BIND_INT(tmp->service_notifications_enabled);
            SET_BIND_INT(tmp->can_submit_commands);

            notify_options_i = 0;
            SET_BIND_INT(notify_options[notify_options_i++]);
            SET_BIND_INT(notify_options[notify_options_i++]);
            SET_BIND_INT(notify_options[notify_options_i++]);
            SET_BIND_INT(notify_options[notify_options_i++]);
            SET_BIND_INT(notify_options[notify_options_i++]);
            SET_BIND_INT(notify_options[notify_options_i++]);
            SET_BIND_INT(notify_options[notify_options_i++]);
            SET_BIND_INT(notify_options[notify_options_i++]);
            SET_BIND_INT(notify_options[notify_options_i++]);
            SET_BIND_INT(notify_options[notify_options_i++]);
            SET_BIND_INT(notify_options[notify_options_i++]);

            SET_BIND_INT(tmp->minimum_value);


            SET_BIND_INT(config_type);
            SET_BIND_INT(object_id);
            SET_BIND_STR(tmp->alias);
            SET_BIND_STR(tmp->email);
            SET_BIND_STR(tmp->pager);
            SET_BIND_STR(tmp->email);
            SET_BIND_STR(tmp->host_notification_period);
            SET_BIND_STR(tmp->service_notification_period);
            SET_BIND_INT(tmp->host_notifications_enabled);
            SET_BIND_INT(tmp->service_notifications_enabled);
            SET_BIND_INT(tmp->can_submit_commands);

            notify_options_i = 0;
            SET_BIND_INT(notify_options[notify_options_i++]);
            SET_BIND_INT(notify_options[notify_options_i++]);
            SET_BIND_INT(notify_options[notify_options_i++]);
            SET_BIND_INT(notify_options[notify_options_i++]);
            SET_BIND_INT(notify_options[notify_options_i++]);
            SET_BIND_INT(notify_options[notify_options_i++]);
            SET_BIND_INT(notify_options[notify_options_i++]);
            SET_BIND_INT(notify_options[notify_options_i++]);
            SET_BIND_INT(notify_options[notify_options_i++]);
            SET_BIND_INT(notify_options[notify_options_i++]);
            SET_BIND_INT(notify_options[notify_options_i++]);

            SET_BIND_INT(tmp->minimum_value);

            tmp = tmp->next;
        }

        /* loop back over for addresses */
        tmp        = contact_list;
        contacts_i = 0;

        RESET_BIND();
        SET_SQL(INSERT INTO 
                    nagios_contact_addresses
                SET
                    instance_id    = 1,
                    contact_id     = ?,
                    address_number = ?,
                    address        = ?
                ON DUPLICATE KEY UPDATE
                    instance_id    = 1,
                    contact_id     = ?,
                    address_number = ?,
                    address        = ?
                );

        while (tmp != NULL) {

            object_id = contact_ids[contact_i];

            for (i = 1; i <= MAX_CONTACT_ADDRESSES + 1; i++) {

                ndomod_mysql_i = 0;

                SET_BIND_INT(object_id);
                SET_BIND_INT(i);
                SET_BIND_STR(tmp->address[i - 1]);

                SET_BIND_INT(object_id);
                SET_BIND_INT(i);
                SET_BIND_STR(tmp->address[i - 1]);

                BIND();
                QUERY();
            }

            contact_i++;
            tmp = tmp->next;
        }

        /* the original handle_contactdefinition command has a strtok
           on the main string for '!' - except that it is never sent
           over the wire - which means it is always blank - if you are
           reading this code, and have a version of ndoutils that matches

           < 3.0.0 (and have ANYTHING other than "" or NULL in the results
           from a query like:

           SELECT command_args FROM nagios_contact_notificationcommands
                GROUP BY command_args;

           ) - please contact me and let me know. find my email with

           ```
           git blame ${this_file}.c | grep heden
           ```
        */

        /* loop back over for addresses */
        tmp        = contact_list;
        contact_i = 0;

        commandsmember * contact_cmd = NULL;
        int notification_type = 0;

        RESET_BIND();
        SET_SQL(
                INSERT INTO
                    nagios_contact_notificationcommands
                SET
                    instance_id       = 1,
                    contact_id        = ?,
                    notification_type = ?,
                    command_object_id = ?
                ON DUPLICATE KEY UPDATE
                    instance_id       = 1,
                    contact_id        = ?,
                    notification_type = ?,
                    command_object_id = ?
                );

        while (tmp != NULL) {

            object_id = contact_ids[contact_i];

            contact_cmd       = tmp->host_notification_command;
            notification_type = NDO_DATA_HOSTNOTIFICATIONCOMMAND;

            while (contact_cmd != NULL) {

                SET_BIND_INT(object_id);
                SET_BIND_INT(notification_type);
                SET_BIND_STR(contact_cmd->command);


                SET_BIND_INT(object_id);
                SET_BIND_INT(notification_type);
                SET_BIND_STR(contact_cmd->command);

                BIND();
                QUERY();

                contact_cmd = contact_cmd->next;
            }


            contact_cmd       = tmp->service_notification_command;
            notification_type = NDO_DATA_SERVICENOTIFICATIONCOMMAND;

            while (contact_cmd != NULL) {

                SET_BIND_INT(object_id);
                SET_BIND_INT(notification_type);
                SET_BIND_STR(contact_cmd->command);


                SET_BIND_INT(object_id);
                SET_BIND_INT(notification_type);
                SET_BIND_STR(contact_cmd->command);

                BIND();
                QUERY();

                contact_cmd = contact_cmd->next;
            }

            object_id++;
            tmp = tmp->next;
        }

        /* loop back over for addresses */
        tmp        = contact_list;
        contact_i = 0;

        while (tmp != NULL) {

            object_id = contact_ids[contact_i];

            ndomod_save_customvars(object_id, NDO2DB_OBJECTTYPE_CONTACT,
                                   tmp->custom_variables);

            object_id++;
            tmp = tmp->next;
        }
    }

    /********************************************************************
        contact groups
    ********************************************************************/
    {
        contactgroup * tmp = contactgroups_list;
        object_id = 0;

        int contactgroup_ids[2048] = { 0 };
        int contactgroup_i = 0;

        RESET_BIND();
        SET_SQL(INSERT INTO 
                    nagios_contactgroups
                SET
                    instance_id            = 1,
                    contactgroup_object_id = ?,
                    config_type            = ?,
                    alias                  = ?
                ON DUPLICATE KEY UPDATE
                    instance_id            = 1,
                    contactgroup_object_id = ?,
                    config_type            = ?,
                    alias                  = ?
                );

        while (tmp != NULL) {

            ndomod_mysql_i = 0;

            object_id = ndomod_get_object_id(NDO_TRUE, 
                                             NDO2DB_OBJECTTYPE_CONTACTGROUP,
                                             tmp->name,
                                             NULL);

            /* store this for later lookup */
            contactgroup_ids[contactgroup_i] = object_id;
            contactgroup_i++;

            SET_BIND_INT(object_id);
            SET_BIND_INT(config_type);
            SET_BIND_STR(tmp->alias);

            SET_BIND_INT(object_id);
            SET_BIND_INT(config_type);
            SET_BIND_STR(tmp->alias);

            BIND();
            QUERY();

            tmp = tmp->next;
        }


        /* loop back over for members */
        tmp = contactgroups_list;
        contactgroup_i = 0;

        contactgroupmember * member = NULL;

        RESET_BIND();
        SET_SQL(
                INSERT INTO
                    nagios_contactgroup_members
                SET
                    instance_id       = 1,
                    contactgroup_id   = ?,
                    contact_object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 10 AND name1 = ? LIMIT 1)
                ON DUPLICATE KEY UPDATE
                    instance_id       = 1,
                    contactgroup_id   = ?,
                    contact_object_id = (SELECT object_id FROM nagios_objects WHERE objecttype_id = 10 AND name1 = ? LIMIT 1)
                );

        while (tmp != NULL) {

            object_id = contactgroup_ids[contactgroup_i];

            member = tmp->members;

            while (member != NULL) {

                SET_BIND_INT(object_id);
                SET_BIND_STR(member->contact_name);

                SET_BIND_INT(object_id);
                SET_BIND_STR(member->contact_name);

                BIND();
                QUERY();
            }

            contactgroup_i++;
            tmp = tmp->next;
        }
    }
}

void ndomod_save_customvars(int object_id, int config_type, customvariablesmember * custom_vars)
{
    customvariablesmember * tmp = custom_vars;

    RESET_BIND();
    SET_SQL(
            INSERT INTO
                nagios_customvariables
            SET
                instance_id       = 1,
                object_id         = ?,
                config_type       = ?,
                has_been_modified = ?,
                varname           = ?,
                varvalue          = ?
            ON DUPLICATE KEY UPDATE
                instance_id       = 1,
                object_id         = ?,
                config_type       = ?,
                has_been_modified = ?,
                varname           = ?,
                varvalue          = ?
            );

    while (tmp != NULL) {

        SET_BIND_INT(object_id);
        SET_BIND_INT(config_type);
        SET_BIND_INT(tmp->has_been_modified);
        SET_BIND_STR(tmp->variable_name);
        SET_BIND_STR(tmp->variable_value);

        SET_BIND_INT(object_id);
        SET_BIND_INT(config_type);
        SET_BIND_INT(tmp->has_been_modified);
        SET_BIND_STR(tmp->variable_name);
        SET_BIND_STR(tmp->variable_value);

        BIND();
        QUERY();

        tmp = tmp->next;
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
        HACK_SQL_QUERY(26, "s", 27, 32);

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
        HACK_SQL_QUERY(19, "downtimehistory", 34, 35);

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
        HACK_SQL_QUERY(14, "downtimehistory", 29, 30);

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


NDOMOD_HANDLER_FUNCTION(timed_event_data)
{

}


NDOMOD_HANDLER_FUNCTION(flapping_data)
{
    
}

NDOMOD_HANDLER_FUNCTION(program_status_data)
{
    RESET_BIND();

    SET_SQL(
        INSERT INTO
            nagios_programstatus
        SET
            instance_id                    = 1,
            status_update_time             = ?,
            program_start_time             = FROM_UNIXTIME(?),
            is_currently_running           = 1,
            process_id                     = ?,
            daemon_mode                    = ?,
            last_command_check             = FROM_UNIXTIME(0),
            last_log_rotation              = FROM_UNIXTIME(?),
            notifications_enabled          = ?,
            active_service_checks_enabled  = ?,
            passive_service_checks_enabled = ?,
            active_host_checks_enabled     = ?,
            passive_host_checks_enabled    = ?,
            event_handlers_enabled         = ?,
            flap_detection_enabled         = ?,
            failure_prediction_enabled     = ?,
            process_performance_data       = ?,
            obsess_over_hosts              = ?,
            obsess_over_services           = ?,
            modified_host_attributes       = ?,
            modified_service_attributes    = ?,
            global_host_event_handler      = ?,
            global_service_event_handler   = ?
        ON DUPLICATE KEY UPDATE
            instance_id                    = 1,
            status_update_time             = ?,
            program_start_time             = FROM_UNIXTIME(?),
            is_currently_running           = 1,
            process_id                     = ?,
            daemon_mode                    = ?,
            last_command_check             = FROM_UNIXTIME(0),
            last_log_rotation              = FROM_UNIXTIME(?),
            notifications_enabled          = ?,
            active_service_checks_enabled  = ?,
            passive_service_checks_enabled = ?,
            active_host_checks_enabled     = ?,
            passive_host_checks_enabled    = ?,
            event_handlers_enabled         = ?,
            flap_detection_enabled         = ?,
            failure_prediction_enabled     = ?,
            process_performance_data       = ?,
            obsess_over_hosts              = ?,
            obsess_over_services           = ?,
            modified_host_attributes       = ?,
            modified_service_attributes    = ?,
            global_host_event_handler      = ?,
            global_service_event_handler   = ?

        );

    SET_BIND_STR(data->timestamp.tv_sec);               // status_update_time
    SET_BIND_INT(data->program_start);                  // program_start_time
    SET_BIND_INT(data->pid);                            // process_id
    SET_BIND_INT(data->daemon_mode);                    // daemon_mode
    SET_BIND_INT(data->last_log_rotation);              // last_log_rotation
    SET_BIND_INT(data->notifications_enabled);          // notifications_enabled
    SET_BIND_INT(data->active_service_checks_enabled);  // active_service_checks_enabled 
    SET_BIND_INT(data->passive_service_checks_enabled); // passive_service_checks_enabled 
    SET_BIND_INT(data->active_host_checks_enabled);     // active_host_checks_enabled 
    SET_BIND_INT(data->passive_host_checks_enabled);    // passive_host_checks_enabled 
    SET_BIND_INT(data->event_handlers_enabled);         // event_handlers_enabled
    SET_BIND_INT(data->flap_detection_enabled);         // failure_prediction_enabled 
    SET_BIND_INT(data->process_performance_data);       // process_performance_data 
    SET_BIND_INT(data->obsess_over_hosts);              // obsess_over_hosts 
    SET_BIND_INT(data->obsess_over_services);           // obsess_over_services
    SET_BIND_INT(data->modified_host_attributes);       // modified_host_attributes 
    SET_BIND_INT(data->modified_service_attributes);    // modified_service_attributes 
    SET_BIND_STR(data->global_host_event_handler);      // global_host_event_handler
    SET_BIND_STR(data->global_service_event_handler);   // global_service_event_handler

    SET_BIND_STR(data->timestamp.tv_sec);               // status_update_time
    SET_BIND_INT(data->program_start);                  // program_start_time
    SET_BIND_INT(data->pid);                            // process_id
    SET_BIND_INT(data->daemon_mode);                    // daemon_mode
    SET_BIND_INT(data->last_log_rotation);              // last_log_rotation
    SET_BIND_INT(data->notifications_enabled);          // notifications_enabled
    SET_BIND_INT(data->active_service_checks_enabled);  // active_service_checks_enabled 
    SET_BIND_INT(data->passive_service_checks_enabled); // passive_service_checks_enabled 
    SET_BIND_INT(data->active_host_checks_enabled);     // active_host_checks_enabled 
    SET_BIND_INT(data->passive_host_checks_enabled);    // passive_host_checks_enabled 
    SET_BIND_INT(data->event_handlers_enabled);         // event_handlers_enabled
    SET_BIND_INT(data->flap_detection_enabled);         // failure_prediction_enabled 
    SET_BIND_INT(data->process_performance_data);       // process_performance_data 
    SET_BIND_INT(data->obsess_over_hosts);              // obsess_over_hosts 
    SET_BIND_INT(data->obsess_over_services);           // obsess_over_services
    SET_BIND_INT(data->modified_host_attributes);       // modified_host_attributes 
    SET_BIND_INT(data->modified_service_attributes);    // modified_service_attributes 
    SET_BIND_STR(data->global_host_event_handler);      // global_host_event_handler
    SET_BIND_STR(data->global_service_event_handler);   // global_service_event_handler

    BIND();
    QUERY();
}


NDOMOD_HANDLER_FUNCTION(host_status_data)
{
    int host_object_id = 0;
    int timeperiod_object_id = 0;
    host *host = (host *)data->object_ptr;
    timeperiod *timeperiod = host->check_period_ptr;
    char *event_handler_name = host->event_handler_ptr == NULL ? "" : host->event_handler_ptr->name;
    char *check_command_name = host->check_command_ptr == NULL ? "" : host->check_command_ptr->name;

    // If data->timestamp.tv_sec < dbinfo.latest_realtime_data_time, return;
    // idk if this goes before or after the bind.

    host_object_id = ndomod_get_object_id(NDO_TRUE, NDO2DB_OBJECTTYPE_HOST, host->host_name, NULL);
    timeperiod_object_id = ndomod_get_object_id(NDO_TRUE, NDO2DB_OBJECTTYPE_TIMEPERIOD, timeperiod->name, NULL);

    RESET_BIND();

    SET_SQL(
        INSERT INTO
            nagios_hoststatus
        SET
            instance_id                   = 1,
            host_object_id                = ?,
            status_update_time            = FROM_UNIXTIME(?),
            output                        = ?,
            long_output                   = ?,
            perfdata                      = ?,
            current_state                 = ?,
            has_been_checked              = ?,
            should_be_scheduled           = ?,
            current_check_attempt         = ?,
            max_check_attempts            = ?,
            last_check                    = FROM_UNIXTIME(?),
            next_check                    = FROM_UNIXTIME(?),
            check_type                    = ?,
            last_state_change             = FROM_UNIXTIME(?),
            last_hard_state_change        = FROM_UNIXTIME(?),
            last_hard_state               = ?,
            last_time_up                  = FROM_UNIXTIME(?),
            last_time_down                = FROM_UNIXTIME(?),
            last_time_unreachable         = FROM_UNIXTIME(?),
            state_type                    = ?,
            last_notification             = FROM_UNIXTIME(?),
            next_notification             = FROM_UNIXTIME(?),
            no_more_notifications         = ?,
            notifications_enabled         = ?,
            problem_has_been_acknowledged = ?,
            acknowledgement_type          = ?,
            current_notification_number   = ?,
            passive_checks_enabled        = ?,
            active_checks_enabled         = ?,
            event_handler_enabled         = ?,
            flap_detection_enabled        = ?,
            is_flapping                   = ?,
            percent_state_change          = ?,
            latency                       = ?,
            execution_time                = ?,
            scheduled_downtime_depth      = ?,
            failure_prediction_enabled    = 0,
            process_performance_data      = ?,
            obsess_over_host              = ?,
            modified_host_attributes      = ?,
            event_handler                 = ?,
            check_command                 = ?,
            normal_check_interval         = ?,
            retry_check_interval          = ?,
            check_timeperiod_object_id    = ?
        ON DUPLICATE KEY UPDATE
            instance_id                   = 1,
            host_object_id                = ?,
            status_update_time            = FROM_UNIXTIME(?),
            output                        = ?,
            long_output                   = ?,
            perfdata                      = ?,
            current_state                 = ?,
            has_been_checked              = ?,
            should_be_scheduled           = ?,
            current_check_attempt         = ?,
            max_check_attempts            = ?,
            last_check                    = FROM_UNIXTIME(?),
            next_check                    = FROM_UNIXTIME(?),
            check_type                    = ?,
            last_state_change             = FROM_UNIXTIME(?),
            last_hard_state_change        = FROM_UNIXTIME(?),
            last_hard_state               = ?,
            last_time_up                  = FROM_UNIXTIME(?),
            last_time_down                = FROM_UNIXTIME(?),
            last_time_unreachable         = FROM_UNIXTIME(?),
            state_type                    = ?,
            last_notification             = FROM_UNIXTIME(?),
            next_notification             = FROM_UNIXTIME(?),
            no_more_notifications         = ?,
            notifications_enabled         = ?,
            problem_has_been_acknowledged = ?,
            acknowledgement_type          = ?,
            current_notification_number   = ?,
            passive_checks_enabled        = ?,
            active_checks_enabled         = ?,
            event_handler_enabled         = ?,
            flap_detection_enabled        = ?,
            is_flapping                   = ?,
            percent_state_change          = ?,
            latency                       = ?,
            execution_time                = ?,
            scheduled_downtime_depth      = ?,
            failure_prediction_enabled    = 0,
            process_performance_data      = ?,
            obsess_over_host              = ?,
            modified_host_attributes      = ?,
            event_handler                 = ?,
            check_command                 = ?,
            normal_check_interval         = ?,
            retry_check_interval          = ?,
            check_timeperiod_object_id    = ?
        );

    SET_BIND_INT(host_object_id);                      // host_object_id
    SET_BIND_INT(data->timestamp.tv_sec);              // status_update_time
    SET_BIND_STR(host->plugin_output);                 // output
    SET_BIND_STR(host->long_plugin_output);            // long_output
    SET_BIND_STR(host->perf_data);                     // perfdata
    SET_BIND_INT(host->current_state);                 // current_state
    SET_BIND_INT(host->has_been_checked);           // has_been_checked
    SET_BIND_INT(host->should_be_scheduled);              // should_be_scheduled
    SET_BIND_INT(host->current_attempt);               // current_check_attempt
    SET_BIND_INT(host->max_attempts);                  // max_check_attempts
    SET_BIND_INT(host->last_check);                    // last_check
    SET_BIND_INT(host->next_check);                    // next_check
    SET_BIND_INT(host->check_type);                    // check_type
    SET_BIND_INT(host->last_state_change);             // last_state_change
    SET_BIND_INT(host->last_hard_state_change);        // last_hard_state_change
    SET_BIND_INT(host->last_hard_state);               // last_hard_state
    SET_BIND_INT(host->last_time_up);                  // last_time_up
    SET_BIND_INT(host->last_time_down);                // last_time_down
    SET_BIND_INT(host->last_time_unreachable);         // last_time_unreachable
    SET_BIND_INT(host->state_type);                    // state_type
    SET_BIND_STR(host->last_notification);             // last_notification
    SET_BIND_STR(host->next_notification);             // next_notification
    SET_BIND_INT(host->no_more_notifications);         // no_more_notifications
    SET_BIND_INT(host->notifications_enabled);         // notifications_enabled
    SET_BIND_INT(host->problem_has_been_acknowledged); // problem_has_been_acknowledged
    SET_BIND_INT(host->acknowledgement_type);          // acknowledgement_type
    SET_BIND_INT(host->current_notification_number);   // current_notification_number
    SET_BIND_INT(host->accept_passive_checks);         // passive_checks_enabled
    SET_BIND_INT(host->checks_enabled);                // active_checks_enabled
    SET_BIND_INT(host->event_handler_enabled);         // event_handler_enabled
    SET_BIND_INT(host->flap_detection_enabled);        // flap_detection_enabled
    SET_BIND_INT(host->is_flapping);                   // is_flapping
    SET_BIND_DOUBLE(host->percent_state_change);       // percent_state_change
    SET_BIND_DOUBLE(host->latency);                    // latency
    SET_BIND_DOUBLE(host->execution_time);             // execution_time
    SET_BIND_INT(host->scheduled_downtime_depth);      // scheduled_downtime_depth
    SET_BIND_INT(host->process_performance_data);      // process_performance_data
    SET_BIND_INT(host->obsess);                        // obsess_over_host
    SET_BIND_INT(host->modified_attributes);           // modified_host_attributes
    SET_BIND_STR(event_handler_name);                  // event_handler
    SET_BIND_STR(check_command_name);                  // check_command
    SET_BIND_DOUBLE(host->check_interval);             // normal_check_interval
    SET_BIND_DOUBLE(host->retry_interval);             // retry_check_interval
    SET_BIND_INT(timeperiod_object_id);                // check_timeperiod_object_id

    SET_BIND_INT(host_object_id);                      // host_object_id
    SET_BIND_INT(data->timestamp.tv_sec);              // status_update_time
    SET_BIND_STR(host->plugin_output);                 // output
    SET_BIND_STR(host->long_plugin_output);            // long_output
    SET_BIND_STR(host->perf_data);                     // perfdata
    SET_BIND_INT(host->current_state);                 // current_state
    SET_BIND_INT(host->should_be_scheduled);           // has_been_checked
    SET_BIND_INT(host->has_been_checked);              // should_be_scheduled
    SET_BIND_INT(host->current_attempt);               // current_check_attempt
    SET_BIND_INT(host->max_attempts);                  // max_check_attempts
    SET_BIND_INT(host->last_check);                    // last_check
    SET_BIND_INT(host->next_check);                    // next_check
    SET_BIND_INT(host->check_type);                    // check_type
    SET_BIND_INT(host->last_state_change);             // last_state_change
    SET_BIND_INT(host->last_hard_state_change);        // last_hard_state_change
    SET_BIND_INT(host->last_hard_state);               // last_hard_state
    SET_BIND_INT(host->last_time_up);                  // last_time_up
    SET_BIND_INT(host->last_time_down);                // last_time_down
    SET_BIND_INT(host->last_time_unreachable);         // last_time_unreachable
    SET_BIND_INT(host->state_type);                    // state_type
    SET_BIND_STR(host->last_notification);             // last_notification
    SET_BIND_STR(host->next_notification);             // next_notification
    SET_BIND_INT(host->no_more_notifications);         // no_more_notifications
    SET_BIND_INT(host->notifications_enabled);         // notifications_enabled
    SET_BIND_INT(host->problem_has_been_acknowledged); // problem_has_been_acknowledged
    SET_BIND_INT(host->acknowledgement_type);          // acknowledgement_type
    SET_BIND_INT(host->current_notification_number);   // current_notification_number
    SET_BIND_INT(host->accept_passive_checks);         // passive_checks_enabled
    SET_BIND_INT(host->checks_enabled);                // active_checks_enabled
    SET_BIND_INT(host->event_handler_enabled);         // event_handler_enabled
    SET_BIND_INT(host->flap_detection_enabled);        // flap_detection_enabled
    SET_BIND_INT(host->is_flapping);                   // is_flapping
    SET_BIND_DOUBLE(host->percent_state_change);       // percent_state_change
    SET_BIND_DOUBLE(host->latency);                    // latency
    SET_BIND_DOUBLE(host->execution_time);             // execution_time
    SET_BIND_INT(host->scheduled_downtime_depth);      // scheduled_downtime_depth
    SET_BIND_INT(host->process_performance_data);      // process_performance_data
    SET_BIND_INT(host->obsess);                        // obsess_over_host
    SET_BIND_INT(host->modified_attributes);           // modified_host_attributes
    SET_BIND_STR(event_handler_name);                  // event_handler
    SET_BIND_STR(check_command_name);                  // check_command
    SET_BIND_DOUBLE(host->check_interval);             // normal_check_interval
    SET_BIND_DOUBLE(host->retry_interval);             // retry_check_interval
    SET_BIND_INT(timeperiod_object_id);                // check_timeperiod_object_id

    BIND();
    QUERY();
}


NDOMOD_HANDLER_FUNCTION(service_status_data)
{

    int service_object_id = 0;
    int timeperiod_object_id = 0;
    service *service = (service *)data->object_ptr;
    timeperiod *timeperiod = service->check_period_ptr;
    char *event_handler_name = service->event_handler_ptr == NULL ? "" : service->event_handler_ptr->name;
    char *check_command_name = service->check_command_ptr == NULL ? "" : service->check_command_ptr->name;

    // If data->timestamp.tv_sec < dbinfo.latest_realtime_data_time, return;
    // idk if this goes before or after the bind.

    service_object_id = ndomod_get_object_id(NDO_TRUE, NDO2DB_OBJECTTYPE_SERVICE, service->host_name, service->description);
    timeperiod_object_id = ndomod_get_object_id(NDO_TRUE, NDO2DB_OBJECTTYPE_TIMEPERIOD, timeperiod->name, NULL);


    RESET_QUERY();

    SET_SQL(
        INSERT INTO
            nagios_servicestatus
        SET
            instance_id                   = 1,
            service_object_id             = ?,
            status_update_time            = FROM_UNIXTIME(?),
            output                        = ?,
            long_output                   = ?,
            perfdata                      = ?,
            current_state                 = ?,
            has_been_checked              = ?,
            should_be_scheduled           = ?,
            current_check_attempt         = ?,
            max_check_attempts            = ?,
            last_check                    = FROM_UNIXTIME(?),
            next_check                    = FROM_UNIXTIME(?),
            check_type                    = ?,
            last_state_change             = FROM_UNIXTIME(?),
            last_hard_state_change        = FROM_UNIXTIME(?),
            last_hard_state               = ?,
            last_time_ok                  = FROM_UNIXTIME(?),
            last_time_warning             = FROM_UNIXTIME(?),
            last_time_unknown             = FROM_UNIXTIME(?),
            last_time_critical            = FROM_UNIXTIME(?),
            state_type                    = ?,
            last_notification             = FROM_UNIXTIME(?),
            next_notification             = FROM_UNIXTIME(?),
            no_more_notifications         = ?,
            notifications_enabled         = ?,
            problem_has_been_acknowledged = ?,
            acknowledgement_type          = ?,
            current_notification_number   = ?,
            passive_checks_enabled        = ?,
            active_checks_enabled         = ?,
            event_handler_enabled         = ?,
            flap_detection_enabled        = ?,
            is_flapping                   = ?,
            percent_state_change          = ?,
            latency                       = ?,
            execution_time                = ?,
            scheduled_downtime_depth      = ?,
            failure_prediction_enabled    = 0,
            process_performance_data      = ?,
            obsess_over_service           = ?,
            modified_service_attributes   = ?,
            event_handler                 = ?,
            check_command                 = ?,
            normal_check_interval         = ?,
            retry_check_interval          = ?,
            check_timeperiod_object_id    = ?
        ON DUPLICATE KEY UPDATE
            instance_id                   = 1,
            service_object_id             = ?,
            status_update_time            = FROM_UNIXTIME(?),
            output                        = ?,
            long_output                   = ?,
            perfdata                      = ?,
            current_state                 = ?,
            has_been_checked              = ?,
            should_be_scheduled           = ?,
            current_check_attempt         = ?,
            max_check_attempts            = ?,
            last_check                    = FROM_UNIXTIME(?),
            next_check                    = FROM_UNIXTIME(?),
            check_type                    = ?,
            last_state_change             = FROM_UNIXTIME(?),
            last_hard_state_change        = FROM_UNIXTIME(?),
            last_hard_state               = ?,
            last_time_ok                  = FROM_UNIXTIME(?),
            last_time_warning             = FROM_UNIXTIME(?),
            last_time_unknown             = FROM_UNIXTIME(?),
            last_time_critical            = FROM_UNIXTIME(?),
            state_type                    = ?,
            last_notification             = FROM_UNIXTIME(?),
            next_notification             = FROM_UNIXTIME(?),
            no_more_notifications         = ?,
            notifications_enabled         = ?,
            problem_has_been_acknowledged = ?,
            acknowledgement_type          = ?,
            current_notification_number   = ?,
            passive_checks_enabled        = ?,
            active_checks_enabled         = ?,
            event_handler_enabled         = ?,
            flap_detection_enabled        = ?,
            is_flapping                   = ?,
            percent_state_change          = ?,
            latency                       = ?,
            execution_time                = ?,
            scheduled_downtime_depth      = ?,
            failure_prediction_enabled    = 0,
            process_performance_data      = ?,
            obsess_over_service           = ?,
            modified_service_attributes   = ?,
            event_handler                 = ?,
            check_command                 = ?,
            normal_check_interval         = ?,
            retry_check_interval          = ?,
            check_timeperiod_object_id    = ?
        );


    SET_BIND_INT(service_object_id);                      // service_object_id
    SET_BIND_INT(data->timestamp.tv_sec);                 // status_update_time
    SET_BIND_STR(service->plugin_output);                 // output
    SET_BIND_STR(service->long_plugin_output);            // long_output
    SET_BIND_STR(service->perf_data);                     // perfdata
    SET_BIND_INT(service->current_state);                 // current_state
    SET_BIND_INT(service->has_been_checked);              // has_been_checked
    SET_BIND_INT(service->should_be_scheduled);           // should_be_scheduled
    SET_BIND_INT(service->current_attempt);               // current_check_attempt
    SET_BIND_INT(service->max_attempts);                  // max_check_attempts
    SET_BIND_INT(service->last_check);                    // last_check
    SET_BIND_INT(service->next_check);                    // next_check
    SET_BIND_INT(service->check_type);                    // check_type
    SET_BIND_INT(service->last_state_change);             // last_state_change
    SET_BIND_INT(service->last_hard_state_change);        // last_hard_state_change
    SET_BIND_INT(service->last_hard_state);               // last_hard_state
    SET_BIND_INT(service->last_time_ok);                  // last_time_ok
    SET_BIND_INT(service->last_time_warning);             // last_time_warning
    SET_BIND_INT(service->last_time_unknown);             // last_time_unknown
    SET_BIND_INT(service->last_time_critical);            // last_time_critical
    SET_BIND_INT(service->state_type);                    // state_type
    SET_BIND_INT(service->last_notification);             // last_notification
    SET_BIND_INT(service->next_notification);             // next_notification
    SET_BIND_INT(service->no_more_notifications);         // no_more_notifications
    SET_BIND_INT(service->notifications_enabled);         // notifications_enabled
    SET_BIND_INT(service->problem_has_been_acknowledged); // problem_has_been_acknowledged
    SET_BIND_INT(service->acknowledgement_type);          // acknowledgement_type
    SET_BIND_INT(service->current_notification_number);   // current_notification_number
    SET_BIND_INT(service->accept_passive_checks);         // passive_checks_enabled
    SET_BIND_INT(service->checks_enabled);                // active_checks_enabled
    SET_BIND_INT(service->event_handler_enabled);         // event_handler_enabled
    SET_BIND_INT(service->flap_detection_enabled);        // flap_detection_enabled
    SET_BIND_INT(service->is_flapping);                   // is_flapping
    SET_BIND_DOUBLE(service->percent_state_change);       // percent_state_change
    SET_BIND_DOUBLE(service->latency);                    // latency
    SET_BIND_DOUBLE(service->execution_time);             // execution_time
    SET_BIND_INT(service->scheduled_downtime_depth);      // scheduled_downtime_depth
    SET_BIND_INT(service->process_performance_data);      // process_performance_data
    SET_BIND_INT(service->obsess);                        // obsess_over_service
    SET_BIND_INT(service->modified_attributes);           // modified_service_attributes
    SET_BIND_STR(service->event_handler_name);            // event_handler
    SET_BIND_STR(service->check_command_name);            // check_command
    SET_BIND_DOUBLE(service->check_interval);             // normal_check_interval
    SET_BIND_DOUBLE(service->retry_interval);             // retry_check_interval
    SET_BIND_INT(timeperiod_object_id);                   // check_timeperiod_object_id

    SET_BIND_INT(service_object_id);                      // service_object_id
    SET_BIND_INT(data->timestamp.tv_sec);                 // status_update_time
    SET_BIND_STR(service->plugin_output);                 // output
    SET_BIND_STR(service->long_plugin_output);            // long_output
    SET_BIND_STR(service->perf_data);                     // perfdata
    SET_BIND_INT(service->current_state);                 // current_state
    SET_BIND_INT(service->has_been_checked);              // has_been_checked
    SET_BIND_INT(service->should_be_scheduled);           // should_be_scheduled
    SET_BIND_INT(service->current_attempt);               // current_check_attempt
    SET_BIND_INT(service->max_attempts);                  // max_check_attempts
    SET_BIND_INT(service->last_check);                    // last_check
    SET_BIND_INT(service->next_check);                    // next_check
    SET_BIND_INT(service->check_type);                    // check_type
    SET_BIND_INT(service->last_state_change);             // last_state_change
    SET_BIND_INT(service->last_hard_state_change);        // last_hard_state_change
    SET_BIND_INT(service->last_hard_state);               // last_hard_state
    SET_BIND_INT(service->last_time_ok);                  // last_time_ok
    SET_BIND_INT(service->last_time_warning);             // last_time_warning
    SET_BIND_INT(service->last_time_unknown);             // last_time_unknown
    SET_BIND_INT(service->last_time_critical);            // last_time_critical
    SET_BIND_INT(service->state_type);                    // state_type
    SET_BIND_INT(service->last_notification);             // last_notification
    SET_BIND_INT(service->next_notification);             // next_notification
    SET_BIND_INT(service->no_more_notifications);         // no_more_notifications
    SET_BIND_INT(service->notifications_enabled);         // notifications_enabled
    SET_BIND_INT(service->problem_has_been_acknowledged); // problem_has_been_acknowledged
    SET_BIND_INT(service->acknowledgement_type);          // acknowledgement_type
    SET_BIND_INT(service->current_notification_number);   // current_notification_number
    SET_BIND_INT(service->accept_passive_checks);         // passive_checks_enabled
    SET_BIND_INT(service->checks_enabled);                // active_checks_enabled
    SET_BIND_INT(service->event_handler_enabled);         // event_handler_enabled
    SET_BIND_INT(service->flap_detection_enabled);        // flap_detection_enabled
    SET_BIND_INT(service->is_flapping);                   // is_flapping
    SET_BIND_DOUBLE(service->percent_state_change);       // percent_state_change
    SET_BIND_DOUBLE(service->latency);                    // latency
    SET_BIND_DOUBLE(service->execution_time);             // execution_time
    SET_BIND_INT(service->scheduled_downtime_depth);      // scheduled_downtime_depth
    SET_BIND_INT(service->process_performance_data);      // process_performance_data
    SET_BIND_INT(service->obsess);                        // obsess_over_service
    SET_BIND_INT(service->modified_attributes);           // modified_service_attributes
    SET_BIND_STR(service->event_handler_name);            // event_handler
    SET_BIND_STR(service->check_command_name);            // check_command
    SET_BIND_DOUBLE(service->check_interval);             // normal_check_interval
    SET_BIND_DOUBLE(service->retry_interval);             // retry_check_interval
    SET_BIND_INT(timeperiod_object_id);                   // check_timeperiod_object_id

    BIND();
    QUERY();
}


NDOMOD_HANDLER_FUNCTION(contact_status_data)
{
    
    int contact_object_id = 0;
    contact *contact = (contact *)data->object_ptr;
    timeperiod *timeperiod = contact->check_period_ptr;
    char *event_handler_name = contact->event_handler_ptr == NULL ? "" : contact->event_handler_ptr->name;
    char *check_command_name = contact->check_command_ptr == NULL ? "" : contact->check_command_ptr->name;

    // If data->timestamp.tv_sec < dbinfo.latest_realtime_data_time, return;
    // idk if this goes before or after the bind.

    contact_object_id = ndomod_get_object_id(NDO_TRUE, NDO2DB_OBJECTTYPE_CONTACT, contact->name, NULL);

    RESET_BIND();

    SET_SQL(
        INSERT INTO nagios_contactstatus
        SET
            instance_id                   = 1,
            contact_object_id             = ?,
            status_update_time            = FROM_UNIXTIME(?),
            host_notifications_enabled    = ?,
            service_notifications_enabled = ?,
            last_host_notification        = FROM_UNIXTIME(?),
            last_service_notification     = FROM_UNIXTIME(?),
            modified_attributes           = ?,
            modified_host_attributes      = ?,
            modified_service_attributes   = ?
        ON DUPLICATE KEY UPDATE
            instance_id                   = 1,
            contact_object_id             = ?,
            status_update_time            = FROM_UNIXTIME(?),
            host_notifications_enabled    = ?,
            service_notifications_enabled = ?,
            last_host_notification        = FROM_UNIXTIME(?),
            last_service_notification     = FROM_UNIXTIME(?),
            modified_attributes           = ?,
            modified_host_attributes      = ?,
            modified_service_attributes   = ?
        );
    
    SET_BIND_INT(contact_object_id);                      // contact_object_id
    SET_BIND_INT(data->timestamp.tv_sec);                 // status_update_time
    SET_BIND_INT(contact->host_notifications_enabled);    // host_notifications_enabled
    SET_BIND_INT(contact->service_notifications_enabled); // service_notifications_enabled
    SET_BIND_INT(contact->last_host_notification);        // last_host_notification
    SET_BIND_INT(contact->last_service_notification);     // last_service_notification
    SET_BIND_INT(contact->modified_attributes);           // modified_attributes
    SET_BIND_INT(contact->modified_host_attributes);      // modified_host_attributes
    SET_BIND_INT(contact->modified_service_attributes);   // modified_service_attributes

    SET_BIND_INT(contact_object_id);                      // contact_object_id
    SET_BIND_INT(data->timestamp.tv_sec);                 // status_update_time
    SET_BIND_INT(contact->host_notifications_enabled);    // host_notifications_enabled
    SET_BIND_INT(contact->service_notifications_enabled); // service_notifications_enabled
    SET_BIND_INT(contact->last_host_notification);        // last_host_notification
    SET_BIND_INT(contact->last_service_notification);     // last_service_notification
    SET_BIND_INT(contact->modified_attributes);           // modified_attributes
    SET_BIND_INT(contact->modified_host_attributes);      // modified_host_attributes
    SET_BIND_INT(contact->modified_service_attributes);   // modified_service_attributes

    BIND();
    QUERY();
}


NDOMOD_HANDLER_FUNCTION(external_command_data)
{
    
}


NDOMOD_HANDLER_FUNCTION(contact_notification_data)
{
    
}


NDOMOD_HANDLER_FUNCTION(contact_notification_method_data)
{
    
}


NDOMOD_HANDLER_FUNCTION(acknowledgement_data)
{
    
}


NDOMOD_HANDLER_FUNCTION(statechange_data)
{
    
}
