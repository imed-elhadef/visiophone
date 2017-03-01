#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/fs.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include "mcp9808.h"
#include "i2c_bus.c"

#define CHECK_I2C_FUNC( var, label ) \
	do { 	if(0 == (var & label)) { \
		fprintf(stderr, "\nError: " \
			#label " function is required. Program halted.\n\n"); \
		exit(1); } \
	} while(0);


static int i2c_write_1b(struct mcp9808 *e, __u8 buf)
{
	int r;
	// we must simulate a plain I2C byte write with SMBus functions
	r = i2c_smbus_write_byte(e->fd, buf);
	if(r < 0)
		fprintf(stderr, "Error i2c_write_1b: %s\n", strerror(errno));
	usleep(10);
         
	return r;
}

static int i2c_write_2b(struct mcp9808 *e, __u8 buf[2])
{
	int r;
	// we must simulate a plain I2C byte write with SMBus functions
	r = i2c_smbus_write_byte_data(e->fd, buf[0], buf[1]);
	if(r < 0)
		fprintf(stderr, "Error i2c_write_2b: %s\n", strerror(errno));
	//usleep(10);
        usleep(1500);
	return r;
}

static int i2c_write_3b(struct mcp9808 *e, __u8 buf[3])
{
	int r;
	// we must simulate a plain I2C byte write with SMBus functions
	// the __u16 data field will be byte swapped by the SMBus protocol
	r = i2c_smbus_write_word_data(e->fd, buf[0], buf[2] << 8 | buf[1]);
	if(r < 0)
		fprintf(stderr, "Error i2c_write_3b: %s\n", strerror(errno));
	usleep(10);
	return r;
}



bool mcp9808_open(char *dev_fqn, int addr, struct mcp9808* e)
{
	int funcs, fd, r;
	e->fd = e->addr = 0;
	e->dev = 0;
	
	fd = open(dev_fqn, O_RDWR);
	if(fd <= 0)
	{
		fprintf(stderr, "Error mcp9808_open: %s\n", strerror(errno));
		return false;
	}

	// get funcs list
	if((r = ioctl(fd, I2C_FUNCS, &funcs) < 0))
	{
		fprintf(stderr, "Error mcp9808_open: %s\n", strerror(errno));
		return false;
	}

	
	// check for req funcs
	CHECK_I2C_FUNC( funcs, I2C_FUNC_SMBUS_READ_BYTE );
	CHECK_I2C_FUNC( funcs, I2C_FUNC_SMBUS_WRITE_BYTE );
	CHECK_I2C_FUNC( funcs, I2C_FUNC_SMBUS_READ_BYTE_DATA );
	CHECK_I2C_FUNC( funcs, I2C_FUNC_SMBUS_WRITE_BYTE_DATA );
	CHECK_I2C_FUNC( funcs, I2C_FUNC_SMBUS_READ_WORD_DATA );
	CHECK_I2C_FUNC( funcs, I2C_FUNC_SMBUS_WRITE_WORD_DATA );

	// set working device
	if( ( r = ioctl(fd, I2C_SLAVE, addr)) < 0)
	{
		fprintf(stderr, "Error mcp9808_open: %s\n", strerror(errno));
		return false;
	}
	e->fd = fd;
	e->addr = addr;
	e->dev = dev_fqn;
         
        uint16_t manuf_id = (uint16_t) i2c_smbus_read_word_data(e->fd, MCP9808_MANUF_ID_REG);
	uint16_t device_id = (uint16_t) i2c_smbus_read_word_data(e->fd, MCP9808_DEVICE_ID_REG);
	e->manuf_id = ((manuf_id & 0x00FF)<<8) | ((manuf_id & 0xFF00)>>8);
	e->device_id = ((device_id & 0x00FF)<<8) | ((device_id & 0xFF00)>>8);
        
        printf("manuf_id = 0x%04x, device_id = 0x%04x\n",e->manuf_id,e->device_id);
        if (e->manuf_id!=0x0054)
        return false;
	if (e->device_id!=0x0400)
        return false;

	return true;
}

int mcp9808_close(struct mcp9808 *e)
{
	close(e->fd);
	e->fd = -1;
	e->dev = 0;
	return 0;
}

float mcp9808_read_temperature(struct mcp9808 *e)
 { 
 	
	// swap order of the msb and lsb bytes
	uint16_t temperature_word = i2c_smbus_read_word_data(e->fd, MCP9808_TEMP_REG);
	uint16_t raw_temperature = ((temperature_word & 0x00FF)<<8) | ((temperature_word & 0xFF00)>>8);
	
	float temperature = raw_temperature & 0x0FFF; // extract the first three bytes
	temperature /= 16.0;
	
	if(raw_temperature & 0x1000) { // check sign bit
		temperature -= 256.0;
	}	
	//printf("temperature: %0.2f\n",temperature);
	
	return temperature;
 
  }


int mcp9808_read_current_byte(struct mcp9808* e)
{
	ioctl(e->fd, BLKFLSBUF); // clear kernel read buffer
	return i2c_smbus_read_byte(e->fd);
}

int mcp9808_read_byte(struct mcp9808* e, __u8 reg_addr)
{
	int r;
	ioctl(e->fd, BLKFLSBUF); // clear kernel read buffer
		r = i2c_write_1b(e, reg_addr);
	if (r < 0)
		return r;
	r = i2c_smbus_read_byte(e->fd);
	return r;
}

int mcp9808_write_byte(struct mcp9808 *e, __u8 reg_addr, __u8 data)
{
	
		__u8 buf[2] = { reg_addr, data };
		return i2c_write_2b(e, buf);
	
}


