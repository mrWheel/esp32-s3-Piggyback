# esp32_s3_piggyback

Native ESP-IDF component and hardware test projects for the ESP32-S3 Piggyback board.

## Requirements

- ESP-IDF 5.1 or newer
- ESP32-S3 target
- USB serial/JTAG or UART monitor at 115200 baud

## Build the interactive test program

Open the repository root in VS Code, or use:

```sh
idf.py set-target esp32s3
idf.py menuconfig
idf.py build flash monitor
```

The root project builds the same source as `examples/basic/testProgram`, so it is immediately useful
after extracting the ZIP. The example directory can also be built as an independent ESP-IDF project.

Every menu choice is a letter A-Z (case-insensitive) followed by Enter. Pressing Enter on an
empty line redraws the menu. `M` opens the SD card diagnostic submenu (see below).
The visual tests print stable labels in the monitor, so observations can be reported without ambiguity.

## Initializing the component

Every application must call `tft_ec11_init()` before any other component function, and
`tft_ec11_inputs_start()` before it can receive encoder/button events. A minimal `app_main()`
looks like this:

```c
#include "esp32_s3_piggyback.h"
#include "esp_log.h"

void app_main(void)
{
    //-- Bring up SPI, GPIO, the ST7789 panel, and apply the Kconfig defaults (rotation, inversion)
    ESP_ERROR_CHECK(tft_ec11_init());

    //-- Start the encoder ISR and the button polling task; queue holds up to 32 pending events
    ESP_ERROR_CHECK(tft_ec11_inputs_start(32));

    tft_ec11_set_background(TFT_EC11_BLACK);
    tft_ec11_set_text_style(TFT_EC11_WHITE, 1);
    tft_ec11_clear();
    tft_ec11_draw_text_simple(10, 10, "Ready");

    tft_ec11_event_t event;

    while (true)
    {
        if (tft_ec11_get_event(&event, portMAX_DELAY))
        {
            if (event.type == TFT_EC11_EVENT_ROTATE)
            {
                ESP_LOGI("app", "rotate steps=%ld", (long)event.steps);
            }
            else
            {
                ESP_LOGI("app", "button=%d press=%d duration=%lums",
                          event.type, event.press, (unsigned long)event.duration_ms);
            }
        }
    }
}
```

## Font sizes

There are five **native bitmap font sizes** (faces). Faces are named after their character cell
(matrix): the glyph bitmap is 1 px narrower than the cell, the extra column is the inter-character
gap. Each face stores its own glyph bitmaps and is drawn pixel-accurate at its own size - glyphs
are never scaled, stretched or interpolated. Pick the face with `tft_ec11_draw_text_font()`:

| Face                   | Glyph  | Character advance | Line height (cleared area) |
|------------------------|--------|-------------------|----------------------------|
| `TFT_EC11_FONT_6x8`    | 5x7    | 6 px (1 px gap)   | 8 px                       |
| `TFT_EC11_FONT_8x12`   | 7x12   | 8 px (1 px gap)   | 12 px                      |
| `TFT_EC11_FONT_9x16`   | 8x16   | 9 px (1 px gap)   | 16 px                      |
| `TFT_EC11_FONT_13x24`  | 12x24  | 13 px (1 px gap)  | 24 px                      |
| `TFT_EC11_FONT_17x32`  | 16x32  | 17 px (1 px gap)  | 32 px                      |

The legacy size names `TFT_EC11_FONT_SMALL`, `TFT_EC11_FONT_MEDIUM` and `TFT_EC11_FONT_LARGE` (and
the long forms `TFT_EC11_FONT_XLARGE` / `TFT_EC11_FONT_XXLARGE`) are kept as aliases of the matrix
names with identical enum values; new code should prefer the `TFT_EC11_FONT_<matrix>` names.

```c
// 6x8 face:  5x7 glyphs, 6 px per character, 8 px tall
tft_ec11_draw_text_font(10, 10, "Temp 21.5 C", TFT_EC11_FONT_6x8, TFT_EC11_BLACK, TFT_EC11_WHITE);

// 8x12 face: 7x12 glyphs, 8 px per character, 12 px tall
tft_ec11_draw_text_font(10, 30, "Temp 21.5 C", TFT_EC11_FONT_8x12, TFT_EC11_BLACK, TFT_EC11_WHITE);

// 9x16 face:  8x16 glyphs, 9 px per character, 16 px tall
tft_ec11_draw_text_font(10, 60, "Temp 21.5 C", TFT_EC11_FONT_9x16, TFT_EC11_BLACK, TFT_EC11_WHITE);

// 13x24 face: 12x24 glyphs, 13 px per character, 24 px tall
tft_ec11_draw_text_font(10, 90, "Temp 21.5 C", TFT_EC11_FONT_13x24, TFT_EC11_BLACK, TFT_EC11_WHITE);

// 17x32 face: 16x32 glyphs, 17 px per character, 32 px tall
tft_ec11_draw_text_font(10, 120, "Temp 21.5 C", TFT_EC11_FONT_17x32, TFT_EC11_BLACK, TFT_EC11_WHITE);
```

All five faces cover printable ASCII 32..126 (space..tilde), with real descenders (g j p q y , ;)
in all faces except the 5x7 one. The 7x12 face and larger draw lowercase as lowercase; the 5x7 face
folds lowercase to uppercase like the legacy API. Text is UTF-8; accented Latin-1 letters fold onto
their base glyph, unknown bytes render as a hollow box.

The 13x24 and 17x32 faces are generated on the development machine from the Unicode BDF bitmaps of
the **Terminus** font (terminus-font 4.49 by Dimitar Toshkov Zhekov, licensed under
the SIL Open Font License 1.1 - full license text in
`components/esp32_s3_piggyback/TERMINUS-OFL-1.1.txt`). The 13x24 face uses the Terminus **bold**
weight (`ter-u24b.bdf`) so its 2 px stems match the visual weight of the 9x16 VGA face on the TFT;
the 17x32 face uses the regular weight (`ter-u32n.bdf`), which already draws 2 px stems at that
size. The faces are stored as `static const uint16_t`
tables (one uint16_t per glyph row, 4,560 + 6,080 bytes of flash) in
`esp32_s3_piggyback_font.c`; the ESP32-S3 only reads the pre-generated bitmaps. Regenerate them
with:

```sh
python3 -I tools/font_gen/generate_fonts.py ter-u24b.bdf ter-u32n.bdf
```

and replace the two blocks between the `BEGIN/END GENERATED` markers in `esp32_s3_piggyback_font.c`
with the printed tables (host-side check after regeneration: `tools/font_check/font_check.c`).

The `font_scale` (1-3) of `tft_ec11_set_text_style()` and `tft_ec11_draw_text_styled()` only
integer-scales the 5x7 face. To get sizes in between without stretched strokes, use
`tft_ec11_draw_text_font()` with `TFT_EC11_FONT_8x12` or larger instead.

## API Reference

All functions are declared in `components/esp32_s3_piggyback/include/esp32_s3_piggyback.h`.

### Lifecycle

#### `esp_err_t tft_ec11_init(void)`

Initializes the SPI bus, backlight GPIO, and the ST7789 panel driver. Applies the default
rotation and inversion settings from Kconfig, turns the backlight on, and clears the screen.
Must be called once before any other component function.

```c
ESP_ERROR_CHECK(tft_ec11_init());
```

#### `esp_err_t tft_ec11_deinit(void)`

Stops input handling (equivalent to `tft_ec11_inputs_stop()`), turns off the backlight, and
releases the panel, SPI bus, and DMA buffers.

```c
tft_ec11_deinit();
```

### Display properties

#### `esp_err_t tft_ec11_set_rotation(uint8_t rotation)`

Sets the display rotation (`0`-`3`). Swaps `tft_ec11_width()`/`tft_ec11_height()` for the
odd rotations (`1` and `3`).

```c
tft_ec11_set_rotation(3);
printf("Now %ux%u\n", tft_ec11_width(), tft_ec11_height());
```

#### `esp_err_t tft_ec11_set_inversion(bool enabled)`

Turns the ST7789 color inversion on or off.

```c
tft_ec11_set_inversion(true);
```

#### `esp_err_t tft_ec11_set_backlight(bool enabled)`

Switches the backlight GPIO on or off.

```c
tft_ec11_set_backlight(false);
vTaskDelay(pdMS_TO_TICKS(1000));
tft_ec11_set_backlight(true);
```

#### `uint16_t tft_ec11_width(void)` / `uint16_t tft_ec11_height(void)`

Return the current logical display size in pixels, taking the active rotation into account.

```c
printf("Display is %u x %u\n", tft_ec11_width(), tft_ec11_height());
```

### Text style

#### `void tft_ec11_set_background(uint16_t color)`

Sets the global background color (RGB565), used by `tft_ec11_clear()` and by the text
functions that use the global style.

```c
tft_ec11_set_background(TFT_EC11_BLUE);
tft_ec11_clear();
```

#### `void tft_ec11_set_text_style(uint16_t foreground, uint8_t scale)`

Sets the global foreground color and monospace font scale (`1`-`3`) used by
`tft_ec11_draw_text_simple()` and `tft_ec11_draw_text()`.

```c
tft_ec11_set_text_style(TFT_EC11_YELLOW, 2);
```

#### `tft_ec11_text_style_t tft_ec11_get_text_style(void)`

Returns the current global style (`background`, `foreground`, `font_scale`).

```c
tft_ec11_text_style_t style = tft_ec11_get_text_style();
printf("bg=%04X fg=%04X scale=%u\n", style.background, style.foreground, style.font_scale);
```

### Drawing

#### `esp_err_t tft_ec11_clear(void)`

Fills the entire screen with the current global background color.

```c
tft_ec11_set_background(TFT_EC11_BLACK);
tft_ec11_clear();
```

#### `esp_err_t tft_ec11_refresh(const uint16_t *frame_rgb565)`

Pushes a full-screen framebuffer (`width * height` RGB565 pixels, row-major) to the panel in
one call. Useful for pre-rendered or animated content.

```c
uint16_t *fb = malloc((size_t)tft_ec11_width() * tft_ec11_height() * sizeof(uint16_t));
for (int i = 0; i < tft_ec11_width() * tft_ec11_height(); i++)
{
    fb[i] = TFT_EC11_GREEN;
}
tft_ec11_refresh(fb);
free(fb);
```

#### `esp_err_t tft_ec11_fill_rect(int x, int y, int width, int height, uint16_t color)`

Draws a solid rectangle, clipped to the screen bounds.

```c
tft_ec11_fill_rect(10, 10, 40, 40, TFT_EC11_RED);
```

#### `esp_err_t tft_ec11_draw_text_simple(int x, int y, const char *text)`

Draws `text` at `(x, y)` using the current global style. The exact pixel area the text
occupies is erased with the background color first, so redrawing shorter text at the same
position never leaves stale characters behind.

```c
tft_ec11_set_text_style(TFT_EC11_WHITE, 1);
tft_ec11_draw_text_simple(0, 0, "RPM: 12345");
// later, overwrites the old text cleanly
tft_ec11_draw_text_simple(0, 0, "RPM: 800");
```

#### `esp_err_t tft_ec11_draw_text_styled(int x, int y, const char *text, uint16_t background, uint8_t font_scale, uint16_t foreground)`

Draws `text` with an explicit background, font scale, and foreground color for this single
call, without changing the global style.

```c
tft_ec11_draw_text_styled(5, 30, "FONT CYAN SIZE 2", TFT_EC11_BLACK, 2, TFT_EC11_CYAN);
```

#### `esp_err_t tft_ec11_draw_text(int x, int y, size_t slot_chars, const char *text)`

Draws `text` using the global style inside a fixed-width slot of `slot_chars` characters.
The whole slot is cleared with the background color first, so it is safe for values whose
length changes between updates, and text is clipped rather than wrapped past the slot.

```c
tft_ec11_set_text_style(TFT_EC11_WHITE, 2);
tft_ec11_draw_text(10, 80, 12, "123456789");
tft_ec11_draw_text(10, 80, 12, "99"); // clears the whole 12-character slot first
```

#### `esp_err_t tft_ec11_draw_text_font(int x, int y, const char *text, tft_ec11_font_t font, uint16_t background, uint16_t foreground)`

Draws `text` in one of five **native bitmap faces**. Each face stores its own glyph bitmaps and is
drawn pixel-accurate at its own size; glyphs are never scaled, blended, or interpolated. The cleared
area is exactly (glyph count × advance width) × cell height, and the advance includes the normal
1 px inter-character gap. Text is UTF-8; accented Latin-1 letters fold onto their base glyph, and
bytes without a dedicated glyph render as a hollow box (same rules as `tft_ec11_draw_text_styled()`).

| Face (preferred) | Legacy alias | Glyph size | Cell (glyph + 1 px gap) | Notes |
|------|------|------------|--------------------------|-------|
| `TFT_EC11_FONT_6x8` | `TFT_EC11_FONT_SMALL` | 5x7 | 6x8 | the built-in 5x7 font, identical to the legacy API at scale 1; lowercase folds to uppercase |
| `TFT_EC11_FONT_8x12` | `TFT_EC11_FONT_MEDIUM` | 7x12 | 8x12 | hand-tuned intermediate face with real descenders (g j p q y) and comma/quotes below the baseline |
| `TFT_EC11_FONT_9x16` | `TFT_EC11_FONT_LARGE` | 8x16 | 9x16 | classic public-domain IBM VGA 8x16 ASCII font with true descenders and full punctuation |
| `TFT_EC11_FONT_13x24` | `TFT_EC11_FONT_XLARGE` | 12x24 | 13x24 | Terminus 12x24 bitmap face (SIL OFL 1.1), generated table with true descenders and full punctuation |
| `TFT_EC11_FONT_17x32` | `TFT_EC11_FONT_XXLARGE` | 16x32 | 17x32 | Terminus 16x32 bitmap face (SIL OFL 1.1), generated table with true descenders and full punctuation |

All five faces cover printable ASCII 32..126. The 13x24 and 17x32 tables are generated offline with
`tools/font_gen/generate_fonts.py` (see "Font sizes" above). The legacy `font_scale` API
(`tft_ec11_set_text_style`, `tft_ec11_draw_text_styled`) keeps working unchanged and only ever
scales the 5x7 face; use `tft_ec11_draw_text_font()` to get intermediate sizes without stretching.

```c
// 6x8 face: 5x7 glyphs in a 6x8 cell
tft_ec11_draw_text_font(10, 10, "Temp 21.5 C", TFT_EC11_FONT_6x8, TFT_EC11_BLACK, TFT_EC11_WHITE);

// 8x12 face: 7x12 glyphs in an 8x12 cell
tft_ec11_draw_text_font(10, 30, "Temp 21.5 C", TFT_EC11_FONT_8x12, TFT_EC11_BLACK, TFT_EC11_WHITE);

// 9x16 face: 8x16 glyphs in a 9x16 cell
tft_ec11_draw_text_font(10, 60, "Temp 21.5 C", TFT_EC11_FONT_9x16, TFT_EC11_BLACK, TFT_EC11_WHITE);

// 13x24 face: 12x24 glyphs in a 13x24 cell
tft_ec11_draw_text_font(10, 90, "Temp 21.5 C", TFT_EC11_FONT_13x24, TFT_EC11_BLACK, TFT_EC11_WHITE);

// 17x32 face: 16x32 glyphs in a 17x32 cell
tft_ec11_draw_text_font(10, 120, "Temp 21.5 C", TFT_EC11_FONT_17x32, TFT_EC11_BLACK, TFT_EC11_WHITE);
```

### Input events

#### `esp_err_t tft_ec11_inputs_start(size_t queue_length)`

Starts the encoder interrupt handler and the button-polling task, and creates the event
queue (`queue_length` entries). Must be called before `tft_ec11_get_event()`.

```c
ESP_ERROR_CHECK(tft_ec11_inputs_start(32));
```

#### `void tft_ec11_inputs_stop(void)`

Stops the button task, removes the encoder ISR handlers, and deletes the event queue.

```c
tft_ec11_inputs_stop();
```

#### `QueueHandle_t tft_ec11_event_queue(void)`

Returns the raw FreeRTOS queue handle, for use with `xQueueReceive()` directly (for example
inside a `select`-style wait on multiple queues).

```c
QueueHandle_t q = tft_ec11_event_queue();
tft_ec11_event_t event;
if (xQueueReceive(q, &event, pdMS_TO_TICKS(100)) == pdTRUE)
{
    // handle event
}
```

#### `bool tft_ec11_get_event(tft_ec11_event_t *event, TickType_t wait)`

Convenience wrapper that waits up to `wait` ticks for the next event and returns `true` if
one was received. Use `portMAX_DELAY` to block indefinitely, or `0` to poll.

```c
tft_ec11_event_t event;

if (tft_ec11_get_event(&event, pdMS_TO_TICKS(500)))
{
    if (event.type == TFT_EC11_EVENT_ROTATE)
    {
        printf("rotate %+ld\n", (long)event.steps);
    }
    else if (event.press == TFT_EC11_PRESS_DOWN)
    {
        printf("button pressed\n");
    }
    else if (event.press == TFT_EC11_PRESS_LONG)
    {
        printf("button held LONG (duration=%lums)\n", (unsigned long)event.duration_ms);
    }
}
```

`tft_ec11_event_t` fields:

| Field         | Meaning                                                                 |
|---------------|--------------------------------------------------------------------------|
| `type`        | `TFT_EC11_EVENT_ROTATE`, `TFT_EC11_EVENT_ENCODER_BUTTON`, or `TFT_EC11_EVENT_AUX_BUTTON` |
| `steps`       | Signed rotation step count (`TFT_EC11_EVENT_ROTATE` only)                |
| `press`       | `TFT_EC11_PRESS_DOWN`, `_SHORT`, `_MEDIUM`, or `_LONG` (button events only) |
| `duration_ms` | Elapsed press time in ms; `0` for `TFT_EC11_PRESS_DOWN`                  |

`TFT_EC11_PRESS_DOWN` is reported immediately when a button is pressed. `TFT_EC11_PRESS_LONG`
is reported as soon as the long-press threshold elapses, without waiting for release; `_SHORT`
and `_MEDIUM` are reported on release, based on how long the button was held.

## SD card diagnostic submenu

The interactive test program's `M` command opens a numbered SD card submenu
(`examples/basic/testProgram/main/sd_card_test.c`). This is a test-program-only diagnostic; it is
not part of the `esp32_s3_piggyback` component's public API, which deliberately excludes SD card
support (see `projectPrompt.md`). It uses its own SPI bus (`SPI3_HOST`) so it does not conflict
with the TFT panel's `SPI2_HOST`:

```text
SD_DTCT  GPIO 7
SD_CS    GPIO 13
SD_SCLK  GPIO 14
SD_MISO  GPIO 18
SD_MOSI  GPIO 21
```

The submenu header reports live card presence (`SDcard detected` / `No SDcard detected`) via
`SD_DTCT`. Options: Initialize SDcard, Format SDcard, Write Testfile, Append Testfile,
Read TestFile, Return to main menu.

## Other examples

- `examples/displayTest`: phase-1 low-level display diagnostics
- `examples/inputTest`: encoder and both push-button event diagnostics
- `examples/sdcardTest`: standalone SD card diagnostic, structured like the two examples above
  (own `main/` project), independent of the `esp32_s3_piggyback` component. Same menu and pins as
  the testProgram SD card diagnostic submenu above.
- `examples/basic/testProgram`: complete menu covering every public component operation, plus the
  SD card diagnostic submenu above

See [projectPrompt.md](projectPrompt.md), [TESTING.md](TESTING.md), and the public API in
`components/esp32_s3_piggyback/include/esp32_s3_piggyback.h`.
