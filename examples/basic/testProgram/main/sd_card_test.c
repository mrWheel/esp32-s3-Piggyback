#include "sd_card_test.h"

#include <stdio.h>

#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

//-- The TFT already uses SPI2_HOST, so the SD card gets its own SPI bus.
#define SD_TEST_HOST        SPI3_HOST
#define SD_TEST_MOUNT_POINT "/sdcard"
#define SD_TEST_FILE_PATH   SD_TEST_MOUNT_POINT "/testfile.txt"

#define SD_TEST_PIN_CS     13
#define SD_TEST_PIN_SCLK   14
#define SD_TEST_PIN_MISO   18
#define SD_TEST_PIN_MOSI   21
#define SD_TEST_PIN_DETECT 7

static const char *TAG = "sd_card_test";

static sdmmc_card_t *s_card = NULL;
static bool s_bus_initialized = false;
static bool s_detect_configured = false;

//-- SD_DTCT is active-low with an internal pull-up: a card closes the switch to
//-- ground, so a detected card reads as level 0.
bool sd_card_test_is_present(void)
{
  if (!s_detect_configured)
  {
    gpio_config_t detect_config = {
      .pin_bit_mask = 1ULL << SD_TEST_PIN_DETECT,
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&detect_config);
    s_detect_configured = true;
  }

  return gpio_get_level(SD_TEST_PIN_DETECT) == 0;
}

esp_err_t sd_card_test_init(void)
{
  if (s_card != NULL)
  {
    ESP_LOGW(TAG, "SD card already initialized");
    return ESP_OK;
  }

  esp_err_t result;

  if (!s_bus_initialized)
  {
    spi_bus_config_t bus_config = {
      .mosi_io_num = SD_TEST_PIN_MOSI,
      .miso_io_num = SD_TEST_PIN_MISO,
      .sclk_io_num = SD_TEST_PIN_SCLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 4000,
    };

    result = spi_bus_initialize(SD_TEST_HOST, &bus_config, SPI_DMA_CH_AUTO);
    if (result != ESP_OK)
    {
      ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(result));
      return result;
    }
    s_bus_initialized = true;
  }

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = SD_TEST_HOST;

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.host_id = SD_TEST_HOST;
  slot_config.gpio_cs = SD_TEST_PIN_CS;
  slot_config.gpio_cd = SD_TEST_PIN_DETECT;

  esp_vfs_fat_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    .max_files = 4,
    .allocation_unit_size = 16 * 1024,
  };

  result = esp_vfs_fat_sdspi_mount(SD_TEST_MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);

  if (result != ESP_OK)
  {
    if (result == ESP_FAIL)
    {
      ESP_LOGE(TAG, "Failed to mount filesystem; card may need to be formatted");
    }
    else
    {
      ESP_LOGE(TAG, "Failed to initialize SD card: %s", esp_err_to_name(result));
    }
    spi_bus_free(SD_TEST_HOST);
    s_bus_initialized = false;
    s_card = NULL;
    return result;
  }

  sdmmc_card_print_info(stdout, s_card);
  return ESP_OK;
}

esp_err_t sd_card_test_format(void)
{
  if (s_card == NULL)
  {
    ESP_LOGE(TAG, "SD card not initialized; run Initialize SDcard first");
    return ESP_ERR_INVALID_STATE;
  }

  esp_err_t result = esp_vfs_fat_sdcard_format(SD_TEST_MOUNT_POINT, s_card);
  if (result != ESP_OK)
  {
    ESP_LOGE(TAG, "Format failed: %s", esp_err_to_name(result));
  }
  return result;
}

esp_err_t sd_card_test_write(const char *text)
{
  if (s_card == NULL)
  {
    ESP_LOGE(TAG, "SD card not initialized; run Initialize SDcard first");
    return ESP_ERR_INVALID_STATE;
  }

  FILE *file = fopen(SD_TEST_FILE_PATH, "w");
  if (!file)
  {
    ESP_LOGE(TAG, "fopen for write failed");
    return ESP_FAIL;
  }

  fprintf(file, "%s\n", text);
  fclose(file);
  return ESP_OK;
}

esp_err_t sd_card_test_append(const char *text)
{
  if (s_card == NULL)
  {
    ESP_LOGE(TAG, "SD card not initialized; run Initialize SDcard first");
    return ESP_ERR_INVALID_STATE;
  }

  FILE *file = fopen(SD_TEST_FILE_PATH, "a");
  if (!file)
  {
    ESP_LOGE(TAG, "fopen for append failed");
    return ESP_FAIL;
  }

  fprintf(file, "%s\n", text);
  fclose(file);
  return ESP_OK;
}

esp_err_t sd_card_test_read(void)
{
  if (s_card == NULL)
  {
    ESP_LOGE(TAG, "SD card not initialized; run Initialize SDcard first");
    return ESP_ERR_INVALID_STATE;
  }

  FILE *file = fopen(SD_TEST_FILE_PATH, "r");
  if (!file)
  {
    ESP_LOGE(TAG, "fopen for read failed; write the testfile first");
    return ESP_FAIL;
  }

  char line[128];
  printf("--- %s ---\n", SD_TEST_FILE_PATH);
  while (fgets(line, sizeof(line), file))
  {
    fputs(line, stdout);
  }
  printf("--- end ---\n");

  fclose(file);
  return ESP_OK;
}
