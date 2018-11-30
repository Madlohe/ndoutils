
NDOMOD_HANDLER_FUNCTION(log_data)
{
    /* this particular function is a bit weird because it starts passing logs to the neb module
       before the database initialization has occured, so if the db hasn't been initialized, we just return */
    if (ndomod_mysql == NULL) {
        return;
    }

    RESET_BIND();

    SET_SQL("
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
            ");

    SET_BIND_INT(0, data->entry_time);
    SET_BIND_INT(1, data->timestamp.tv_sec);
    SET_BIND_INT(2, data->timestamp.tv_usec);
    SET_BIND_INT(3, data->data_type);
    SET_BIND_STR(4, data->data);

    BIND();
    QUERY();
}



// 1980
NDOMOD_HANDLER_FUNCTION(system_command_data)
{
    RESET_BIND();

    SET_SQL("
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
            ");

    SET_BIND_INT(0, data->start_time.tv_sec);
    SET_BIND_INT(1, data->start_time.tv_usec);
    SET_BIND_INT(2, data->end_time.tv_sec);
    SET_BIND_INT(3, data->end_time.tv_usec);
    SET_BIND_STR(4, data->command_line);
    SET_BIND_INT(5, data->timeout);
    SET_BIND_INT(6, data->early_timeout);
    SET_BIND_INT(7, data->expiration_time);
    SET_BIND_INT(8, data->return_code);
    SET_BIND_INT(9, data->output);
    SET_BIND_INT(10, data->long_output);

    SET_BIND_INT(11, data->start_time.tv_sec);
    SET_BIND_INT(12, data->start_time.tv_usec);
    SET_BIND_INT(13, data->end_time.tv_sec);
    SET_BIND_INT(14, data->end_time.tv_usec);
    SET_BIND_STR(15, data->command_line);
    SET_BIND_INT(16, data->timeout);
    SET_BIND_INT(17, data->early_timeout);
    SET_BIND_INT(18, data->expiration_time);
    SET_BIND_INT(19, data->return_code);
    SET_BIND_INT(20, data->output);
    SET_BIND_INT(21, data->long_output);

    BIND();
    QUERY();
}


NDOMOD_HANDLER_FUNCTION(event_handler_data)
{

        es[0] = ndo_escape_buffer(ehanddata->host_name);
        es[1] = ndo_escape_buffer(ehanddata->service_description);
        es[2] = ndo_escape_buffer(ehanddata->command_name);
        es[3] = ndo_escape_buffer(ehanddata->command_args);
        es[4] = ndo_escape_buffer(ehanddata->command_line);
        es[5] = ndo_escape_buffer(ehanddata->output);
        /* Preparing if eventhandler will have long_output in the future */
        es[6] = ndo_escape_buffer(ehanddata->output);

        {
            struct ndo_broker_data event_handler_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, ehanddata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, ehanddata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, ehanddata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, ehanddata->timestamp),
                NDODATA_STRING(NDO_DATA_HOST, es[0]),
                NDODATA_STRING(NDO_DATA_SERVICE, es[1]),
                NDODATA_INTEGER(NDO_DATA_STATETYPE, ehanddata->state_type),
                NDODATA_INTEGER(NDO_DATA_STATE, ehanddata->state),
                NDODATA_TIMESTAMP(NDO_DATA_STARTTIME, ehanddata->start_time),
                NDODATA_TIMESTAMP(NDO_DATA_ENDTIME, ehanddata->end_time),
                NDODATA_INTEGER(NDO_DATA_TIMEOUT, ehanddata->timeout),
                NDODATA_STRING(NDO_DATA_COMMANDNAME, es[2]),
                NDODATA_STRING(NDO_DATA_COMMANDARGS, es[3]),
                NDODATA_STRING(NDO_DATA_COMMANDLINE, es[4]),
                NDODATA_INTEGER(NDO_DATA_EARLYTIMEOUT, ehanddata->early_timeout),
                NDODATA_FLOATING_POINT(NDO_DATA_EXECUTIONTIME, ehanddata->execution_time),
                NDODATA_INTEGER(NDO_DATA_RETURNCODE, ehanddata->return_code),
                NDODATA_STRING(NDO_DATA_OUTPUT, es[5]),
                NDODATA_STRING(NDO_DATA_LONGOUTPUT, es[6] )
            };

    SET_SQL("
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
                command_object_id = ?,
                command_args      = ?,
                command_line      = ?
                timeout           = ?,
                early_timeout     = ?,
                execution_time    = ?,
                return_code       = ?
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
                command_object_id = ?,
                command_args      = ?,
                command_line      = ?
                timeout           = ?,
                early_timeout     = ?,
                execution_time    = ?,
                return_code       = ?
                output            = ?,
                long_output       = ?
            ");

    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EVENTHANDLERTYPE],&eventhandler_type);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATE],&state);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATETYPE],&state_type);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_TIMEOUT],&timeout);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EARLYTIMEOUT],&early_timeout);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_RETURNCODE],&return_code);
    result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_EXECUTIONTIME],&execution_time);
    result=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_STARTTIME],&start_time);
    result=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_ENDTIME],&end_time);

    es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_COMMANDARGS]);
    es[1]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_COMMANDLINE]);
    es[2]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_OUTPUT]);
    es[3]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_LONGOUTPUT]);

    ts[0]=ndo2db_db_timet_to_sql(idi,start_time.tv_sec);
    ts[1]=ndo2db_db_timet_to_sql(idi,end_time.tv_sec);

    /* get the object id */
    if(eventhandler_type==SERVICE_EVENTHANDLER || eventhandler_type==GLOBAL_SERVICE_EVENTHANDLER)
        result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_SERVICE,idi->buffered_input[NDO_DATA_HOST],idi->buffered_input[NDO_DATA_SERVICE],&object_id);
    else
        result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOST,idi->buffered_input[NDO_DATA_HOST],NULL,&object_id);

    /* get the command id */
    result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_COMMAND,idi->buffered_input[NDO_DATA_COMMANDNAME],NULL,&command_id);

    /* save entry to db */
    if(asprintf(&buf,"instance_id='%lu', eventhandler_type='%d', object_id='%lu', state='%d', state_type='%d', start_time=%s, start_time_usec='%lu', end_time=%s, 
        end_time_usec='%lu', command_object_id='%lu', command_args='%s', command_line='%s', timeout='%d', early_timeout='%d', execution_time='%lf', return_code='%d', output='%s', long_output='%s'"
            ,idi->dbinfo.instance_id
            ,eventhandler_type
            ,object_id
            ,state
            ,state_type
            ,ts[0]
            ,start_time.tv_usec
            ,ts[1]
            ,end_time.tv_usec
            ,command_id
            ,es[0]
            ,es[1]
            ,timeout
            ,early_timeout
            ,execution_time
            ,return_code
            ,es[2]
            ,es[3]
           )==-1)
        buf=NULL;

    if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
            ,ndo2db_db_tablenames[NDO2DB_DBTABLE_EVENTHANDLERS]
            ,buf
            ,buf
           )==-1)
        buf1=NULL;
}

NDOMOD_HANDLER_FUNCTION(notification_data)
{

        es[0] = ndo_escape_buffer(notdata->host_name);
        es[1] = ndo_escape_buffer(notdata->service_description);
        es[2] = ndo_escape_buffer(notdata->output);
        /* Preparing if notifications will have long_output in the future */
        es[3] = ndo_escape_buffer(notdata->output);
        es[4] = ndo_escape_buffer(notdata->ack_author);
        es[5] = ndo_escape_buffer(notdata->ack_data);

        {
            struct ndo_broker_data notification_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, notdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, notdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, notdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, notdata->timestamp),
                NDODATA_INTEGER(NDO_DATA_NOTIFICATIONTYPE, notdata->notification_type),
                NDODATA_TIMESTAMP(NDO_DATA_STARTTIME, notdata->start_time),
                NDODATA_TIMESTAMP(NDO_DATA_ENDTIME, notdata->end_time),
                NDODATA_STRING(NDO_DATA_HOST, es[0]),
                NDODATA_STRING(NDO_DATA_SERVICE, es[1]),
                NDODATA_INTEGER(NDO_DATA_NOTIFICATIONREASON, notdata->reason_type),
                NDODATA_INTEGER(NDO_DATA_STATE, notdata->state),
                NDODATA_STRING(NDO_DATA_OUTPUT, es[2]),
                NDODATA_STRING(NDO_DATA_LONGOUTPUT, es[3]),
                NDODATA_STRING(NDO_DATA_ACKAUTHOR, es[4]),
                NDODATA_STRING(NDO_DATA_ACKDATA, es[5]),
                NDODATA_INTEGER(NDO_DATA_ESCALATED, notdata->escalated),
                NDODATA_INTEGER(NDO_DATA_CONTACTSNOTIFIED, notdata->contacts_notified )
            };


    SET_SQL("
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
        ");


    /* convert vars */
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFICATIONTYPE],&notification_type);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_NOTIFICATIONREASON],&notification_reason);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATE],&state);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ESCALATED],&escalated);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CONTACTSNOTIFIED],&contacts_notified);

    result=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_STARTTIME],&start_time);
    result=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_ENDTIME],&end_time);

    es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_OUTPUT]);
    es[1]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_OUTPUT]);

    ts[0]=ndo2db_db_timet_to_sql(idi,start_time.tv_sec);
    ts[1]=ndo2db_db_timet_to_sql(idi,end_time.tv_sec);

    /* get the object id */
    if(notification_type==SERVICE_NOTIFICATION)
        result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_SERVICE,idi->buffered_input[NDO_DATA_HOST],idi->buffered_input[NDO_DATA_SERVICE],&object_id);
    if(notification_type==HOST_NOTIFICATION)
        result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOST,idi->buffered_input[NDO_DATA_HOST],NULL,&object_id);

    /* save entry to db */
    if(asprintf(&buf,"instance_id='%lu', notification_type='%d', notification_reason='%d', start_time=%s, start_time_usec='%lu', end_time=%s, end_time_usec='%lu',
     object_id='%lu', state='%d', output='%s', long_output='%s', escalated='%d', contacts_notified='%d'"
            ,idi->dbinfo.instance_id
            ,notification_type
            ,notification_reason
            ,ts[0]
            ,start_time.tv_usec
            ,ts[1]
            ,end_time.tv_usec
            ,object_id
            ,state
            ,es[0]
            ,es[1]
            ,escalated
            ,contacts_notified
           )==-1)
        buf=NULL;

    if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
            ,ndo2db_db_tablenames[NDO2DB_DBTABLE_NOTIFICATIONS]
            ,buf
            ,buf
           )==-1)
        buf1=NULL;

}

NDOMOD_HANDLER_FUNCTION(service_check_data)
{

        es[0] = ndo_escape_buffer(scdata->host_name);
        es[1] = ndo_escape_buffer(scdata->service_description);
        es[2] = ndo_escape_buffer(scdata->command_name);
        es[3] = ndo_escape_buffer(scdata->command_args);
        es[4] = ndo_escape_buffer(scdata->command_line);
        es[5] = ndo_escape_buffer(scdata->output);
        es[6] = ndo_escape_buffer(scdata->long_output);
        es[7] = ndo_escape_buffer(scdata->perf_data);

        {
            struct ndo_broker_data service_check_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, scdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, scdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, scdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, scdata->timestamp),
                NDODATA_STRING(NDO_DATA_HOST, es[0]),
                NDODATA_STRING(NDO_DATA_SERVICE, es[1]),
                NDODATA_INTEGER(NDO_DATA_CHECKTYPE, scdata->check_type),
                NDODATA_INTEGER(NDO_DATA_CURRENTCHECKATTEMPT, scdata->current_attempt),
                NDODATA_INTEGER(NDO_DATA_MAXCHECKATTEMPTS, scdata->max_attempts),
                NDODATA_INTEGER(NDO_DATA_STATETYPE, scdata->state_type),
                NDODATA_INTEGER(NDO_DATA_STATE, scdata->state),
                NDODATA_INTEGER(NDO_DATA_TIMEOUT, scdata->timeout),
                NDODATA_STRING(NDO_DATA_COMMANDNAME, es[2]),
                NDODATA_STRING(NDO_DATA_COMMANDARGS, es[3]),
                NDODATA_STRING(NDO_DATA_COMMANDLINE, es[4]),
                NDODATA_TIMESTAMP(NDO_DATA_STARTTIME, scdata->start_time),
                NDODATA_TIMESTAMP(NDO_DATA_ENDTIME, scdata->end_time),
                NDODATA_INTEGER(NDO_DATA_EARLYTIMEOUT, scdata->early_timeout),
                NDODATA_FLOATING_POINT(NDO_DATA_EXECUTIONTIME, scdata->execution_time),
                NDODATA_FLOATING_POINT(NDO_DATA_LATENCY, scdata->latency),
                NDODATA_INTEGER(NDO_DATA_RETURNCODE, scdata->return_code),
                NDODATA_STRING(NDO_DATA_OUTPUT, es[5]),
                NDODATA_STRING(NDO_DATA_LONGOUTPUT, es[6]),
                NDODATA_STRING(NDO_DATA_PERFDATA, es[7] )
            };


    SET_SQL("
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
                command_object_id     = ?,
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
        ");


    /* only process some types of service checks... */
    if(type!=NEBTYPE_SERVICECHECK_INITIATE && type!=NEBTYPE_SERVICECHECK_PROCESSED)
        return NDO_OK;

    /* skip precheck events - they aren't useful to us */
    if(type==NEBTYPE_SERVICECHECK_ASYNC_PRECHECK)
        return NDO_OK;

    /* covert vars */
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CHECKTYPE],&check_type);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CURRENTCHECKATTEMPT],&current_check_attempt);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_MAXCHECKATTEMPTS],&max_check_attempts);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATE],&state);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATETYPE],&state_type);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_TIMEOUT],&timeout);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EARLYTIMEOUT],&early_timeout);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_RETURNCODE],&return_code);
    result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_EXECUTIONTIME],&execution_time);
    result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_LATENCY],&latency);
    result=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_STARTTIME],&start_time);
    result=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_ENDTIME],&end_time);

    es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_COMMANDARGS]);
    es[1]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_COMMANDLINE]);
    es[2]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_OUTPUT]);
    es[3]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_LONGOUTPUT]);
    es[4]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_PERFDATA]);

    ts[0]=ndo2db_db_timet_to_sql(idi,start_time.tv_sec);
    ts[1]=ndo2db_db_timet_to_sql(idi,end_time.tv_sec);

    /* get the object id */
    result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_SERVICE,idi->buffered_input[NDO_DATA_HOST],idi->buffered_input[NDO_DATA_SERVICE],&object_id);

    /* get the command id */
    if(idi->buffered_input[NDO_DATA_COMMANDNAME]!=NULL && strcmp(idi->buffered_input[NDO_DATA_COMMANDNAME],""))
        result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_COMMAND,idi->buffered_input[NDO_DATA_COMMANDNAME],NULL,&command_id);
    else
        command_id=0L;

    /* save entry to db */
    if(asprintf(&buf1,"instance_id='%lu', service_object_id='%lu', check_type='%d', 
        current_check_attempt='%d', max_check_attempts='%d', state='%d', state_type='%d', start_time=%s, 
        start_time_usec='%lu', end_time=%s, end_time_usec='%lu', timeout='%d', 
        early_timeout='%d', execution_time='%lf', latency='%lf', return_code='%d', 
        output='%s', long_output='%s', perfdata='%s'"
            ,idi->dbinfo.instance_id
            ,object_id
            ,check_type
            ,current_check_attempt
            ,max_check_attempts
            ,state
            ,state_type
            ,ts[0]
            ,start_time.tv_usec
            ,ts[1]
            ,end_time.tv_usec
            ,timeout
            ,early_timeout
            ,execution_time
            ,latency
            ,return_code
            ,es[2]
            ,es[3]
            ,es[4]
           )==-1)
        buf1=NULL;

    if(asprintf(&buf,"INSERT INTO %s SET %s, command_object_id='%lu', command_args='%s', command_line='%s' ON DUPLICATE KEY UPDATE %s"
            ,ndo2db_db_tablenames[NDO2DB_DBTABLE_SERVICECHECKS]
            ,buf1
            ,command_id
            ,es[0]
            ,es[1]
            ,buf1
           )==-1)
        buf=NULL;

}

// TODO: remove the is_raw specification from the tables
NDOMOD_HANDLER_FUNCTION(host_check_data)
{
        /* Nagios XI MOD */
        /* send only the data we really use */
        if (hcdata->type != NEBTYPE_HOSTCHECK_PROCESSED) {
            break;
        }

        es[0] = ndo_escape_buffer(hcdata->host_name);
        es[1] = ndo_escape_buffer(hcdata->command_name);
        es[2] = ndo_escape_buffer(hcdata->command_args);
        es[3] = ndo_escape_buffer(hcdata->command_line);
        es[4] = ndo_escape_buffer(hcdata->output);
        es[5] = ndo_escape_buffer(hcdata->long_output);
        es[6] = ndo_escape_buffer(hcdata->perf_data);

        {
            struct ndo_broker_data host_check_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, hcdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, hcdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, hcdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, hcdata->timestamp),
                NDODATA_STRING(NDO_DATA_HOST, es[0]),
                NDODATA_INTEGER(NDO_DATA_CHECKTYPE, hcdata->check_type),
                NDODATA_INTEGER(NDO_DATA_CURRENTCHECKATTEMPT, hcdata->current_attempt),
                NDODATA_INTEGER(NDO_DATA_MAXCHECKATTEMPTS, hcdata->max_attempts),
                NDODATA_INTEGER(NDO_DATA_STATETYPE, hcdata->state_type),
                NDODATA_INTEGER(NDO_DATA_STATE, hcdata->state),
                NDODATA_INTEGER(NDO_DATA_TIMEOUT, hcdata->timeout),
                NDODATA_STRING(NDO_DATA_COMMANDNAME, es[1]),
                NDODATA_STRING(NDO_DATA_COMMANDARGS, es[2]),
                NDODATA_STRING(NDO_DATA_COMMANDLINE, es[3]),
                NDODATA_TIMESTAMP(NDO_DATA_STARTTIME, hcdata->start_time),
                NDODATA_TIMESTAMP(NDO_DATA_ENDTIME, hcdata->end_time),
                NDODATA_INTEGER(NDO_DATA_EARLYTIMEOUT, hcdata->early_timeout),
                NDODATA_FLOATING_POINT(NDO_DATA_EXECUTIONTIME, hcdata->execution_time),
                NDODATA_FLOATING_POINT(NDO_DATA_LATENCY, hcdata->latency),
                NDODATA_INTEGER(NDO_DATA_RETURNCODE, hcdata->return_code),
                NDODATA_STRING(NDO_DATA_OUTPUT, es[4]),
                NDODATA_STRING(NDO_DATA_LONGOUTPUT, es[5]),
                NDODATA_STRING(NDO_DATA_PERFDATA, es[6] )
            };

    SET_SQL("
            INSERT INTO
                nagios_hostchecks
            SET
                instance_id           = 1,
                start_time            = FROM_UNIXTIME(?),
                start_time_usec       = ?,
                end_time              = FROM_UNIXTIME(?),
                end_time_usec         = ?,
                host_object_id        = ?,
                is_raw_check          = ?,
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
                command_object_id     = ?,
                command_args          = ?,
                command_line          = ?
            ON DUPLICATE KEY UPDATE
                instance_id           = 1,
                start_time            = FROM_UNIXTIME(?),
                start_time_usec       = ?,
                end_time              = FROM_UNIXTIME(?),
                end_time_usec         = ?,
                host_object_id        = ?,
                is_raw_check          = ?,
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
        ");

    /* skip precheck events - they aren't useful to us */
    if(type==NEBTYPE_HOSTCHECK_ASYNC_PRECHECK || type==NEBTYPE_HOSTCHECK_SYNC_PRECHECK)
        return NDO_OK;

    /* covert vars */
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CHECKTYPE],&check_type);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_CURRENTCHECKATTEMPT],&current_check_attempt);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_MAXCHECKATTEMPTS],&max_check_attempts);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATE],&state);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_STATETYPE],&state_type);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_TIMEOUT],&timeout);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EARLYTIMEOUT],&early_timeout);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_RETURNCODE],&return_code);
    result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_EXECUTIONTIME],&execution_time);
    result=ndo2db_convert_string_to_double(idi->buffered_input[NDO_DATA_LATENCY],&latency);
    result=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_STARTTIME],&start_time);
    result=ndo2db_convert_string_to_timeval(idi->buffered_input[NDO_DATA_ENDTIME],&end_time);

    es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_COMMANDARGS]);
    es[1]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_COMMANDLINE]);
    es[2]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_OUTPUT]);
    es[3]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_LONGOUTPUT]);
    es[4]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_PERFDATA]);

    ts[0]=ndo2db_db_timet_to_sql(idi,start_time.tv_sec);
    ts[1]=ndo2db_db_timet_to_sql(idi,end_time.tv_sec);

    /* get the object id */
    result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOST,idi->buffered_input[NDO_DATA_HOST],NULL,&object_id);

    /* get the command id */
    if(idi->buffered_input[NDO_DATA_COMMANDNAME]!=NULL && strcmp(idi->buffered_input[NDO_DATA_COMMANDNAME],""))
        result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_COMMAND,idi->buffered_input[NDO_DATA_COMMANDNAME],NULL,&command_id);
    else
        command_id=0L;

    /* is this a raw check? */
    if(type==NEBTYPE_HOSTCHECK_RAW_START || type==NEBTYPE_HOSTCHECK_RAW_END)
        is_raw_check=1;
    else
        is_raw_check=0;

    /* save entry to db */
    if(asprintf(&buf1,"instance_id='%lu', host_object_id='%lu', 
        check_type='%d', is_raw_check='%d', current_check_attempt='%d', 
        max_check_attempts='%d', state='%d', state_type='%d', 
        start_time=%s, start_time_usec='%lu', end_time=%s, 
        end_time_usec='%lu', timeout='%d', early_timeout='%d', 
        execution_time='%lf', latency='%lf', return_code='%d', output='%s', long_output='%s', perfdata='%s'"
            ,idi->dbinfo.instance_id
            ,object_id
            ,check_type
            ,is_raw_check
            ,current_check_attempt
            ,max_check_attempts
            ,state
            ,state_type
            ,ts[0]
            ,start_time.tv_usec
            ,ts[1]
            ,end_time.tv_usec
            ,timeout
            ,early_timeout
            ,execution_time
            ,latency
            ,return_code
            ,es[2]
            ,es[3]
            ,es[4]
           )==-1)
        buf1=NULL;

    if(asprintf(&buf,"INSERT INTO %s SET %s, command_object_id='%lu', command_args='%s', command_line='%s' ON DUPLICATE KEY UPDATE %s"
            ,ndo2db_db_tablenames[NDO2DB_DBTABLE_HOSTCHECKS]
            ,buf1
            ,command_id
            ,es[0]
            ,es[1]
            ,buf1
           )==-1)
        buf=NULL;
}

NDOMOD_HANDLER_FUNCTION(comment_data)
{

        es[0] = ndo_escape_buffer(comdata->host_name);
        es[1] = ndo_escape_buffer(comdata->service_description);
        es[2] = ndo_escape_buffer(comdata->author_name);
        es[3] = ndo_escape_buffer(comdata->comment_data);

        {
            struct ndo_broker_data comment_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, comdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, comdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, comdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, comdata->timestamp),
                NDODATA_INTEGER(NDO_DATA_COMMENTTYPE, comdata->comment_type),
                NDODATA_STRING(NDO_DATA_HOST, es[0]),
                NDODATA_STRING(NDO_DATA_SERVICE, es[1]),
                NDODATA_UNSIGNED_LONG(NDO_DATA_ENTRYTIME, (unsigned long)comdata->entry_time),
                NDODATA_STRING(NDO_DATA_AUTHORNAME, es[2]),
                NDODATA_STRING(NDO_DATA_COMMENT, es[3]),
                NDODATA_INTEGER(NDO_DATA_PERSISTENT, comdata->persistent),
                NDODATA_INTEGER(NDO_DATA_SOURCE, comdata->source),
                NDODATA_INTEGER(NDO_DATA_ENTRYTYPE, comdata->entry_type),
                NDODATA_INTEGER(NDO_DATA_EXPIRES, comdata->expires),
                NDODATA_UNSIGNED_LONG(NDO_DATA_EXPIRATIONTIME, (unsigned long)comdata->expire_time),
                NDODATA_UNSIGNED_LONG(NDO_DATA_COMMENTID, comdata->comment_id )
            };

    SET_SQL("
            INSERT INTO
                nagios_commenthistory
            SET
                instance_id         = 1,
                comment_type        = ?,
                entry_type          = ?,
                object_id           = ?,
                comment_time        = ?,
                internal_comment_id = ?,
                author_name         = ?,
                comment_data        = ?,
                is_persistent       = ?,
                comment_source      = ?,
                expires             = ?,
                expiration_time     = ?,
                entry_time          = ?,
                entry_time_usec     = ?
            ON DUPLICATE KEY UPDATE
                instance_id         = 1,
                comment_type        = ?,
                entry_type          = ?,
                object_id           = ?,
                comment_time        = ?,
                internal_comment_id = ?,
                author_name         = ?,
                comment_data        = ?,
                is_persistent       = ?,
                comment_source      = ?,
                expires             = ?,
                expiration_time     = ?
        ");

    /* convert vars */
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_COMMENTTYPE],&comment_type);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_ENTRYTYPE],&entry_type);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_PERSISTENT],&is_persistent);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_SOURCE],&comment_source);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_EXPIRES],&expires);

    result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_COMMENTID],&internal_comment_id);
    result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_ENTRYTIME],&comment_time);
    result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_EXPIRATIONTIME],&expire_time);

    es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_AUTHORNAME]);
    es[1]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_COMMENT]);

    ts[0]=ndo2db_db_timet_to_sql(idi,tstamp.tv_sec);
    ts[1]=ndo2db_db_timet_to_sql(idi,comment_time);
    ts[2]=ndo2db_db_timet_to_sql(idi,expire_time);

    /* get the object id */
    if(comment_type==SERVICE_COMMENT)
        result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_SERVICE,idi->buffered_input[NDO_DATA_HOST],idi->buffered_input[NDO_DATA_SERVICE],&object_id);
    if(comment_type==HOST_COMMENT)
        result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOST,idi->buffered_input[NDO_DATA_HOST],NULL,&object_id);

    /* ADD HISTORICAL COMMENTS */
    /* save a record of comments that get added (or get loaded and weren't previously recorded).... */
    if(type==NEBTYPE_COMMENT_ADD || type==NEBTYPE_COMMENT_LOAD){

        /* save entry to db */
        if(asprintf(&buf,"instance_id='%lu', comment_type='%d', entry_type='%d', object_id='%lu', comment_time=%s,
         internal_comment_id='%lu', author_name='%s', comment_data='%s', is_persistent='%d', comment_source='%d', expires='%d', expiration_time=%s"
                ,idi->dbinfo.instance_id
                ,comment_type
                ,entry_type
                ,object_id
                ,ts[1]
                ,internal_comment_id
                ,es[0]
                ,es[1]
                ,is_persistent
                ,comment_source
                ,expires
                ,ts[2]
               )==-1)
            buf=NULL;

        if(asprintf(&buf1,"INSERT INTO %s SET %s, entry_time=%s, entry_time_usec='%lu' ON DUPLICATE KEY UPDATE %s"
                ,ndo2db_db_tablenames[NDO2DB_DBTABLE_COMMENTHISTORY]
                ,buf
                ,ts[0]
                ,tstamp.tv_usec
                ,buf
               )==-1)
            buf1=NULL;
}

NDOMOD_HANDLER_FUNCTION(downtime_data)
{

        es[0] = ndo_escape_buffer(downdata->host_name);
        es[1] = ndo_escape_buffer(downdata->service_description);
        es[2] = ndo_escape_buffer(downdata->author_name);
        es[3] = ndo_escape_buffer(downdata->comment_data);

        {
            struct ndo_broker_data downtime_data[] = {
                NDODATA_INTEGER(NDO_DATA_TYPE, downdata->type),
                NDODATA_INTEGER(NDO_DATA_FLAGS, downdata->flags),
                NDODATA_INTEGER(NDO_DATA_ATTRIBUTES, downdata->attr),
                NDODATA_TIMESTAMP(NDO_DATA_TIMESTAMP, downdata->timestamp),
                NDODATA_INTEGER(NDO_DATA_DOWNTIMETYPE, downdata->downtime_type),
                NDODATA_STRING(NDO_DATA_HOST, es[0]),
                NDODATA_STRING(NDO_DATA_SERVICE, es[1]),
                NDODATA_UNSIGNED_LONG(NDO_DATA_ENTRYTIME, (unsigned long)downdata->entry_time),
                NDODATA_STRING(NDO_DATA_AUTHORNAME, es[2]),
                NDODATA_STRING(NDO_DATA_COMMENT, es[3]),
                NDODATA_UNSIGNED_LONG(NDO_DATA_STARTTIME, (unsigned long)downdata->start_time),
                NDODATA_UNSIGNED_LONG(NDO_DATA_ENDTIME, (unsigned long)downdata->end_time),
                NDODATA_INTEGER(NDO_DATA_FIXED, downdata->fixed),
                NDODATA_UNSIGNED_LONG(NDO_DATA_DURATION, (unsigned long)downdata->duration),
                NDODATA_UNSIGNED_LONG(NDO_DATA_TRIGGEREDBY, (unsigned long)downdata->triggered_by),
                NDODATA_UNSIGNED_LONG(NDO_DATA_DOWNTIMEID, (unsigned long)downdata->downtime_id),
            };

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");


    /* convert vars */
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_DOWNTIMETYPE],&downtime_type);
    result=ndo2db_convert_string_to_int(idi->buffered_input[NDO_DATA_FIXED],&fixed);

    result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_DURATION],&duration);
    result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_DOWNTIMEID],&internal_downtime_id);
    result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_TRIGGEREDBY],&triggered_by);
    result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_ENTRYTIME],&entry_time);
    result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_STARTTIME],&start_time);
    result=ndo2db_convert_string_to_unsignedlong(idi->buffered_input[NDO_DATA_ENDTIME],&end_time);

    es[0]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_AUTHORNAME]);
    es[1]=ndo2db_db_escape_string(idi,idi->buffered_input[NDO_DATA_COMMENT]);

    ts[0]=ndo2db_db_timet_to_sql(idi,tstamp.tv_sec);
    ts[1]=ndo2db_db_timet_to_sql(idi,entry_time);
    ts[2]=ndo2db_db_timet_to_sql(idi,start_time);
    ts[3]=ndo2db_db_timet_to_sql(idi,end_time);

    /* get the object id */
    if(downtime_type==SERVICE_DOWNTIME)
        result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_SERVICE,idi->buffered_input[NDO_DATA_HOST],idi->buffered_input[NDO_DATA_SERVICE],&object_id);
    if(downtime_type==HOST_DOWNTIME)
        result=ndo2db_get_object_id_with_insert(idi,NDO2DB_OBJECTTYPE_HOST,idi->buffered_input[NDO_DATA_HOST],NULL,&object_id);

    /* HISTORICAL DOWNTIME */

    /* save a record of scheduled downtime that gets added (or gets loaded and wasn't previously recorded).... */
    if(type==NEBTYPE_DOWNTIME_ADD || type==NEBTYPE_DOWNTIME_LOAD){

        /* save entry to db */
        if(asprintf(&buf,"instance_id='%lu', downtime_type='%d', object_id='%lu', entry_time=%s, author_name='%s', comment_data='%s', internal_downtime_id='%lu', triggered_by_id='%lu', is_fixed='%d', duration='%lu', scheduled_start_time=%s, scheduled_end_time=%s"
                ,idi->dbinfo.instance_id
                ,downtime_type
                ,object_id
                ,ts[1]
                ,es[0]
                ,es[1]
                ,internal_downtime_id
                ,triggered_by
                ,fixed
                ,duration
                ,ts[2]
                ,ts[3]
               )==-1)
            buf=NULL;

        if(asprintf(&buf1,"INSERT INTO %s SET %s ON DUPLICATE KEY UPDATE %s"
                ,ndo2db_db_tablenames[NDO2DB_DBTABLE_DOWNTIMEHISTORY]
                ,buf
                ,buf
               )==-1)
            buf1=NULL;


}

NDOMOD_HANDLER_FUNCTION()
{
    

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
}


NDOMOD_HANDLER_FUNCTION()
{
    

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
}


NDOMOD_HANDLER_FUNCTION()
{
    

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
}


NDOMOD_HANDLER_FUNCTION()
{
    

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
}


NDOMOD_HANDLER_FUNCTION()
{
    

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
}

NDOMOD_HANDLER_FUNCTION()
{
    

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
}


NDOMOD_HANDLER_FUNCTION()
{
    

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}

NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}

NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}

NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}

NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}

NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}

NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}

NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}


NDOMOD_HANDLER_FUNCTION()
{

    SET_SQL("
            INSERT INTO
                nagios_
            SET
                instance_id = 1,
            ON DUPLICATE KEY UPDATE
                instance_id = 1,
            ");
    
}