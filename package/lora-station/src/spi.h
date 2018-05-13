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
#ifndef __SPI_H__
#define __SPI_H__

#define LGW_SPI_SUCCESS     0
#define LGW_SPI_ERROR       -1
#define LGW_BURST_CHUNK     15

int SpiInit(void);
int Spi_write(uint8_t address, uint8_t data);
int Spi_read(uint8_t address, uint8_t *data);
int Spi_wb(uint8_t address, uint8_t *data, uint16_t size);
int Spi_rb(uint8_t address, uint8_t *data, uint16_t size);
int Spi_Close(void);

#endif //__SPI_H__
