# esp32-s3-Piggyback - projectPrompt

## Goal

This is a reference for building this reusable ESP-IDF component from scratch for the hardware
defined by:

	●	https://github.com/mrWheel/TFT_LCD_Display_EC11_ESP32S3_Board

The component provides a clean, hardware-specific abstraction for:

	●	the 2.4 inch SPI TFT LCD display with ST7789 controller,
	●	the EC11 rotary encoder,
	●	the EC11 push button,
	●	the additional push button on the board.

The component is specific to this board/display combination. It is not a generic display
framework for arbitrary TFT panels.

The purpose is to encapsulate all difficult, board-specific display and input handling in one
reusable ESP-IDF component so applications no longer need to know about ST7789 initialization
details, panel inversion quirks, SPI transfers, quadrature decoding, button debouncing, or
press-duration classification.

The component must remain suitable for publication in the Espressif Component Registry.

Development environment:

	●	VSCode
	●	ESP-IDF
	●	ESP32-S3

All source-code comments, documentation, API documentation, menuconfig descriptions, log
messages, and example documentation must be written in English.

- Use Allman brace style.
- Use 2 space indents.
- Use "//-- " for comments (2 slashes, 2 dashes).
- Never use "/* .. */" for comments.
- Place comments above the code they describe.
- Use native ESP-IDF APIs only; no Arduino framework, no Adafruit_GFX/Adafruit_ST7789, no
  PlatformIO.



## Hardware reference

	●	https://github.com/mrWheel/TFT_LCD_Display_EC11_ESP32S3_Board

This repository defines the physical ESP32-S3 board and hardware connections. The proven
Groovebox (https://github.com/mrWheel/TheGrooveboxProject) and Ultimate Timer
(https://github.com/mrWheel/ultimateTimer) projects contain prior working implementations for
the same display and board generation and document hard-earned hardware knowledge (ST7789
initialization, inverted panel behavior, quadrature handling, press classification). Consult
them before changing low-level display or input behavior; do not replace proven hardware-specific
behavior with a theoretically "standard" ST7789 implementation without verifying it on the
actual board.



## Critical hardware fact: inverted TFT panel

The TFT panel used on this board behaves as an inverted ST7789 panel. This is a hardware
characteristic, not an optional theme setting.

The component must normalize this internally so application code can use normal RGB565 color
values: requesting black displays black, white displays white, red/green/blue display correctly.
Any required inversion, byte swapping, RGB/BGR manipulation, or panel command is an implementation
detail hidden inside the component. Application code must never need to "fix" colors itself.

Relevant hardware-level concerns that the component's low-level driver must get right:

	●	ST7789 SWRESET / SLPOUT / COLMOD / MADCTL / INVON-INVOFF / DISPON sequence;
	●	RGB/BGR ordering and RGB565 byte order;
	●	rotation-dependent MADCTL values;
	●	CASET / RASET / RAMWR addressing for arbitrary rectangles;
	●	no mirrored coordinate system and no one-pixel offset errors at any rotation.



## Board GPIO reference (defaults)

These are the default Kconfig pin assignments for the physical board, defined in
`components/esp32_s3_piggyback/Kconfig`. They must not be hardcoded/scattered through the source;
every pin below is a `CONFIG_TFT_EC11_PIN_*` (or similarly named) menuconfig option so it can be
changed when required.

### TFT LCD

```text
Backlight   GPIO 2   (CONFIG_TFT_EC11_PIN_BL)
Reset       GPIO 4   (CONFIG_TFT_EC11_PIN_RST)
Chip Select GPIO 5   (CONFIG_TFT_EC11_PIN_CS)
SPI SCLK    GPIO 12  (CONFIG_TFT_EC11_PIN_SCLK)
SPI MOSI    GPIO 11  (CONFIG_TFT_EC11_PIN_MOSI)
D/C         GPIO 15  (CONFIG_TFT_EC11_PIN_DC)
```

The TFT does not require MISO. Uses `SPI2_HOST`.

### EC11 rotary encoder

```text
A           GPIO 16  (CONFIG_TFT_EC11_PIN_ENC_A)
B           GPIO 17  (CONFIG_TFT_EC11_PIN_ENC_B)
Push button GPIO 6   (CONFIG_TFT_EC11_PIN_ENC_BUTTON)
```

### Additional push button

```text
KEY0        GPIO 1   (CONFIG_TFT_EC11_PIN_AUX_BUTTON)
```

### Display dimensions

```text
320 x 240 pixels
```

See "Kconfig options" below for the complete list, including SPI clock, default rotation,
inversion/BGR defaults, and input timing thresholds.



## Component name and API prefix

Component name: `esp32_s3_piggyback`. Public API prefix: `tft_ec11_`, used consistently across
all public functions, types, and constants (declared in
`components/esp32_s3_piggyback/include/esp32_s3_piggyback.h`).

The component is a plain ESP-IDF C API using a singleton hardware instance; there is no opaque
handle and no C++ class, since there is exactly one display/input instance per board.



## Project structure (as built)

```text
esp32-s3-Piggyback/
├── CMakeLists.txt
├── projectPrompt.md
├── README.md
├── main/                            (builds the same sources as examples/basic/testProgram,
│                                      so opening the repo root in VS Code is immediately useful)
├── components/
│   └── esp32_s3_piggyback/
│       ├── CMakeLists.txt
│       ├── Kconfig
│       ├── idf_component.yml
│       ├── include/
│       │   └── esp32_s3_piggyback.h
│       └── (implementation sources: display/ST7789 driving, built-in font, input handling)
└── examples/
    ├── displayTest/                 low-level display diagnostics
    ├── inputTest/                   encoder and push-button event diagnostics
    ├── sdcardTest/                  standalone SD card diagnostic menu (own main/, no
    │                                 dependency on the esp32_s3_piggyback component)
    └── basic/
        └── testProgram/             full interactive menu covering every public API function,
                                      plus a testProgram-only SD card diagnostic submenu
```

Keep low-level ST7789 code separate from high-level text/drawing primitives, input handling
separate from display handling, and the public header free of private implementation details.



## Public API

All functions are declared in `components/esp32_s3_piggyback/include/esp32_s3_piggyback.h`.

### Lifecycle

`tft_ec11_init(void)` initializes the SPI bus, backlight GPIO, and the ST7789 panel driver,
applies the default rotation/inversion from Kconfig, turns the backlight on, and clears the
screen. Must be called once before any other component function.

`tft_ec11_deinit(void)` stops input handling, turns off the backlight, and releases the panel,
SPI bus, and DMA buffers.

### Display properties

`tft_ec11_set_rotation(uint8_t rotation)` sets rotation `0`-`3` and swaps
`tft_ec11_width()`/`tft_ec11_height()` for the odd rotations. The component applies the correct
MADCTL value internally; application code must never manipulate MADCTL directly.

`tft_ec11_set_inversion(bool enabled)` toggles ST7789 color inversion.

`tft_ec11_set_backlight(bool enabled)` switches the backlight GPIO.

`tft_ec11_width(void)` / `tft_ec11_height(void)` return the current logical display size,
already accounting for the active rotation.

### Text style

`tft_ec11_set_background(uint16_t color)` sets the global background color (RGB565), used by
`tft_ec11_clear()` and by the global-style text functions. Changing it does not repaint the
screen; a full clear/refresh is a separate explicit operation.

`tft_ec11_set_text_style(uint16_t foreground, uint8_t scale)` sets the global foreground color
and monospace font scale (`1`-`3`).

`tft_ec11_get_text_style(void)` returns the current global style
(`background`, `foreground`, `font_scale`) as `tft_ec11_text_style_t`.

### Drawing

`tft_ec11_clear(void)` fills the entire screen with the current global background color. This
is the only operation allowed to repaint the full screen; normal text/drawing operations must
never do so implicitly.

`tft_ec11_refresh(const uint16_t *frame_rgb565)` pushes a full-screen framebuffer
(`width * height` RGB565 pixels, row-major) in one call.

`tft_ec11_fill_rect(int x, int y, int width, int height, uint16_t color)` draws a solid
rectangle, clipped to the screen bounds.

`tft_ec11_draw_text_simple(int x, int y, const char *text)` draws `text` at `(x, y)` using the
current global style. The exact pixel area the text occupies is erased with the background color
first, so redrawing shorter text at the same position leaves no stale characters.

`tft_ec11_draw_text_styled(int x, int y, const char *text, uint16_t background,
uint8_t font_scale, uint16_t foreground)` draws with an explicit one-off style, without touching
the global style.

`tft_ec11_draw_text(int x, int y, size_t slot_chars, const char *text)` draws using the global
style inside a fixed-width slot of `slot_chars` characters. The whole slot is cleared with the
background color first, so it is safe for values whose length changes between updates.

Invariants that apply to every drawing function:

	●	text wrapping never occurs; text that does not fit is clipped at the right boundary,
		never wrapped or scrolled;
	●	replacing text never redraws more than the bounding rectangle of the text being replaced,
		and never triggers a full-screen clear;
	●	all drawing is clipped to the screen (or slot) boundary and never writes outside the
		visible display RAM area.

### Input events

`tft_ec11_inputs_start(size_t queue_length)` starts the encoder ISR and the button-polling task
and creates the event queue. Must be called before `tft_ec11_get_event()`.

`tft_ec11_inputs_stop(void)` stops the button task, removes the encoder ISR handlers, and
deletes the event queue.

`tft_ec11_event_queue(void)` returns the raw FreeRTOS queue handle for direct
`xQueueReceive()` use (e.g. inside a `select`-style wait on multiple queues).

`tft_ec11_get_event(tft_ec11_event_t *event, TickType_t wait)` waits up to `wait` ticks for the
next event; `portMAX_DELAY` blocks indefinitely, `0` polls.

`tft_ec11_event_t` fields:

| Field         | Meaning                                                                    |
|---------------|------------------------------------------------------------------------------|
| `type`        | `TFT_EC11_EVENT_ROTATE`, `TFT_EC11_EVENT_ENCODER_BUTTON`, or `TFT_EC11_EVENT_AUX_BUTTON` |
| `steps`       | Signed rotation step count (`TFT_EC11_EVENT_ROTATE` only); positive = clockwise |
| `press`       | `TFT_EC11_PRESS_DOWN`, `_SHORT`, `_MEDIUM`, or `_LONG` (button events only)  |
| `duration_ms` | Elapsed press time in ms; `0` for `TFT_EC11_PRESS_DOWN`                      |

Quadrature decoding must not lose steps at normal human rotation speed and must accumulate
signed steps when the consumer reads slower than the encoder turns.

`TFT_EC11_PRESS_DOWN` is reported immediately on press. `_LONG` is reported as soon as the long
threshold elapses, without waiting for release, and a second event must not be emitted on
release afterward. `_SHORT`/`_MEDIUM` are reported on release, classified by held duration.
Both the encoder button and the auxiliary button use the same classification model with
independently configurable thresholds and use proper debounce handling.



## Built-in font

Documented here as the current source of truth for character/text behavior (implemented in
`esp32_s3_piggyback_font.c` and `esp32_s3_piggyback.c`).

Character coverage:

	●	fixed 5x7 pixel glyphs, monospace, scaled via the integer `font_scale` text style value;
	●	digits 0-9, letters A-Z (lowercase a-z folded to uppercase before lookup), space;
	●	punctuation: `! " # $ % & ' ( ) * + , - . / : ; < = > ? @ [ \ ] ^ _ ` { | } ~`;
	●	any other byte falls back to a generic placeholder glyph (a filled box outline) instead
		of failing.

Accented letters:

	●	accented Latin-1 letters (e.g. À Á Â Ã Ä Å, È É Ê Ë, Ì Í Î Ï, Ñ, Ò Ó Ô Õ Ö Ø, Ù Ú Û Ü, Ý
		and their lowercase equivalents) are folded onto their unaccented base letter (e.g.
		á -> A) instead of being drawn with a dedicated accent glyph;
	●	letters without a defined base-letter mapping (e.g. æ, ß, ð, þ) are not folded and render
		as the generic placeholder glyph.

Text encoding:

	●	text passed to the drawing API (`tft_ec11_draw_text*`) is UTF-8;
	●	accented Latin-1 letters are encoded in UTF-8 as a 2-byte sequence (lead byte
		`0xC2`/`0xC3`); the drawing code decodes such sequences into a single Latin-1 code point
		before the font lookup runs;
	●	other multi-byte UTF-8 sequences (3- and 4-byte) are not decoded and render as the
		generic placeholder glyph, one glyph per lead byte;
	●	the number of glyphs actually rendered (not the raw UTF-8 byte length) determines the
		width of the region cleared/redrawn, so accented text does not clear more background
		than necessary.



## Kconfig options

Defined in `components/esp32_s3_piggyback/Kconfig`, menu "TFT LCD Display EC11":

```text
TFT_EC11_PIN_CS               TFT CS GPIO                        default 5
TFT_EC11_PIN_DC                TFT DC GPIO                        default 15
TFT_EC11_PIN_RST               TFT reset GPIO                     default 4
TFT_EC11_PIN_BL                TFT backlight GPIO                 default 2
TFT_EC11_PIN_SCLK              TFT SCLK GPIO                       default 12
TFT_EC11_PIN_MOSI              TFT MOSI GPIO                       default 11
TFT_EC11_PIN_ENC_A             Encoder A GPIO                      default 16
TFT_EC11_PIN_ENC_B             Encoder B GPIO                      default 17
TFT_EC11_PIN_ENC_BUTTON        Encoder push GPIO                   default 6
TFT_EC11_PIN_AUX_BUTTON        Auxiliary push GPIO                 default 1
TFT_EC11_SPI_CLOCK_HZ          Display SPI clock (Hz)              default 40000000  (1e6-80e6)
TFT_EC11_DEFAULT_ROTATION      Default rotation (0-3)              default 3
TFT_EC11_DEFAULT_INVERT        Enable ST7789 panel inversion       default n
TFT_EC11_BGR                   Use BGR color order                 default n
TFT_EC11_ENCODER_TRANSITIONS   Quadrature transitions per detent    default 4  (1-4)
TFT_EC11_DEBOUNCE_MS           Button debounce (ms)                default 30  (5-200)
TFT_EC11_MEDIUM_MS             Medium press boundary (ms)          default 450  (100-3000)
TFT_EC11_LONG_MS               Long press boundary (ms)            default 900  (200-10000)
```

Do not expose fixed ST7789 protocol constants (opcodes, mandatory init sequence values) as
menuconfig options; only board-specific and tunable values belong here.



## Logging and error handling

Use ESP-IDF logging (`ESP_LOGE`/`ESP_LOGW`/`ESP_LOGI`/`ESP_LOGD`). Normal operation must not spam
the console: `ERROR` for hardware/API failure, `WARN` for unexpected encoder state or queue
overflow, `INFO` for initialization summary, `DEBUG` for ST7789 init and input diagnostics.

Public functions that can fail return `esp_err_t` and must check underlying ESP-IDF return
values (SPI init/device registration, GPIO configuration, queue/mutex creation). Drawing that
falls partially outside the screen is clipped rather than treated as an error. SPI display
operations are protected by a mutex so multiple application tasks cannot corrupt one another's
transfers.



## Thread safety

Public display APIs share a mutex around SPI transfers, so calls from multiple tasks are safe
with respect to each other. Input events are delivered through a FreeRTOS queue, which is safe
for one or more consumers.



## Examples

### displayTest

Low-level display diagnostics, independent of the reusable component's text/input layer. Proves
ST7789 initialization, inversion/color correctness, rotation/geometry, and partial-rectangle
writes directly against the SPI driver. Remains in the repository as the primary hardware
diagnostic example.

### inputTest

Prints encoder and button events to UART (e.g. `ENCODER steps=+1`, `ENCODER_BUTTON SHORT`,
`BUTTON LONG`). The display is not required beyond optional visual feedback.

### sdcardTest

Standalone SD card diagnostic, structured like `displayTest`/`inputTest` (own `main/`,
`CMakeLists.txt`, `sdkconfig.defaults`), independent of the `esp32_s3_piggyback` component and
its own project. Presents the same numbered menu as testProgram's `M` submenu (Initialize
SDcard, Format SDcard, Write Testfile, Append Testfile, Read TestFile, Exit), built from its own
copy of `sd_card_test.c`/`sd_card_test.h`. Uses the same board SD card pins/SPI bus documented
under the testProgram SD card diagnostic submenu below.

### basic/testProgram

Demonstrates every public component function through an interactive USB Serial/JTAG menu.
Commands are single letters A-Z, case-insensitive, executed only after Enter; the prompt and
every typed character are echoed immediately and flushed so the prompt stays visible while
waiting for input. An empty line redraws the menu instead of exiting.

Menu highlights: `A`-`D` draw one exact corner each; `E`/`F` draw eight numbered/color-named
stripes; `H` cycles rotation (default `3`) and reports the new width/height; `I` cycles the
background color; `J` opens a numeric text-settings submenu (background, font size, font color,
x/y position, enter text, show settings) that echoes each choice and only acts after Enter;
`P`/`Q` toggle raw panel inversion for diagnosing incorrect panel behavior (expected production
setting: `Q` / INV ON); `U`/`V`/`W`/`X` listen for rotate/encoder-button/aux-button/all events for
a fixed duration; `Z` runs a combined self-test. The menu demonstrates that changing one field
never redraws the entire screen.

#### SD card diagnostic submenu (testProgram-only)

Command `M` opens a numbered SD card submenu (`sd_card_test.c`/`sd_card_test.h`). This is a
deliberate, explicitly requested exception scoped to the test program only: SD card handling
remains a non-goal of the reusable `esp32_s3_piggyback` component and is not exposed through its
public header.

Board SD card pins (SPI mode), as provided for this board revision:

```text
SD_DTCT  GPIO 7
SD_CS    GPIO 13
SD_SCLK  GPIO 14
SD_MISO  GPIO 18
SD_MOSI  GPIO 21
```

The SD card uses its own SPI bus (`SPI3_HOST`); the TFT panel keeps `SPI2_HOST`. Card presence is
read directly from `SD_DTCT` (active-low, internal pull-up) and shown in the submenu header
(`SDcard detected` / `No SDcard detected`), independently of whether the card has been mounted
yet. Submenu operations: Initialize SDcard, Format SDcard, Write Testfile, Append Testfile, Read
TestFile, Return to main menu.

Because the repository root also builds the same test-program sources through its own
`main/CMakeLists.txt` (for the VSCode ESP-IDF extension), any future change to this submenu's
source files or component requirements must be mirrored into both
`examples/basic/testProgram/main/CMakeLists.txt` and the root `main/CMakeLists.txt`.



## Test strategy

Where hardware-independent logic exists, use unit tests where practical: RGB888-to-RGB565
conversion, clipping calculations, text bounding boxes, text-slot replacement calculations,
press-duration classification, quadrature transition table, encoder detent accumulation.
Hardware-dependent behavior must be tested with the actual board.



## Documentation

Keep `README.md` and this file in sync with the actual implementation — the code is the single
source of truth. Document: supported board revision, display controller, resolution, default
GPIO map, inversion behavior, RGB565 color model, initialization, rotation, text API, no-wrap
behavior, partial updates, event API, press thresholds, menuconfig options, examples, and known
limitations. Explain why the panel requires inverted-display handling and that this is
deliberately normalized inside the component, so users of the component never rediscover the
issue themselves.



## Component Registry readiness

Keep the component structured for later publication through the Espressif Component Registry:
a correct `idf_component.yml`, a clean and stable public header, and no exposed private ESP-IDF
driver objects or internal ST7789 state. Do not publish anything automatically.



## Coding style

Use clean, maintainable C: descriptive names, no magic GPIO numbers in source, no unexplained
ST7789 magic values (document every initialization command), small focused functions, avoid
unnecessary global mutable state, always check ESP-IDF return values, no Arduino APIs, no
application-specific UI logic in the component itself.



## Non-goals

The component is not intended to provide:

	●	LVGL integration;
	●	touch screen support;
	●	arbitrary display controller support;
	●	arbitrary board support;
	●	animation framework;
	●	menu system;
	●	Groovebox-specific or Ultimate-Timer-specific UI;
	●	image decoder / JPEG/PNG rendering;
	●	SD card handling (see the testProgram-only exception under Examples above);
	●	Wi-Fi handling;
	●	audio handling.

Keep the component focused.
