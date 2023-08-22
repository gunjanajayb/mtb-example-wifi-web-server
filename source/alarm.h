#ifndef ALARM_H_
#define ALARM_H_

void alarm_task(void *arg);
void rtc_task(void *arg);
int json_parser_snippet(void);
void init_RTC(struct tm *date_time);
#endif
