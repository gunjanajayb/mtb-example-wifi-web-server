#ifndef ALARM_H_
#define ALARM_H_


void alarm_task(void *arg);
void rtc_task(void *arg);
int json_parser_snippet(void);
void read_ID(char* id, uint8_t len);

#if 1	//write to EEPROM
void write_ID(char* id, uint8_t len);
#endif

#endif
