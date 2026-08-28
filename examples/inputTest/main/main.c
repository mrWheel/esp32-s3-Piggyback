#include <stdio.h>
#include "esp32_s3_piggyback.h"
void app_main(void){ESP_ERROR_CHECK(tft_ec11_inputs_start(32));puts("Rotate EC11 and press both buttons. Events follow:");tft_ec11_event_t e;while(1)if(tft_ec11_get_event(&e,portMAX_DELAY)){if(e.type==TFT_EC11_EVENT_ROTATE)printf("ROTATE %+ld\n",(long)e.steps);else printf("%s %s %lums\n",e.type==TFT_EC11_EVENT_ENCODER_BUTTON?"EC_PUSH":"EXTRA_PUSH",e.press==TFT_EC11_PRESS_SHORT?"SHORT":e.press==TFT_EC11_PRESS_MEDIUM?"MEDIUM":"LONG",(unsigned long)e.duration_ms);}}
