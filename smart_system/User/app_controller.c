 #include "app_controller.h"
#include "app_internal.h"
#include "adc.h"

App_Config_t g_cfg = {
  {8U, 0U},   /* Fish light on 08:00 */
  {22U, 0U},  /* Fish light off 22:00 */
  {9U, 0U},   /* Fill light on 09:00 */
  {17U, 0U},  /* Fill light off 17:00 */
  2U,         /* feed times/day */
  {{8U, 30U}, {18U, 30U}, {21U, 0U}},
  2U          /* servo swings per feed */
};

uint8_t g_menuActive = 0U;
uint8_t g_menuItem = APP_ITEM_FISH_ON;
uint8_t g_timeEditField = APP_EDIT_FIELD_HOUR;

uint8_t g_feedState = APP_FEED_STATE_IDLE;
uint32_t g_feedStepTick = 0U;
uint8_t g_feedCyclesRemaining = 0U;
uint8_t g_feedDoneMask = 0U;
uint32_t g_lastDateKey = 0U;

uint8_t g_fishLightOn = 0U;
uint8_t g_fillLightOn = 0U;
uint8_t g_waterLevel = APP_WATER_LEVEL_DRY;
uint16_t g_waterAdcRaw = 0U;
uint8_t g_waterPercent = 0U;

void AppController_Init(void)
{
  OLED_Init();
  DS1302_Init();
  (void)HAL_ADCEx_Calibration_Start(&hadc1);
  App_InitRtcIfNeeded();
  if (App_LoadConfigFromFlash() == 0U)
  {
    App_ConfigSanitize(&g_cfg);
  }

  /* Start RGBW PWM outputs (TIM3 CH1~CH4) and SG90 PWM output (TIM1 CH1). */
  (void)HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  (void)HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  (void)HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
  (void)HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
  (void)HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

  /* Initial state */
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0U);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0U);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0U);
  App_SetFishLight(0U);
  App_SetFillLight(0U);
  App_SetServoPulse(APP_SERVO_PULSE_MID_US);
  App_WaterLevelUpdate();
}

void AppController_Task(void)
{
  static uint32_t lastRefreshTick = 0U;
  static DS1302_TimeTypeDef timeNow = {0};

  App_HandleKeys();
  App_FeedUpdate();

  if ((HAL_GetTick() - lastRefreshTick) >= 200U)
  {
    lastRefreshTick = HAL_GetTick();
    App_WaterLevelUpdate();

    if (DS1302_GetDateTime(&timeNow) == HAL_OK && App_TimeIsValid(&timeNow) != 0U)
    {
      App_RunSchedule(&timeNow);
      App_ShowDateTime(&timeNow);
    }
    else
    {
      /* Retry once after re-initialization to recover from unstable startup. */
      DS1302_Init();
      App_InitRtcIfNeeded();
      if (DS1302_GetDateTime(&timeNow) == HAL_OK && App_TimeIsValid(&timeNow) != 0U)
      {
        App_RunSchedule(&timeNow);
        App_ShowDateTime(&timeNow);
      }
      else
      {
        DS1302_RawTimeTypeDef rawTime = {0};

        OLED_Clear();
        OLED_ShowString(0, 0, "1302 \xE6\x97\xB6\xE9\x97\xB4 ?", OLED_6X8);
        if (DS1302_GetRawDateTime(&rawTime) == HAL_OK)
        {
          OLED_ShowString(0, 16, "0:", OLED_6X8);
          OLED_ShowHexNum(12, 16, rawTime.second, 2, OLED_6X8);
          OLED_ShowString(30, 16, "1:", OLED_6X8);
          OLED_ShowHexNum(42, 16, rawTime.minute, 2, OLED_6X8);
          OLED_ShowString(60, 16, "2:", OLED_6X8);
          OLED_ShowHexNum(72, 16, rawTime.hour, 2, OLED_6X8);

          OLED_ShowString(0, 32, "3:", OLED_6X8);
          OLED_ShowHexNum(12, 32, rawTime.date, 2, OLED_6X8);
          OLED_ShowString(30, 32, "4:", OLED_6X8);
          OLED_ShowHexNum(48, 32, rawTime.month, 2, OLED_6X8);
          OLED_ShowString(66, 32, "5:", OLED_6X8);
          OLED_ShowHexNum(78, 32, rawTime.week, 2, OLED_6X8);

          OLED_ShowString(0, 48, "6:", OLED_6X8);
          OLED_ShowHexNum(12, 48, rawTime.year, 2, OLED_6X8);
        }
        OLED_Update();
      }
    }
  }
}
