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

#include "board.h"
#include "gpio.h"
#include <stdint.h>
#include <time.h>

#define SPI_CS			    2               
#define SCLK                        3
#define MOSI                        1
#define MISO                        0

void spi_delay()
{
	for(int i=0;i<50;i++)
		asm("nop");
}

void SpiInit()
{
	gpio_setdirecton(SPI_CS, 1);
	gpio_setdirecton(SCLK, 1);
	gpio_setdirecton(MOSI, 1);
	gpio_setdirecton(MISO, 0);
	gpio_set(SCLK, 0);                    //CPOL=0 
	gpio_set(MOSI, 0);
	gpio_set(SPI_CS, 1);
}

static uint8_t spi_readwrite(uint8_t dat)
{
	uint8_t i, ret = 0;
	for (i = 0; i<8; i++)
	{
		ret <<= 1;
		if (dat & 0x80)
			gpio_set(MOSI, 1); //MOSI = 1;
		else
			gpio_set(MOSI, 0); //MOSI = 0;

		spi_delay();

		if (gpio_read(MISO))
			ret |= 1;

		gpio_set(SCLK, 1);// SCK = 1;
		spi_delay();
		gpio_set(SCLK, 0);// SCK = 0;
		dat <<= 1;
	}
	return ret;
}

int Spi_write(uint8_t address, uint8_t data)
{
	gpio_set(SPI_CS, 0);
	spi_readwrite(0x80 | (address & 0x7F));
	spi_readwrite(data);
	gpio_set(SPI_CS, 1);
	usleep(10);
	return 0;
}

int Spi_read(uint8_t address, uint8_t *data)
{                  
	gpio_set(SPI_CS, 0);
	spi_readwrite(address & 0x7F);
	*data = spi_readwrite(0x0);
	gpio_set(SPI_CS, 1);
	usleep(10);
	return 0;
}

int Spi_wb(uint8_t address, uint8_t *data, uint16_t size)
{
	gpio_set(SPI_CS, 0);
	spi_readwrite(0x80 | (address & 0x7F));
	for (int i = 0; i < size; i++) {
		spi_readwrite(data[i]);
	}
	gpio_set(SPI_CS, 1);
	usleep(10);
	return 0;
}

int Spi_rb(uint8_t address, uint8_t *data, uint16_t size)
{
	gpio_set(SPI_CS, 0);
	spi_readwrite(address & 0x7F);
	for (int i = 0; i < size; i++) {
		*data++ = spi_readwrite(0x0);
	}
	gpio_set(SPI_CS, 1);
	usleep(10);
	return 0;
}

int Spi_Close(void)
{
	return 0;
}
