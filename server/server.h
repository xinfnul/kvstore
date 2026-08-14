#ifndef KVSTORE_SERVER_H
#define KVSTORE_SERVER_H

#include <stdint.h>

typedef struct server server_t;

/*
 * Create a server.
 *
 * host:
 * 	Local address to bind to. NULL means all local interfaces.
 *
 * port:
 * 	TCP port in host byte order.
 *
 * wal_path:
 * 	Path to WAL file backing the database. If it already
 * 	contains records, they are replayed on open.
 *
 * Returns:
 * 	Allocated server on success.
 * 	NULL on failure ( bad arguments, bind failure, WAL failure, ... )
 */
server_t *server_create(const char *host, uint16_t port, const char *wal_path);

/*
 * Run the server.
 *
 * This is a blocking call that never returns under normal operation.
 * The server accepts one connection at a time and, once accepted,
 * serves requests on that connection sequentially until the client
 * disconnects or a network error occurs, then goes back to accpeting
 * the next connection.
 *
 * Returns:
 * 	-1 is the server could not continue accpeting connection
 *
 * Does not return on success; the loop runs forever.
 */
int server_run(server_t *server);

/*
 * Stop ( if needed ) and release all resources held by the server,
 * including the underlying database and listening socket.
 */
void server_destroy(server_t *server);

#endif
