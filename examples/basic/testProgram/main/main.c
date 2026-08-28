#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "esp32_s3_piggyback.h"

static const uint16_t colors[] = {
    TFT_EC11_RED,
    TFT_EC11_GREEN,
    TFT_EC11_BLUE,
    TFT_EC11_CYAN,
    TFT_EC11_MAGENTA,
    TFT_EC11_YELLOW,
    TFT_EC11_WHITE,
    TFT_EC11_BLACK
};

static const char *names[] = {
    "RED",
    "GREEN",
    "BLUE",
    "CYAN",
    "MAGENTA",
    "YELLOW",
    "WHITE",
    "BLACK"
};

static void menu(void)
{
    puts("\n=== TFT LCD DISPLAY EC11 TESTPROGRAM ===\n"
         "Letters A t/m Z, case insensitive, altijd afsluiten met [Enter]\n"
         "A-D  hoekblok linksboven/rechtsboven/linksonder/rechtsonder\n"
         "E    verticale kleurstroken V1-V8 (links naar rechts)\n"
         "F    horizontale kleurstroken H1-H8 (boven naar onder)\n"
         "G    kleur/inversie-referentie\n"
         "H    rotatie volgende stand\n"
         "I    achtergrondkleur instellen + clear\n"
         "J    fontkleur en grootte 1\n"
         "K    fontgrootte 2 en tekst\n"
         "L    tekstslot overwrite/no-wrap test\n"
         "M    dit menu opnieuw tonen\n"
         "N    expliciete volledige refresh (framebuffer)\n"
         "O    partial-update test (omgeving moet gelijk blijven)\n"
         "P    inversion UIT\n"
         "Q    inversion AAN\n"
         "R    backlight UIT (5 seconden) en AAN\n"
         "S    scherm expliciet leegmaken\n"
         "T    afmetingen en huidige tekststijl tonen\n"
         "U    encoder-events volgen (10 seconden)\n"
         "V    encoderknop-events volgen (10 seconden)\n"
         "W    extra-knop-events volgen (10 seconden)\n"
         "X    alle input-events volgen (30 seconden)\n"
         "Y    clipping/no-wrap rechterrand\n"
         "Z    gecombineerde self-test\n");
}

static bool read_line(char *line, size_t line_size)
{
    static bool skip_lf = false;
    size_t length = 0;

    while (true) {
        char character;
        ssize_t result = read(STDIN_FILENO, &character, 1);

        if (result == 1) {
            if (character == '\r' || character == '\n') {
                if (skip_lf && character == '\n') {
                    skip_lf = false;
                    continue;
                }

                skip_lf = character == '\r';
                write(STDOUT_FILENO, "\n", 1);
                fsync(STDOUT_FILENO);
                line[length] = '\0';
                return true;
            }

            write(STDOUT_FILENO, &character, 1);
            fsync(STDOUT_FILENO);

            if (length + 1 < line_size) {
                line[length++] = character;
            }
            continue;
        }

        if (result == 0 || (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        return false;
    }
}

static void corner(int n)
{
    int z = 40;
    int x = (n & 1) ? tft_ec11_width() - z : 0;
    int y = (n & 2) ? tft_ec11_height() - z : 0;

    tft_ec11_fill_rect(x, y, z, z, colors[n]);
    printf("Hoek %c getekend op x=%d y=%d, verwacht %s\n", 'A' + n, x, y, names[n]);
}

static void stripes(bool vertical)
{
    tft_ec11_set_background(TFT_EC11_BLACK);
    tft_ec11_clear();

    for (int i = 0; i < 8; i++) {
        int x = vertical ? i * tft_ec11_width() / 8 : 0;
        int y = vertical ? 0 : i * tft_ec11_height() / 8;
        int w = vertical ? (i + 1) * tft_ec11_width() / 8 - x : tft_ec11_width();
        int h = vertical ? tft_ec11_height() : (i + 1) * tft_ec11_height() / 8 - y;

        tft_ec11_fill_rect(x, y, w, h, colors[i]);
        printf("%c%d = %s\n", vertical ? 'V' : 'H', i + 1, names[i]);
    }
}

static void color_reference(void)
{
    stripes(true);
    tft_ec11_set_text_style(TFT_EC11_WHITE, 1);
    tft_ec11_set_background(TFT_EC11_BLACK);
    puts("Verwacht links->rechts: RED GREEN BLUE CYAN MAGENTA YELLOW WHITE BLACK");
}

static void events(int seconds, int filter)
{
    printf("Luistert %d seconden; draai/druk nu...\n", seconds);

    TickType_t until = xTaskGetTickCount() + pdMS_TO_TICKS(seconds * 1000);
    tft_ec11_event_t e;

    while ((int32_t)(until - xTaskGetTickCount()) > 0) {
        if (tft_ec11_get_event(&e, pdMS_TO_TICKS(100)) && (filter < 0 || e.type == filter)) {
            if (e.type == TFT_EC11_EVENT_ROTATE) {
                printf("ROTATE signed steps=%ld\n", (long)e.steps);
            } else {
                printf("%s press=%s duration=%lums\n",
                       e.type == TFT_EC11_EVENT_ENCODER_BUTTON ? "ENCODER_BUTTON" : "AUX_BUTTON",
                       e.press == TFT_EC11_PRESS_SHORT ? "SHORT" :
                       e.press == TFT_EC11_PRESS_MEDIUM ? "MEDIUM" : "LONG",
                       (unsigned long)e.duration_ms);
            }
        }
    }
}

static void full_refresh(void)
{
    size_t n = (size_t)tft_ec11_width() * tft_ec11_height();
    uint16_t *fb = malloc(n * sizeof(uint16_t));

    if (!fb) {
        puts("Geen geheugen");
        return;
    }

    for (int y = 0; y < tft_ec11_height(); y++) {
        for (int x = 0; x < tft_ec11_width(); x++) {
            fb[(size_t)y * tft_ec11_width() + x] = colors[((x / 20) + (y / 20)) & 7];
        }
    }

    ESP_ERROR_CHECK(tft_ec11_refresh(fb));
    vTaskDelay(pdMS_TO_TICKS(50));
    free(fb);
    puts("Expliciete full-screen framebuffer refresh gereed");
}

static void partial(void)
{
    for (int y = 0; y < tft_ec11_height(); y += 20) {
        for (int x = 0; x < tft_ec11_width(); x += 20) {
            tft_ec11_fill_rect(x, y, 20, 20, ((x + y) / 20) & 1 ? TFT_EC11_BLUE : TFT_EC11_GREEN);
        }
    }

    for (int i = 0; i < 6; i++) {
        tft_ec11_fill_rect(100, 90, 80, 40, (i & 1) ? TFT_EC11_RED : TFT_EC11_YELLOW);
        vTaskDelay(pdMS_TO_TICKS(300));
    }

    puts("Alleen rechthoek x100 y90 80x40 mocht veranderen");
}

static void text_overwrite(void)
{
    tft_ec11_set_background(TFT_EC11_BLUE);
    tft_ec11_set_text_style(TFT_EC11_WHITE, 2);
    tft_ec11_draw_text(10, 80, 12, "123456789");
    vTaskDelay(pdMS_TO_TICKS(1200));
    tft_ec11_draw_text(10, 80, 12, "99");
    puts("Verwacht alleen 99; oude tekens gewist; geen wrapping");
}

void app_main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    ESP_ERROR_CHECK(tft_ec11_init());
    ESP_ERROR_CHECK(tft_ec11_inputs_start(32));
    menu();

    char line[64];
    uint8_t rotation = CONFIG_TFT_EC11_DEFAULT_ROTATION;

    while (true) {
        write(STDOUT_FILENO, "Choice => ", 10);
        fsync(STDOUT_FILENO);

        if (!read_line(line, sizeof(line))) {
            break;
        }

        char c = 0;

        for (char *p = line; *p; p++) {
            if (isalpha((unsigned char)*p)) {
                c = toupper((unsigned char)*p);
                break;
            }
        }

        if (!c) {
            puts("Voer A t/m Z in en druk Enter");
            continue;
        }

        if (c >= 'A' && c <= 'D') {
            corner(c - 'A');
        } else {
            switch (c) {
            case 'E':
                stripes(true);
                break;
            case 'F':
                stripes(false);
                break;
            case 'G':
                color_reference();
                break;
            case 'H':
                rotation = (rotation + 1) & 3;
                tft_ec11_set_rotation(rotation);
                tft_ec11_clear();
                printf("Rotatie %u, afmetingen %ux%u\n", rotation, tft_ec11_width(), tft_ec11_height());
                break;
            case 'I':
                tft_ec11_set_background(TFT_EC11_BLUE);
                tft_ec11_clear();
                puts("Achtergrond BLUE");
                break;
            case 'J':
                tft_ec11_set_text_style(TFT_EC11_YELLOW, 1);
                tft_ec11_draw_text(5, 5, 30, "FONT YELLOW SIZE 1");
                break;
            case 'K':
                tft_ec11_set_text_style(TFT_EC11_CYAN, 2);
                tft_ec11_draw_text(5, 30, 20, "FONT CYAN SIZE 2");
                break;
            case 'L':
                text_overwrite();
                break;
            case 'M':
                menu();
                break;
            case 'N':
                full_refresh();
                break;
            case 'O':
                partial();
                break;
            case 'P':
                tft_ec11_set_inversion(false);
                puts("INV OFF");
                break;
            case 'Q':
                tft_ec11_set_inversion(true);
                puts("INV ON");
                break;
            case 'R':
                tft_ec11_set_backlight(false);
                vTaskDelay(pdMS_TO_TICKS(5000));
                tft_ec11_set_backlight(true);
                puts("Backlight weer aan");
                break;
            case 'S':
                tft_ec11_clear();
                break;
            case 'T': {
                tft_ec11_text_style_t s = tft_ec11_get_text_style();
                printf("%ux%u bg=%04X fg=%04X scale=%u\n",
                       tft_ec11_width(),
                       tft_ec11_height(),
                       s.background, s.foreground, s.font_scale);
                break;
            }
            case 'U':
                events(10, TFT_EC11_EVENT_ROTATE);
                break;
            case 'V':
                events(10, TFT_EC11_EVENT_ENCODER_BUTTON);
                break;
            case 'W':
                events(10, TFT_EC11_EVENT_AUX_BUTTON);
                break;
            case 'X':
                events(30, -1);
                break;
            case 'Y':
                tft_ec11_draw_text(tft_ec11_width() - 24, 10, 20, "NO WRAP PAST EDGE");
                puts("Tekst moet aan rechterrand clippen en niet wrappen");
                break;
            case 'Z':
                for (int i = 0; i < 4; i++) {
                    corner(i);
                }
                color_reference();
                text_overwrite();
                partial();
                puts("Gecombineerde self-test gereed");
                break;
            default:
                puts("Ongeldige keuze; A t/m Z + Enter");
                break;
            }
        }
    }
}

