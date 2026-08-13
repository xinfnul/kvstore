#ifndef KVSTORE_COMMAND_H
#define KVSTORE_COMMAND_H

#include <stddef.h>
#include <stdint.h>

#include <kvstore/database.h>
#include <kvstore/protocol.h>

/*
 * Execute a decoded protocol request against the database.
 *
 * The request must have been successfully decoded by the protocol layer.
 *
 * The response is written into the supplied protocol_response_t;
 *
 * For GET:
 * 	- PROTOCOL_STATUS_OK:
 * 		response->data points to the value owned by the database.
 * 	- PROTOCOL_STATUS_NOT_FOUND:
 * 		response->data is NULL.
 *
 * For SET and DEL:
 * 	response->data is NULL.
 *
 * THe response does not own response->data. THe database owns the
 * returned value, and it remains valid according to database_get().
 *
 * Returns:
 * 	0 on successful command execution.
 * -1 on invalid arguments or internal failure.
 */
int command_execute(database_t *db, const protocol_request_t *request,
                    protocol_response_t *response);

#endif
