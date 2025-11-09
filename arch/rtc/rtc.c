#include <stdint.h>
#include <rtc.h>
#include <io.h>

/* Read a value from RTC register */
uint8_t rtc_read_register(uint8_t reg) {
    outb(RTC_INDEX_PORT, reg);
    return inb(RTC_DATA_PORT);
}

/* Wait for RTC update to complete */
void rtc_wait_for_update(void) {
    /* Wait for update in progress bit to clear */
    while (rtc_read_register(RTC_REG_STATUS_A) & RTC_STATUS_A_UIP) {
        /* Wait */
    }
}

/* Check if RTC is in binary mode */
int rtc_is_binary_mode(void) {
    return (rtc_read_register(RTC_REG_STATUS_B) & RTC_STATUS_B_BIN) != 0;
}

/* Check if RTC is in 24-hour format */
int rtc_is_24hour_format(void) {
    return (rtc_read_register(RTC_REG_STATUS_B) & RTC_STATUS_B_24H) != 0;
}

/* Convert BCD to binary */
static uint8_t bcd_to_bin(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

/* Read RTC time (returns seconds since midnight) */
uint32_t rtc_read_time_seconds(void) {
    uint8_t second, minute, hour;
    int binary = rtc_is_binary_mode();
    int hour24 = rtc_is_24hour_format();
    
    /* Wait for update to complete */
    rtc_wait_for_update();
    
    /* Read time registers */
    second = rtc_read_register(RTC_REG_SECOND);
    minute = rtc_read_register(RTC_REG_MINUTE);
    hour = rtc_read_register(RTC_REG_HOUR);
    
    /* Handle 12-hour format - extract PM bit before conversion */
    uint8_t pm_bit = 0;
    if (!hour24) {
        pm_bit = hour & 0x80; /* PM bit */
        hour = hour & 0x7F;    /* Clear PM bit */
    }
    
    /* Convert from BCD if necessary */
    if (!binary) {
        second = bcd_to_bin(second);
        minute = bcd_to_bin(minute);
        hour = bcd_to_bin(hour);
    }
    
    /* Handle 12-hour format conversion to 24-hour */
    if (!hour24) {
        if (pm_bit) {
            /* PM */
            if (hour != 12) {
                hour += 12;
            }
        } else {
            /* AM */
            if (hour == 12) {
                hour = 0;
            }
        }
    }
    
    return (uint32_t)hour * 3600 + (uint32_t)minute * 60 + (uint32_t)second;
}

/* Read RTC date and time */
void rtc_read_datetime(uint8_t *second, uint8_t *minute, uint8_t *hour,
                       uint8_t *day, uint8_t *month, uint8_t *year) {
    int binary = rtc_is_binary_mode();
    int hour24 = rtc_is_24hour_format();
    
    /* Wait for update to complete */
    rtc_wait_for_update();
    
    /* Read time registers */
    uint8_t s = rtc_read_register(RTC_REG_SECOND);
    uint8_t m = rtc_read_register(RTC_REG_MINUTE);
    uint8_t h = rtc_read_register(RTC_REG_HOUR);
    
    /* Read date registers */
    uint8_t d = rtc_read_register(RTC_REG_DAY);
    uint8_t mo = rtc_read_register(RTC_REG_MONTH);
    uint8_t y = rtc_read_register(RTC_REG_YEAR);
    
    /* Handle 12-hour format - extract PM bit before conversion */
    uint8_t pm_bit = 0;
    if (!hour24) {
        pm_bit = h & 0x80; /* PM bit */
        h = h & 0x7F;       /* Clear PM bit */
    }
    
    /* Convert from BCD if necessary */
    if (!binary) {
        s = bcd_to_bin(s);
        m = bcd_to_bin(m);
        h = bcd_to_bin(h);
        d = bcd_to_bin(d);
        mo = bcd_to_bin(mo);
        y = bcd_to_bin(y);
    }
    
    /* Handle 12-hour format conversion to 24-hour */
    if (!hour24) {
        if (pm_bit) {
            /* PM */
            if (h != 12) {
                h += 12;
            }
        } else {
            /* AM */
            if (h == 12) {
                h = 0;
            }
        }
    }
    
    if (second) *second = s;
    if (minute) *minute = m;
    if (hour) *hour = h;
    if (day) *day = d;
    if (month) *month = mo;
    if (year) *year = y;
}

