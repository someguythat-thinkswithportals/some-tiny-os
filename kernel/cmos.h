#ifndef CMOS_H
#define CMOS_H

#include <stdint.h>

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} rtc_datetime_t;

void cmos_read_datetime(rtc_datetime_t* dt);

#endif
