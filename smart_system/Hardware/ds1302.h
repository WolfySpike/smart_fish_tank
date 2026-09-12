 #ifndef __DS1302_H
 #define __DS1302_H

 #include "main.h"
 #include <stdint.h>

 /*
  * Default IO pin: PB14.
  * If you later rename this pin in CubeMX/main.h, you can redefine these macros.
  */
 #ifndef DS1302_IO_Pin
 #define DS1302_IO_Pin GPIO_PIN_14
 #endif

 #ifndef DS1302_IO_GPIO_Port
 #define DS1302_IO_GPIO_Port GPIOB
 #endif

typedef struct
{
    uint8_t year;    /* 0~99, e.g. 26 -> 2026 */
    uint8_t month;   /* 1~12 */
     uint8_t date;    /* 1~31 */
     uint8_t week;    /* 1~7 */
     uint8_t hour;    /* 0~23 */
     uint8_t minute;  /* 0~59 */
    uint8_t second;  /* 0~59 */
} DS1302_TimeTypeDef;

typedef struct
{
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t date;
    uint8_t month;
    uint8_t week;
    uint8_t year;
} DS1302_RawTimeTypeDef;

void DS1302_Init(void);

HAL_StatusTypeDef DS1302_SetDateTime(const DS1302_TimeTypeDef *time);
HAL_StatusTypeDef DS1302_GetDateTime(DS1302_TimeTypeDef *time);
HAL_StatusTypeDef DS1302_GetRawDateTime(DS1302_RawTimeTypeDef *time);

 uint8_t DS1302_IsClockHalted(void);

 const char *DS1302_WeekdayString(uint8_t week);

 #endif /* __DS1302_H */
