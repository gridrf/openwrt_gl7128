/*
 * THE FOLLOWING FIRMWARE IS PROVIDED: (1) "AS IS" WITH NO WARRANTY; AND 
 * (2)TO ENABLE ACCESS TO CODING INFORMATION TO GUIDE AND FACILITATE CUSTOMER.
 * CONSEQUENTLY, SEMTECH SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT OR
 * CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT
 * OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION
 * CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 * 
 * Copyright (C) SEMTECH S.A.
 */
/*! 
 * \file       spi.h
 * \brief      SPI hardware driver
 *
 * \version    1.0
 * \date       Feb 12 2010
 * \author     Miguel Luis
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
