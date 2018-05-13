/*
Copyright (C) 2018  GridRF Radio Team(tech@gridrf.com)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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
