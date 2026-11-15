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
#include "sd_card_test.h"

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

static void menu(uint8_t rotation)
{
    tft_ec11_text_style_t style = tft_ec11_get_text_style();
    printf("\n=== TFT LCD DISPLAY EC11 TESTPROGRAM ===\n"
         "Letters A t/m Z, case insensitive, altijd afsluiten met [Enter]\n"
         "A-D  hoekblok linksboven/rechtsboven/linksonder/rechtsonder\n"
         "E    verticale kleurstroken V1-V8 (links naar rechts)\n"
         "F    horizontale kleurstroken H1-H8 (boven naar onder)\n"
         "G    kleur/inversie-referentie\n"
         "H    rotatie volgende stand (actueel %u)\n"
         "I    achtergrondkleur instellen + clear (actueel %04X)\n"
         "J    tekstinstellingen en tekst schrijven\n"
         "K    fontgrootte 2 en tekst\n"
         "L    tekstslot overwrite/no-wrap test\n"
         "M    SDcard submenu (init/format/write/append/read)\n"
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
         "Z    gecombineerde self-test\n", rotation, style.background);
}

static bool read_line(char *line, size_t line_size)
{
    static bool skip_lf = false;
    size_t length = 0;

    while (true) {
        char character;
        ssize_t result = read(STDIN_FILENO, &character, 1);

        if (result == 1) {
            if (skip_lf) {
                skip_lf = false;
                if (character == '\n') {
                    continue;
                }
            }

            if (character == '\r' || character == '\n') {
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
    static const int positions[][2] = {
        { 0, 0 },
        { 0, 1 },
        { 1, 0 },
        { 1, 1 }
    };
    static const int color_indices[] = { 0, 1, 2, 3 };
    int x = positions[n][0] ? tft_ec11_width() - z : 0;
    int y = positions[n][1] ? tft_ec11_height() - z : 0;

    tft_ec11_fill_rect(x, y, z, z, colors[color_indices[n]]);
    printf("Hoek %c getekend op x=%d y=%d, verwacht %s\n", 'A' + n, x, y, names[color_indices[n]]);
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
            until = xTaskGetTickCount() + pdMS_TO_TICKS(seconds * 1000);

            if (e.type == TFT_EC11_EVENT_ROTATE) {
                printf("ROTATE signed steps=%ld\n", (long)e.steps);
            } else if (e.press == TFT_EC11_PRESS_DOWN) {
                printf("%s pressed\n", e.type == TFT_EC11_EVENT_ENCODER_BUTTON ? "ENCODER_BUTTON" : "AUX_BUTTON");
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

//-- The four lines below together cover the complete printable ASCII range
//-- (32..126); the font face option draws them with tft_ec11_draw_text_font
//-- so glyph coverage can be checked on the display itself.
static const char *font_sample_lines[] = {
    " !\"#$%&'()*+,-./012345678",
    ":;<=>?@ABCDEFGHIJKLMNO",
    "PQRSTUVWXYZ[\\]^_`abcdefgh",
    "ijklmnopqrstuvwxyz{|}~"
};

static const char *font_face_names[] = {
    "SMALL (5x7 glyph, 6x8 cell)",
    "MEDIUM (7x12 glyph, 8x12 cell)",
    "LARGE (8x16 glyph, 9x16 cell)"
};

static const uint8_t font_face_cell_heights[] = { 8, 12, 16 };

static void draw_font_sample(tft_ec11_font_t font)
{
    tft_ec11_text_style_t style = tft_ec11_get_text_style();
    int cell_height = font_face_cell_heights[font];
    int top = tft_ec11_height() - (int)(sizeof(font_sample_lines) / sizeof(font_sample_lines[0])) * cell_height;

    for (size_t i = 0; i < sizeof(font_sample_lines) / sizeof(font_sample_lines[0]); i++)
    {
        tft_ec11_draw_text_font(0, top + (int)i * cell_height, font_sample_lines[i], font,
                                style.background, style.foreground);
    }
}

static void text_settings_menu(void)
{
    static const uint16_t text_colors[] = {
        TFT_EC11_WHITE, TFT_EC11_YELLOW, TFT_EC11_CYAN,
        TFT_EC11_RED, TFT_EC11_GREEN
    };
    static const char *text_color_names[] = {
        "WHITE", "YELLOW", "CYAN", "RED", "GREEN"
    };
    static const uint16_t background_colors[] = {
        TFT_EC11_BLACK, TFT_EC11_BLUE, TFT_EC11_WHITE, TFT_EC11_YELLOW
    };
    static const char *background_names[] = {
        "BLACK", "BLUE", "WHITE", "YELLOW"
    };
    static size_t text_color_index = 0;
    static size_t background_index = 0;
    static uint8_t font_scale = 1;
    static tft_ec11_font_t font_face = TFT_EC11_FONT_SMALL;
    static int text_x = 0;
    static int text_y = 0;
    char line[64];

    while (true) {
        tft_ec11_text_style_t style = tft_ec11_get_text_style();
        for (size_t i = 0; i < sizeof(background_colors) / sizeof(background_colors[0]); i++) {
            if (background_colors[i] == style.background) {
                background_index = i;
                break;
            }
        }
        for (size_t i = 0; i < sizeof(text_colors) / sizeof(text_colors[0]); i++) {
            if (text_colors[i] == style.foreground) {
                text_color_index = i;
                break;
            }
        }
         printf("\n1  background: %s\n"
             "2  font size: %u\n"
             "3  font color: %s\n"
             "4  x-position: %d\n"
             "5  y-position: %d\n"
             "6  enter text\n"
             "7  show selected settings\n"
             "8  return to main menu\n"
             "9  font face: %s (redraws full ASCII sample)\n",
               background_names[background_index], style.font_scale,
               text_color_names[text_color_index], text_x, text_y,
               font_face_names[font_face]);
        write(STDOUT_FILENO, "Text choice => ", 15);
        fsync(STDOUT_FILENO);

        if (!read_line(line, sizeof(line))) {
            return;
        }

        char choice = 0;
        for (char *p = line; *p; p++) {
            if (isdigit((unsigned char)*p)) {
                choice = *p;
                break;
            }
        }

        switch (choice) {
        case '1':
            background_index = (background_index + 1) % (sizeof(background_colors) / sizeof(background_colors[0]));
            tft_ec11_set_background(background_colors[background_index]);
            printf("Selected background: %s\n", background_names[background_index]);
            break;
        case '2':
            font_scale = font_scale == 3 ? 1 : font_scale + 1;
            tft_ec11_set_text_style(style.foreground, font_scale);
            printf("Selected font size: %u\n", font_scale);
            break;
        case '3':
            text_color_index = (text_color_index + 1) % (sizeof(text_colors) / sizeof(text_colors[0]));
            tft_ec11_set_text_style(text_colors[text_color_index], font_scale);
            printf("Selected foreground: %s\n", text_color_names[text_color_index]);
            break;
        case '4':
        case '5': {
            char position_choice = choice == '4' ? 'X' : 'Y';
            int *coordinate = choice == '4' ? &text_x : &text_y;
            int maximum = choice == '4' ? tft_ec11_width() - 1 : tft_ec11_height() - 1;
            char prompt[16];
            snprintf(prompt, sizeof(prompt), "%c-pos: ", position_choice);
            write(STDOUT_FILENO, prompt, strlen(prompt));
            fsync(STDOUT_FILENO);
            if (read_line(line, sizeof(line))) {
                int value = atoi(line);
                if (value >= 0 && value <= maximum) {
                    *coordinate = value;
                } else {
                    puts("Position outside the display");
                }
            }
            break;
        }
        case '6': {
            const char *prompt = "Enter text: ";
            write(STDOUT_FILENO, prompt, strlen(prompt));
            fsync(STDOUT_FILENO);
            if (read_line(line, sizeof(line))) {
                if (font_face == TFT_EC11_FONT_SMALL) {
                    tft_ec11_draw_text_simple(text_x, text_y, line);
                } else {
                    tft_ec11_draw_text_font(text_x, text_y, line, font_face,
                                            style.background, style.foreground);
                }
            }
            break;
        }
        case '7':
            printf("Selected: background=%s size=%u foreground=%s x=%d y=%d\n",
                   background_names[background_index], style.font_scale,
                   text_color_names[text_color_index], text_x, text_y);
            break;
        case '8':
            return;
        case '9':
            font_face = (font_face + 1) % (sizeof(font_face_names) / sizeof(font_face_names[0]));
            printf("Font face: %s\n", font_face_names[font_face]);
            draw_font_sample(font_face);
            break;
        default:
            puts("Use 1, 2, 3, 4, 5, 6, 7, 8, or 9");
            break;
        }
    }
}

static void sd_card_menu(void)
{
    char line[128];

    while (true) {
        printf("\n=== SDcard === (%s)\n"
             "1  Initialize SDcard\n"
             "2  Format SDcard\n"
             "3  Write Testfile\n"
             "4  Append Testfile\n"
             "5  Read TestFile\n"
             "9  Return to main menu\n",
             sd_card_test_is_present() ? "SDcard detected" : "No SDcard detected");
        write(STDOUT_FILENO, "SDcard choice => ", 17);
        fsync(STDOUT_FILENO);

        if (!read_line(line, sizeof(line))) {
            return;
        }

        char choice = 0;
        for (char *p = line; *p; p++) {
            if (isdigit((unsigned char)*p)) {
                choice = *p;
                break;
            }
        }

        switch (choice) {
        case '1':
            if (sd_card_test_init() == ESP_OK) {
                puts("SDcard initialized");
            }
            break;
        case '2':
            if (sd_card_test_format() == ESP_OK) {
                puts("SDcard formatted");
            }
            break;
        case '3': {
            const char *prompt = "Enter text: ";
            write(STDOUT_FILENO, prompt, strlen(prompt));
            fsync(STDOUT_FILENO);
            if (read_line(line, sizeof(line)) && sd_card_test_write(line) == ESP_OK) {
                puts("Testfile written");
            }
            break;
        }
        case '4': {
            const char *prompt = "Enter text: ";
            write(STDOUT_FILENO, prompt, strlen(prompt));
            fsync(STDOUT_FILENO);
            if (read_line(line, sizeof(line)) && sd_card_test_append(line) == ESP_OK) {
                puts("Testfile appended");
            }
            break;
        }
        case '5':
            sd_card_test_read();
            break;
        case '9':
            return;
        default:
            puts("Use 1, 2, 3, 4, 5, or 9");
            break;
        }
    }
}

void app_main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    ESP_ERROR_CHECK(tft_ec11_init());
    ESP_ERROR_CHECK(tft_ec11_inputs_start(32));

    char line[64];
    uint8_t rotation = CONFIG_TFT_EC11_DEFAULT_ROTATION;
    menu(rotation);

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
            menu(rotation);
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
                menu(rotation);
                break;
            case 'I': {
                static const uint16_t background_colors[] = {
                    TFT_EC11_BLACK, TFT_EC11_BLUE, TFT_EC11_WHITE, TFT_EC11_YELLOW
                };
                size_t current = 0;
                tft_ec11_text_style_t style = tft_ec11_get_text_style();
                for (size_t i = 0; i < sizeof(background_colors) / sizeof(background_colors[0]); i++) {
                    if (background_colors[i] == style.background) {
                        current = i;
                        break;
                    }
                }
                tft_ec11_set_background(background_colors[(current + 1) % (sizeof(background_colors) / sizeof(background_colors[0]))]);
                tft_ec11_clear();
                printf("Achtergrond %04X\n", tft_ec11_get_text_style().background);
                break;
            }
            case 'J':
                text_settings_menu();
                break;
            case 'K':
                tft_ec11_set_text_style(TFT_EC11_CYAN, 2);
                tft_ec11_draw_text(5, 30, 20, "FONT CYAN SIZE 2");
                break;
            case 'L':
                text_overwrite();
                break;
            case 'M':
                sd_card_menu();
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

