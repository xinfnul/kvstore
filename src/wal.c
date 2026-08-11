#include "wal.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define WAL_MAGIC 0x57414C31u // "WAL1"

#define WAL_HEADER_SIZE 17u
#define WAL_MAX_KEY_SIZE (1024u * 1024u)
#define WAL_MAX_VALUE_SIZE (64u * 1024u * 1024u)

struct wal {
  int fd;
};

/*
 * WAL record:
 *
 * 	magic			4 bytes
 * 	operation		1 byte
 * 	key_length		4 bytes
 * 	value_length	4 bytes
 * 	reserved 		4 bytes
 * 	key 			key_length bytes
 * 	value 			value_length bytes
 *
 * All integer fields are stored in big-edian order.
 */

static void encode_u32(uint8_t *buffer, uint32_t value) {
  buffer[0] = (uint8_t)(value >> 24);
  buffer[1] = (uint8_t)(value >> 16);
  buffer[2] = (uint8_t)(value >> 8);
  buffer[3] = (uint8_t)value;
}

static uint32_t decode_u32(const uint8_t *buffer) {
  return ((uint32_t)buffer[0] << 24) | ((uint32_t)buffer[1] << 16) |
         ((uint32_t)buffer[2] << 8) | (uint32_t)buffer[3];
}

static bool write_all(int fd, const void *buffer, size_t length) {
  const uint8_t *data = buffer;

  while (length > 0) {
    ssize_t written = write(fd, data, length);

    if (written < 0) {
      if (errno == EINTR) {
        continue;
      }

      return false;
    }

    if (written == 0) {
      return false;
    }

    data += (size_t)written;
    length -= (size_t)written;
  }

  return true;
}

static bool read_all(int fd, void *buffer, size_t length) {
  uint8_t *data = buffer;

  while (length > 0) {
    size_t read_count = read(fd, data, length);

    if (read_count < 0) {
      if (errno == EINTR) {
        continue;
      }

      return false;
    }

    if (read_count == 0) {
      return false;
    }

    data += (size_t)read_count;
    length -= (size_t)read_count;
  }

  return true;
}

static bool wal_append(wal_t *wal, wal_operation_t operation, const char *key,
                       size_t key_length, const char *value,
                       size_t value_length) {
  if (wal == NULL || key == NULL) {
    return false;
  }

  if (operation != WAL_OP_SET && operation != WAL_OP_DELETE) {
    return false;
  }

  if (key_length == 0 || key_length > WAL_MAX_KEY_SIZE) {
    return false;
  }

  if (value_length == 0 || value_length > WAL_MAX_VALUE_SIZE) {
    return false;
  }

  uint8_t header[WAL_HEADER_SIZE];

  encode_u32(header, WAL_MAGIC);

  header[4] = (uint8_t)operation;

  encode_u32(header + 5, (uint32_t)key_length);
  encode_u32(header + 9, (uint32_t)value_length);

  /*
   * Reserved field.
   * Kept for future WAL format extensions.
   */
  encode_u32(header + 13, 0);

  if (!write_all(wal->fd, header, sizeof(header))) {
    return false;
  }

  if (!write_all(wal->fd, key, key_length)) {
    return false;
  }

  if (value_length > 0 && value != NULL) {
    if (!write_all(wal->fd, value, value_length)) {
      return false;
    }
  }

  /*
   * Durability boundary:
   * don't report success until the WAL has reached stable storage.
   */
  if (fsync(wal->fd) != 0) {
    return false;
  }

  return true;
}

// ------------------------------------------------------------------------------

wal_t *wal_open(const char *path) {
  if (path == NULL) {
    return NULL;
  }

  int fd = open(path, O_RDWR | O_CREAT | O_APPEND, 0644);

  if (fd < 0) {
    return NULL;
  }

  wal_t *wal = malloc(sizeof(*wal));

  if (wal == NULL) {
    close(fd);
    return NULL;
  }

  wal->fd = fd;

  return wal;
}

void wal_close(wal_t *wal) {
  if (wal == NULL) {
    return;
  }

  close(wal->fd);
  free(wal);
}

bool wal_set(wal_t *wal, const char *key, size_t key_length, const char *value,
             size_t value_length) {
  if (value == NULL || value_length == 0) {
    return false;
  }

  return wal_append(wal, WAL_OP_SET, key, key_length, value, value_length);
}

bool wal_delete(wal_t *wal, const char *key, size_t key_length) {
  return wal_append(wal, WAL_OP_DELETE, key, key_length, NULL, 0);
}

bool wal_replay(wal_t *wal, wal_replay_callback callback, void *user_data) {
  if (wal == NULL || callback == NULL) {
    return false;
  }

  /*
   * Save the current append position.
   * We don't want replay to interfere with future writes.
   */
  if (lseek(wal->fd, 0, SEEK_SET) == (off_t)-1) {
    return false;
  }

  uint8_t header[WAL_HEADER_SIZE];

  for (;;) {
    ssize_t first_byte = read(wal->fd, header, 1);

    if (first_byte < 0) {
      if (errno == EINTR) {
        continue;
      }

      return false;
    }

    /*
     * Clean EOF: there are no more records
     */
    if (first_byte == 0) {
      break;
    }

    /*
     * We alredy consumed one byte.
     * Read the remaining header.
     */
    if (!read_all(wal->fd, header + 1, WAL_HEADER_SIZE - 1)) {
      return false;
    }

    uint32_t magic = decode_u32(header);

    if (magic != WAL_MAGIC) {
      return false;
    }

    wal_operation_t operation = (wal_operation_t)header[4];

    if (operation != WAL_OP_SET && operation != WAL_OP_DELETE) {
      return false;
    }

    uint32_t key_length = decode_u32(header + 5);
    uint32_t value_length = decode_u32(header + 9);
    uint32_t reserved = decode_u32(header + 13);

    if (reserved != 0) {
      return false;
    }

    if (key_length == 0 || key_length > WAL_MAX_KEY_SIZE) {
      return false;
    }

    if (value_length > WAL_MAX_VALUE_SIZE) {
      return false;
    }

    if (operation == WAL_OP_DELETE && value_length != 0) {
      return false;
    }

    char *key = malloc((size_t)key_length);

    if (key == NULL) {
      return false;
    }

    if (!read_all(wal->fd, key, key_length)) {
      free(key);
      return false;
    }

    char *value = NULL;

    if (value_length > 0) {
      value = malloc((size_t)value_length);

      if (value == NULL) {
        free(key);
        return false;
      }

      if (!read_all(wal->fd, value, value_length)) {
        free(value);
        free(key);
        return false;
      }
    }

    bool result =
        callback(operation, key, key_length, value, value_length, user_data);

    free(value);
    free(key);

    if (!result) {
      return false;
    }
  }

  /*
   * Restore the file offset to the end because this WAL
   * is append-only snf future operations used O_APPEND.
   */
  if (lseek(wal->fd, 0, SEEK_END) == (off_t)-1) {
    return false;
  }

  return true;
}
