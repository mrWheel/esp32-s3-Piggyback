#pragma once

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

bool sd_card_test_is_present(void);
esp_err_t sd_card_test_init(void);
esp_err_t sd_card_test_format(void);
esp_err_t sd_card_test_write(const char *text);
esp_err_t sd_card_test_append(const char *text);
esp_err_t sd_card_test_read(void);

#ifdef __cplusplus
}
#endif
