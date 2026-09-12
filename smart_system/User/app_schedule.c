#include "app_internal.h"

void App_RunSchedule(const DS1302_TimeTypeDef *time)
{
  uint8_t i;
  uint16_t nowMinute;
  uint16_t fishOnMinute;
  uint16_t fishOffMinute;
  uint16_t fillOnMinute;
  uint16_t fillOffMinute;
  uint8_t fishOn;
  uint8_t fillOn;
  uint32_t dateKey;

  if (time == NULL)
  {
    return;
  }

  dateKey = App_DateKey(time);
  if (dateKey != g_lastDateKey)
  {
    g_lastDateKey = dateKey;
    g_feedDoneMask = 0U;
  }

  nowMinute = App_MinutesOfDay(time->hour, time->minute);
  fishOnMinute = App_MinutesOfDay(g_cfg.fishOn.hour, g_cfg.fishOn.minute);
  fishOffMinute = App_MinutesOfDay(g_cfg.fishOff.hour, g_cfg.fishOff.minute);
  fillOnMinute = App_MinutesOfDay(g_cfg.fillOn.hour, g_cfg.fillOn.minute);
  fillOffMinute = App_MinutesOfDay(g_cfg.fillOff.hour, g_cfg.fillOff.minute);

  fishOn = App_IsInTimeWindow(nowMinute, fishOnMinute, fishOffMinute);
  fillOn = App_IsInTimeWindow(nowMinute, fillOnMinute, fillOffMinute);

  App_SetFishLight(fishOn);
  App_SetFillLight(fillOn);

  for (i = 0U; i < g_cfg.feedTimesPerDay; i++)
  {
    uint8_t mask = (uint8_t)(1U << i);
    if ((g_feedDoneMask & mask) == 0U)
    {
      if (App_TimeMatches(time, &g_cfg.feedTimes[i]) != 0U)
      {
        App_FeedStart(g_cfg.feedCycles);
        g_feedDoneMask = (uint8_t)(g_feedDoneMask | mask);
      }
    }
  }
}

void App_InitRtcIfNeeded(void)
{
  DS1302_TimeTypeDef now = {0};
  DS1302_TimeTypeDef defaultTime = {0};

  if (App_ParseBuildTime(&defaultTime) == 0U)
  {
    defaultTime.year = 26U;
    defaultTime.month = 4U;
    defaultTime.date = 17U;
    defaultTime.week = 5U;
    defaultTime.hour = 12U;
    defaultTime.minute = 0U;
    defaultTime.second = 0U;
  }

#if APP_RTC_SYNC_FROM_BUILD_AT_BOOT
  (void)DS1302_SetDateTime(&defaultTime);
#else
  if (DS1302_IsClockHalted())
  {
    (void)DS1302_SetDateTime(&defaultTime);
    return;
  }

  if (DS1302_GetDateTime(&now) != HAL_OK || App_TimeIsValid(&now) == 0U)
  {
    (void)DS1302_SetDateTime(&defaultTime);
  }
#endif
}
