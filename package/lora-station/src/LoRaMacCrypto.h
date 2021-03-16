#ifndef __LORAMACCRYPTO_H__
#define __LORAMACCRYPTO_H__

#include <stdint.h>

void LoRaMacComputeMic(const uint8_t *buffer, uint16_t size, const uint8_t *key, uint32_t address, uint8_t dir, uint32_t sequenceCounter, uint32_t *mic);
void LoRaMacPayloadEncrypt(const uint8_t *buffer, uint16_t size, const uint8_t *key, uint32_t address, uint8_t dir, uint32_t sequenceCounter, uint8_t *encBuffer);
void LoRaMacPayloadDecrypt(const uint8_t *buffer, uint16_t size, const uint8_t *key, uint32_t address, uint8_t dir, uint32_t sequenceCounter, uint8_t *decBuffer);
void LoRaMacJoinComputeMic(const uint8_t *buffer, uint16_t size, const uint8_t *key, uint32_t *mic);
void LoRaMacJoinDecrypt(const uint8_t *buffer, uint16_t size, const uint8_t *key, uint8_t *decBuffer);
void LoRaMacJoinComputeSKeys(const uint8_t *key, const uint8_t *appNonce, uint16_t devNonce, uint8_t *nwkSKey, uint8_t *appSKey);

#endif
