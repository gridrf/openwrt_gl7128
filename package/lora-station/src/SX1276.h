#ifndef __SX1276_H__
#define __SX1276_H__

#include "board.h"
#include "ILoRaChip.h"
#include "RadioEvent.h"
#include "GpioIrqHandler.h"
#include "GpioControl.h"
#include "TimerHandler.h"
#include "TimerEvent.h"
#include "sx1276Regs-Fsk.h"
#include "sx1276Regs-LoRa.h"


//#define XTAL_FREQ                                   25000000 //32000000
//#define FREQ_STEP                                   47.6837158203125 //61.03515625
#define XTAL_FREQ                                   32000000 //32000000
#define FREQ_STEP                                   61.03515625

#define RX_BUFFER_SIZE                              256

typedef enum
{
	RF_IDLE = 0,   //!< The radio is idle
	RF_RX_RUNNING, //!< The radio is in reception state
	RF_TX_RUNNING, //!< The radio is in transmission state
	RF_CAD,        //!< The radio is doing channel activity detection
}RadioState_t;


typedef struct
{
	int8_t   Power;
	uint32_t Fdev;
	uint32_t Bandwidth;
	uint32_t BandwidthAfc;
	uint32_t Datarate;
	uint16_t PreambleLen;
	bool     FixLen;
	uint8_t  PayloadLen;
	bool     CrcOn;
	bool     IqInverted;
	bool     RxContinuous;
	uint32_t TxTimeout;
	uint32_t RxSingleTimeout;
}RadioFskSettings_t;

typedef struct
{
	uint8_t  PreambleDetected;
	uint8_t  SyncWordDetected;
	int8_t   RssiValue;
	int32_t  AfcValue;
	uint8_t  RxGain;
	uint16_t Size;
	uint16_t NbBytes;
	uint8_t  FifoThresh;
	uint8_t  ChunkSize;
}RadioFskPacketHandler_t;

typedef struct
{
	int8_t   Power;
	uint32_t Bandwidth;
	uint32_t Datarate;
	bool     LowDatarateOptimize;
	uint8_t  Coderate;
	uint16_t PreambleLen;
	bool     FixLen;
	uint8_t  PayloadLen;
	bool     CrcOn;
	bool     FreqHopOn;
	uint8_t  HopPeriod;
	bool     TxIqInverted;
	bool     RxIqInverted;
	bool     RxContinuous;
	uint32_t TxTimeout;
	bool     PublicNetwork;
}RadioLoRaSettings_t;

typedef struct
{
	int8_t SnrValue;
	int16_t RssiValue;
	uint8_t Size;
}RadioLoRaPacketHandler_t;

typedef struct
{
	RadioState_t             State;
	RadioModems_t            Modem;
	uint32_t                 Channel;
	RadioFskSettings_t       Fsk;
	RadioFskPacketHandler_t  FskPacketHandler;
	RadioLoRaSettings_t      LoRa;
	RadioLoRaPacketHandler_t LoRaPacketHandler;
}RadioSettings_t;

class SX1276:public ILoRaChip, public GPIO_IRQ_Handler, public TimerHandler
{
private:
	RadioSettings_t          Settings;

	TimerEvent_t TxTimeoutTimer;
	TimerEvent_t RxTimeoutTimer;
	TimerEvent_t RxTimeoutSyncWord;
	uint8_t RxTxBuffer[RX_BUFFER_SIZE];
	RadioEvent *_event;
	TimerEvent *_timer;

private:
	void Reset();
	void RxChainCalibration(void);
	void WriteReg(uint8_t addr, uint8_t data);
	uint8_t ReadReg(uint8_t addr);
	void WriteBuffer(uint8_t addr, uint8_t *buffer, uint8_t size);
	void ReadBuffer(uint8_t addr, uint8_t *buffer, uint8_t size);
	void WriteFifo(uint8_t *buffer, uint8_t size);
	void ReadFifo(uint8_t *buffer, uint8_t size);

	void SetOpMode(uint8_t opMode);
	void SetModem(RadioModems_t modem);
	void SetSleep(void);
	void SetStby(void);
	void SetTx(uint32_t timeout);
	uint8_t GetFskBandwidthRegValue(uint32_t bandwidth);
	bool IsChannelFree(RadioModems_t modem, uint32_t freq, int16_t rssiThresh, uint32_t maxCarrierSenseTime);
	uint32_t Random(void);
	uint32_t GetTimeOnAir(RadioModems_t modem, uint8_t pktLen);
	void StartCad(void);
	int16_t ReadRssi(RadioModems_t modem);
	void SetMaxPayloadLength(RadioModems_t modem, uint8_t max);

	void SetAntSw(uint8_t opMode);
	uint8_t GetPaSelect(uint32_t channel);

public:
	bool OnTimeoutIrq(void *user);
	void OnDio0Irq(void);
	void OnDio1Irq(void);
	void OnDio2Irq(void);
	void OnDio3Irq(void);
	void OnDio4Irq(void);
	void OnDio5Irq(void);

public:
	SX1276(RadioEvent *evt, TimerEvent *timer);
	~SX1276();

	void Init();
	void SetPublicNetwork(bool enable);
	void SetChannel(uint32_t freq);
	void SetRfTxPower(int8_t power);
	void SetTxContinuousWave(uint32_t freq, int8_t power, uint16_t time);
	void SetTxConfig(RadioModems_t modem, int8_t power, uint32_t fdev,
		uint32_t bandwidth, uint32_t datarate,
		uint8_t coderate, uint16_t preambleLen,
		bool fixLen, bool crcOn, bool freqHopOn,
		uint8_t hopPeriod, bool iqInverted, uint32_t timeout);

	void SetRxConfig(RadioModems_t modem, uint32_t bandwidth,
		uint32_t datarate, uint8_t coderate,
		uint32_t bandwidthAfc, uint16_t preambleLen,
		uint16_t symbTimeout, bool fixLen,
		uint8_t payloadLen,
		bool crcOn, bool freqHopOn, uint8_t hopPeriod,
		bool iqInverted, bool rxContinuous);

	void Rx(uint32_t timeout);
	void Send(uint8_t *buffer, uint8_t size);
	void Sleep(void) { SetSleep(); };
};

#endif

