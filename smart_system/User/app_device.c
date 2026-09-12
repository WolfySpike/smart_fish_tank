#include "app_internal.h"
#include "adc.h"

static void App_SetRygIndicator(uint16_t red, uint16_t yellow, uint16_t green)
{
  if (red > 999U)   { red = 999U; }
  if (yellow > 999U) { yellow = 999U; }
  if (green > 999U)  { green = 999U; }

  /* TIM3 mapping in this project: CH1=PA6(R), CH2=PA7(Y), CH3=PB0(G) */
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, red);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, yellow);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, green);
}

static void App_SetWaterLevelColor(uint8_t level)
{
  switch (level)
  {
    case APP_WATER_LEVEL_LOW:
      App_SetRygIndicator(0U, 0U, APP_WATER_RGB_DUTY); /* Green */
      break;
    case APP_WATER_LEVEL_MID:
      App_SetRygIndicator(0U, APP_WATER_RGB_DUTY, 0U); /* Yellow */
      break;
    case APP_WATER_LEVEL_HIGH:
      App_SetRygIndicator(APP_WATER_RGB_DUTY, 0U, 0U); /* Red */
      break;
    case APP_WATER_LEVEL_DRY:
    default:
      App_SetRygIndicator(APP_WATER_RGB_DUTY, 0U, 0U); /* Red */
      break;
  }
}

static uint8_t App_WaterAdcToPercent(uint16_t adcRaw)
{
  uint32_t scaled;
  uint32_t minVal;
  uint32_t maxVal;
  uint32_t span;

  minVal = APP_WATER_ADC_PCT_MIN;
  maxVal = APP_WATER_ADC_PCT_MAX;
  if (maxVal <= minVal)
  {
    return 0U;
  }

  if (adcRaw <= minVal)
  {
    return 0U;
  }
  if (adcRaw >= maxVal)
  {
    return 100U;
  }

  span = maxVal - minVal;
  scaled = (((uint32_t)adcRaw - minVal) * 100U + span / 2U) / span;
  return (uint8_t)scaled;
}

void App_WaterLevelUpdate(void)
{
  uint16_t adcNow = 0U;

  if (HAL_ADC_Start(&hadc1) != HAL_OK)
  {
    return;
  }

  if (HAL_ADC_PollForConversion(&hadc1, 10U) != HAL_OK)
  {
    (void)HAL_ADC_Stop(&hadc1);
    return;
  }

  adcNow = (uint16_t)HAL_ADC_GetValue(&hadc1);
  (void)HAL_ADC_Stop(&hadc1);

  if (g_waterAdcRaw == 0U)
  {
    g_waterAdcRaw = adcNow;
  }
  else
  {
    g_waterAdcRaw = (uint16_t)(((uint32_t)g_waterAdcRaw * 3U + adcNow) / 4U);
  }

  adcNow = g_waterAdcRaw;

#if !APP_WATER_ADC_WET_HIGH
  adcNow = (uint16_t)(APP_WATER_ADC_MAX - adcNow);
#endif

  g_waterPercent = App_WaterAdcToPercent(adcNow);

  if (adcNow < APP_WATER_ADC_DRY_TH)
  {
    g_waterLevel = APP_WATER_LEVEL_DRY;         /* <1200: RED */
  }
  else if (adcNow < APP_WATER_ADC_MID_TH)
  {
    g_waterLevel = APP_WATER_LEVEL_LOW;         /* 1200~1599: GREEN */
  }
  else if (adcNow < APP_WATER_ADC_HIGH_TH)
  {
    g_waterLevel = APP_WATER_LEVEL_MID;         /* 1600~1899: YELLOW */
  }
  else
  {
    g_waterLevel = APP_WATER_LEVEL_HIGH;        /* >=1900: RED */
  }

  App_SetWaterLevelColor(g_waterLevel);
}

const char *App_WaterLevelText(void)
{
  switch (g_waterLevel)
  {
    case APP_WATER_LEVEL_LOW:
      return "1";
    case APP_WATER_LEVEL_MID:
      return "2";
    case APP_WATER_LEVEL_HIGH:
      return "3";
    case APP_WATER_LEVEL_DRY:
    default:
      return "0";
  }
}

void App_SetFishLight(uint8_t on)
{
  g_fishLightOn = on ? 1U : 0U;
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, g_fishLightOn ? APP_FISH_LIGHT_DUTY : 0U);
}

void App_SetFillLight(uint8_t on)
{
  g_fillLightOn = on ? 1U : 0U;
  HAL_GPIO_WritePin(FILL_LIGHT_GPIO_Port, FILL_LIGHT_Pin, g_fillLightOn ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void App_SetServoPulse(uint16_t pulseUs)
{
  if (pulseUs < APP_SERVO_PULSE_MIN_US)
  {
    pulseUs = APP_SERVO_PULSE_MIN_US;
  }
  if (pulseUs > APP_SERVO_PULSE_MAX_US)
  {
    pulseUs = APP_SERVO_PULSE_MAX_US;
  }
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulseUs);
}

void App_FeedStart(uint8_t cycles)
{
  if (g_feedState != APP_FEED_STATE_IDLE)
  {
    return;
  }

  if (cycles < APP_FEED_CYCLES_MIN)
  {
    cycles = APP_FEED_CYCLES_MIN;
  }
  if (cycles > APP_FEED_CYCLES_MAX)
  {
    cycles = APP_FEED_CYCLES_MAX;
  }

  g_feedCyclesRemaining = cycles;
  App_SetServoPulse(APP_SERVO_SWING_A_US);
  g_feedState = APP_FEED_STATE_TO_B;
  g_feedStepTick = HAL_GetTick();
}

void App_FeedUpdate(void)
{
  uint32_t now = HAL_GetTick();

  switch (g_feedState)
  {
    case APP_FEED_STATE_TO_B:
      if ((now - g_feedStepTick) >= APP_SERVO_STEP_HOLD_MS)
      {
        App_SetServoPulse(APP_SERVO_SWING_B_US);
        g_feedState = APP_FEED_STATE_TO_A;
        g_feedStepTick = now;
      }
      break;

    case APP_FEED_STATE_TO_A:
      if ((now - g_feedStepTick) >= APP_SERVO_STEP_HOLD_MS)
      {
        if (g_feedCyclesRemaining > 0U)
        {
          g_feedCyclesRemaining--;
        }

        if (g_feedCyclesRemaining > 0U)
        {
          App_SetServoPulse(APP_SERVO_SWING_A_US);
          g_feedState = APP_FEED_STATE_TO_B;
          g_feedStepTick = now;
        }
        else
        {
          App_SetServoPulse(APP_SERVO_PULSE_MID_US);
          g_feedState = APP_FEED_STATE_FINISH;
          g_feedStepTick = now;
        }
      }
      break;

    case APP_FEED_STATE_FINISH:
      if ((now - g_feedStepTick) >= APP_SERVO_RETURN_HOLD_MS)
      {
        g_feedState = APP_FEED_STATE_IDLE;
      }
      break;

    default:
      g_feedState = APP_FEED_STATE_IDLE;
      App_SetServoPulse(APP_SERVO_PULSE_MID_US);
      break;
  }
}
