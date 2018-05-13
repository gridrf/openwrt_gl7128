/* Copyright (C) 2018  GridRF Radio Team(tech@gridrf.com)

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
#include "PHYPayload.h"
#include "LoRaMacCrypto.h"
#include <stdio.h>


#define LORAWAN_NWKSKEY                             { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C }

/*!
* AES encryption/decryption cipher application session key
*/
#define LORAWAN_APPSKEY                             { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C }

static uint8_t NwkSKey[] = LORAWAN_NWKSKEY;
static uint8_t AppKey[] = LORAWAN_APPSKEY;

PHYPayload::PHYPayload(bool DOWNLink)
	:MACPacket(NULL), MIC(0), FPort(0), micChecked(false), AppData_size(0), _DOWNLink(DOWNLink)
{
}

PHYPayload::PHYPayload(uint8_t *buffer, int size)
	: _DOWNLink(false)
{
	uint32_t compMic = 0;
	uint8_t MACPayload_size = 0;
	uint8_t MACPayload[255] = {0};

	Packet pkt(buffer, size);

	uint8_t MHDRData = pkt.ReadByte();

	//MAC Header (MHDR field)
	//7..5 4..2 1..0
	//MType RFU Major
	mhdr.MType = ReadBits(MHDRData, 5, 3);
	mhdr.RFU = ReadBits(MHDRData, 2, 3);
	mhdr.Major = ReadBits(MHDRData, 0, 2);
	/*
	MType Description
	000 Join Request
	001 Join Accept
	010 Unconfirmed Data Up
	011 Unconfirmed Data Down
	100 Confirmed Data Up
	101 Confirmed Data Down
	110 RFU
	111 Proprietary
	*/
	/*
	Major bits Description
	00 LoRaWAN R1
	01..11 RFU
	*/
	MACPayload_size = pkt.Available() - 4;
	pkt.ReadBuffer(MACPayload, MACPayload_size);
	this->MIC = pkt.ReadInt32();

	MACPacket = new Packet(MACPayload, MACPayload_size);
	//MACPayload = FHDR FPort FRMPayload
	//FHDR = DevAddr FCtrl FCnt FOpts
	//Size(bytes)      4       1     2  0..15
	//            FHDR DevAddr FCtrl FCnt FOpts
	fhdr.DevAddr = MACPacket->ReadInt32();
	fhdr.FCtrl = MACPacket->ReadByte();
	fhdr.FCnt = MACPacket->ReadInt16();

	LoRaMacComputeMic(buffer, size - 4, NwkSKey, fhdr.DevAddr, _DOWNLink ? 1: 0, fhdr.FCnt, &compMic);

	micChecked = (compMic == this->MIC);
}

PHYPayload::~PHYPayload()
{
	if (MACPacket) {
		delete(MACPacket);
	}
}

uint8_t PHYPayload::ReadBits(uint8_t value, uint8_t startBit, uint8_t count)
{
	uint8_t cmp = 0x0;
	for (int i = 0; i < count; i++) {
		cmp |= 1 << i;
	}

	return (value >> startBit) & cmp;
}

bool PHYPayload::Decode()
{
	if (!micChecked) return false;

	printf("DevAddr=%d\n", fhdr.DevAddr);

	//Bit#  7    6        5   4        [3..0]
	//FCtrl ADR ADRACKReq ACK FPending FOptsLen
	
	fctrl.ADR = ReadBits(fhdr.FCtrl, 7, 1);
	fctrl.ADRACKReq = ReadBits(fhdr.FCtrl, 6, 1);
	fctrl.ACK = ReadBits(fhdr.FCtrl, 5, 1);
	fctrl.RFU_FPending = ReadBits(fhdr.FCtrl, 4, 1);
	fctrl.FOptsLen = ReadBits(fhdr.FCtrl, 0, 4);

	printf("FOptsLen=%d\n", fctrl.FOptsLen);

	if (fctrl.FOptsLen > 0) {
		MACPacket->ReadBuffer(fhdr.FOpts, fctrl.FOptsLen);
	}

	this->FPort = MACPacket->ReadByte();
	this->AppData_size = MACPacket->Available();
	uint8_t FRMPayload[256] = { 0 };
	MACPacket->ReadBuffer(FRMPayload, AppData_size);

	LoRaMacPayloadDecrypt(FRMPayload, AppData_size, AppKey, fhdr.DevAddr, _DOWNLink ? 1 : 0, fhdr.FCnt, AppData);
	return true;
}

void PHYPayload::SetFHDR(FHDR_t *_fhdr)
{ 
	memcpy(&fhdr, _fhdr, sizeof(FHDR_t));
}

void PHYPayload::SetMHDR(MHDR_t *_mhdr) 
{
	memcpy(&mhdr, _mhdr, sizeof(MHDR_t));
}

void PHYPayload::SetFCtrl(FCtrl_t *_fctrl) 
{
	memcpy(&fctrl, _fctrl, sizeof(FCtrl_t));
}

bool PHYPayload::Encode(Packet *txpkt)
{
	uint8_t mhdrData = 0;

	txpkt->SetPosition(0);

	mhdrData |= (mhdr.MType & 0x7) << 5;
	mhdrData |= (mhdr.RFU & 0x7) << 2;
	mhdrData |= mhdr.Major & 0x3;
	txpkt->WriteByte(mhdrData);
	//fhdr

	uint8_t FCtrlData = 0;
	FCtrlData |= (fctrl.ADR & 0x1) << 7;
	FCtrlData |= (fctrl.ADRACKReq & 0x1) << 6;
	FCtrlData |= (fctrl.ACK & 0x1) << 5;
	FCtrlData |= (fctrl.RFU_FPending & 0x1) << 4;
	FCtrlData |= (fctrl.FOptsLen & 0x0F);

	fhdr.FCtrl = FCtrlData;

	txpkt->WriteInt32(fhdr.DevAddr);
	txpkt->WriteByte(fhdr.FCtrl);
	txpkt->WriteInt16(fhdr.FCnt);
	
	if (fctrl.FOptsLen > 0) {
		txpkt->WriteBuffer(fhdr.FOpts, fctrl.FOptsLen);
	}

	if (AppData_size > 0) {
		uint8_t app_encode[256] = { 0 };
		LoRaMacPayloadEncrypt(this->AppData, this->AppData_size, AppKey, fhdr.DevAddr, _DOWNLink ? 1 : 0, fhdr.FCnt, app_encode);
		txpkt->WriteBuffer(app_encode, AppData_size);
	}
	//mic
	uint32_t compMic = 0;
	LoRaMacComputeMic(txpkt->GetBuffer(), txpkt->GetLength(), NwkSKey, fhdr.DevAddr, _DOWNLink ? 1 : 0, fhdr.FCnt, &compMic);
	txpkt->WriteInt32(compMic);
	return true;
}

void PHYPayload::WriteAppData(uint8_t *buffer, uint8_t size)
{
	memcpy(AppData, buffer, size);
	AppData_size = size;
}