/*
 * eeprom.c
 *
 *  Created on: 26-May-2023
 *      Author: gb
 */
/*******************************************************************************
* Header Files
*******************************************************************************/
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"


/*******************************************************************************
* Macros
*******************************************************************************/

/* I2C bus frequency */
#define I2C_FREQ                (400000UL)

#define EEPROM_BLOCK_SIZE		(256UL)

#define DEVICE_ID_LOCATION		(0UL)
#define WIFI_SSID_LOCATION		(8UL)
#define WIFI_PWD_LOCATION		(40UL)
#define CONTROL_INFO_LOCATION	(104UL)
#define FLORATE_LOCATION		(116UL)

#define WRITE_DELAY		(1000UL)

/*******************************************************************************
* Global Variables
*******************************************************************************/
cyhal_i2c_t mI2C;
cyhal_i2c_cfg_t mI2C_cfg;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/


/*******************************************************************************
* Function Definitions
*******************************************************************************/

/*******************************************************************************
* Function Name: handle_error
********************************************************************************
* Summary:
* User defined error handling function
*
* Parameters:
*  uint32_t status - status indicates success or failure
*
* Return:
*  void
*
*******************************************************************************/
static void handle_error(uint32_t status)
{
    if (status != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }
}


void initEEPROM(void)
{
    cy_rslt_t result;

    mI2C_cfg.is_slave = false;
    mI2C_cfg.address = 0;
    mI2C_cfg.frequencyhal_hz = I2C_FREQ;

    /* Init I2C master */
    result = cyhal_i2c_init(&mI2C, CYBSP_I2C_SDA, CYBSP_I2C_SCL, NULL);
    /* I2C master init failed. Stop program execution */
    handle_error(result);

    /* Configure I2C Master */
    result = cyhal_i2c_configure(&mI2C, &mI2C_cfg);
    /* I2C master configuration failed. Stop program execution */
    handle_error(result);

    printf("I2C_init Done\r\n\n");
}

static cy_rslt_t readEEPROMsection(uint8_t* buffer,uint8_t size,uint8_t location)
{
	cy_rslt_t result;
	uint8_t slave_Addr = 0x50 | (uint8_t)(location / EEPROM_BLOCK_SIZE);
	uint8_t off_loc = location % EEPROM_BLOCK_SIZE ;

	result = cyhal_i2c_master_write(&mI2C, slave_Addr,&off_loc, 1, 0, false);

	if(result == CY_RSLT_SUCCESS)
	{
		return cyhal_i2c_master_read(&mI2C, slave_Addr,buffer, size, 0, true);
	}

	return result;
}

static cy_rslt_t writeEEPROMsection(uint8_t* buffer,uint8_t size,uint8_t location)
{
	uint8_t slave_Addr = 0x50 | (uint8_t)(location / EEPROM_BLOCK_SIZE);
	uint8_t off_location = location % EEPROM_BLOCK_SIZE ;
	uint8_t local_buf[17];
	cy_rslt_t result;
	uint8_t tempSize = 0;
	uint8_t index = 0;

	tempSize = 16 - (off_location % 16);

	if(size <= tempSize)	//if data with-in page limit
	{
		memset(local_buf,0x00,17);
		local_buf[0] = off_location;
		memcpy(&local_buf[1],buffer,size);
		cyhal_i2c_master_write(&mI2C, slave_Addr,local_buf, size + 1, 0, true);
		printf("abc 1 \r\n");
		cyhal_system_delay_ms(WRITE_DELAY);
	}
	else
	{
		memset(local_buf,0x00,17);
		local_buf[0] = off_location;
		tempSize = 16 - (off_location % 16);
		memcpy(&local_buf[1],&buffer[index],tempSize);
		cyhal_i2c_master_write(&mI2C, slave_Addr,local_buf, tempSize + 1, 0, true);
		printf("abc 2 %d\r\n",index);
		cyhal_system_delay_ms(WRITE_DELAY);
		index += tempSize;
		off_location += tempSize;

		while(index < size)
		{
			memset(local_buf,0x00,17);
			local_buf[0] = off_location;
			tempSize = (((size - index) <= 16) ? (size - index) : 16);
			memcpy(&local_buf[1],&buffer[index],tempSize);
			cyhal_i2c_master_write(&mI2C, slave_Addr,local_buf, tempSize + 1, 0, true);
			printf("abc 3 %d %d\r\n",index,size);
			cyhal_system_delay_ms(WRITE_DELAY);
			index += tempSize;
			off_location += tempSize;
		}
	}
	return result;
}

void printEEPROMContent()
{
	uint8_t i = 0;
	uint8_t j = 0;
	uint8_t buf[16] = {0};

	printf("\r\n");
	for(i=0;i<16;i++)
	{
		readEEPROMsection(buf,16,(i * 16));
		printf("read page %d:",i);
		for(j=0;j<16;j++)
		{
			printf("%X ",buf[j]);
		}
		printf("\r\n");
	}
}

void readControlInfo(uint8_t* buffer,uint8_t size)
{
	readEEPROMsection(buffer,size,CONTROL_INFO_LOCATION);
}

void writeControlInfo(uint8_t* buffer,uint8_t size)
{
	int i = 0;
	printf("writeControlInfo %d\r\n",size);
	for(i=0;i<size;i++)
	{
		printf("%X ",buffer[i]);
	}
	printf("\r\n");
	writeEEPROMsection(buffer,size,CONTROL_INFO_LOCATION);
}

void readSSID(uint8_t* buffer, uint8_t size)
{
	readEEPROMsection(buffer,size,WIFI_SSID_LOCATION);
}

void writeSSID(uint8_t* buffer, uint8_t size)
{
	int i = 0;
	printf("writeSSID %d\r\n",size);
	for(i=0;i<size;i++)
	{
		printf("%X ",buffer[i]);
	}
	printf("\r\n");
	writeEEPROMsection(buffer,size,WIFI_SSID_LOCATION);
}

void readPWD(uint8_t* buffer, uint8_t size)
{
	readEEPROMsection(buffer,size,WIFI_PWD_LOCATION);
}

void writePWD(uint8_t* buffer, uint8_t size)
{
	int i = 0;
	printf("writePWD %d\r\n",size);
	for(i=0;i<size;i++)
	{
		printf("%X ",buffer[i]);
	}
	printf("\r\n");
	writeEEPROMsection(buffer,size,WIFI_PWD_LOCATION);
}

void readDeviceID(uint8_t* buffer, uint8_t size)
{
	readEEPROMsection(buffer,size,DEVICE_ID_LOCATION);
}

void writeDeviceID(uint8_t* buffer, uint8_t size)
{
	int i = 0;
	printf("writeDeviceID %d\r\n",size);
	for(i=0;i<size;i++)
	{
		printf("%X ",buffer[i]);
	}
	printf("\r\n");
	writeEEPROMsection(buffer,size,DEVICE_ID_LOCATION);
}

void readFlowData(uint8_t* buffer, uint8_t size)
{
	readEEPROMsection(buffer,size,FLORATE_LOCATION);
}

void writeFlowData(uint8_t* buffer, uint8_t size)
{
	int i = 0;
	printf("writeFlowData %d\r\n",size);
	for(i=0;i<size;i++)
	{
		printf("%X ",buffer[i]);
	}
	printf("\r\n");
	writeEEPROMsection(buffer,size,FLORATE_LOCATION);
}
