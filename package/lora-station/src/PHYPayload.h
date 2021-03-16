#ifndef __PHYPAYLOAD_H__
#define __PHYPAYLOAD_H__

#include "Packet.h"
#include <string.h>

typedef struct
{
	uint8_t MType;
	uint8_t RFU;
	uint8_t Major;
}MHDR_t;

typedef struct
{
	uint8_t ADR;
	uint8_t	 ADRACKReq;
	uint8_t ACK;
	uint8_t  RFU_FPending;
	uint8_t  FOptsLen;
}FCtrl_t;

typedef struct
{
	uint32_t DevAddr;
	uint8_t	 FCtrl;
	uint16_t FCnt;
	uint8_t  FOpts[16];
}FHDR_t;

class PHYPayload
{
private:
	Packet *MACPacket;
	FHDR_t fhdr;
	MHDR_t mhdr;
	FCtrl_t fctrl;
	uint8_t FPort;
	int MIC;
	uint8_t AppData_size;
	uint8_t AppData[256];
	bool micChecked;
	bool _DOWNLink;
private:
	uint8_t ReadBits(uint8_t value, uint8_t startBit, uint8_t count);

public:
	PHYPayload(bool DOWNLink);
	PHYPayload(uint8_t *buffer, int size);
	~PHYPayload();

	FHDR_t *GetFHDR() { return &fhdr; };
	MHDR_t *GetMHDR() { return &mhdr; };
	FCtrl_t *GetFCtrl() { return &fctrl; };
	uint8_t GetFPort() { return FPort; };
	uint8_t GetAppDataSize() { return AppData_size; };
	uint8_t *GetAppData() {	return AppData;	};


	void SetFHDR(FHDR_t *_fhdr);
	void SetMHDR(MHDR_t *_mhdr);
	void SetFCtrl(FCtrl_t *_fctrl);
	void SetFPort(uint8_t _port) {  FPort = _port; };
	void WriteAppData(uint8_t *buffer, uint8_t size);

	bool Decode();
	bool Encode(Packet *txpkt);
};

#endif
