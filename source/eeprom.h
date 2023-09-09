/*
 * eeprom.h
 *
 *  Created on: 26-May-2023
 *      Author: gb
 */

#ifndef SOURCE_EEPROM_H_
#define SOURCE_EEPROM_H_

void initEEPROM(void);
void readControlInfo(uint8_t* buffer,uint8_t size);
void writeControlInfo(uint8_t* buffer,uint8_t size);
void readSSID(uint8_t* buffer, uint8_t size,uint8_t inx);
void writeSSID(uint8_t* buffer, uint8_t size,uint8_t inx);
void readPWD(uint8_t* buffer, uint8_t size,uint8_t inx);
void writePWD(uint8_t* buffer, uint8_t size,uint8_t inx);
void readDeviceID(uint8_t* buffer, uint8_t size);
void writeDeviceID(uint8_t* buffer, uint8_t size);
void readFlowData(uint8_t* buffer, uint8_t size);
void writeFlowData(uint8_t* buffer, uint8_t size);
void readCredentialIndex(uint8_t* buffer, uint8_t size);
void writeCredentialIndex(uint8_t* buffer, uint8_t size);
void printEEPROMContent();
#endif /* SOURCE_EEPROM_H_ */
