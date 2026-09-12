#include "app_internal.h"

void App_ConfigSanitize(App_Config_t *cfg)
{
  uint8_t i;

  if (cfg == NULL)
  {
    return;
  }

  if (App_TimeOfDayIsValid(&cfg->fishOn) == 0U)   { cfg->fishOn.hour = 8U;  cfg->fishOn.minute = 0U; }
  if (App_TimeOfDayIsValid(&cfg->fishOff) == 0U)  { cfg->fishOff.hour = 22U; cfg->fishOff.minute = 0U; }
  if (App_TimeOfDayIsValid(&cfg->fillOn) == 0U)   { cfg->fillOn.hour = 9U;  cfg->fillOn.minute = 0U; }
  if (App_TimeOfDayIsValid(&cfg->fillOff) == 0U)  { cfg->fillOff.hour = 17U; cfg->fillOff.minute = 0U; }

  if (cfg->feedTimesPerDay < 1U) { cfg->feedTimesPerDay = 1U; }
  if (cfg->feedTimesPerDay > APP_FEED_MAX_PER_DAY) { cfg->feedTimesPerDay = APP_FEED_MAX_PER_DAY; }

  for (i = 0U; i < APP_FEED_MAX_PER_DAY; i++)
  {
    if (App_TimeOfDayIsValid(&cfg->feedTimes[i]) == 0U)
    {
      cfg->feedTimes[i].hour = (uint8_t)(8U + i * 4U);
      cfg->feedTimes[i].minute = 0U;
    }
  }

  if (cfg->feedCycles < APP_FEED_CYCLES_MIN) { cfg->feedCycles = APP_FEED_CYCLES_MIN; }
  if (cfg->feedCycles > APP_FEED_CYCLES_MAX) { cfg->feedCycles = APP_FEED_CYCLES_MAX; }
}

static uint32_t App_ConfigChecksum(const App_Config_t *cfg)
{
  const uint8_t *p;
  uint32_t sum = 0U;
  uint32_t i;

  if (cfg == NULL)
  {
    return 0U;
  }

  p = (const uint8_t *)cfg;
  for (i = 0U; i < (uint32_t)sizeof(App_Config_t); i++)
  {
    sum = (sum << 5) - sum + p[i]; /* djb2-like rolling checksum */
  }
  return sum;
}

static uint32_t App_ConfigFlashAddress(void)
{
  uint32_t flashKb = (uint32_t)(*((uint16_t *)FLASHSIZE_BASE));
  uint32_t flashEnd = FLASH_BASE + flashKb * 1024U;
  return (flashEnd - FLASH_PAGE_SIZE);
}

uint8_t App_LoadConfigFromFlash(void)
{
  uint32_t addr;
  const App_ConfigFlash_t *stored;
  App_Config_t tmp;

  addr = App_ConfigFlashAddress();
  stored = (const App_ConfigFlash_t *)addr;

  if (stored->magic != APP_CFG_FLASH_MAGIC)
  {
    return 0U;
  }
  if (stored->version != APP_CFG_FLASH_VERSION)
  {
    return 0U;
  }
  if (stored->checksum != App_ConfigChecksum(&stored->cfg))
  {
    return 0U;
  }

  tmp = stored->cfg;
  App_ConfigSanitize(&tmp);
  g_cfg = tmp;
  return 1U;
}

void App_SaveConfigToFlash(void)
{
  App_ConfigFlash_t blob;
  FLASH_EraseInitTypeDef erase = {0};
  uint32_t pageError = 0U;
  uint32_t addr;
  uint32_t i;
  const uint8_t *raw;
  uint32_t words;

  App_ConfigSanitize(&g_cfg);

  blob.magic = APP_CFG_FLASH_MAGIC;
  blob.version = APP_CFG_FLASH_VERSION;
  blob.reserved = 0U;
  blob.cfg = g_cfg;
  blob.checksum = App_ConfigChecksum(&blob.cfg);

  addr = App_ConfigFlashAddress();

  (void)HAL_FLASH_Unlock();

  erase.TypeErase = FLASH_TYPEERASE_PAGES;
  erase.PageAddress = addr;
  erase.NbPages = 1U;
  if (HAL_FLASHEx_Erase(&erase, &pageError) != HAL_OK)
  {
    (void)HAL_FLASH_Lock();
    return;
  }

  raw = (const uint8_t *)&blob;
  words = ((uint32_t)sizeof(App_ConfigFlash_t) + 3U) / 4U;
  for (i = 0U; i < words; i++)
  {
    uint32_t w = 0xFFFFFFFFUL;
    uint32_t base = i * 4U;
    if (base < sizeof(App_ConfigFlash_t))
    {
      w = 0U;
      if (base + 0U < sizeof(App_ConfigFlash_t)) { w |= (uint32_t)raw[base + 0U] << 0; }
      if (base + 1U < sizeof(App_ConfigFlash_t)) { w |= (uint32_t)raw[base + 1U] << 8; }
      if (base + 2U < sizeof(App_ConfigFlash_t)) { w |= (uint32_t)raw[base + 2U] << 16; }
      if (base + 3U < sizeof(App_ConfigFlash_t)) { w |= (uint32_t)raw[base + 3U] << 24; }
    }

    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i * 4U, w) != HAL_OK)
    {
      break;
    }
  }

  (void)HAL_FLASH_Lock();
}
