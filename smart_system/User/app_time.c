#include "app_internal.h"

uint8_t App_TimeIsValid(const DS1302_TimeTypeDef *time)
{
  if (time == NULL)
  {
    return 0U;
  }

  if (time->month < 1U || time->month > 12U)
  {
    return 0U;
  }

  if (time->date < 1U || time->date > 31U)
  {
    return 0U;
  }

  if (time->hour > 23U || time->minute > 59U || time->second > 59U)
  {
    return 0U;
  }

  return 1U;
}

static uint8_t App_WeekdayFromYmd(uint16_t fullYear, uint8_t month, uint8_t day)
{
  uint16_t y = fullYear;
  uint8_t m = month;
  static const uint8_t monthOffset[] = {0U, 3U, 2U, 5U, 0U, 3U, 5U, 1U, 4U, 6U, 2U, 4U};
  uint8_t w;

  if (m < 1U || m > 12U || day < 1U || day > 31U)
  {
    return 1U;
  }

  if (m < 3U)
  {
    y--;
  }

  /* Sakamoto algorithm: w=0 is Sunday, 1..6 is Monday..Saturday */
  w = (uint8_t)((y + y / 4U - y / 100U + y / 400U + monthOffset[m - 1U] + day) % 7U);
  return (w == 0U) ? 7U : w; /* DS1302: Monday=1 ... Sunday=7 */
}

uint8_t App_ParseBuildTime(DS1302_TimeTypeDef *time)
{
  static const char *monthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  const char dateStr[] = __DATE__; /* "Mmm dd yyyy" */
  const char timeStr[] = __TIME__; /* "hh:mm:ss" */
  uint8_t month = 0U;
  uint8_t i;
  uint16_t fullYear;
  uint8_t dayTens;

  if (time == NULL)
  {
    return 0U;
  }

  for (i = 0U; i < 12U; i++)
  {
    if ((dateStr[0] == monthNames[i][0]) &&
        (dateStr[1] == monthNames[i][1]) &&
        (dateStr[2] == monthNames[i][2]))
    {
      month = (uint8_t)(i + 1U);
      break;
    }
  }

  if (month == 0U)
  {
    return 0U;
  }

  dayTens = (dateStr[4] == ' ') ? 0U : (uint8_t)(dateStr[4] - '0');
  fullYear = (uint16_t)((dateStr[7] - '0') * 1000U +
                        (dateStr[8] - '0') * 100U +
                        (dateStr[9] - '0') * 10U +
                        (dateStr[10] - '0'));

  time->year = (uint8_t)(fullYear % 100U);
  time->month = month;
  time->date = (uint8_t)(dayTens * 10U + (uint8_t)(dateStr[5] - '0'));
  time->hour = (uint8_t)((timeStr[0] - '0') * 10U + (timeStr[1] - '0'));
  time->minute = (uint8_t)((timeStr[3] - '0') * 10U + (timeStr[4] - '0'));
  time->second = (uint8_t)((timeStr[6] - '0') * 10U + (timeStr[7] - '0'));
  time->week = App_WeekdayFromYmd(fullYear, time->month, time->date);

  return (App_TimeIsValid(time) != 0U) ? 1U : 0U;
}

uint16_t App_MinutesOfDay(uint8_t hour, uint8_t minute)
{
  return (uint16_t)((uint16_t)hour * 60U + minute);
}

uint8_t App_IsInTimeWindow(uint16_t now, uint16_t start, uint16_t stop)
{
  if (start == stop)
  {
    return 0U;
  }

  if (start < stop)
  {
    return ((now >= start) && (now < stop)) ? 1U : 0U;
  }

  return ((now >= start) || (now < stop)) ? 1U : 0U;
}

uint32_t App_DateKey(const DS1302_TimeTypeDef *time)
{
  return ((uint32_t)time->year << 16) | ((uint32_t)time->month << 8) | (uint32_t)time->date;
}

uint8_t App_TimeMatches(const DS1302_TimeTypeDef *time, const App_TimeOfDay_t *point)
{
  if (point == NULL)
  {
    return 0U;
  }
  return ((time->hour == point->hour) && (time->minute == point->minute)) ? 1U : 0U;
}

void App_AddMinutes(App_TimeOfDay_t *time, int16_t delta)
{
  int16_t total;

  if (time == NULL)
  {
    return;
  }

  total = (int16_t)((int16_t)time->hour * 60 + time->minute);
  total = (int16_t)(total + delta);

  while (total < 0)
  {
    total = (int16_t)(total + 1440);
  }
  while (total >= 1440)
  {
    total = (int16_t)(total - 1440);
  }

  time->hour = (uint8_t)(total / 60);
  time->minute = (uint8_t)(total % 60);
}

uint8_t App_TimeOfDayIsValid(const App_TimeOfDay_t *time)
{
  if (time == NULL)
  {
    return 0U;
  }
  if (time->hour > 23U || time->minute > 59U)
  {
    return 0U;
  }
  return 1U;
}
