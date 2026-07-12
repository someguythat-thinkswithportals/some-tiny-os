#include "cmos.h"
#include "io.h"

#define CMOS_INDEX 0x70
#define CMOS_DATA  0x71

#define CMOS_SEC    0x00
#define CMOS_MIN    0x02
#define CMOS_HOUR   0x04
#define CMOS_DAY    0x07
#define CMOS_MON    0x08
#define CMOS_YEAR   0x09
#define CMOS_STAT_A 0x0A
#define CMOS_STAT_B 0x0B

#define CMOS_UIP    0x80

static uint8_t cmos_read_reg(uint8_t reg) {
    outb(CMOS_INDEX, reg);
    return inb(CMOS_DATA);
}

static uint8_t bcd_to_bin(uint8_t val) {
    return (val & 0x0F) + ((val >> 4) * 10);
}

void cmos_read_datetime(rtc_datetime_t* dt) {
    while (cmos_read_reg(CMOS_STAT_A) & CMOS_UIP);

    uint8_t regb = cmos_read_reg(CMOS_STAT_B);
    int bcd = !(regb & 0x04);

    uint8_t sec   = cmos_read_reg(CMOS_SEC);
    uint8_t min   = cmos_read_reg(CMOS_MIN);
    uint8_t hour  = cmos_read_reg(CMOS_HOUR);
    uint8_t day   = cmos_read_reg(CMOS_DAY);
    uint8_t mon   = cmos_read_reg(CMOS_MON);
    uint8_t year  = cmos_read_reg(CMOS_YEAR);
    uint8_t century = cmos_read_reg(0x32);

    if (bcd) {
        sec = bcd_to_bin(sec);
        min = bcd_to_bin(min);
        hour = bcd_to_bin(hour);
        day = bcd_to_bin(day);
        mon = bcd_to_bin(mon);
        year = bcd_to_bin(year);
        century = bcd_to_bin(century);
    }

    dt->second = sec;
    dt->minute = min;
    dt->hour = hour;
    dt->day = day;
    dt->month = mon;
    dt->year = century * 100 + year;
}
