#include "SX1276.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "gpio.h"
#include "spi.h"
#include "board.h"


/*!
* Constant values need to compute the RSSI value
*/
#define RSSI_OFFSET_LF                              -164
#define RSSI_OFFSET_HF                              -157

#define RF_MID_BAND_THRESH                          525000000

/*!
* Sync word for Private LoRa networks
*/
#define LORA_MAC_PRIVATE_SYNCWORD                   0x12
/*!
* Sync word for Public LoRa networks
*/
#define LORA_MAC_PUBLIC_SYNCWORD                    0x34

typedef struct
{
	RadioModems_t Modem;
	uint8_t       Addr;
	uint8_t       Value;
}RadioRegisters_t;

#define RADIO_INIT_REGISTERS_VALUE                \
{                                                 \
    { MODEM_FSK , REG_LNA                , 0x23 },\
    { MODEM_FSK , REG_RXCONFIG           , 0x1E },\
    { MODEM_FSK , REG_RSSICONFIG         , 0xD2 },\
    { MODEM_FSK , REG_AFCFEI             , 0x01 },\
    { MODEM_FSK , REG_PREAMBLEDETECT     , 0xAA },\
    { MODEM_FSK , REG_OSC                , 0x07 },\
    { MODEM_FSK , REG_SYNCCONFIG         , 0x12 },\
    { MODEM_FSK , REG_SYNCVALUE1         , 0xC1 },\
    { MODEM_FSK , REG_SYNCVALUE2         , 0x94 },\
    { MODEM_FSK , REG_SYNCVALUE3         , 0xC1 },\
    { MODEM_FSK , REG_PACKETCONFIG1      , 0xD8 },\
    { MODEM_FSK , REG_FIFOTHRESH         , 0x8F },\
    { MODEM_FSK , REG_IMAGECAL           , 0x02 },\
    { MODEM_FSK , REG_DIOMAPPING1        , 0x00 },\
    { MODEM_FSK , REG_DIOMAPPING2        , 0x30 },\
    { MODEM_LORA, REG_LR_PAYLOADMAXLENGTH, 0x40 },\
}                                                 \

const RadioRegisters_t RadioRegsInit[] = RADIO_INIT_REGISTERS_VALUE;

/*!
* FSK bandwidth definition
*/
typedef struct
{
	uint32_t bandwidth;
	uint8_t  RegValue;
}FskBandwidth_t;

const FskBandwidth_t FskBandwidths[] =
{
	{ 2600  , 0x17 },
	{ 3100  , 0x0F },
	{ 3900  , 0x07 },
	{ 5200  , 0x16 },
	{ 6300  , 0x0E },
	{ 7800  , 0x06 },
	{ 10400 , 0x15 },
	{ 12500 , 0x0D },
	{ 15600 , 0x05 },
	{ 20800 , 0x14 },
	{ 25000 , 0x0C },
	{ 31300 , 0x04 },
	{ 41700 , 0x13 },
	{ 50000 , 0x0B },
	{ 62500 , 0x03 },
	{ 83333 , 0x12 },
	{ 100000, 0x0A },
	{ 125000, 0x02 },
	{ 166700, 0x11 },
	{ 200000, 0x09 },
	{ 250000, 0x01 },
	{ 300000, 0x00 }, // Invalid Bandwidth
};


SX1276::SX1276(RadioEvent *evt, TimerEvent *timer)
	:_event(evt), _timer(timer)
{
	memset(&Settings, 0, sizeof(Settings));
}


SX1276::~SX1276()
{
	SetSleep();
}

void SX1276::Init()
{
	_timer->RegisterTimer(this, &RxTimeoutTimer);
	_timer->RegisterTimer(this, &TxTimeoutTimer);
	_timer->RegisterTimer(this, &RxTimeoutSyncWord);

	this->Reset();
	this->RxChainCalibration();
	this->SetOpMode(RF_OPMODE_SLEEP);

	for (int i = 0; i < sizeof(RadioRegsInit) / sizeof(RadioRegisters_t); i++)
	{
		SetModem(RadioRegsInit[i].Modem);
		WriteReg(RadioRegsInit[i].Addr, RadioRegsInit[i].Value);
	}

	SetModem(MODEM_FSK);
	Settings.State = RF_IDLE;

	//printf("SX1276::Init\n");
}


void SX1276::SetChannel(uint32_t freq)
{
	Settings.Channel = freq;
	freq = (uint32_t)((double)freq / (double)FREQ_STEP);
	WriteReg(REG_FRFMSB, (uint8_t)((freq >> 16) & 0xFF));
	WriteReg(REG_FRFMID, (uint8_t)((freq >> 8) & 0xFF));
	WriteReg(REG_FRFLSB, (uint8_t)(freq & 0xFF));
}


bool SX1276::IsChannelFree( RadioModems_t modem, uint32_t freq, int16_t rssiThresh, uint32_t maxCarrierSenseTime )
{
    bool status = true;
    int16_t rssi = 0;
    uint32_t carrierSenseTime = 0;

    SetModem( modem );

    SetChannel( freq );

    SetOpMode( RF_OPMODE_RECEIVER );

    DelayMs( 1 );

    carrierSenseTime = _timer->TimerGetCurrentTime( );

    // Perform carrier sense for maxCarrierSenseTime
    while(_timer->TimerGetElapsedTime( carrierSenseTime ) < maxCarrierSenseTime )
    {
        rssi = ReadRssi( modem );

        if( rssi > rssiThresh )
        {
            status = false;
            break;
        }
    }
    SetSleep( );
    return status;
}


uint32_t SX1276::Random( void )
{
    uint8_t i;
    uint32_t rnd = 0;

    /*
     * Radio setup for random number generation
     */
    // Set LoRa modem ON
    SetModem( MODEM_LORA );

    // Disable LoRa modem interrupts
    WriteReg( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                  RFLR_IRQFLAGS_RXDONE |
                  RFLR_IRQFLAGS_PAYLOADCRCERROR |
                  RFLR_IRQFLAGS_VALIDHEADER |
                  RFLR_IRQFLAGS_TXDONE |
                  RFLR_IRQFLAGS_CADDONE |
                  RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                  RFLR_IRQFLAGS_CADDETECTED );

    // Set radio in continuous reception
    SetOpMode( RF_OPMODE_RECEIVER );

    for( i = 0; i < 32; i++ )
    {
        DelayMs( 1 );
        // Unfiltered RSSI value reading. Only takes the LSB value
        rnd |= ( ( uint32_t )ReadReg( REG_LR_RSSIWIDEBAND ) & 0x01 ) << i;
    }

    SetSleep( );

    return rnd;
}

void SX1276::RxChainCalibration(void)
{
	uint8_t regPaConfigInitVal;
	uint32_t initialFreq;

	// Save context
	regPaConfigInitVal = ReadReg(REG_PACONFIG);
	initialFreq = (double)(((uint32_t)ReadReg(REG_FRFMSB) << 16) |
		((uint32_t)ReadReg(REG_FRFMID) << 8) |
		((uint32_t)ReadReg(REG_FRFLSB))) * (double)FREQ_STEP;

	// Cut the PA just in case, RFO output, power = -1 dBm
	WriteReg(REG_PACONFIG, 0x00);

	// Launch Rx chain calibration for LF band
	WriteReg(REG_IMAGECAL, (ReadReg(REG_IMAGECAL) & RF_IMAGECAL_IMAGECAL_MASK) | RF_IMAGECAL_IMAGECAL_START);
	while ((ReadReg(REG_IMAGECAL) & RF_IMAGECAL_IMAGECAL_RUNNING) == RF_IMAGECAL_IMAGECAL_RUNNING)
	{
	}

	// Sets a Frequency in HF band
	SetChannel(434000000);

	// Launch Rx chain calibration for HF band
	WriteReg(REG_IMAGECAL, (ReadReg(REG_IMAGECAL) & RF_IMAGECAL_IMAGECAL_MASK) | RF_IMAGECAL_IMAGECAL_START);
	while ((ReadReg(REG_IMAGECAL) & RF_IMAGECAL_IMAGECAL_RUNNING) == RF_IMAGECAL_IMAGECAL_RUNNING)
	{
	}

	// Restore context
	WriteReg(REG_PACONFIG, regPaConfigInitVal);
	SetChannel(initialFreq);
}

uint8_t SX1276::GetFskBandwidthRegValue(uint32_t bandwidth)
{
	for (int i = 0; i < (sizeof(FskBandwidths) / sizeof(FskBandwidth_t)) - 1; i++)
	{
		if ((bandwidth >= FskBandwidths[i].bandwidth) && (bandwidth < FskBandwidths[i + 1].bandwidth))
		{
			return FskBandwidths[i].RegValue;
		}
	}
	// ERROR: Value not found
	while (1);
}

void SX1276::SetRxConfig(RadioModems_t modem, uint32_t bandwidth,
	uint32_t datarate, uint8_t coderate,
	uint32_t bandwidthAfc, uint16_t preambleLen,
	uint16_t symbTimeout, bool fixLen,
	uint8_t payloadLen,
	bool crcOn, bool freqHopOn, uint8_t hopPeriod,
	bool iqInverted, bool rxContinuous)
{
	SetModem(modem);

	switch (modem)
	{
	case MODEM_FSK:
	{
		Settings.Fsk.Bandwidth = bandwidth;
		Settings.Fsk.Datarate = datarate;
		Settings.Fsk.BandwidthAfc = bandwidthAfc;
		Settings.Fsk.FixLen = fixLen;
		Settings.Fsk.PayloadLen = payloadLen;
		Settings.Fsk.CrcOn = crcOn;
		Settings.Fsk.IqInverted = iqInverted;
		Settings.Fsk.RxContinuous = rxContinuous;
		Settings.Fsk.PreambleLen = preambleLen;
		Settings.Fsk.RxSingleTimeout = (uint32_t)(symbTimeout * ((1.0 / (double)datarate) * 8.0) * 1000);

		datarate = (uint16_t)((double)XTAL_FREQ / (double)datarate);
		WriteReg(REG_BITRATEMSB, (uint8_t)(datarate >> 8));
		WriteReg(REG_BITRATELSB, (uint8_t)(datarate & 0xFF));

		WriteReg(REG_RXBW, GetFskBandwidthRegValue(bandwidth));
		WriteReg(REG_AFCBW, GetFskBandwidthRegValue(bandwidthAfc));

		WriteReg(REG_PREAMBLEMSB, (uint8_t)((preambleLen >> 8) & 0xFF));
		WriteReg(REG_PREAMBLELSB, (uint8_t)(preambleLen & 0xFF));

		if (fixLen == 1)
		{
			WriteReg(REG_PAYLOADLENGTH, payloadLen);
		}
		else
		{
			WriteReg(REG_PAYLOADLENGTH, 0xFF); // Set payload length to the maximum
		}

		WriteReg(REG_PACKETCONFIG1,
			(ReadReg(REG_PACKETCONFIG1) &
				RF_PACKETCONFIG1_CRC_MASK &
				RF_PACKETCONFIG1_PACKETFORMAT_MASK) |
				((fixLen == 1) ? RF_PACKETCONFIG1_PACKETFORMAT_FIXED : RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE) |
			(crcOn << 4));
		WriteReg(REG_PACKETCONFIG2, (ReadReg(REG_PACKETCONFIG2) | RF_PACKETCONFIG2_DATAMODE_PACKET));
	}
	break;
	case MODEM_LORA:
	{
		if (bandwidth > 2)
		{
			// Fatal error: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
			while (1);
		}
		bandwidth += 7;
		Settings.LoRa.Bandwidth = bandwidth;
		Settings.LoRa.Datarate = datarate;
		Settings.LoRa.Coderate = coderate;
		Settings.LoRa.PreambleLen = preambleLen;
		Settings.LoRa.FixLen = fixLen;
		Settings.LoRa.PayloadLen = payloadLen;
		Settings.LoRa.CrcOn = crcOn;
		Settings.LoRa.FreqHopOn = freqHopOn;
		Settings.LoRa.HopPeriod = hopPeriod;
		Settings.LoRa.RxIqInverted = iqInverted;
		Settings.LoRa.RxContinuous = rxContinuous;

		if (datarate > 12)
		{
			datarate = 12;
		}
		else if (datarate < 6)
		{
			datarate = 6;
		}

		if (((bandwidth == 7) && ((datarate == 11) || (datarate == 12))) ||
			((bandwidth == 8) && (datarate == 12)))
		{
			Settings.LoRa.LowDatarateOptimize = 0x01;
		}
		else
		{
			Settings.LoRa.LowDatarateOptimize = 0x00;
		}

		WriteReg(REG_LR_MODEMCONFIG1,
			(ReadReg(REG_LR_MODEMCONFIG1) &
				RFLR_MODEMCONFIG1_BW_MASK &
				RFLR_MODEMCONFIG1_CODINGRATE_MASK &
				RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK) |
				(bandwidth << 4) | (coderate << 1) |
			fixLen);

		WriteReg(REG_LR_MODEMCONFIG2,
			(ReadReg(REG_LR_MODEMCONFIG2) &
				RFLR_MODEMCONFIG2_SF_MASK &
				RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK &
				RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK) |
				(datarate << 4) | (crcOn << 2) |
			((symbTimeout >> 8) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK));

		WriteReg(REG_LR_MODEMCONFIG3,
			(ReadReg(REG_LR_MODEMCONFIG3) &
				RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK) |
				(Settings.LoRa.LowDatarateOptimize << 3));

		WriteReg(REG_LR_SYMBTIMEOUTLSB, (uint8_t)(symbTimeout & 0xFF));

		WriteReg(REG_LR_PREAMBLEMSB, (uint8_t)((preambleLen >> 8) & 0xFF));
		WriteReg(REG_LR_PREAMBLELSB, (uint8_t)(preambleLen & 0xFF));

		if (fixLen == 1)
		{
			WriteReg(REG_LR_PAYLOADLENGTH, payloadLen);
		}

		if (Settings.LoRa.FreqHopOn == true)
		{
			WriteReg(REG_LR_PLLHOP, (ReadReg(REG_LR_PLLHOP) & RFLR_PLLHOP_FASTHOP_MASK) | RFLR_PLLHOP_FASTHOP_ON);
			WriteReg(REG_LR_HOPPERIOD, Settings.LoRa.HopPeriod);
		}

		if ((bandwidth == 9) && (Settings.Channel > RF_MID_BAND_THRESH))
		{
			// ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
			WriteReg(REG_LR_TEST36, 0x02);
			WriteReg(REG_LR_TEST3A, 0x64);
		}
		else if (bandwidth == 9)
		{
			// ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
			WriteReg(REG_LR_TEST36, 0x02);
			WriteReg(REG_LR_TEST3A, 0x7F);
		}
		else
		{
			// ERRATA 2.1 - Sensitivity Optimization with a 500 kHz Bandwidth
			WriteReg(REG_LR_TEST36, 0x03);
		}

		if (datarate == 6)
		{
			WriteReg(REG_LR_DETECTOPTIMIZE,
				(ReadReg(REG_LR_DETECTOPTIMIZE) &
					RFLR_DETECTIONOPTIMIZE_MASK) |
				RFLR_DETECTIONOPTIMIZE_SF6);
			WriteReg(REG_LR_DETECTIONTHRESHOLD,
				RFLR_DETECTIONTHRESH_SF6);
		}
		else
		{
			WriteReg(REG_LR_DETECTOPTIMIZE,
				(ReadReg(REG_LR_DETECTOPTIMIZE) &
					RFLR_DETECTIONOPTIMIZE_MASK) |
				RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12);
			WriteReg(REG_LR_DETECTIONTHRESHOLD,
				RFLR_DETECTIONTHRESH_SF7_TO_SF12);
		}
	}
	break;
	}
}


void SX1276::SetTxConfig(RadioModems_t modem, int8_t power, uint32_t fdev,
	uint32_t bandwidth, uint32_t datarate,
	uint8_t coderate, uint16_t preambleLen,
	bool fixLen, bool crcOn, bool freqHopOn,
	uint8_t hopPeriod, bool iqInverted, uint32_t timeout)
{
	SetModem(modem);

	SetRfTxPower(power);

	switch (modem)
	{
	case MODEM_FSK:
	{
		Settings.Fsk.Power = power;
		Settings.Fsk.Fdev = fdev;
		Settings.Fsk.Bandwidth = bandwidth;
		Settings.Fsk.Datarate = datarate;
		Settings.Fsk.PreambleLen = preambleLen;
		Settings.Fsk.FixLen = fixLen;
		Settings.Fsk.CrcOn = crcOn;
		Settings.Fsk.IqInverted = iqInverted;
		Settings.Fsk.TxTimeout = timeout;

		fdev = (uint16_t)((double)fdev / (double)FREQ_STEP);
		WriteReg(REG_FDEVMSB, (uint8_t)(fdev >> 8));
		WriteReg(REG_FDEVLSB, (uint8_t)(fdev & 0xFF));

		datarate = (uint16_t)((double)XTAL_FREQ / (double)datarate);
		WriteReg(REG_BITRATEMSB, (uint8_t)(datarate >> 8));
		WriteReg(REG_BITRATELSB, (uint8_t)(datarate & 0xFF));

		WriteReg(REG_PREAMBLEMSB, (preambleLen >> 8) & 0x00FF);
		WriteReg(REG_PREAMBLELSB, preambleLen & 0xFF);

		WriteReg(REG_PACKETCONFIG1,
			(ReadReg(REG_PACKETCONFIG1) &
				RF_PACKETCONFIG1_CRC_MASK &
				RF_PACKETCONFIG1_PACKETFORMAT_MASK) |
				((fixLen == 1) ? RF_PACKETCONFIG1_PACKETFORMAT_FIXED : RF_PACKETCONFIG1_PACKETFORMAT_VARIABLE) |
			(crcOn << 4));
		WriteReg(REG_PACKETCONFIG2, (ReadReg(REG_PACKETCONFIG2) | RF_PACKETCONFIG2_DATAMODE_PACKET));
	}
	break;
	case MODEM_LORA:
	{
		Settings.LoRa.Power = power;
		if (bandwidth > 2)
		{
			// Fatal error: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
			while (1);
		}
		bandwidth += 7;
		Settings.LoRa.Bandwidth = bandwidth;
		Settings.LoRa.Datarate = datarate;
		Settings.LoRa.Coderate = coderate;
		Settings.LoRa.PreambleLen = preambleLen;
		Settings.LoRa.FixLen = fixLen;
		Settings.LoRa.FreqHopOn = freqHopOn;
		Settings.LoRa.HopPeriod = hopPeriod;
		Settings.LoRa.CrcOn = crcOn;
		Settings.LoRa.TxIqInverted = iqInverted;
		Settings.LoRa.TxTimeout = timeout;

		if (datarate > 12)
		{
			datarate = 12;
		}
		else if (datarate < 6)
		{
			datarate = 6;
		}
		if (((bandwidth == 7) && ((datarate == 11) || (datarate == 12))) ||
			((bandwidth == 8) && (datarate == 12)))
		{
			Settings.LoRa.LowDatarateOptimize = 0x01;
		}
		else
		{
			Settings.LoRa.LowDatarateOptimize = 0x00;
		}

		if (Settings.LoRa.FreqHopOn == true)
		{
			WriteReg(REG_LR_PLLHOP, (ReadReg(REG_LR_PLLHOP) & RFLR_PLLHOP_FASTHOP_MASK) | RFLR_PLLHOP_FASTHOP_ON);
			WriteReg(REG_LR_HOPPERIOD, Settings.LoRa.HopPeriod);
		}

		WriteReg(REG_LR_MODEMCONFIG1,
			(ReadReg(REG_LR_MODEMCONFIG1) &
				RFLR_MODEMCONFIG1_BW_MASK &
				RFLR_MODEMCONFIG1_CODINGRATE_MASK &
				RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK) |
				(bandwidth << 4) | (coderate << 1) |
			fixLen);

		WriteReg(REG_LR_MODEMCONFIG2,
			(ReadReg(REG_LR_MODEMCONFIG2) &
				RFLR_MODEMCONFIG2_SF_MASK &
				RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK) |
				(datarate << 4) | (crcOn << 2));

		WriteReg(REG_LR_MODEMCONFIG3,
			(ReadReg(REG_LR_MODEMCONFIG3) &
				RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK) |
				(Settings.LoRa.LowDatarateOptimize << 3));

		WriteReg(REG_LR_PREAMBLEMSB, (preambleLen >> 8) & 0x00FF);
		WriteReg(REG_LR_PREAMBLELSB, preambleLen & 0xFF);

		if (datarate == 6)
		{
			WriteReg(REG_LR_DETECTOPTIMIZE,
				(ReadReg(REG_LR_DETECTOPTIMIZE) &
					RFLR_DETECTIONOPTIMIZE_MASK) |
				RFLR_DETECTIONOPTIMIZE_SF6);
			WriteReg(REG_LR_DETECTIONTHRESHOLD,
				RFLR_DETECTIONTHRESH_SF6);
		}
		else
		{
			WriteReg(REG_LR_DETECTOPTIMIZE,
				(ReadReg(REG_LR_DETECTOPTIMIZE) &
					RFLR_DETECTIONOPTIMIZE_MASK) |
				RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12);
			WriteReg(REG_LR_DETECTIONTHRESHOLD,
				RFLR_DETECTIONTHRESH_SF7_TO_SF12);
		}
	}
	break;
	}
}

uint32_t SX1276::GetTimeOnAir( RadioModems_t modem, uint8_t pktLen )
{
    uint32_t airTime = 0;

    switch( modem )
    {
    case MODEM_FSK:
        {
            airTime = round( ( 8 * (Settings.Fsk.PreambleLen +
                                     ( ( ReadReg( REG_SYNCCONFIG ) & ~RF_SYNCCONFIG_SYNCSIZE_MASK ) + 1 ) +
                                     ( ( Settings.Fsk.FixLen == 0x01 ) ? 0.0 : 1.0 ) +
                                     ( ( ( ReadReg( REG_PACKETCONFIG1 ) & ~RF_PACKETCONFIG1_ADDRSFILTERING_MASK ) != 0x00 ) ? 1.0 : 0 ) +
                                     pktLen +
                                     ( ( Settings.Fsk.CrcOn == 0x01 ) ? 2.0 : 0 ) ) /
                                     Settings.Fsk.Datarate ) * 1000 );
        }
        break;
    case MODEM_LORA:
        {
            double bw = 0.0;
            // REMARK: When using LoRa modem only bandwidths 125, 250 and 500 kHz are supported
            switch( Settings.LoRa.Bandwidth )
            {
            //case 0: // 7.8 kHz
            //    bw = 7800;
            //    break;
            //case 1: // 10.4 kHz
            //    bw = 10400;
            //    break;
            //case 2: // 15.6 kHz
            //    bw = 15600;
            //    break;
            //case 3: // 20.8 kHz
            //    bw = 20800;
            //    break;
            //case 4: // 31.2 kHz
            //    bw = 31200;
            //    break;
            //case 5: // 41.4 kHz
            //    bw = 41400;
            //    break;
            //case 6: // 62.5 kHz
            //    bw = 62500;
            //    break;
            case 7: // 125 kHz
                bw = 125000;
                break;
            case 8: // 250 kHz
                bw = 250000;
                break;
            case 9: // 500 kHz
                bw = 500000;
                break;
            }

            // Symbol rate : time for one symbol (secs)
            double rs = bw / ( 1 << Settings.LoRa.Datarate );
            double ts = 1 / rs;
            // time of preamble
            double tPreamble = ( Settings.LoRa.PreambleLen + 4.25 ) * ts;
            // Symbol length of payload and time
            double tmp = ceil( ( 8 * pktLen - 4 * Settings.LoRa.Datarate +
                                 28 + 16 * Settings.LoRa.CrcOn -
                                 ( Settings.LoRa.FixLen ? 20 : 0 ) ) /
                                 ( double )( 4 * ( Settings.LoRa.Datarate -
                                 ( ( Settings.LoRa.LowDatarateOptimize > 0 ) ? 2 : 0 ) ) ) ) *
                                 ( Settings.LoRa.Coderate + 4 );
            double nPayload = 8 + ( ( tmp > 0 ) ? tmp : 0 );
            double tPayload = nPayload * ts;
            // Time on air
            double tOnAir = tPreamble + tPayload;
            // return ms secs
            airTime = floor( tOnAir * 1000 + 0.999 );
        }
        break;
    }
    return airTime;
}


void SX1276::Send(uint8_t *buffer, uint8_t size)
{
	uint32_t txTimeout = 0;

	switch (Settings.Modem)
	{
	case MODEM_FSK:
	{
		Settings.FskPacketHandler.NbBytes = 0;
		Settings.FskPacketHandler.Size = size;

		if (Settings.Fsk.FixLen == false)
		{
			WriteFifo((uint8_t*)&size, 1);
		}
		else
		{
			WriteReg(REG_PAYLOADLENGTH, size);
		}

		if ((size > 0) && (size <= 64))
		{
			Settings.FskPacketHandler.ChunkSize = size;
		}
		else
		{
			memcpy(RxTxBuffer, buffer, size);
			Settings.FskPacketHandler.ChunkSize = 32;
		}

		// Write payload buffer
		WriteFifo(buffer, Settings.FskPacketHandler.ChunkSize);
		Settings.FskPacketHandler.NbBytes += Settings.FskPacketHandler.ChunkSize;
		txTimeout = Settings.Fsk.TxTimeout;
	}
	break;
	case MODEM_LORA:
	{
		if (Settings.LoRa.TxIqInverted == true)
		{
			WriteReg(REG_LR_INVERTIQ, ((ReadReg(REG_LR_INVERTIQ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_ON));
			WriteReg(REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON);
		}
		else
		{
			WriteReg(REG_LR_INVERTIQ, ((ReadReg(REG_LR_INVERTIQ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF));
			WriteReg(REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF);
		}

		Settings.LoRaPacketHandler.Size = size;

		// Initializes the payload size
		WriteReg(REG_LR_PAYLOADLENGTH, size);

		// Full buffer used for Tx
		WriteReg(REG_LR_FIFOTXBASEADDR, 0);
		WriteReg(REG_LR_FIFOADDRPTR, 0);

		// FIFO operations can not take place in Sleep mode
		if ((ReadReg(REG_OPMODE) & ~RF_OPMODE_MASK) == RF_OPMODE_SLEEP)
		{
			SetStby();
			DelayMs(1);
		}
		// Write payload buffer
		WriteFifo(buffer, size);
		txTimeout = Settings.LoRa.TxTimeout;
	}
	break;
	}

	SetTx(txTimeout);
}


void SX1276::SetSleep(void)
{
	_timer->TimerStop(&RxTimeoutTimer);
	_timer->TimerStop(&TxTimeoutTimer);

	SetOpMode(RF_OPMODE_SLEEP);
	Settings.State = RF_IDLE;
}


void SX1276::SetStby(void)
{
	_timer->TimerStop(&RxTimeoutTimer);
	_timer->TimerStop(&TxTimeoutTimer);

	SetOpMode(RF_OPMODE_STANDBY);
	Settings.State = RF_IDLE;
}


void SX1276::Rx(uint32_t timeout)
{
	bool rxContinuous = false;

	switch (Settings.Modem)
	{
	case MODEM_FSK:
	{
		rxContinuous = Settings.Fsk.RxContinuous;

		// DIO0=PayloadReady
		// DIO1=FifoLevel
		// DIO2=SyncAddr
		// DIO3=FifoEmpty
		// DIO4=Preamble
		// DIO5=ModeReady
		WriteReg(REG_DIOMAPPING1, (ReadReg(REG_DIOMAPPING1) & RF_DIOMAPPING1_DIO0_MASK &
			RF_DIOMAPPING1_DIO1_MASK &
			RF_DIOMAPPING1_DIO2_MASK) |
			RF_DIOMAPPING1_DIO0_00 |
			RF_DIOMAPPING1_DIO1_00 |
			RF_DIOMAPPING1_DIO2_11);

		WriteReg(REG_DIOMAPPING2, (ReadReg(REG_DIOMAPPING2) & RF_DIOMAPPING2_DIO4_MASK &
			RF_DIOMAPPING2_MAP_MASK) |
			RF_DIOMAPPING2_DIO4_11 |
			RF_DIOMAPPING2_MAP_PREAMBLEDETECT);

		Settings.FskPacketHandler.FifoThresh = ReadReg(REG_FIFOTHRESH) & 0x3F;

		WriteReg(REG_RXCONFIG, RF_RXCONFIG_AFCAUTO_ON | RF_RXCONFIG_AGCAUTO_ON | RF_RXCONFIG_RXTRIGER_PREAMBLEDETECT);

		Settings.FskPacketHandler.PreambleDetected = false;
		Settings.FskPacketHandler.SyncWordDetected = false;
		Settings.FskPacketHandler.NbBytes = 0;
		Settings.FskPacketHandler.Size = 0;
	}
	break;
	case MODEM_LORA:
	{
		if (Settings.LoRa.RxIqInverted == true)
		{
			WriteReg(REG_LR_INVERTIQ, ((ReadReg(REG_LR_INVERTIQ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK) | RFLR_INVERTIQ_RX_ON | RFLR_INVERTIQ_TX_OFF));
			WriteReg(REG_LR_INVERTIQ2, RFLR_INVERTIQ2_ON);
		}
		else
		{
			WriteReg(REG_LR_INVERTIQ, ((ReadReg(REG_LR_INVERTIQ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK) | RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF));
			WriteReg(REG_LR_INVERTIQ2, RFLR_INVERTIQ2_OFF);
		}

		// ERRATA 2.3 - Receiver Spurious Reception of a LoRa Signal
		if (Settings.LoRa.Bandwidth < 9)
		{
			WriteReg(REG_LR_DETECTOPTIMIZE, ReadReg(REG_LR_DETECTOPTIMIZE) & 0x7F);
			WriteReg(REG_LR_TEST30, 0x00);
			switch (Settings.LoRa.Bandwidth)
			{
			case 0: // 7.8 kHz
				WriteReg(REG_LR_TEST2F, 0x48);
				SetChannel(Settings.Channel + 7810);
				break;
			case 1: // 10.4 kHz
				WriteReg(REG_LR_TEST2F, 0x44);
				SetChannel(Settings.Channel + 10420);
				break;
			case 2: // 15.6 kHz
				WriteReg(REG_LR_TEST2F, 0x44);
				SetChannel(Settings.Channel + 15620);
				break;
			case 3: // 20.8 kHz
				WriteReg(REG_LR_TEST2F, 0x44);
				SetChannel(Settings.Channel + 20830);
				break;
			case 4: // 31.2 kHz
				WriteReg(REG_LR_TEST2F, 0x44);
				SetChannel(Settings.Channel + 31250);
				break;
			case 5: // 41.4 kHz
				WriteReg(REG_LR_TEST2F, 0x44);
				SetChannel(Settings.Channel + 41670);
				break;
			case 6: // 62.5 kHz
				WriteReg(REG_LR_TEST2F, 0x40);
				break;
			case 7: // 125 kHz
				WriteReg(REG_LR_TEST2F, 0x40);
				break;
			case 8: // 250 kHz
				WriteReg(REG_LR_TEST2F, 0x40);
				break;
			}
		}
		else
		{
			WriteReg(REG_LR_DETECTOPTIMIZE, ReadReg(REG_LR_DETECTOPTIMIZE) | 0x80);
		}

		rxContinuous = Settings.LoRa.RxContinuous;

		if (Settings.LoRa.FreqHopOn == true)
		{
			WriteReg(REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
											 //RFLR_IRQFLAGS_RXDONE |
											 //RFLR_IRQFLAGS_PAYLOADCRCERROR |
				RFLR_IRQFLAGS_VALIDHEADER |
				RFLR_IRQFLAGS_TXDONE |
				RFLR_IRQFLAGS_CADDONE |
				//RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
				RFLR_IRQFLAGS_CADDETECTED);

			// DIO0=RxDone, DIO2=FhssChangeChannel
			WriteReg(REG_DIOMAPPING1, (ReadReg(REG_DIOMAPPING1) & RFLR_DIOMAPPING1_DIO0_MASK & RFLR_DIOMAPPING1_DIO2_MASK) | RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO2_00);
		}
		else
		{
			WriteReg(REG_LR_IRQFLAGSMASK, //RFLR_IRQFLAGS_RXTIMEOUT |
											 //RFLR_IRQFLAGS_RXDONE |
											 //RFLR_IRQFLAGS_PAYLOADCRCERROR |
				RFLR_IRQFLAGS_VALIDHEADER |
				RFLR_IRQFLAGS_TXDONE |
				RFLR_IRQFLAGS_CADDONE |
				RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
				RFLR_IRQFLAGS_CADDETECTED);

			// DIO0=RxDone
			WriteReg(REG_DIOMAPPING1, (ReadReg(REG_DIOMAPPING1) & RFLR_DIOMAPPING1_DIO0_MASK) | RFLR_DIOMAPPING1_DIO0_00);
		}
		WriteReg(REG_LR_FIFORXBASEADDR, 0);
		WriteReg(REG_LR_FIFOADDRPTR, 0);
	}
	break;
	}

	memset(RxTxBuffer, 0, (size_t)RX_BUFFER_SIZE);

	Settings.State = RF_RX_RUNNING;
	if (timeout != 0)
	{
		_timer->TimerSetValue(&RxTimeoutTimer, timeout);
		_timer->TimerStart(&RxTimeoutTimer);
	}

	if (Settings.Modem == MODEM_FSK)
	{
		SetOpMode(RF_OPMODE_RECEIVER);

		if (rxContinuous == false)
		{
			_timer->TimerSetValue(&RxTimeoutSyncWord, Settings.Fsk.RxSingleTimeout);
			_timer->TimerStart(&RxTimeoutSyncWord);
		}
	}
	else
	{
		if (rxContinuous == true)
		{
			SetOpMode(RFLR_OPMODE_RECEIVER);
		}
		else
		{
			SetOpMode(RFLR_OPMODE_RECEIVER_SINGLE);
		}

	}
}


void SX1276::SetTx(uint32_t timeout)
{
	_timer->TimerSetValue(&TxTimeoutTimer, timeout);

	switch (Settings.Modem)
	{
	case MODEM_FSK:
	{
		// DIO0=PacketSent
		// DIO1=FifoEmpty
		// DIO2=FifoFull
		// DIO3=FifoEmpty
		// DIO4=LowBat
		// DIO5=ModeReady
		WriteReg(REG_DIOMAPPING1, (ReadReg(REG_DIOMAPPING1) & RF_DIOMAPPING1_DIO0_MASK &
			RF_DIOMAPPING1_DIO1_MASK &
			RF_DIOMAPPING1_DIO2_MASK) |
			RF_DIOMAPPING1_DIO1_01);

		WriteReg(REG_DIOMAPPING2, (ReadReg(REG_DIOMAPPING2) & RF_DIOMAPPING2_DIO4_MASK &
			RF_DIOMAPPING2_MAP_MASK));
		Settings.FskPacketHandler.FifoThresh = ReadReg(REG_FIFOTHRESH) & 0x3F;
	}
	break;
	case MODEM_LORA:
	{
		if (Settings.LoRa.FreqHopOn == true)
		{
			WriteReg(REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
				RFLR_IRQFLAGS_RXDONE |
				RFLR_IRQFLAGS_PAYLOADCRCERROR |
				RFLR_IRQFLAGS_VALIDHEADER |
				//RFLR_IRQFLAGS_TXDONE |
				RFLR_IRQFLAGS_CADDONE |
				//RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
				RFLR_IRQFLAGS_CADDETECTED);

			// DIO0=TxDone, DIO2=FhssChangeChannel
			WriteReg(REG_DIOMAPPING1, (ReadReg(REG_DIOMAPPING1) & RFLR_DIOMAPPING1_DIO0_MASK & RFLR_DIOMAPPING1_DIO2_MASK) | RFLR_DIOMAPPING1_DIO0_01 | RFLR_DIOMAPPING1_DIO2_00);
		}
		else
		{
			WriteReg(REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
				RFLR_IRQFLAGS_RXDONE |
				RFLR_IRQFLAGS_PAYLOADCRCERROR |
				RFLR_IRQFLAGS_VALIDHEADER |
				//RFLR_IRQFLAGS_TXDONE |
				RFLR_IRQFLAGS_CADDONE |
				RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
				RFLR_IRQFLAGS_CADDETECTED);

			// DIO0=TxDone
			WriteReg(REG_DIOMAPPING1, (ReadReg(REG_DIOMAPPING1) & RFLR_DIOMAPPING1_DIO0_MASK) | RFLR_DIOMAPPING1_DIO0_01);
		}
	}
	break;
	}

	Settings.State = RF_TX_RUNNING;
	_timer->TimerStart(&TxTimeoutTimer);
	SetOpMode(RF_OPMODE_TRANSMITTER);
}


void SX1276::StartCad( void )
{
    switch( Settings.Modem )
    {
    case MODEM_FSK:
        {

        }
        break;
    case MODEM_LORA:
        {
            WriteReg( REG_LR_IRQFLAGSMASK, RFLR_IRQFLAGS_RXTIMEOUT |
                                        RFLR_IRQFLAGS_RXDONE |
                                        RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                        RFLR_IRQFLAGS_VALIDHEADER |
                                        RFLR_IRQFLAGS_TXDONE |
                                        //RFLR_IRQFLAGS_CADDONE |
                                        RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL // |
                                        //RFLR_IRQFLAGS_CADDETECTED
                                        );

            // DIO3=CADDone
            WriteReg( REG_DIOMAPPING1, ( ReadReg( REG_DIOMAPPING1 ) & RFLR_DIOMAPPING1_DIO3_MASK ) | RFLR_DIOMAPPING1_DIO3_00 );

            Settings.State = RF_CAD;
            SetOpMode( RFLR_OPMODE_CAD );
        }
        break;
    default:
        break;
    }
}

void SX1276::SetTxContinuousWave( uint32_t freq, int8_t power, uint16_t time )
{
    uint32_t timeout = ( uint32_t )( time * 1000 );

    SetChannel( freq );

    SetTxConfig( MODEM_FSK, power, 0, 0, 4800, 0, 5, false, false, 0, 0, 0, timeout );

    WriteReg( REG_PACKETCONFIG2, ( ReadReg( REG_PACKETCONFIG2 ) & RF_PACKETCONFIG2_DATAMODE_MASK ) );
    // Disable radio interrupts
    WriteReg( REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_11 | RF_DIOMAPPING1_DIO1_11 );
    WriteReg( REG_DIOMAPPING2, RF_DIOMAPPING2_DIO4_10 | RF_DIOMAPPING2_DIO5_10 );

    _timer->TimerSetValue( &TxTimeoutTimer, timeout );

    Settings.State = RF_TX_RUNNING;
	_timer->TimerStart( &TxTimeoutTimer );
    SetOpMode( RF_OPMODE_TRANSMITTER );
}

int16_t SX1276::ReadRssi( RadioModems_t modem )
{
    int16_t rssi = 0;

    switch( modem )
    {
    case MODEM_FSK:
        rssi = -( ReadReg( REG_RSSIVALUE ) >> 1 );
        break;
    case MODEM_LORA:
        if( Settings.Channel > RF_MID_BAND_THRESH )
        {
            rssi = RSSI_OFFSET_HF + ReadReg( REG_LR_RSSIVALUE );
        }
        else
        {
            rssi = RSSI_OFFSET_LF + ReadReg( REG_LR_RSSIVALUE );
        }
        break;
    default:
        rssi = -1;
        break;
    }
    return rssi;
}


void SX1276::Reset()
{
	DelayMs(20);
	// Set RESET pin to 0
	gpio_set(RADIO_RESET, 0);

	// Wait 1 ms
	DelayMs(10);

	// Configure RESET as input
	gpio_set(RADIO_RESET, 1);

	// Wait 6 ms
	DelayMs(60);
}

void SX1276::SetOpMode(uint8_t opMode)
{
	SetAntSw(opMode);

	WriteReg(REG_OPMODE, (ReadReg(REG_OPMODE) & RF_OPMODE_MASK) | opMode);
}

void SX1276::SetModem(RadioModems_t modem)
{
	if ((ReadReg(REG_OPMODE) & RFLR_OPMODE_LONGRANGEMODE_ON) != 0)
	{
		Settings.Modem = MODEM_LORA;
	}
	else
	{
		Settings.Modem = MODEM_FSK;
	}

	if (Settings.Modem == modem)
	{
		return;
	}

	Settings.Modem = modem;
	switch (Settings.Modem)
	{
	default:
	case MODEM_FSK:
		SetSleep();
		WriteReg(REG_OPMODE, (ReadReg(REG_OPMODE) & RFLR_OPMODE_LONGRANGEMODE_MASK) | RFLR_OPMODE_LONGRANGEMODE_OFF);

		WriteReg(REG_DIOMAPPING1, 0x00);
		WriteReg(REG_DIOMAPPING2, 0x30); // DIO5=ModeReady
		break;
	case MODEM_LORA:
		SetSleep();
		WriteReg(REG_OPMODE, (ReadReg(REG_OPMODE) & RFLR_OPMODE_LONGRANGEMODE_MASK) | RFLR_OPMODE_LONGRANGEMODE_ON);

		WriteReg(REG_DIOMAPPING1, 0x00);
		WriteReg(REG_DIOMAPPING2, 0x00);
		break;
	}
}


void SX1276::WriteReg(uint8_t addr, uint8_t data)
{
	Spi_write(addr, data);
}

uint8_t SX1276::ReadReg(uint8_t addr)
{
	uint8_t data;
	Spi_read(addr, &data);
	return data;
}

void SX1276::WriteBuffer(uint8_t addr, uint8_t *buffer, uint8_t size)
{
	Spi_wb(addr, buffer, size);
}

void SX1276::ReadBuffer(uint8_t addr, uint8_t *buffer, uint8_t size)
{
	Spi_rb(addr, buffer, size);
}

void SX1276::WriteFifo(uint8_t *buffer, uint8_t size)
{
	this->WriteBuffer(0, buffer, size);
}

void SX1276::ReadFifo(uint8_t *buffer, uint8_t size)
{
	this->ReadBuffer(0, buffer, size);
}


void SX1276::SetMaxPayloadLength( RadioModems_t modem, uint8_t max )
{
    SetModem( modem );

    switch( modem )
    {
    case MODEM_FSK:
        if( Settings.Fsk.FixLen == false )
        {
            WriteReg( REG_PAYLOADLENGTH, max );
        }
        break;
    case MODEM_LORA:
        WriteReg( REG_LR_PAYLOADMAXLENGTH, max );
        break;
    }
}
void SX1276::SetPublicNetwork(bool enable)
{
	SetModem(MODEM_LORA);
	Settings.LoRa.PublicNetwork = enable;
	if (enable == true)
	{
		// Change LoRa modem SyncWord
		WriteReg(REG_LR_SYNCWORD, LORA_MAC_PUBLIC_SYNCWORD);
	}
	else
	{
		// Change LoRa modem SyncWord
		WriteReg(REG_LR_SYNCWORD, LORA_MAC_PRIVATE_SYNCWORD);
	}
}

bool SX1276::OnTimeoutIrq(void *user)
{
	switch (Settings.State)
	{
	case RF_RX_RUNNING:
		if (Settings.Modem == MODEM_FSK)
		{
			Settings.FskPacketHandler.PreambleDetected = false;
			Settings.FskPacketHandler.SyncWordDetected = false;
			Settings.FskPacketHandler.NbBytes = 0;
			Settings.FskPacketHandler.Size = 0;

			// Clear Irqs
			WriteReg(REG_IRQFLAGS1, RF_IRQFLAGS1_RSSI |
				RF_IRQFLAGS1_PREAMBLEDETECT |
				RF_IRQFLAGS1_SYNCADDRESSMATCH);
			WriteReg(REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN);

			if (Settings.Fsk.RxContinuous == true)
			{
				// Continuous mode restart Rx chain
				WriteReg(REG_RXCONFIG, ReadReg(REG_RXCONFIG) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK);
				_timer->TimerStart(&RxTimeoutSyncWord);
			}
			else
			{
				Settings.State = RF_IDLE;
				_timer->TimerStop(&RxTimeoutSyncWord);
			}
		}
		if (_event != NULL)
		{
			_event->OnRxTimeout();
		}
		break;
	case RF_TX_RUNNING:
		// Tx timeout shouldn't happen.
		// But it has been observed that when it happens it is a result of a corrupted SPI transfer
		// it depends on the platform design.
		//
		// The workaround is to put the radio in a known state. Thus, we re-initialize it.

		// BEGIN WORKAROUND
/*
		// Reset the radio
		Reset();

		// Calibrate Rx chain
		RxChainCalibration();

		// Initialize radio default values
		SetOpMode(RF_OPMODE_SLEEP);

		for (uint8_t i = 0; i < sizeof(RadioRegsInit) / sizeof(RadioRegisters_t); i++)
		{
			SetModem(RadioRegsInit[i].Modem);
			WriteReg(RadioRegsInit[i].Addr, RadioRegsInit[i].Value);
		}
		SetModem(MODEM_FSK);

		// Restore previous network type setting.
		SetPublicNetwork(Settings.LoRa.PublicNetwork);
		// END WORKAROUND
*/
		Settings.State = RF_IDLE;
		if (_event != NULL)
		{
			_event->OnTxTimeout();
		}
		break;
	default:
		break;
	}
	return true;
}

void SX1276::OnDio0Irq(void)
{
	volatile uint8_t irqFlags = 0;

	switch (Settings.State)
	{
	case RF_RX_RUNNING:
		//_timer->TimerStop( &RxTimeoutTimer );
		// RxDone interrupt
		switch (Settings.Modem)
		{
		case MODEM_FSK:
			if (Settings.Fsk.CrcOn == true)
			{
				irqFlags = ReadReg(REG_IRQFLAGS2);
				if ((irqFlags & RF_IRQFLAGS2_CRCOK) != RF_IRQFLAGS2_CRCOK)
				{
					// Clear Irqs
					WriteReg(REG_IRQFLAGS1, RF_IRQFLAGS1_RSSI |
						RF_IRQFLAGS1_PREAMBLEDETECT |
						RF_IRQFLAGS1_SYNCADDRESSMATCH);
					WriteReg(REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN);

					_timer->TimerStop(&RxTimeoutTimer);

					if (Settings.Fsk.RxContinuous == false)
					{
						_timer->TimerStop(&RxTimeoutSyncWord);
						Settings.State = RF_IDLE;
					}
					else
					{
						// Continuous mode restart Rx chain
						WriteReg(REG_RXCONFIG, ReadReg(REG_RXCONFIG) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK);
						_timer->TimerStart(&RxTimeoutSyncWord);
					}

					if (_event != NULL) 
					{
						_event->OnRxError();
					}
					Settings.FskPacketHandler.PreambleDetected = false;
					Settings.FskPacketHandler.SyncWordDetected = false;
					Settings.FskPacketHandler.NbBytes = 0;
					Settings.FskPacketHandler.Size = 0;
					break;
				}
			}

			// Read received packet size
			if ((Settings.FskPacketHandler.Size == 0) && (Settings.FskPacketHandler.NbBytes == 0))
			{
				if (Settings.Fsk.FixLen == false)
				{
					ReadFifo((uint8_t*)&Settings.FskPacketHandler.Size, 1);
				}
				else
				{
					Settings.FskPacketHandler.Size = ReadReg(REG_PAYLOADLENGTH);
				}
				ReadFifo(RxTxBuffer + Settings.FskPacketHandler.NbBytes, Settings.FskPacketHandler.Size - Settings.FskPacketHandler.NbBytes);
				Settings.FskPacketHandler.NbBytes += (Settings.FskPacketHandler.Size - Settings.FskPacketHandler.NbBytes);
			}
			else
			{
				ReadFifo(RxTxBuffer + Settings.FskPacketHandler.NbBytes, Settings.FskPacketHandler.Size - Settings.FskPacketHandler.NbBytes);
				Settings.FskPacketHandler.NbBytes += (Settings.FskPacketHandler.Size - Settings.FskPacketHandler.NbBytes);
			}

			_timer->TimerStop(&RxTimeoutTimer);

			if (Settings.Fsk.RxContinuous == false)
			{
				Settings.State = RF_IDLE;
				_timer->TimerStop(&RxTimeoutSyncWord);
			}
			else
			{
				// Continuous mode restart Rx chain
				WriteReg(REG_RXCONFIG, ReadReg(REG_RXCONFIG) | RF_RXCONFIG_RESTARTRXWITHOUTPLLLOCK);
				_timer->TimerStart(&RxTimeoutSyncWord);
			}

			if (_event != NULL)
			{
				_event->OnRxDone(RxTxBuffer, Settings.FskPacketHandler.Size, Settings.FskPacketHandler.RssiValue, 0);
			}
			Settings.FskPacketHandler.PreambleDetected = false;
			Settings.FskPacketHandler.SyncWordDetected = false;
			Settings.FskPacketHandler.NbBytes = 0;
			Settings.FskPacketHandler.Size = 0;
			break;
		case MODEM_LORA:
		{
			int8_t snr = 0;

			// Clear Irq
			WriteReg(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE);

			irqFlags = ReadReg(REG_LR_IRQFLAGS);
			if ((irqFlags & RFLR_IRQFLAGS_PAYLOADCRCERROR_MASK) == RFLR_IRQFLAGS_PAYLOADCRCERROR)
			{
				// Clear Irq
				WriteReg(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_PAYLOADCRCERROR);

				if (Settings.LoRa.RxContinuous == false)
				{
					Settings.State = RF_IDLE;
				}
				_timer->TimerStop(&RxTimeoutTimer);

				if (_event != NULL)
				{
					_event->OnRxError();
				}
				break;
			}

			Settings.LoRaPacketHandler.SnrValue = ReadReg(REG_LR_PKTSNRVALUE);
			if (Settings.LoRaPacketHandler.SnrValue & 0x80) // The SNR sign bit is 1
			{
				// Invert and divide by 4
				snr = ((~Settings.LoRaPacketHandler.SnrValue + 1) & 0xFF) >> 2;
				snr = -snr;
			}
			else
			{
				// Divide by 4
				snr = (Settings.LoRaPacketHandler.SnrValue & 0xFF) >> 2;
			}

			int16_t rssi = ReadReg(REG_LR_PKTRSSIVALUE);
			if (snr < 0)
			{
				if (Settings.Channel > RF_MID_BAND_THRESH)
				{
					Settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_HF + rssi + (rssi >> 4) +
						snr;
				}
				else
				{
					Settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_LF + rssi + (rssi >> 4) +
						snr;
				}
			}
			else
			{
				if (Settings.Channel > RF_MID_BAND_THRESH)
				{
					Settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_HF + rssi + (rssi >> 4);
				}
				else
				{
					Settings.LoRaPacketHandler.RssiValue = RSSI_OFFSET_LF + rssi + (rssi >> 4);
				}
			}

			Settings.LoRaPacketHandler.Size = ReadReg(REG_LR_RXNBBYTES);
			WriteReg(REG_LR_FIFOADDRPTR, ReadReg(REG_LR_FIFORXCURRENTADDR));
			ReadFifo(RxTxBuffer, Settings.LoRaPacketHandler.Size);

			if (Settings.LoRa.RxContinuous == false)
			{
				Settings.State = RF_IDLE;
			}
			_timer->TimerStop(&RxTimeoutTimer);

			if (_event != NULL)
			{
				_event->OnRxDone(RxTxBuffer, Settings.LoRaPacketHandler.Size, Settings.LoRaPacketHandler.RssiValue, Settings.LoRaPacketHandler.SnrValue);
			}
		}
		break;
		default:
			break;
		}
		break;
	case RF_TX_RUNNING:
		_timer->TimerStop(&TxTimeoutTimer);
		// TxDone interrupt
		switch (Settings.Modem)
		{
		case MODEM_LORA:
			// Clear Irq
			WriteReg(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE);
			// Intentional fall through
		case MODEM_FSK:
		default:
			Settings.State = RF_IDLE;
			if (_event != NULL)
			{
				_event->OnTxDone();
			}
			break;
		}
		break;
	default:
		break;
	}
}

void SX1276::OnDio1Irq(void)
{
	switch (Settings.State)
	{
	case RF_RX_RUNNING:
		switch (Settings.Modem)
		{
		case MODEM_FSK:
			// FifoLevel interrupt
			// Read received packet size
			if ((Settings.FskPacketHandler.Size == 0) && (Settings.FskPacketHandler.NbBytes == 0))
			{
				if (Settings.Fsk.FixLen == false)
				{
					ReadFifo((uint8_t*)&Settings.FskPacketHandler.Size, 1);
				}
				else
				{
					Settings.FskPacketHandler.Size = ReadReg(REG_PAYLOADLENGTH);
				}
			}

			if ((Settings.FskPacketHandler.Size - Settings.FskPacketHandler.NbBytes) > Settings.FskPacketHandler.FifoThresh)
			{
				ReadFifo((RxTxBuffer + Settings.FskPacketHandler.NbBytes), Settings.FskPacketHandler.FifoThresh);
				Settings.FskPacketHandler.NbBytes += Settings.FskPacketHandler.FifoThresh;
			}
			else
			{
				ReadFifo((RxTxBuffer + Settings.FskPacketHandler.NbBytes), Settings.FskPacketHandler.Size - Settings.FskPacketHandler.NbBytes);
				Settings.FskPacketHandler.NbBytes += (Settings.FskPacketHandler.Size - Settings.FskPacketHandler.NbBytes);
			}
			break;
		case MODEM_LORA:
			// Sync time out
			_timer->TimerStop(&RxTimeoutTimer);
			// Clear Irq
			WriteReg(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXTIMEOUT);

			Settings.State = RF_IDLE;
			if (_event != NULL)
			{
				_event->OnRxTimeout();
			}
			break;
		default:
			break;
		}
		break;
	case RF_TX_RUNNING:
		switch (Settings.Modem)
		{
		case MODEM_FSK:
			// FifoEmpty interrupt
			if ((Settings.FskPacketHandler.Size - Settings.FskPacketHandler.NbBytes) > Settings.FskPacketHandler.ChunkSize)
			{
				WriteFifo((RxTxBuffer + Settings.FskPacketHandler.NbBytes), Settings.FskPacketHandler.ChunkSize);
				Settings.FskPacketHandler.NbBytes += Settings.FskPacketHandler.ChunkSize;
			}
			else
			{
				// Write the last chunk of data
				WriteFifo(RxTxBuffer + Settings.FskPacketHandler.NbBytes, Settings.FskPacketHandler.Size - Settings.FskPacketHandler.NbBytes);
				Settings.FskPacketHandler.NbBytes += Settings.FskPacketHandler.Size - Settings.FskPacketHandler.NbBytes;
			}
			break;
		case MODEM_LORA:
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

void SX1276::OnDio2Irq(void)
{
	switch (Settings.State)
	{
	case RF_RX_RUNNING:
		switch (Settings.Modem)
		{
		case MODEM_FSK:
			// Checks if DIO4 is connected. If it is not PreambleDetected is set to true.
			//if (SX1276.DIO4.port == NULL)
			//{
			//	Settings.FskPacketHandler.PreambleDetected = true;
			//}

			if ((Settings.FskPacketHandler.PreambleDetected == true) && (Settings.FskPacketHandler.SyncWordDetected == false))
			{
				_timer->TimerStop(&RxTimeoutSyncWord);

				Settings.FskPacketHandler.SyncWordDetected = true;

				Settings.FskPacketHandler.RssiValue = -(ReadReg(REG_RSSIVALUE) >> 1);

				Settings.FskPacketHandler.AfcValue = (int32_t)(double)(((uint16_t)ReadReg(REG_AFCMSB) << 8) |
					(uint16_t)ReadReg(REG_AFCLSB)) *
					(double)FREQ_STEP;
				Settings.FskPacketHandler.RxGain = (ReadReg(REG_LNA) >> 5) & 0x07;
			}
			break;
		case MODEM_LORA:
			if (Settings.LoRa.FreqHopOn == true)
			{
				// Clear Irq
				WriteReg(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL);

				if (_event != NULL)
				{
					_event->OnFhssChangeChannel((ReadReg(REG_LR_HOPCHANNEL) & RFLR_HOPCHANNEL_CHANNEL_MASK));
				}
			}
			break;
		default:
			break;
		}
		break;
	case RF_TX_RUNNING:
		switch (Settings.Modem)
		{
		case MODEM_FSK:
			break;
		case MODEM_LORA:
			if (Settings.LoRa.FreqHopOn == true)
			{
				// Clear Irq
				WriteReg(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL);

				if (_event != NULL)
				{
					_event->OnFhssChangeChannel((ReadReg(REG_LR_HOPCHANNEL) & RFLR_HOPCHANNEL_CHANNEL_MASK));
				}
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

void SX1276::OnDio3Irq(void)
{
	switch (Settings.Modem)
	{
	case MODEM_FSK:
		break;
	case MODEM_LORA:
		if ((ReadReg(REG_LR_IRQFLAGS) & RFLR_IRQFLAGS_CADDETECTED) == RFLR_IRQFLAGS_CADDETECTED)
		{
			// Clear Irq
			WriteReg(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDETECTED | RFLR_IRQFLAGS_CADDONE);
			if (_event != NULL)
			{
				_event->OnCadDone(true);
			}
		}
		else
		{
			// Clear Irq
			WriteReg(REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDONE);
			if (_event != NULL)
			{
				_event->OnCadDone(false);
			}
		}
		break;
	default:
		break;
	}
}

void SX1276::OnDio4Irq(void)
{
	switch (Settings.Modem)
	{
	case MODEM_FSK:
	{
		if (Settings.FskPacketHandler.PreambleDetected == false)
		{
			Settings.FskPacketHandler.PreambleDetected = true;
		}
	}
	break;
	case MODEM_LORA:
		break;
	default:
		break;
	}
}

void SX1276::OnDio5Irq(void)
{
	switch (Settings.Modem)
	{
	case MODEM_FSK:
		break;
	case MODEM_LORA:
		break;
	default:
		break;
	}
}


void SX1276::SetRfTxPower(int8_t power)
{
	uint8_t paConfig = 0;
	uint8_t paDac = 0;

	paConfig = ReadReg(REG_PACONFIG);
	paDac = ReadReg(REG_PADAC);

	paConfig = (paConfig & RF_PACONFIG_PASELECT_MASK) | GetPaSelect(Settings.Channel);
	paConfig = (paConfig & RF_PACONFIG_MAX_POWER_MASK) | 0x70;

	if ((paConfig & RF_PACONFIG_PASELECT_PABOOST) == RF_PACONFIG_PASELECT_PABOOST)
	{
		if (power > 17)
		{
			paDac = (paDac & RF_PADAC_20DBM_MASK) | RF_PADAC_20DBM_ON;
		}
		else
		{
			paDac = (paDac & RF_PADAC_20DBM_MASK) | RF_PADAC_20DBM_OFF;
		}
		if ((paDac & RF_PADAC_20DBM_ON) == RF_PADAC_20DBM_ON)
		{
			if (power < 5)
			{
				power = 5;
			}
			if (power > 20)
			{
				power = 20;
			}
			paConfig = (paConfig & RF_PACONFIG_OUTPUTPOWER_MASK) | (uint8_t)((uint16_t)(power - 5) & 0x0F);
		}
		else
		{
			if (power < 2)
			{
				power = 2;
			}
			if (power > 17)
			{
				power = 17;
			}
			paConfig = (paConfig & RF_PACONFIG_OUTPUTPOWER_MASK) | (uint8_t)((uint16_t)(power - 2) & 0x0F);
		}
	}
	else
	{
		if (power < -1)
		{
			power = -1;
		}
		if (power > 14)
		{
			power = 14;
		}
		paConfig = (paConfig & RF_PACONFIG_OUTPUTPOWER_MASK) | (uint8_t)((uint16_t)(power + 1) & 0x0F);
	}
	WriteReg(REG_PACONFIG, paConfig);
	WriteReg(REG_PADAC, paDac);
	//printf("TX_POWER = %d\n", power);
}

uint8_t SX1276::GetPaSelect(uint32_t channel)
{
	if (channel < RF_MID_BAND_THRESH)
	{
		return RF_PACONFIG_PASELECT_PABOOST;
	}
	else
	{
		return RF_PACONFIG_PASELECT_RFO;
	}
}

void SX1276::SetAntSw(uint8_t opMode)
{
	switch (opMode)
	{
	case RFLR_OPMODE_TRANSMITTER:
		gpio_set(RADIO_RTX, 1);
		break;
	case RFLR_OPMODE_RECEIVER:
	case RFLR_OPMODE_RECEIVER_SINGLE:
	case RFLR_OPMODE_CAD:
	default:
		gpio_set(RADIO_RTX, 0);
		break;
	}
}

