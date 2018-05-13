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
#ifndef __PACKET_H__
#define __PACKET_H__

#include <stdint.h>

#define UDP_BUFFER_SIZE 1024

class Packet
{
private:
	uint8_t buffer[UDP_BUFFER_SIZE];
	int _size;
	uint8_t *data_ptr;
	bool isWriter;

public:
	Packet();
	Packet(uint8_t *data, int size);
	~Packet();


	void dump(const char *filename);

	int Available() { return _size - (data_ptr - buffer); };
	void SetPosition(uint16_t pos);
	int GetLength() { return _size; };
	uint8_t *GetBuffer() { return buffer; };
	void Skip(int pos);
	uint8_t *GetPtr() { return  data_ptr; };
	uint16_t GetPos() { return  data_ptr - buffer; };
	uint8_t Peek() { return  data_ptr[0]; };

	uint8_t ReadByte();
	uint16_t ReadInt16();
	uint32_t ReadInt32();
	uint64_t ReadInt64();
	void ReadBuffer(uint8_t *data, uint32_t size);

	void WriteByte(uint8_t value);
	void WriteInt16(uint16_t value);
	void WriteInt32(uint32_t value);
	void WriteInt64(uint64_t value);
	void WriteBuffer(uint8_t *buffer, uint32_t size);
	void WriteString(const char *value);
	void WriteStringFmt(const char *fmt, ...);

	static uint32_t Random(uint32_t min_num, uint32_t max_num);
	void WriteSize(uint16_t value);
};

#endif
