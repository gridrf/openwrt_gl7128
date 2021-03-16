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
