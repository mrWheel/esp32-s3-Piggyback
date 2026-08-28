#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp32_s3_piggyback.h"
void app_main(void){ESP_ERROR_CHECK(tft_ec11_init());const uint16_t c[]={TFT_EC11_RED,TFT_EC11_GREEN,TFT_EC11_BLUE,TFT_EC11_WHITE,TFT_EC11_BLACK};for(int inv=0;inv<2;inv++){tft_ec11_set_inversion(inv);printf("INV %s\n",inv?"ON":"OFF");for(int r=0;r<4;r++){tft_ec11_set_rotation(r);printf("rotation=%d size=%ux%u\n",r,tft_ec11_width(),tft_ec11_height());for(int i=0;i<5;i++){tft_ec11_set_background(c[i]);tft_ec11_clear();vTaskDelay(pdMS_TO_TICKS(700));}int z=30;tft_ec11_fill_rect(0,0,z,z,TFT_EC11_RED);tft_ec11_fill_rect(tft_ec11_width()-z,0,z,z,TFT_EC11_GREEN);tft_ec11_fill_rect(0,tft_ec11_height()-z,z,z,TFT_EC11_BLUE);tft_ec11_fill_rect(tft_ec11_width()-z,tft_ec11_height()-z,z,z,TFT_EC11_YELLOW);vTaskDelay(pdMS_TO_TICKS(1500));}}tft_ec11_set_inversion(true);puts("Display phase-1 sequence complete");}
