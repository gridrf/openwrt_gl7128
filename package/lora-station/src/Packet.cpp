#include "Packet.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


Packet::Packet()
	:isWriter(true), _size(0)
{
	data_ptr = this->buffer;
}


Packet::Packet(uint8_t *data, int size)
	:_size(size), isWriter(false)
{
	memcpy(this->buffer, data, size);
	data_ptr = this->buffer;
}


Packet::~Packet()
{
}

void Packet::dump(const char *filename)
{
	FILE *f = fopen(filename, "wb+");
	fwrite(buffer, 1, _size, f);
	fclose(f);
}

void Packet::SetPosition(uint16_t pos) {
	data_ptr = buffer + pos;
};

void Packet::Skip(int pos) {
	data_ptr += pos;

	if (isWriter) _size += pos;
};

uint32_t Packet::Random(uint32_t min_num, uint32_t max_num)
{
	return (uint32_t)(min_num + (max_num - min_num)*rand() / (RAND_MAX + 1.0));
}


uint8_t Packet::ReadByte()
{
	return *data_ptr++;
}

uint16_t Packet::ReadInt16()
{
	uint16_t value = (data_ptr[0] & 0xFF) | (data_ptr[1] << 8);
	data_ptr += 2;
	return value;
}

uint32_t Packet::ReadInt32()
{
	uint32_t value = data_ptr[0] & 0xFF | (data_ptr[1] << 8) | (data_ptr[2] << 16) | (data_ptr[3] << 24);
	data_ptr += 4;
	return value;
}

uint64_t Packet::ReadInt64()
{
	uint64_t val;
	uint8_t *val_ptr = (uint8_t *)&val;
	val_ptr[0] = *data_ptr++;
	val_ptr[1] = *data_ptr++;
	val_ptr[2] = *data_ptr++;
	val_ptr[3] = *data_ptr++;
	val_ptr[4] = *data_ptr++;
	val_ptr[5] = *data_ptr++;
	val_ptr[6] = *data_ptr++;
	val_ptr[7] = *data_ptr++;

	return val;
}

void Packet::ReadBuffer(uint8_t *data, uint32_t size)
{
	memcpy(data, data_ptr, size);
	data_ptr += size;
}

void Packet::WriteByte(uint8_t value)
{
	*data_ptr++ = value;
	_size++;
}

void Packet::WriteInt16(uint16_t value)
{
	data_ptr[0] = value & 0xFF;
	data_ptr[1] = value >> 8;
	data_ptr += 2;
	_size += 2;
}

void Packet::WriteInt32(uint32_t value)
{
	data_ptr[0] = value & 0xFF;
	data_ptr[1] = (value >> 8);
	data_ptr[2] = (value >> 16);
	data_ptr[3] = (value >> 24);
	data_ptr += 4;
	_size += 4;
}

void Packet::WriteInt64(uint64_t value)
{
	uint8_t *val_ptr = (uint8_t *)&value;
	*data_ptr++ = val_ptr[0];
	*data_ptr++ = val_ptr[1];
	*data_ptr++ = val_ptr[2];
	*data_ptr++ = val_ptr[3];
	*data_ptr++ = val_ptr[4];
	*data_ptr++ = val_ptr[5];
	*data_ptr++ = val_ptr[6];
	*data_ptr++ = val_ptr[7];
	_size += 8;
}

void Packet::WriteBuffer(uint8_t *buffer, uint32_t size)
{
	memcpy(data_ptr, buffer, size);
	data_ptr += size;
	_size += size;
}

void Packet::WriteSize(uint16_t value)
{
	data_ptr[0] = value & 0xFF;
	data_ptr[1] = value >> 8;
}
void Packet::WriteString(const char *value)
{
	int len = strlen(value);
	strcpy((char *)data_ptr, value);
	data_ptr += len;
	_size += len;
}
void Packet::WriteStringFmt(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	char temp[100] = { 0 };
	vsnprintf(temp, 100, fmt, ap);
	va_end(ap);	WriteString(temp);
}
