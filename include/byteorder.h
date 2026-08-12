#ifndef KVSTORE_BYTEORDER_H
#define KVSTORE_BYTEORDER_H

#include <stdint.h>

void encode_u32(uint8_t *buffer, uint32_t value);
uint32_t decode_u32(const uint8_t *buffer);

#endif
