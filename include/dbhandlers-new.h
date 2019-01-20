
#ifndef NDO2DB_DBHANDLERS_NEW_H_INCLUDED
#define NDO2DB_DBHANDLERS_NEW_H_INCLUDED

#include "ndo2db.h"
#include "../include/nagios/nebstructs.h"


int ndomod_get_object_id(int insert, int object_type, char * name1, char * name2);
void ndomod_clear_tables();
void ndomod_set_all_objects_as_inactive();

void ndomod_handle_##type(nebstruct_##type * data)

void ndo2db_handle_process_data(nebstruct_process_data * data);
void ndo2db_handle_log_data(nebstruct_log_data * data);
void ndo2db_handle_system_command_data(nebstruct_system_command_data * data);
void ndo2db_handle_event_handler_data(nebstruct_event_handler_data * data);
void ndo2db_handle_notification_data(nebstruct_notification_data * data);
void ndo2db_handle_service_check_data(nebstruct_service_check_data * data);
void ndo2db_handle_host_check_data(nebstruct_host_check_data * data);
void ndo2db_handle_comment_data(nebstruct_comment_data * data);
void ndo2db_handle_downtime_data(nebstruct_downtime_data * data);
void ndo2db_handle_timed_event_data(nebstruct_timed_event_data * data);
void ndo2db_handle_flapping_data(nebstruct_flapping_data * data);
void ndo2db_handle_program_status_data(nebstruct_program_status_data * data);
void ndo2db_handle_host_status_data(nebstruct_host_status_data * data);
void ndo2db_handle_service_status_data(nebstruct_service_status_data * data);
void ndo2db_handle_contact_status_data(nebstruct_contact_status_data * data);
void ndo2db_handle_external_command_data(nebstruct_external_command_data * data);
void ndo2db_handle_contact_notification_data(nebstruct_contact_notification_data * data);
void ndo2db_handle_contact_notification_method_data(nebstruct_contact_notification_method_data * data);
void ndo2db_handle_acknowledgement_data(nebstruct_acknowledgement_data * data);
void ndo2db_handle_statechange_data(nebstruct_statechange_data * data);

#endif