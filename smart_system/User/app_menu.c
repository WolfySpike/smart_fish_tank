#include "app_internal.h"

#include <stdio.h>

#define APP_MENU_PAGE_FISH 0U
#define APP_MENU_PAGE_FILL 1U
#define APP_MENU_PAGE_FEED 2U

#ifdef KEY3_Pin
#define APP_HAS_KEY3 1U
#else
#define APP_HAS_KEY3 0U
#endif

#ifdef KEY4_Pin
#define APP_HAS_KEY4 1U
#else
#define APP_HAS_KEY4 0U
#endif

#define APP_KEY_COUNT (3U + APP_HAS_KEY3 + APP_HAS_KEY4)

static uint8_t App_KeyPressEvent(uint8_t index)
{
  static uint8_t lastRaw[APP_KEY_COUNT] = {0U};
  static uint8_t stable[APP_KEY_COUNT] = {0U};
  static uint32_t changeTick[APP_KEY_COUNT] = {0U};
  static GPIO_TypeDef *const keyPorts[APP_KEY_COUNT] = {
    KEY0_GPIO_Port,
    KEY1_GPIO_Port,
    KEY2_GPIO_Port,
#if APP_HAS_KEY3
    KEY3_GPIO_Port,
#endif
#if APP_HAS_KEY4
    KEY4_GPIO_Port,
#endif
  };
  static const uint16_t keyPins[APP_KEY_COUNT] = {
    KEY0_Pin,
    KEY1_Pin,
    KEY2_Pin,
#if APP_HAS_KEY3
    KEY3_Pin,
#endif
#if APP_HAS_KEY4
    KEY4_Pin,
#endif
  };
  uint8_t raw;
  uint32_t now;

  if (index >= APP_KEY_COUNT)
  {
    return 0U;
  }

  now = HAL_GetTick();
  raw = (HAL_GPIO_ReadPin(keyPorts[index], keyPins[index]) == GPIO_PIN_RESET) ? 1U : 0U; /* active low */

  if (raw != lastRaw[index])
  {
    lastRaw[index] = raw;
    changeTick[index] = now;
  }

  if ((now - changeTick[index]) >= APP_KEY_DEBOUNCE_MS)
  {
    if (stable[index] != raw)
    {
      stable[index] = raw;
      if (stable[index] != 0U)
      {
        return 1U; /* rising edge: key pressed */
      }
    }
  }

  return 0U;
}

static uint8_t App_KeyRepeatStepEvent(uint8_t index)
{
  static uint8_t holdActive[APP_KEY_COUNT] = {0U};
  static uint32_t holdStartTick[APP_KEY_COUNT] = {0U};
  static uint32_t lastRepeatTick[APP_KEY_COUNT] = {0U};
  static GPIO_TypeDef *const keyPorts[APP_KEY_COUNT] = {
    KEY0_GPIO_Port,
    KEY1_GPIO_Port,
    KEY2_GPIO_Port,
#if APP_HAS_KEY3
    KEY3_GPIO_Port,
#endif
#if APP_HAS_KEY4
    KEY4_GPIO_Port,
#endif
  };
  static const uint16_t keyPins[APP_KEY_COUNT] = {
    KEY0_Pin,
    KEY1_Pin,
    KEY2_Pin,
#if APP_HAS_KEY3
    KEY3_Pin,
#endif
#if APP_HAS_KEY4
    KEY4_Pin,
#endif
  };
  uint8_t rawPressed;
  uint32_t now;

  if (index >= APP_KEY_COUNT)
  {
    return 0U;
  }

  now = HAL_GetTick();
  rawPressed = (HAL_GPIO_ReadPin(keyPorts[index], keyPins[index]) == GPIO_PIN_RESET) ? 1U : 0U;

  /* First short press event (debounced). */
  if (App_KeyPressEvent(index) != 0U)
  {
    holdActive[index] = 1U;
    holdStartTick[index] = now;
    lastRepeatTick[index] = now;
    return 1U;
  }

  /* Long-press auto repeat. */
  if ((holdActive[index] != 0U) && (rawPressed != 0U))
  {
    if ((now - holdStartTick[index]) >= APP_KEY_LONGPRESS_START_MS)
    {
      if ((now - lastRepeatTick[index]) >= APP_KEY_REPEAT_MS)
      {
        lastRepeatTick[index] = now;
        return 1U;
      }
    }
  }
  else if (rawPressed == 0U)
  {
    holdActive[index] = 0U;
  }

  return 0U;
}

#if !(APP_HAS_KEY3 && APP_HAS_KEY4)
static uint8_t App_KeyIsPressed(uint8_t index)
{
  static GPIO_TypeDef *const keyPorts[APP_KEY_COUNT] = {
    KEY0_GPIO_Port,
    KEY1_GPIO_Port,
    KEY2_GPIO_Port,
#if APP_HAS_KEY3
    KEY3_GPIO_Port,
#endif
#if APP_HAS_KEY4
    KEY4_GPIO_Port,
#endif
  };
  static const uint16_t keyPins[APP_KEY_COUNT] = {
    KEY0_Pin,
    KEY1_Pin,
    KEY2_Pin,
#if APP_HAS_KEY3
    KEY3_Pin,
#endif
#if APP_HAS_KEY4
    KEY4_Pin,
#endif
  };

  if (index >= APP_KEY_COUNT)
  {
    return 0U;
  }

  return (HAL_GPIO_ReadPin(keyPorts[index], keyPins[index]) == GPIO_PIN_RESET) ? 1U : 0U;
}
#endif

static uint8_t App_MenuItemIsTimeItem(uint8_t item)
{
  switch (item)
  {
    case APP_ITEM_FISH_ON:
    case APP_ITEM_FISH_OFF:
    case APP_ITEM_FILL_ON:
    case APP_ITEM_FILL_OFF:
    case APP_ITEM_FEED_1:
    case APP_ITEM_FEED_2:
    case APP_ITEM_FEED_3:
      return 1U;
    default:
      return 0U;
  }
}

static uint8_t App_MenuItemIsEnabled(uint8_t item)
{
  if (item == APP_ITEM_FEED_2)
  {
    return (g_cfg.feedTimesPerDay >= 2U) ? 1U : 0U;
  }
  if (item == APP_ITEM_FEED_3)
  {
    return (g_cfg.feedTimesPerDay >= 3U) ? 1U : 0U;
  }
  return 1U;
}

static uint8_t App_MenuPageOfItem(uint8_t item)
{
  if ((item == APP_ITEM_FISH_ON) || (item == APP_ITEM_FISH_OFF))
  {
    return APP_MENU_PAGE_FISH;
  }
  if ((item == APP_ITEM_FILL_ON) || (item == APP_ITEM_FILL_OFF))
  {
    return APP_MENU_PAGE_FILL;
  }
  return APP_MENU_PAGE_FEED;
}

static void App_MenuEnterPage(uint8_t page)
{
  switch (page)
  {
    case APP_MENU_PAGE_FISH:
      g_menuItem = APP_ITEM_FISH_ON;
      g_timeEditField = APP_EDIT_FIELD_HOUR;
      break;

    case APP_MENU_PAGE_FILL:
      g_menuItem = APP_ITEM_FILL_ON;
      g_timeEditField = APP_EDIT_FIELD_HOUR;
      break;

    default:
      g_menuItem = APP_ITEM_FEED_TIMES;
      g_timeEditField = APP_EDIT_FIELD_HOUR;
      break;
  }
}

static void App_MenuExitAndSave(void)
{
  g_menuActive = 0U;
  g_menuItem = APP_ITEM_FISH_ON;
  g_timeEditField = APP_EDIT_FIELD_HOUR;
  App_SaveConfigToFlash();
}

static void App_MenuNextPageOrExit(void)
{
  uint8_t page = App_MenuPageOfItem(g_menuItem);

  if (page == APP_MENU_PAGE_FISH)
  {
    App_MenuEnterPage(APP_MENU_PAGE_FILL);
    return;
  }
  if (page == APP_MENU_PAGE_FILL)
  {
    App_MenuEnterPage(APP_MENU_PAGE_FEED);
    return;
  }

  App_MenuExitAndSave();
}

static void App_AdjustCurrentSetting(int8_t delta)
{
  App_TimeOfDay_t *timeItem = NULL;
  int16_t v;
  int16_t minuteDelta;

  if (App_MenuItemIsTimeItem(g_menuItem) != 0U)
  {
    switch (g_menuItem)
    {
      case APP_ITEM_FISH_ON:  timeItem = &g_cfg.fishOn; break;
      case APP_ITEM_FISH_OFF: timeItem = &g_cfg.fishOff; break;
      case APP_ITEM_FILL_ON:  timeItem = &g_cfg.fillOn; break;
      case APP_ITEM_FILL_OFF: timeItem = &g_cfg.fillOff; break;
      case APP_ITEM_FEED_1:   timeItem = &g_cfg.feedTimes[0]; break;
      case APP_ITEM_FEED_2:   timeItem = &g_cfg.feedTimes[1]; break;
      case APP_ITEM_FEED_3:   timeItem = &g_cfg.feedTimes[2]; break;
      default: break;
    }

    if (timeItem != NULL)
    {
      if (g_timeEditField == APP_EDIT_FIELD_HOUR)
      {
        minuteDelta = (delta >= 0) ? 60 : -60;
      }
      else
      {
        minuteDelta = (delta >= 0) ? (int16_t)APP_SETTING_MINUTE_STEP : (int16_t)(-(int16_t)APP_SETTING_MINUTE_STEP);
      }
      App_AddMinutes(timeItem, minuteDelta);
      return;
    }
  }

  switch (g_menuItem)
  {
    case APP_ITEM_FEED_TIMES:
      v = (int16_t)g_cfg.feedTimesPerDay + delta;
      if (v < 1) { v = 1; }
      if (v > (int16_t)APP_FEED_MAX_PER_DAY) { v = (int16_t)APP_FEED_MAX_PER_DAY; }
      g_cfg.feedTimesPerDay = (uint8_t)v;
      break;

    case APP_ITEM_FEED_CYCLES:
      v = (int16_t)g_cfg.feedCycles + delta;
      if (v < (int16_t)APP_FEED_CYCLES_MIN) { v = (int16_t)APP_FEED_CYCLES_MIN; }
      if (v > (int16_t)APP_FEED_CYCLES_MAX) { v = (int16_t)APP_FEED_CYCLES_MAX; }
      g_cfg.feedCycles = (uint8_t)v;
      break;

    default:
      break;
  }
}

static void App_MenuSelectNextInDualTimePage(uint8_t itemA, uint8_t itemB)
{
  if (g_menuItem == itemA)
  {
    if (g_timeEditField == APP_EDIT_FIELD_HOUR)
    {
      g_timeEditField = APP_EDIT_FIELD_MINUTE;
    }
    else
    {
      g_menuItem = itemB;
      g_timeEditField = APP_EDIT_FIELD_HOUR;
    }
  }
  else
  {
    if (g_timeEditField == APP_EDIT_FIELD_HOUR)
    {
      g_timeEditField = APP_EDIT_FIELD_MINUTE;
    }
    else
    {
      g_menuItem = itemA;
      g_timeEditField = APP_EDIT_FIELD_HOUR;
    }
  }
}

static void App_MenuSelectPrevInDualTimePage(uint8_t itemA, uint8_t itemB)
{
  if (g_menuItem == itemA)
  {
    if (g_timeEditField == APP_EDIT_FIELD_MINUTE)
    {
      g_timeEditField = APP_EDIT_FIELD_HOUR;
    }
    else
    {
      g_menuItem = itemB;
      g_timeEditField = APP_EDIT_FIELD_MINUTE;
    }
  }
  else
  {
    if (g_timeEditField == APP_EDIT_FIELD_MINUTE)
    {
      g_timeEditField = APP_EDIT_FIELD_HOUR;
    }
    else
    {
      g_menuItem = itemA;
      g_timeEditField = APP_EDIT_FIELD_MINUTE;
    }
  }
}

static void App_MenuSelectNextInFeedPage(void)
{
  switch (g_menuItem)
  {
    case APP_ITEM_FEED_TIMES:
      g_menuItem = APP_ITEM_FEED_CYCLES;
      g_timeEditField = APP_EDIT_FIELD_HOUR;
      break;

    case APP_ITEM_FEED_CYCLES:
      g_menuItem = APP_ITEM_FEED_1;
      g_timeEditField = APP_EDIT_FIELD_HOUR;
      break;

    case APP_ITEM_FEED_1:
      if (g_timeEditField == APP_EDIT_FIELD_HOUR)
      {
        g_timeEditField = APP_EDIT_FIELD_MINUTE;
      }
      else if (g_cfg.feedTimesPerDay >= 2U)
      {
        g_menuItem = APP_ITEM_FEED_2;
        g_timeEditField = APP_EDIT_FIELD_HOUR;
      }
      else
      {
        g_menuItem = APP_ITEM_FEED_TIMES;
        g_timeEditField = APP_EDIT_FIELD_HOUR;
      }
      break;

    case APP_ITEM_FEED_2:
      if (g_timeEditField == APP_EDIT_FIELD_HOUR)
      {
        g_timeEditField = APP_EDIT_FIELD_MINUTE;
      }
      else if (g_cfg.feedTimesPerDay >= 3U)
      {
        g_menuItem = APP_ITEM_FEED_3;
        g_timeEditField = APP_EDIT_FIELD_HOUR;
      }
      else
      {
        g_menuItem = APP_ITEM_FEED_TIMES;
        g_timeEditField = APP_EDIT_FIELD_HOUR;
      }
      break;

    case APP_ITEM_FEED_3:
      if (g_timeEditField == APP_EDIT_FIELD_HOUR)
      {
        g_timeEditField = APP_EDIT_FIELD_MINUTE;
      }
      else
      {
        g_menuItem = APP_ITEM_FEED_TIMES;
        g_timeEditField = APP_EDIT_FIELD_HOUR;
      }
      break;

    default:
      g_menuItem = APP_ITEM_FEED_TIMES;
      g_timeEditField = APP_EDIT_FIELD_HOUR;
      break;
  }
}

static void App_MenuSelectPrevInFeedPage(void)
{
  switch (g_menuItem)
  {
    case APP_ITEM_FEED_TIMES:
      if (g_cfg.feedTimesPerDay >= 3U)
      {
        g_menuItem = APP_ITEM_FEED_3;
      }
      else if (g_cfg.feedTimesPerDay >= 2U)
      {
        g_menuItem = APP_ITEM_FEED_2;
      }
      else
      {
        g_menuItem = APP_ITEM_FEED_1;
      }
      g_timeEditField = APP_EDIT_FIELD_MINUTE;
      break;

    case APP_ITEM_FEED_CYCLES:
      g_menuItem = APP_ITEM_FEED_TIMES;
      g_timeEditField = APP_EDIT_FIELD_HOUR;
      break;

    case APP_ITEM_FEED_1:
      if (g_timeEditField == APP_EDIT_FIELD_MINUTE)
      {
        g_timeEditField = APP_EDIT_FIELD_HOUR;
      }
      else
      {
        g_menuItem = APP_ITEM_FEED_CYCLES;
        g_timeEditField = APP_EDIT_FIELD_HOUR;
      }
      break;

    case APP_ITEM_FEED_2:
      if (g_timeEditField == APP_EDIT_FIELD_MINUTE)
      {
        g_timeEditField = APP_EDIT_FIELD_HOUR;
      }
      else
      {
        g_menuItem = APP_ITEM_FEED_1;
        g_timeEditField = APP_EDIT_FIELD_MINUTE;
      }
      break;

    case APP_ITEM_FEED_3:
      if (g_timeEditField == APP_EDIT_FIELD_MINUTE)
      {
        g_timeEditField = APP_EDIT_FIELD_HOUR;
      }
      else
      {
        g_menuItem = APP_ITEM_FEED_2;
        g_timeEditField = APP_EDIT_FIELD_MINUTE;
      }
      break;

    default:
      g_menuItem = APP_ITEM_FEED_TIMES;
      g_timeEditField = APP_EDIT_FIELD_HOUR;
      break;
  }
}

static void App_MenuSelectNextInPage(void)
{
  uint8_t page = App_MenuPageOfItem(g_menuItem);

  if (page == APP_MENU_PAGE_FISH)
  {
    App_MenuSelectNextInDualTimePage(APP_ITEM_FISH_ON, APP_ITEM_FISH_OFF);
  }
  else if (page == APP_MENU_PAGE_FILL)
  {
    App_MenuSelectNextInDualTimePage(APP_ITEM_FILL_ON, APP_ITEM_FILL_OFF);
  }
  else
  {
    App_MenuSelectNextInFeedPage();
  }
}

static void App_MenuSelectPrevInPage(void)
{
  uint8_t page = App_MenuPageOfItem(g_menuItem);

  if (page == APP_MENU_PAGE_FISH)
  {
    App_MenuSelectPrevInDualTimePage(APP_ITEM_FISH_ON, APP_ITEM_FISH_OFF);
  }
  else if (page == APP_MENU_PAGE_FILL)
  {
    App_MenuSelectPrevInDualTimePage(APP_ITEM_FILL_ON, APP_ITEM_FILL_OFF);
  }
  else
  {
    App_MenuSelectPrevInFeedPage();
  }
}

static void App_MenuEnsureSelectionValid(void)
{
  if (App_MenuItemIsEnabled(g_menuItem) == 0U)
  {
    g_menuItem = APP_ITEM_FEED_TIMES;
    g_timeEditField = APP_EDIT_FIELD_HOUR;
  }
}

static void App_FormatMenuTime(const App_TimeOfDay_t *time, uint8_t selected, char *buf, uint8_t bufSize)
{
  uint8_t blinkOff;

  if ((time == NULL) || (buf == NULL) || (bufSize == 0U))
  {
    return;
  }

  if (selected == 0U)
  {
    (void)snprintf(buf, bufSize, "%02u:%02u", time->hour, time->minute);
    return;
  }

  blinkOff = ((HAL_GetTick() / 500U) & 1U) ? 1U : 0U;
  if (blinkOff == 0U)
  {
    (void)snprintf(buf, bufSize, "%02u:%02u", time->hour, time->minute);
    return;
  }

  if (g_timeEditField == APP_EDIT_FIELD_HOUR)
  {
    (void)snprintf(buf, bufSize, "  :%02u", time->minute);
  }
  else
  {
    (void)snprintf(buf, bufSize, "%02u:  ", time->hour);
  }
}

void App_HandleKeys(void)
{
#if !(APP_HAS_KEY3 && APP_HAS_KEY4)
  static uint8_t comboState = 0U; /* 0: idle, 1: timing, 2: handled */
  static uint32_t comboTick = 0U;
  uint32_t now;
  uint8_t bothPressed;
#endif
  uint8_t k0;
  uint8_t k1;
  uint8_t k2;
  uint8_t k3 = 0U;
  uint8_t k4 = 0U;

#if !(APP_HAS_KEY3 && APP_HAS_KEY4)
  now = HAL_GetTick();
#endif
  k0 = App_KeyPressEvent(0U);
  k1 = 0U;
  k2 = 0U;

#if APP_HAS_KEY3
  k3 = App_KeyPressEvent(3U);
#endif
#if APP_HAS_KEY4
  k4 = App_KeyPressEvent(4U);
#endif

  if (g_menuActive == 0U)
  {
    k1 = App_KeyPressEvent(1U);

    if (k0 != 0U)
    {
      g_menuActive = 1U;
      App_MenuEnterPage(APP_MENU_PAGE_FISH);
    }

    if (k1 != 0U)
    {
      /* Quick manual feed trigger when not editing. */
      App_FeedStart(g_cfg.feedCycles);
    }
    return;
  }

  if (k0 != 0U)
  {
    App_MenuNextPageOrExit();
#if !(APP_HAS_KEY3 && APP_HAS_KEY4)
    comboState = 0U;
    comboTick = 0U;
#endif
    return;
  }

  App_MenuEnsureSelectionValid();

#if APP_HAS_KEY3
  if (k3 != 0U)
  {
    App_MenuSelectPrevInPage();
    return;
  }
#endif

#if APP_HAS_KEY4
  if (k4 != 0U)
  {
    App_MenuSelectNextInPage();
    return;
  }
#endif

#if !(APP_HAS_KEY3 && APP_HAS_KEY4)
  /* Backward compatibility: use K1+K2 to select next field if K3/K4 are absent. */
  bothPressed = (uint8_t)((App_KeyIsPressed(1U) != 0U) && (App_KeyIsPressed(2U) != 0U));
  if (bothPressed != 0U)
  {
    (void)App_KeyPressEvent(1U);
    (void)App_KeyPressEvent(2U);

    if (comboState == 0U)
    {
      comboState = 1U;
      comboTick = now;
    }
    else if ((comboState == 1U) && ((now - comboTick) >= APP_KEY_DEBOUNCE_MS))
    {
      App_MenuSelectNextInPage();
      comboState = 2U;
    }
    return;
  }

  comboState = 0U;
  comboTick = 0U;
#endif

  k1 = App_KeyRepeatStepEvent(1U);
  k2 = App_KeyRepeatStepEvent(2U);

  /* Menu mode: K1 increase, K2 decrease. */
  if (k1 != 0U)
  {
    App_AdjustCurrentSetting(+1);
  }

  if (k2 != 0U)
  {
    App_AdjustCurrentSetting(-1);
  }

  App_MenuEnsureSelectionValid();
}

void App_ShowDateTime(const DS1302_TimeTypeDef *time)
{
  char line0[40];
  char line1[40];
  char line2[40];
  char line3[40];
  char line4[40];
  char line5[40];
  uint8_t i;
  uint8_t doneCount = 0U;

  if (g_menuActive != 0U)
  {
    uint8_t page = App_MenuPageOfItem(g_menuItem);

    if ((page == APP_MENU_PAGE_FISH) || (page == APP_MENU_PAGE_FILL))
    {
      char tOn[8];
      char tOff[8];
      uint8_t showFish = (page == APP_MENU_PAGE_FISH) ? 1U : 0U;
      uint8_t selOn = 0U;
      uint8_t selOff = 0U;
      if (showFish != 0U)
      {
        selOn = (g_menuItem == APP_ITEM_FISH_ON) ? 1U : 0U;
        selOff = (g_menuItem == APP_ITEM_FISH_OFF) ? 1U : 0U;

        App_FormatMenuTime(&g_cfg.fishOn, selOn, tOn, (uint8_t)sizeof(tOn));
        App_FormatMenuTime(&g_cfg.fishOff, selOff, tOff, (uint8_t)sizeof(tOff));
        (void)snprintf(line1, sizeof(line1), "%cON : %s", selOn ? '>' : ' ', tOn);
        (void)snprintf(line2, sizeof(line2), "%cOFF: %s", selOff ? '>' : ' ', tOff);
        (void)snprintf(line0, sizeof(line0), "\xE8\xAE\xBE\xE7\xBD\xAE \xE9\xB1\xBC\xE7\xBC\xB8\xE7\x81\xAF");
      }
      else
      {
        selOn = (g_menuItem == APP_ITEM_FILL_ON) ? 1U : 0U;
        selOff = (g_menuItem == APP_ITEM_FILL_OFF) ? 1U : 0U;

        App_FormatMenuTime(&g_cfg.fillOn, selOn, tOn, (uint8_t)sizeof(tOn));
        App_FormatMenuTime(&g_cfg.fillOff, selOff, tOff, (uint8_t)sizeof(tOff));
        (void)snprintf(line1, sizeof(line1), "%cON : %s", selOn ? '>' : ' ', tOn);
        (void)snprintf(line2, sizeof(line2), "%cOFF: %s", selOff ? '>' : ' ', tOff);
        (void)snprintf(line0, sizeof(line0), "\xE8\xAE\xBE\xE7\xBD\xAE \xE8\xA1\xA5\xE5\x85\x89\xE7\x81\xAF");
      }

      OLED_Clear();
      OLED_ShowString(0, 0, line0, OLED_8X16);
      OLED_ShowString(0, 16, line1, OLED_8X16);
      OLED_ShowString(0, 32, line2, OLED_8X16);
      OLED_Update();
      return;
    }

    {
      char t1[8], t2[8], t3[8];
      char t2disp[8], t3disp[8];
      uint8_t selFeedTimes = (g_menuItem == APP_ITEM_FEED_TIMES) ? 1U : 0U;
      uint8_t selFeedCycles = (g_menuItem == APP_ITEM_FEED_CYCLES) ? 1U : 0U;
      uint8_t selT1 = (g_menuItem == APP_ITEM_FEED_1) ? 1U : 0U;
      uint8_t selT2 = (g_menuItem == APP_ITEM_FEED_2) ? 1U : 0U;
      uint8_t selT3 = (g_menuItem == APP_ITEM_FEED_3) ? 1U : 0U;

      App_FormatMenuTime(&g_cfg.feedTimes[0], selT1, t1, (uint8_t)sizeof(t1));
      App_FormatMenuTime(&g_cfg.feedTimes[1], selT2, t2, (uint8_t)sizeof(t2));
      App_FormatMenuTime(&g_cfg.feedTimes[2], selT3, t3, (uint8_t)sizeof(t3));

      if (g_cfg.feedTimesPerDay >= 2U) {(void)snprintf(t2disp, sizeof(t2disp), "%s", t2);}
      else {(void)snprintf(t2disp, sizeof(t2disp), "--:--");}
      if (g_cfg.feedTimesPerDay >= 3U) {(void)snprintf(t3disp, sizeof(t3disp), "%s", t3);}
      else {(void)snprintf(t3disp, sizeof(t3disp), "--:--");}

      (void)snprintf(line0, sizeof(line0), "\xE8\xAE\xBE\xE7\xBD\xAE \xE5\x96\x82\xE9\xA3\x9F\xE8\x8F\x9C\xE5\x8D\x95");
      (void)snprintf(line1, sizeof(line1), "%cFeed Times:%u", selFeedTimes ? '>' : ' ', g_cfg.feedTimesPerDay);
      (void)snprintf(line2, sizeof(line2), "%cRotate:%u", selFeedCycles ? '>' : ' ', g_cfg.feedCycles);
      (void)snprintf(line3, sizeof(line3), "%cTime1:%s", selT1 ? '>' : ' ', t1);
      (void)snprintf(line4, sizeof(line4), "%cTime2:%s", selT2 ? '>' : ' ', t2disp);
      (void)snprintf(line5, sizeof(line5), "%cTime3:%s", selT3 ? '>' : ' ', t3disp);

      OLED_Clear();
      OLED_ShowString(0, 0, line0, OLED_8X16);
      OLED_ShowString(0, 16, line1, OLED_6X8);
      OLED_ShowString(0, 24, line2, OLED_6X8);
      OLED_ShowString(0, 32, line3, OLED_6X8);
      OLED_ShowString(0, 40, line4, OLED_6X8);
      OLED_ShowString(0, 48, line5, OLED_6X8);
    }

    OLED_Update();
    return;
  }

  (void)snprintf(line0, sizeof(line0), "20%02u-%02u-%02u %s",
                 (unsigned int)time->year,
                 (unsigned int)time->month,
                 (unsigned int)time->date,
                 DS1302_WeekdayString(time->week));
  (void)snprintf(line1, sizeof(line1), "%02u:%02u",
                 (unsigned int)time->hour,
                 (unsigned int)time->minute);

  for (i = 0U; i < g_cfg.feedTimesPerDay; i++)
  {
    if ((g_feedDoneMask & (uint8_t)(1U << i)) != 0U)
    {
      doneCount++;
    }
  }

  (void)snprintf(line2, sizeof(line2), "\xE9\xB1\xBC\xE7\xBC\xB8%s \xE8\xA1\xA5\xE5\x85\x89%s",
                 (g_fishLightOn != 0U) ? "ON" : "OFF",
                 (g_fillLightOn != 0U) ? "ON" : "OFF");
  (void)snprintf(line3, sizeof(line3), "\xE5\x96\x82\xE9\xA3\x9F%u/%u \xE6\xB0\xB4\xE4\xBD\x8D%u%%",
                 doneCount,
                 g_cfg.feedTimesPerDay,
                 (unsigned int)g_waterPercent);
  (void)snprintf(line4, sizeof(line4), "Press K1 to feed");

  OLED_Clear();
  OLED_ShowString(0, 0, line0, OLED_6X8);
  OLED_ShowString(0, 8, line1, OLED_8X16);
  OLED_ShowString(0, 24, line2, OLED_8X16);
  OLED_ShowString(0, 40, line3, OLED_8X16);
  OLED_ShowString(0, 56, line4, OLED_6X8);
  OLED_Update();
}
