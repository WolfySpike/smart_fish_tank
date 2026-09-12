#include "ds1302.h"

/* DS1302 register addresses (write command base, LSB first) */
 #define DS1302_REG_SECOND      0x80
 #define DS1302_REG_MINUTE      0x82
 #define DS1302_REG_HOUR        0x84
 #define DS1302_REG_DATE        0x86
 #define DS1302_REG_MONTH       0x88
 #define DS1302_REG_WEEK        0x8A
 #define DS1302_REG_YEAR        0x8C
 #define DS1302_REG_WP          0x8E

#define DS1302_CMD_READ_MASK   0x01
#define DS1302_DELAY_IO_US     2U
#define DS1302_DELAY_CE_US     4U

 static void DS1302_DelayUs(uint32_t us)
 {
     uint32_t start = DWT->CYCCNT;
     const uint32_t ticks = us * (HAL_RCC_GetHCLKFreq() / 1000000U);
     while ((DWT->CYCCNT - start) < ticks)
     {
     }
 }

 static void DS1302_EnableDwtDelay(void)
 {
     CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
     DWT->CYCCNT = 0;
     DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
 }

static void DS1302_IO_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
     GPIO_InitStruct.Pin = DS1302_IO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DS1302_IO_GPIO_Port, &GPIO_InitStruct);
}

static void DS1302_IO_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS1302_IO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DS1302_IO_GPIO_Port, &GPIO_InitStruct);
}

static void DS1302_WriteBit(uint8_t bit)
{
    HAL_GPIO_WritePin(DS1302_IO_GPIO_Port, DS1302_IO_Pin, bit ? GPIO_PIN_SET : GPIO_PIN_RESET);
    DS1302_DelayUs(DS1302_DELAY_IO_US);
    HAL_GPIO_WritePin(DS1302_CLK_GPIO_Port, DS1302_CLK_Pin, GPIO_PIN_SET);
    DS1302_DelayUs(DS1302_DELAY_IO_US);
    HAL_GPIO_WritePin(DS1302_CLK_GPIO_Port, DS1302_CLK_Pin, GPIO_PIN_RESET);
    DS1302_DelayUs(DS1302_DELAY_IO_US);
}

static uint8_t DS1302_ReadBit(void)
{
    uint8_t bit;
    bit = (uint8_t)(HAL_GPIO_ReadPin(DS1302_IO_GPIO_Port, DS1302_IO_Pin) == GPIO_PIN_SET);
    HAL_GPIO_WritePin(DS1302_CLK_GPIO_Port, DS1302_CLK_Pin, GPIO_PIN_SET);
    DS1302_DelayUs(DS1302_DELAY_IO_US);
    HAL_GPIO_WritePin(DS1302_CLK_GPIO_Port, DS1302_CLK_Pin, GPIO_PIN_RESET);
    DS1302_DelayUs(DS1302_DELAY_IO_US);
    return bit;
}

static uint8_t DS1302_ReadByte(void)
{
    uint8_t i;
    uint8_t data = 0U;

    for (i = 0; i < 8; i++)
    {
        if (DS1302_ReadBit() != 0U)
        {
            data |= (uint8_t)(1U << i); /* LSB first */
        }
    }

    return data;
}

 static void DS1302_WriteByte(uint8_t data)
 {
     uint8_t i;
     for (i = 0; i < 8; i++)
     {
         DS1302_WriteBit(data & 0x01U); /* LSB first */
         data >>= 1;
     }
 }

static void DS1302_Begin(void)
{
    HAL_GPIO_WritePin(DS1302_CLK_GPIO_Port, DS1302_CLK_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DS1302_RST_GPIO_Port, DS1302_RST_Pin, GPIO_PIN_SET);
    DS1302_DelayUs(DS1302_DELAY_CE_US);
}

static void DS1302_End(void)
{
    HAL_GPIO_WritePin(DS1302_RST_GPIO_Port, DS1302_RST_Pin, GPIO_PIN_RESET);
    DS1302_DelayUs(DS1302_DELAY_CE_US);
}

 static uint8_t DS1302_DecToBcd(uint8_t dec)
 {
     return (uint8_t)(((dec / 10U) << 4) | (dec % 10U));
 }

 static uint8_t DS1302_BcdToDec(uint8_t bcd)
 {
     return (uint8_t)(((bcd >> 4) * 10U) + (bcd & 0x0FU));
 }

 static void DS1302_WriteReg(uint8_t reg, uint8_t data)
 {
     DS1302_IO_Output();
     DS1302_Begin();
     DS1302_WriteByte(reg & (uint8_t)~DS1302_CMD_READ_MASK);
     DS1302_WriteByte(data);
     DS1302_End();
 }

static uint8_t DS1302_ReadReg(uint8_t reg)
{
    uint8_t data;
    DS1302_IO_Output();
    DS1302_Begin();
    DS1302_WriteByte(reg | DS1302_CMD_READ_MASK);
    DS1302_IO_Input();
    DS1302_DelayUs(DS1302_DELAY_IO_US);
    data = DS1302_ReadByte();
    DS1302_End();
    DS1302_IO_Output();
    return data;
}

void DS1302_Init(void)
{
     DS1302_EnableDwtDelay();
     DS1302_IO_Output();
    HAL_GPIO_WritePin(DS1302_IO_GPIO_Port, DS1302_IO_Pin, GPIO_PIN_SET);
     HAL_GPIO_WritePin(DS1302_RST_GPIO_Port, DS1302_RST_Pin, GPIO_PIN_RESET);
     HAL_GPIO_WritePin(DS1302_CLK_GPIO_Port, DS1302_CLK_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
}

 HAL_StatusTypeDef DS1302_SetDateTime(const DS1302_TimeTypeDef *time)
 {
     if (time == 0)
     {
         return HAL_ERROR;
     }

     if (time->month < 1U || time->month > 12U ||
         time->date < 1U || time->date > 31U ||
         time->week < 1U || time->week > 7U ||
         time->hour > 23U || time->minute > 59U || time->second > 59U)
     {
         return HAL_ERROR;
     }

     DS1302_WriteReg(DS1302_REG_WP, 0x00U); /* disable write protect */

     DS1302_WriteReg(DS1302_REG_SECOND, (uint8_t)(DS1302_DecToBcd(time->second) & 0x7FU));
     DS1302_WriteReg(DS1302_REG_MINUTE, DS1302_DecToBcd(time->minute));
     DS1302_WriteReg(DS1302_REG_HOUR, DS1302_DecToBcd(time->hour)); /* 24-hour mode */
     DS1302_WriteReg(DS1302_REG_DATE, DS1302_DecToBcd(time->date));
     DS1302_WriteReg(DS1302_REG_MONTH, DS1302_DecToBcd(time->month));
     DS1302_WriteReg(DS1302_REG_WEEK, DS1302_DecToBcd(time->week));
     DS1302_WriteReg(DS1302_REG_YEAR, DS1302_DecToBcd(time->year));

     DS1302_WriteReg(DS1302_REG_WP, 0x80U); /* enable write protect */
     return HAL_OK;
 }

HAL_StatusTypeDef DS1302_GetDateTime(DS1302_TimeTypeDef *time)
{
     uint8_t sec;
     uint8_t min;
     uint8_t hour;
     uint8_t date;
     uint8_t mon;
     uint8_t week;
     uint8_t year;

     if (time == 0)
     {
         return HAL_ERROR;
     }

     sec = DS1302_ReadReg(DS1302_REG_SECOND);
     min = DS1302_ReadReg(DS1302_REG_MINUTE);
     hour = DS1302_ReadReg(DS1302_REG_HOUR);
     date = DS1302_ReadReg(DS1302_REG_DATE);
     mon = DS1302_ReadReg(DS1302_REG_MONTH);
     week = DS1302_ReadReg(DS1302_REG_WEEK);
     year = DS1302_ReadReg(DS1302_REG_YEAR);

     time->second = DS1302_BcdToDec((uint8_t)(sec & 0x7FU));
     time->minute = DS1302_BcdToDec((uint8_t)(min & 0x7FU));
     if (hour & 0x80U)
     {
         /* 12-hour mode fallback */
         uint8_t h = DS1302_BcdToDec((uint8_t)(hour & 0x1FU));
         if (hour & 0x20U)
         {
             if (h < 12U)
             {
                 h = (uint8_t)(h + 12U);
             }
         }
         else if (h == 12U)
         {
             h = 0U;
         }
         time->hour = h;
     }
     else
     {
         time->hour = DS1302_BcdToDec((uint8_t)(hour & 0x3FU));
     }
     time->date = DS1302_BcdToDec((uint8_t)(date & 0x3FU));
     time->month = DS1302_BcdToDec((uint8_t)(mon & 0x1FU));
     time->week = DS1302_BcdToDec((uint8_t)(week & 0x07U));
     time->year = DS1302_BcdToDec(year);

    return HAL_OK;
}

HAL_StatusTypeDef DS1302_GetRawDateTime(DS1302_RawTimeTypeDef *time)
{
    if (time == 0)
    {
        return HAL_ERROR;
    }

    time->second = DS1302_ReadReg(DS1302_REG_SECOND);
    time->minute = DS1302_ReadReg(DS1302_REG_MINUTE);
    time->hour = DS1302_ReadReg(DS1302_REG_HOUR);
    time->date = DS1302_ReadReg(DS1302_REG_DATE);
    time->month = DS1302_ReadReg(DS1302_REG_MONTH);
    time->week = DS1302_ReadReg(DS1302_REG_WEEK);
    time->year = DS1302_ReadReg(DS1302_REG_YEAR);

    return HAL_OK;
}

 uint8_t DS1302_IsClockHalted(void)
 {
     return (uint8_t)((DS1302_ReadReg(DS1302_REG_SECOND) & 0x80U) ? 1U : 0U);
 }

 const char *DS1302_WeekdayString(uint8_t week)
 {
     static const char *weeks[] = {"?", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
     if (week >= 1U && week <= 7U)
     {
         return weeks[week];
     }
     return weeks[0];
 }
