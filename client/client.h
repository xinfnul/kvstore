#ifndef KVSTORE_CLIENT_H
#define KVSTORE_CLIENT_H

#include <kvstore/protocol.h>

#include <stdint.h>

typedef struct client client_t;

/*
 * Connect to a kvstore server.
 *
 * host:
 * 	Remote hostname or IPv4/IPv6 address.
 *
 * port:
 * 	TCP port in host byte order.
 *
 * Returns:
 * 	Allocated, connected client on success.
 * 	NULL on failure.
 */
client_t *client_connect(const char *host, uint16_t port);

/*
 * SET key = value.
 *
 * key/value must be non-NULL is their respective length is non-zero.
 * value/value_length may be zero-length ( an empty value is valid ).
 *
 * On success, `*out_status` holds the server's response status
 * ( normally PROTOCOL_STATUS_OK ).
 *
 * Returns:
 * 	0 on success.
 * -1 on network or protocol failure; *out_status is untouched.
 */
int client_set(client_t *client, const uint8_t *key, uint32_t key_length,
               const uint8_t *value, uint32_t value_length,
               protocol_status_t *out_status);

/*
 * GET key.
 *
 * On success with PROTOCOL_STATUS_OK, *out_value is a newly malloc'd
 * buffer the caller must free(), and *out_value_length is its size
 * ( may be 0 for an empty value; *out_value is still a valid pointer
 * to a zero-length allocation in that case, never NULL, so the
 * caller can free() it unconditionally after a successful OK GET ).
 *
 * On any other status ( NOT_FOUND, INVALID, ERROR ), *out_value is set
 * to NULL and *out_length is 0; nothing needs to be freed.
 *
 * Returns:
 * 	0 on success.
 * -1 on network or protocol failure; *out_status is untouched.
 */
int client_get(client_t *client, const uint8_t *key, uint32_t key_length,
               protocol_status_t *out_status, uint8_t **out_value,
               uint32_t *out_value_length);

/*
 * DEL key.
 *
 * On success, *out_status holds the server's response status
 * ( PROTOCOL_STATUS_OK if it is existed, PROTOCOL_STATUS_NOT_FOUND
 * otherwise. )
 *
 * Returns:
 * 	0 on success.
 * -1 on network or protocol failure; *out_status is untouched.
 */
int client_del(client_t *client, const uint8_t *key, uint32_t key_length,
               protocol_status_t *out_status);

/*
 * A human-readable string describing the last
 * client_connect/client_set/client_get/client_del call on THIS
 * client_failed. Only meaningful immediately after a call that
 * returned -1.
 */
const char *client_last_error_string(const client_t *client);

/*
 * Close the connection and release the client.
 * Safe to call with NULL.
 */
void client_disconnect(client_t *client);

#endif
