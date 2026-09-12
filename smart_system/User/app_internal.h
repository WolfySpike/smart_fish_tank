#ifndef APP_INTERNAL_H
#define APP_INTERNAL_H

#include "main.h"
#include "tim.h"
#include "gpio.h"
#include "OLED.h"
#include "ds1302.h"

#define APP_RTC_SYNC_FROM_BUILD_AT_BOOT 0U
#define APP_KEY_DEBOUNCE_MS             20U
#define APP_KEY_LONGPRESS_START_MS      500U
#define APP_KEY_REPEAT_MS               120U
#define APP_SETTING_MINUTE_STEP         1U
#define APP_SERVO_PULSE_MIN_US          500U
#define APP_SERVO_PULSE_MID_US          1500U
#define APP_SERVO_PULSE_MAX_US          2500U
#define APP_SERVO_SWING_A_US            2300U
#define APP_SERVO_SWING_B_US            700U
#define APP_SERVO_STEP_HOLD_MS          350U
#define APP_SERVO_RETURN_HOLD_MS        250U
#define APP_FISH_LIGHT_DUTY             900U

#define APP_FEED_MAX_PER_DAY            3U
#define APP_FEED_CYCLES_MIN             1U
#define APP_FEED_CYCLES_MAX             5U

#define APP_CFG_FLASH_MAGIC             0x46454431UL /* "FED1" */
#define APP_CFG_FLASH_VERSION           1U

#ifndef FLASH_PAGE_SIZE
#define FLASH_PAGE_SIZE                 1024U
#endif

#ifndef FLASHSIZE_BASE
#define FLASHSIZE_BASE                  0x1FFFF7E0UL
#endif

#define APP_FEED_STATE_IDLE             0U
#define APP_FEED_STATE_TO_B             1U
#define APP_FEED_STATE_TO_A             2U
#define APP_FEED_STATE_FINISH           3U

#define APP_ITEM_FISH_ON                0U
#define APP_ITEM_FISH_OFF               1U
#define APP_ITEM_FILL_ON                2U
#define APP_ITEM_FILL_OFF               3U
#define APP_ITEM_FEED_TIMES             4U
#define APP_ITEM_FEED_CYCLES            5U
#define APP_ITEM_FEED_1                 6U
#define APP_ITEM_FEED_2                 7U
#define APP_ITEM_FEED_3                 8U
#define APP_ITEM_COUNT                  9U

#define APP_EDIT_FIELD_HOUR             0U
#define APP_EDIT_FIELD_MINUTE           1U

#define APP_WATER_RGB_DUTY              850U
#define APP_WATER_ADC_MAX               4095U
#define APP_WATER_ADC_WET_HIGH          1U
/*
 * Water level thresholds (ADC raw, 12-bit):
 * <1200: DRY(RED), 1200~1999: LOW(GREEN), 2000~2099: MID(YELLOW), >=2100: HIGH(RED)
 */
#define APP_WATER_ADC_DRY_TH            1200U
#define APP_WATER_ADC_MID_TH            1800U
#define APP_WATER_ADC_HIGH_TH           2000U

/* Percentage display mapping range */
#define APP_WATER_ADC_PCT_MIN           1200U
#define APP_WATER_ADC_PCT_MAX           2000U

#define APP_WATER_LEVEL_DRY             0U
#define APP_WATER_LEVEL_LOW             1U
#define APP_WATER_LEVEL_MID             2U
#define APP_WATER_LEVEL_HIGH            3U

typedef struct
{
  uint8_t hour;
  uint8_t minute;
} App_TimeOfDay_t;

typedef struct
{
  App_TimeOfDay_t fishOn;
  App_TimeOfDay_t fishOff;
  App_TimeOfDay_t fillOn;
  App_TimeOfDay_t fillOff;
  uint8_t feedTimesPerDay;
  App_TimeOfDay_t feedTimes[APP_FEED_MAX_PER_DAY];
  uint8_t feedCycles;
} App_Config_t;

typedef struct
{
  uint32_t magic;
  uint16_t version;
  uint16_t reserved;
  App_Config_t cfg;
  uint32_t checksum;
} App_ConfigFlash_t;

extern App_Config_t g_cfg;
extern uint8_t g_menuActive;
extern uint8_t g_menuItem;
extern uint8_t g_timeEditField;

extern uint8_t g_feedState;
extern uint32_t g_feedStepTick;
extern uint8_t g_feedCyclesRemaining;
extern uint8_t g_feedDoneMask;
extern uint32_t g_lastDateKey;

extern uint8_t g_fishLightOn;
extern uint8_t g_fillLightOn;
extern uint8_t g_waterLevel;
extern uint16_t g_waterAdcRaw;
extern uint8_t g_waterPercent;

uint8_t App_TimeIsValid(const DS1302_TimeTypeDef *time);
uint8_t App_ParseBuildTime(DS1302_TimeTypeDef *time);
uint16_t App_MinutesOfDay(uint8_t hour, uint8_t minute);
uint8_t App_IsInTimeWindow(uint16_t now, uint16_t start, uint16_t stop);
uint32_t App_DateKey(const DS1302_TimeTypeDef *time);
uint8_t App_TimeMatches(const DS1302_TimeTypeDef *time, const App_TimeOfDay_t *point);
void App_AddMinutes(App_TimeOfDay_t *time, int16_t delta);
uint8_t App_TimeOfDayIsValid(const App_TimeOfDay_t *time);

void App_ConfigSanitize(App_Config_t *cfg);
uint8_t App_LoadConfigFromFlash(void);
void App_SaveConfigToFlash(void);

void App_SetFishLight(uint8_t on);
void App_SetFillLight(uint8_t on);
void App_WaterLevelUpdate(void);
const char *App_WaterLevelText(void);
void App_SetServoPulse(uint16_t pulseUs);
void App_FeedStart(uint8_t cycles);
void App_FeedUpdate(void);

void App_HandleKeys(void);
void App_ShowDateTime(const DS1302_TimeTypeDef *time);

void App_RunSchedule(const DS1302_TimeTypeDef *time);
void App_InitRtcIfNeeded(void);

#endif /* APP_INTERNAL_H */
