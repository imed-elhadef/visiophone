/****************************************************************************/
 //   copyright            : (C) by 2016 Imed Elhadef
   
  
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef _MCP9808_H_
#define _MCP9808_H_


#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdbool.h>
#include <stdint.h>


#define MCP9808_ADR                0x18// The three pins A0,A1 et A2 are connected to GND (A verifier)

#define MCP9808_UPPER_TMP_REG      0x02
#define MCP9808_LOWER_TMP_REG      0x03
#define MCP9808_TEMP_REG           0x05
#define MCP9808_MANUF_ID_REG	   0x06
#define MCP9808_DEVICE_ID_REG	   0x07


struct mcp9808
{
	char *dev; 	// device file i.e. /dev/i2c-N
	int addr;	// i2c address
	int fd;		// file descriptor
        int manuf_id;  //sensor specific data
        int device_id; //sensor specific data
};

/*
 * opens the MCP9808 device at [dev_fqn] (i.e. /dev/i2c-N) whose address is
 * [addr] and set the mcp9808
 */
bool mcp9808_open(char *dev_fqn, int addr, struct mcp9808 *e);
/*
 * closes the mcp9808 device [e] 
 */
int mcp9808_close(struct mcp9808 *e);

/**
 * Returns the measured temperature in celsius.
 * 
 */
float mcp9808_read_temperature(struct mcp9808 *e);

/*
 * read and returns the mcp9808 byte at reg address [reg_addr] 
 * Note: mcp9808 must have been selected by ioctl(fd,I2C_SLAVE,address) 
 */
int mcp9808_read_byte(struct mcp9808* e, __u8 reg_addr);
/*
 * read the current byte
 * Note: mcp9808 must have been selected by ioctl(fd,I2C_SLAVE,address) 
 */
int mcp9808_read_current_byte(struct mcp9808 *e);
/*
 * writes [data] at reg address [reg_addr] 
 * Note: mcp9808 must have been selected by ioctl(fd,I2C_SLAVE,address) 
 */
int mcp9808_write_byte(struct mcp9808 *e, __u8 reg_addr, __u8 data);



#endif

