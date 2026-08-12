#include <kvstore/byteorder.h>

void encode_u32(uint8_t *buffer, uint32_t value) {
  buffer[0] = (uint8_t)(value >> 24);
  buffer[1] = (uint8_t)(value >> 16);
  buffer[2] = (uint8_t)(value >> 8);
  buffer[3] = (uint8_t)value;
}

uint32_t decode_u32(const uint8_t *buffer) {
  return ((uint32_t)buffer[0] << 24) | ((uint32_t)buffer[1] << 16) |
         ((uint32_t)buffer[2] << 8) | (uint32_t)buffer[3];
}
