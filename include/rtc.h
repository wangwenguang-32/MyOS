#ifndef RTC_H
#define RTC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* RTC register addresses */
#define RTC_REG_SECOND     0x00
#define RTC_REG_MINUTE     0x02
#define RTC_REG_HOUR       0x04
#define RTC_REG_DAY        0x07
#define RTC_REG_MONTH      0x08
#define RTC_REG_YEAR       0x09
#define RTC_REG_STATUS_A   0x0A
#define RTC_REG_STATUS_B   0x0B
#define RTC_REG_STATUS_C   0x0C

/* RTC I/O ports */
#define RTC_INDEX_PORT     0x70
#define RTC_DATA_PORT      0x71

/* RTC Status B flags */
#define RTC_STATUS_B_24H   0x02  /* 24-hour format */
#define RTC_STATUS_B_BIN   0x04  /* Binary mode */
#define RTC_STATUS_B_DM    0x04  /* Data mode (binary) */

/* RTC Status A flags */
#define RTC_STATUS_A_UIP   0x80  /* Update in progress */

/* Read a value from RTC register */
uint8_t rtc_read_register(uint8_t reg);

/* Wait for RTC update to complete */
void rtc_wait_for_update(void);

/* Read RTC time (returns seconds since midnight) */
uint32_t rtc_read_time_seconds(void);

/* Read RTC date and time */
void rtc_read_datetime(uint8_t *second, uint8_t *minute, uint8_t *hour,
                       uint8_t *day, uint8_t *month, uint8_t *year);

/* Check if RTC is in binary mode */
int rtc_is_binary_mode(void);

/* Check if RTC is in 24-hour format */
int rtc_is_24hour_format(void);

#ifdef __cplusplus
}
#endif

#endif /* RTC_H */

