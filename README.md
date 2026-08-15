# kvstore

A small TCP key-value store, written in C, with a write-ahead log for
durability. Built as a layered exercise in systems design: network
I/O, a custom binary wire protocol, command dispatch, an in-memory
store, and crash-durable persistence, each isolated behind its own
header.

Supports three operations: `SET`, `GET`, `DEL`.

## Architecture

```
                ┌──────────────────────┐
                │       Client         │
                │  CLI / custom client │
                └──────────┬───────────┘
                           │
                        TCP/IP
                           │
                           ▼
                ┌──────────────────────┐
                │     KVStore Server   │
                │                      │
                │   Network Layer      │
                │          │           │
                │   Protocol Layer     │
                │          │           │
                │   Command Layer      │
                │          │           │
                │   Database Engine    │
                │          │           │
                │   In-Memory Store    │
                └──────────┬───────────┘
                           │
                           ▼
                ┌──────────────────────┐
                │ Persistent Storage   │
                │      / WAL           │
                └──────────────────────┘
```

Each layer only knows about the one below it:

| Layer      | Files                                    | Responsibility                                             |
|------------|-------------------------------------------|--------------------------------------------------------------|
| Network    | `src/network.c`, `include/network.h`       | Accept connections, connect out, send/receive raw bytes.     |
| Protocol   | `src/protocol.c`, `include/protocol.h`     | Encode/decode the binary wire format.                        |
| Command    | `src/command.c`, `include/command.h`       | Interpret a decoded request as SET/GET/DEL and run it.       |
| Database   | `src/database.c`, `include/database.h`     | In-memory key-value store, backed by the WAL.                |
| Storage    | `src/hashmap.c`, `include/hashmap.h`, `src/wal.c`, `include/wal.h` | Hash map for in-memory lookups; append-only write-ahead log (`wal.c`/`wal.h`) for crash durability — every SET/DEL is written and `fsync`'d to the WAL before the in-memory map is updated, and replayed on startup. |
| Server     | `server/`                                  | Wires network + protocol + command + database together; the listening process. |
| Client     | `client/`                                  | A library + CLI that speaks the same protocol as a client.   |

The server is intentionally **single-threaded and blocking**: it
accepts one connection, serves requests on it sequentially until the
client disconnects, then accepts the next one. There is no
concurrency, no thread pool, no async I/O.

## Wire protocol

All multi-byte integers are big-endian.

**Request:**

```
version       1 byte
type          1 byte   (1=SET, 2=GET, 3=DEL)
key_length    4 bytes
key           key_length bytes
value_length  4 bytes  (0 for GET/DEL)
value         value_length bytes
```

**Response:**

```
version       1 byte
status        1 byte   (0=OK, 1=NOT_FOUND, 2=INVALID, 3=ERROR)
data_length   4 bytes
data          data_length bytes
```

Max key size: 1 MiB. Max value size: 64 MiB (`PROTOCOL_MAX_KEY_SIZE` /
`PROTOCOL_MAX_VALUE_SIZE` in `include/protocol.h`).

Because this rides over a raw TCP stream, a single `recv()` can
return fewer bytes than a full message. Both `server/server.c` and
`client/client.c` implement a `read_full()`-style helper that loops
on `network_receive()` until the exact number of bytes needed has
arrived, before handing the buffer to `protocol_decode_request()` /
`protocol_decode_response()`.

## Building

Requires a C11 compiler and [Meson](https://mesonbuild.com/).

```sh
meson setup build
ninja -C build
```

This produces two executables under `build/`:

- `server/kvstore-server`
- `client/kvstore-client`

Run the test suite with:

```sh
meson test -C build
```

## Running the server

```sh
kvstore-server [wal_path] [port] [host]
```

All arguments are optional and positional:

| Arg        | Default        | Meaning                                    |
|------------|----------------|----------------------------------------------|
| `wal_path` | `kvstore.wal`  | Path to the write-ahead log file. Replayed on startup if it already contains records. |
| `port`     | `6363`         | TCP port to listen on.                        |
| `host`     | all interfaces | Local address to bind to.                     |

```sh
kvstore-server data.wal 6969 127.0.0.1
```

The server logs connect/disconnect events and WAL errors to stderr.
It runs forever, handling one connection at a time.

## Running the client

```sh
kvstore-client <host> [port] [SET <key> <value...> | GET <key> | DEL <key>]
```

One-shot usage:

```sh
kvstore-client 127.0.0.1 6969 SET name kvstore
# OK

kvstore-client 127.0.0.1 6969 GET name
# kvstore

kvstore-client 127.0.0.1 6969 DEL name
# OK

kvstore-client 127.0.0.1 6969 GET name
# NOT_FOUND
```

Any extra arguments after `<key>` for `SET` are joined with spaces
into the value, so a multi-word value doesn't need shell quoting.

If no command is given, the client drops into an interactive REPL:

```sh
$ kvstore-client 127.0.0.1 6969
kvstore-client connected. Commands: SET <key> <value>, GET <key>, DEL <key>, QUIT
> SET name kvstore rocks
OK
> GET name
kvstore rocks
> DEL name
OK
> QUIT
```

### Using `client.h` as a library

```c
#include <kvstore/client.h>

kvstore_client_t *client = client_connect("127.0.0.1", 6363);

protocol_status_t status;
client_set(client, (const uint8_t *)"k", 1, (const uint8_t *)"v", 1, &status);

uint8_t *value = NULL;
uint32_t value_length = 0;
client_get(client, (const uint8_t *)"k", 1, &status, &value, &value_length);
// status == PROTOCOL_STATUS_OK, value == "v", free(value) when done

client_disconnect(client);
```

## Testing

`tests/` uses Meson's test runner and covers each layer in isolation:

- `test_hashmap.c`, `test_wal.c`, `test_database.c`, `test_protocol.c`,
  `test_command.c`, `test_network.c` — unit tests per layer.

## Known problems / limitations

- **No concurrency.** One connection at a time, one request at a
  time — a slow or stalled client blocks every other client.
- **Values can't contain NUL bytes.** Storage is NUL-terminated C
  strings internally, so a `\0` mid-value silently truncates it, even
  though the wire protocol advertises binary-safe values up to 64 MiB.
- **WAL never compacts.** Strictly append-only — repeated SET/DEL on
  the same key just keeps growing the file and slows down replay.
- **No auth or encryption.** Anyone who can reach the port has full
  read/write access, in plaintext.
- **No timeouts or graceful shutdown.** A client that never finishes
  a request hangs the server forever; there's no signal handling to
  drain and exit cleanly.
- **Fatal accept errors.** A single failed `accept()` exits the whole
  server instead of retrying.
- **Noisy logging.** Every SET logs its full value to stderr
  unconditionally — no log levels, no way to turn it off.
