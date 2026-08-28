# esp32-s3-Piggyback - projectPrompt

## Goal

Develop a production-quality reusable ESP-IDF component for the hardware defined by:

	●	https://github.com/mrWheel/TFT_LCD_Display_EC11_ESP32S3_Board

The component shall provide a clean, hardware-specific abstraction for:

	●	the 2.4 inch SPI TFT LCD display with ST7789 controller,
	●	the EC11 rotary encoder,
	●	the EC11 push button,
	●	the additional push button on the board.

The component is intended specifically for this board/display combination. It is not required to become a generic display framework for arbitrary TFT panels.

The main purpose is to encapsulate all difficult, board-specific display and input handling in one reusable ESP-IDF component so applications no longer need to know about ST7789 initialization details, panel inversion quirks, SPI transfers, quadrature decoding, button debouncing, or press-duration classification.

The component must be suitable for later publication in the Espressif Component Registry.

Development environment:

	●	VSCode
	●	ESP-IDF
	●	ESP32-S3

All source-code comments, documentation, API documentation, menuconfig descriptions, log messages, and example documentation must be written in English.

- Use Allman style coding
- Use 2 space indents
- Use “//— “ for comments (2 slashes, 2 dashes)
- Never use “/* .. */“ for comments
- Set commands above the code it is referring to



## Existing reference implementations

Before implementing the component, study these existing projects carefully:

## Hardware

https://github.com/mrWheel/TFT_LCD_Display_EC11_ESP32S3_Board

This repository defines the physical ESP32-S3 board and hardware connections.

## Groovebox reference

https://github.com/mrWheel/TheGrooveboxProject

This project contains a proven working implementation for the same display and board generation.

Important items to investigate include:

	●	ST7789 initialization
	●	SPI configuration
	●	display reset sequence
	●	display inversion
	●	display rotation
	●	display dimensions
	●	text drawing
	●	background handling
	●	partial screen updates
	●	color handling
	●	EC11 GPIO configuration
	●	EC11 quadrature decoding
	●	encoder push button behavior
	●	additional push button behavior

Do not blindly rewrite this code.

First determine exactly which parts are essential hardware behavior and which parts are application-specific Groovebox behavior.

Only reusable hardware behavior belongs in this component.

## Ultimate Timer reference

https://github.com/mrWheel/ultimateTimer

Pay special attention to:

	●	DisplayDriverClass.md
	●	InputClass.md
	●	the actual display implementation
	●	the actual input implementation

The Ultimate Timer project already documents important behavior of this hardware:

	●	ST7789 controller
	●	320 x 240 visible display
	●	SPI interface
	●	inverted panel behavior
	●	display rotation support
	●	disabled text wrapping
	●	EC11 quadrature handling
	●	short / medium / long push events

Use this as reference material, but design the new ESP-IDF API independently.

The final implementation must use native ESP-IDF APIs and must not depend on the Arduino framework, Adafruit_GFX, Adafruit_ST7789, or PlatformIO.



## Critical hardware fact: inverted TFT panel

The TFT panel used on this board behaves as an inverted ST7789 panel.

This is not an optional theme setting.

It is a hardware characteristic that has already caused significant development effort in the existing projects.

The implementation must therefore explicitly test and document:

	●	ST7789 INVON
	●	ST7789 INVOFF
	●	RGB/BGR ordering
	●	RGB565 color transmission
	●	MADCTL configuration
	●	rotation-dependent MADCTL values
	●	the relationship between requested RGB565 colors and physically displayed colors

Do not assume that a conventional ST7789 initialization sequence produces correct colors.

Do not “fix” incorrect colors in application code.

The component must normalize this behavior internally so that application code can use normal RGB565 color values.

For example, when the application requests:

	●	black, the display API should display black;
	●	white, the display API should display white;
	●	red, the display API should display red;
	●	green, the display API should display green;
	●	blue, the display API should display blue.

Any required inversion, byte swapping, RGB/BGR manipulation, or panel command must remain an implementation detail inside the component.



## Current board GPIO reference

The currently proven R3 firmware uses these GPIO assignments for this board:

### TFT LCD

```text
Backlight   GPIO 2
Reset       GPIO 4
Chip Select GPIO 5
SPI SCLK    GPIO 12
SPI MOSI    GPIO 11
D/C         GPIO 15
```

The TFT does not require MISO.

### EC11 rotary encoder

```text
Push button GPIO 6
A           GPIO 16
B           GPIO 17
```

Additional push button

```text
KEY0        GPIO 1
```

### Display dimensions

```text
320 x 240 pixels
```

These values shall be the default component configuration.

They must not be scattered through the source code.

They must be defined through Kconfig/menuconfig so they can be changed when required.

The defaults must match the physical board.



## Component name

Use this working component name:

```text
esp32-s3-piggyback
```

The public API should preferably use a shorter neutral prefix such as:

```text
board_ui_
```

or:

```text
tft_ec11_
```

Choose one prefix and use it consistently.

Avoid C++ classes unless there is a strong technical reason.

Prefer an idiomatic ESP-IDF C API with opaque handles or a singleton hardware instance where appropriate.



## Proposed project structure

Create a normal ESP-IDF project containing the reusable component and examples.

Suggested structure:

```text
tft_lcd_display_ec11/
├── CMakeLists.txt
├── sdkconfig.defaults
├── projectPrompt.md
├── README.md
├── components/
│   └── tft_lcd_display_ec11/
│       ├── CMakeLists.txt
│       ├── Kconfig
│       ├── idf_component.yml
│       ├── include/
│       │   └── tft_lcd_display_ec11.h
│       ├── private_include/
│       │   ├── tft_lcd_display_ec11_internal.h
│       │   ├── st7789_lowlevel.h
│       │   └── input_lowlevel.h
│       ├── src/
│       │   ├── tft_lcd_display_ec11.c
│       │   ├── st7789_lowlevel.c
│       │   ├── display_text.c
│       │   └── input_lowlevel.c
│       └── README.md
└── examples/
    ├── displayTest/
    │   ├── CMakeLists.txt
    │   ├── sdkconfig.defaults
    │   └── main/
    │       ├── CMakeLists.txt
    │       └── display_test.c
    ├── inputTest/
    │   └── ...
    └── basic/
        └── ...
```

The exact internal file split may change if there is a better technical reason, but keep:

	●	low-level ST7789 code separate from high-level text/display primitives;
	●	input handling separate from display handling;
	●	public API separate from private implementation details.



Development strategy

Development must be performed in phases.

Do not implement the complete component before the low-level display behavior is proven.

The display is the highest-risk part of this project.

The phases below are mandatory.



Phase 1 — displayTest

Purpose

Create the smallest possible native ESP-IDF program that proves the physical TFT panel can be controlled correctly.

This phase is intentionally low-level.

Do not yet implement:

	●	reusable text replacement logic,
	●	encoder support,
	●	push button events,
	●	application UI abstractions,
	●	menu systems,
	●	widgets,
	●	themes,
	●	tiles,
	●	lists,
	●	status screens.

The goal is to establish a known-good ST7789 hardware implementation first.

Rules

Phase 1 must use:

	●	native ESP-IDF SPI master driver;
	●	native ESP-IDF GPIO APIs;
	●	explicit ST7789 commands;
	●	no Arduino framework;
	●	no Adafruit libraries;
	●	no third-party graphics library unless absolutely required for comparison testing.

Prefer direct low-level ST7789 access.

The test application should make the command/data stream understandable and traceable.



Phase 1.1 — SPI and hardware reset

Implement only:

	●	SPI bus initialization;
	●	CS;
	●	D/C;
	●	RESET;
	●	backlight;
	●	command transfer;
	●	data transfer.

Provide internal functions comparable to:

```c
esp_err_t st7789_write_command(uint8_t command);
esp_err_t st7789_write_data(const void *data, size_t length);
esp_err_t st7789_hardware_reset(void);
```

Confirm the hardware reset timing from the ST7789 datasheet and/or the proven existing implementations.

Log the initialization sequence at debug level.



Phase 1.2 — minimum ST7789 initialization

Implement a minimal known-good initialization sequence.

Investigate and explicitly document at least:

```text
SWRESET
SLPOUT
COLMOD
MADCTL
INVON / INVOFF
DISPON
```

Also determine whether the panel requires additional porch, gate, VCOM, gamma, or power settings.

Do not copy an initialization table without validating its relevance to this exact display.

Compare behavior with the working Groovebox and Ultimate Timer code.



Phase 1.3 — address window and raw pixel transfer

Implement:

```c
set_address_window(x0, y0, x1, y1)
```

using the ST7789 commands:

```text
CASET
RASET
RAMWR
```

Then implement raw RGB565 transfers.

At this point it must be possible to fill:

	●	one pixel;
	●	a small rectangle;
	●	the entire screen.



Phase 1.4 — mandatory color test

Create a deterministic full-screen color test.

Display these screens one after another:

```text
BLACK
WHITE
RED
GREEN
BLUE
YELLOW
CYAN
MAGENTA
```

Each screen should remain visible long enough for manual inspection.

Also draw a fixed RGB test pattern containing at least:

```text
RED | GREEN | BLUE
WHITE | GRAY | BLACK
CYAN | MAGENTA | YELLOW
```

The serial monitor must state exactly which logical color is currently requested.

Example:

```text
Requested RGB565: RED 0xF800
```

This is specifically intended to detect:

	●	inversion errors;
	●	RGB/BGR errors;
	●	byte-order errors;
	●	RGB565 conversion errors.

The test must make it possible to experimentally determine whether:

```text
INVON
```

or:

```text
INVOFF
```

is correct for this physical panel.

Do not continue to Phase 2 until ordinary application RGB565 values produce physically correct colors.



Phase 1.5 — orientation and geometry test

Test all four ST7789 rotations:

```text
0
1
2
3
```

For every rotation:

	●	clear screen;
	●	draw a border;
	●	draw corner markers;
	●	identify top-left;
	●	identify top-right;
	●	identify bottom-left;
	●	identify bottom-right;
	●	print active width and height over UART.

Use clearly different corner colors or labels.

Verify:

	●	correct MADCTL;
	●	no mirrored coordinate system;
	●	correct X/Y orientation;
	●	correct width/height;
	●	correct address offsets;
	●	no one-pixel or hidden-area offset problems.

If this physical board should only expose landscape orientations to applications, still test all four rotations at low level.



Phase 1.6 — primitive drawing test

Implement and test low-level primitives:

```text
drawPixel()
drawHorizontalLine()
drawVerticalLine()
fillRect()
fillScreen()
```

These functions must use bounded / clipped coordinates and must never write outside the physical display RAM area intended for the visible panel.



Phase 1.7 — performance test

Measure approximately:

	●	complete 320 x 240 fill time;
	●	small rectangle update time;
	●	representative text-area update time.

The goal is not premature optimization.

The goal is to select a safe SPI clock and confirm that partial updates are fast enough for UI use.

The test should make it easy to try multiple SPI clock frequencies through menuconfig.

Choose reliability over maximum SPI clock.

Document the final tested value.



Phase 1 acceptance criteria

Phase 1 is complete only when all of the following are true:

	1.	The panel initializes reliably after cold boot and reset.
	2.	Backlight control works.
	3.	Black is physically black.
	4.	White is physically white.
	5.	Red, green, and blue are physically correct.
	6.	RGB565 byte order is confirmed.
	7.	Correct inversion mode is confirmed.
	8.	Rotation works.
	9.	Addressing is not mirrored.
	10.	Full 320 x 240 geometry is usable.
	11.	Partial rectangles can be updated without refreshing the full display.
	12.	Repeated resets do not leave the TFT in an invalid state.
	13.	The exact working ST7789 initialization is documented.

Only after these criteria are met may the reusable display abstraction be implemented.



Phase 2 — reusable display API

Build the reusable component API on top of the verified Phase 1 low-level driver.

The application should not need to know ST7789 commands.



Display initialization

Provide a public initialization function.

Example direction:

```c
esp_err_t board_ui_init(const board_ui_config_t *config);
```

or:

```c
esp_err_t board_ui_init(void);
```

when all configuration is supplied by menuconfig.

Initialization must configure:

	●	SPI;
	●	display;
	●	inversion;
	●	initial rotation;
	●	backlight;
	●	input GPIOs when input support is enabled.

Initialization should leave the display in a deterministic state.



Background color

The component shall maintain a logical current background color.

Provide:

```c
esp_err_t board_ui_set_background_color(uint16_t rgb565);
uint16_t board_ui_get_background_color(void);
```

Changing the configured background color must not automatically repaint the complete screen.

A separate explicit full-screen refresh / clear API shall be used for that.



Full screen clear / refresh

Provide an explicit operation such as:

```c
esp_err_t board_ui_clear(void);
```

which fills the entire screen with the configured background color.

Also provide:

```c
esp_err_t board_ui_fill_screen(uint16_t rgb565);
```

if useful.

A full-screen redraw must only happen when explicitly requested.

Normal text writing operations must never clear or refresh the complete display.

This is a strict project requirement.



Screen rotation

Provide an API such as:

```c
esp_err_t board_ui_set_rotation(board_ui_rotation_t rotation);
board_ui_rotation_t board_ui_get_rotation(void);
int board_ui_get_width(void);
int board_ui_get_height(void);
```

Rotation values may map to ST7789 rotation values 0..3.

After rotation, width and height must report the logical visible dimensions.

The component must internally apply the correct MADCTL values and offsets.

Applications must not manipulate MADCTL directly.



Text configuration

The component must maintain a current text state consisting of at least:

	●	foreground color;
	●	font;
	●	font size / scale;
	●	background color.

Provide APIs comparable to:

```c
esp_err_t board_ui_set_text_color(uint16_t rgb565);
esp_err_t board_ui_set_text_size(uint8_t size);
```

If multiple fonts are supported:

```c
esp_err_t board_ui_set_font(board_ui_font_t font);
```

Do not create an unnecessarily complicated font engine for the first version.

Start with a small built-in bitmap font that is reliable and easy to render.

Additional fonts may be added later.



Text drawing

The main text API must support:

```c
esp_err_t board_ui_write_text(
    int x,
    int y,
    const char *text
);
```

The important behavior is:

When new text is written at a location where text was previously drawn through this API, the old rendered text area must be erased using the current/configured background color before the new text is rendered.

Example:

```text
First:
RPM: 12345
Later:
RPM: 800
```

The remaining characters 45 must not remain visible.

The component must erase the previous bounding rectangle before drawing the replacement text.

Do not solve this by clearing the complete screen.



Text region / text slot model

Because arbitrary (x,y) coordinates are not sufficient to reliably know what old text must be removed, design a simple reusable mechanism.

Preferred approach:

Provide named or handle-based text slots.

Example concept:

```c
board_ui_text_handle_t status_text;
board_ui_create_text(
    &status_text,
    x,
    y,
    width,
    height
);
board_ui_text_write(status_text, "Ready");
board_ui_text_write(status_text, "Playing");
```

The component can then:

	1.	clear only the slot rectangle with its configured background color;
	2.	render the new string;
	3.	never affect other screen areas.

A simpler coordinate-cache approach is acceptable only if it is deterministic and documented.

The API should remain easy to use.

Do not create a full GUI/widget system.



No text wrapping

Text wrapping is forbidden.

This is a strict invariant.

If text reaches the right boundary of:

	●	the display;
	●	or the configured text slot;

it must never continue on the next line.

Choose and document one deterministic overflow behavior.

Preferred options are:

	1.	clip at the right boundary; or
	2.	truncate characters that do not fit.

Do not wrap.

Do not scroll automatically.

Do not alter the Y coordinate automatically.



Text clipping

Drawing text must never overwrite unrelated UI regions outside its allowed area.

When using a text slot, clip all rendering to the slot.

When using raw coordinate text drawing, clip to screen boundaries.



Partial update requirement

Normal operations must update the smallest reasonable display rectangle.

Examples:

	●	changing one text field must only erase/redraw that field;
	●	changing one number must not redraw the complete screen;
	●	changing text color must not repaint existing text;
	●	changing background configuration must not repaint existing screen content.

The low-level SPI driver should expose efficient rectangular pixel writes.

Avoid maintaining a full 320 x 240 framebuffer unless measurements prove that it provides a real advantage.

A framebuffer is not required for this component.



Basic drawing primitives

Provide at least:

```text
drawPixel
drawLine or horizontal/vertical line
drawRect
fillRect
fillScreen
```

Keep these APIs hardware-level and predictable.

Do not add application-specific buttons, menus, tiles, timers, Groovebox controls, etc.



RGB565 color helpers

Provide convenient helpers/macros for normal RGB values.

For example:

```c
uint16_t board_ui_rgb565(uint8_t red, uint8_t green, uint8_t blue);
```

Provide standard constants for at least:

```text
BLACK
WHITE
RED
GREEN
BLUE
YELLOW
CYAN
MAGENTA
GRAY
```

These constants must represent normal logical colors.

The application must never need to know that the physical panel is inverted.



Phase 3 — EC11 rotary encoder

After display behavior is stable, implement EC11 support.

Use native ESP-IDF GPIO facilities.

The existing projects use quadrature decoding where a full EC11 detent corresponds to four quadrature state transitions.

Preserve correct detent behavior.

Do not report every electrical edge as an application step.



Encoder rotation events

The application must be able to receive the number of steps rotated since the previous event/read.

Use a signed value:

```text
positive = clockwise / right
negative = counter-clockwise / left
```

Example event structure:

```c
typedef struct
{
    board_ui_event_type_t type;
    int32_t steps;
} board_ui_event_t;
```

Possible results:

```text
+1
+3
-1
-5
```

This is preferable to forcing the application to process one LEFT or RIGHT event for every detent.

The implementation must not lose steps during normal human rotation speed.

Consider accumulation when the consumer reads events slower than the EC11 turns.



Encoder direction

Provide configurable logical direction:

```c
board_ui_set_encoder_reversed(bool reversed);
```

or equivalent menuconfig setting.

Physical A/B wiring must not leak into application logic.



Encoder push button

The EC11 push button must generate one of:

```text
SHORT
MEDIUM
LONG
```

Use configurable thresholds.

Current proven defaults are:

```text
short minimum:   50 ms
medium:         700 ms
long:          1500 ms
```

The exact classification rules must be documented.

Recommended semantics:

```text
< short minimum      -> ignored as bounce/noise
short..medium        -> SHORT
medium..long         -> MEDIUM
>= long              -> LONG
```

Prefer generating the final classified event on button release.

If a LONG event is intentionally emitted while the button is still held, document that behavior carefully and guarantee that a second event is not emitted on release.

Use proper debounce handling.



Phase 4 — additional push button

The extra board button shall use the same press classification model:

```text
SHORT
MEDIUM
LONG
```

It shall have independently configurable thresholds.

Current proven defaults:

```text
short minimum:   50 ms
medium:         700 ms
long:          1500 ms
```

The button is active-low on the current hardware unless Phase 4 testing proves otherwise.

The active level should be configurable if this does not complicate the API unnecessarily.



Unified event interface

Prefer one public event model for all controls.

Example:

```c
typedef enum
{
    BOARD_UI_EVENT_NONE = 0,
    BOARD_UI_EVENT_ENCODER_ROTATE,
    BOARD_UI_EVENT_ENCODER_SHORT,
    BOARD_UI_EVENT_ENCODER_MEDIUM,
    BOARD_UI_EVENT_ENCODER_LONG,
    BOARD_UI_EVENT_BUTTON_SHORT,
    BOARD_UI_EVENT_BUTTON_MEDIUM,
    BOARD_UI_EVENT_BUTTON_LONG
} board_ui_event_type_t;
typedef struct
{
    board_ui_event_type_t type;
    int32_t value;
} board_ui_event_t;
```

For:

```text
BOARD_UI_EVENT_ENCODER_ROTATE
```

value contains the signed number of encoder steps.

For push events:

```text
value = 0
```

is sufficient.



Event delivery architecture

Use an ESP-IDF-native mechanism.

Preferred architecture:

```text
GPIO / polling / decoder
        ↓
component input task
        ↓
FreeRTOS queue
        ↓
application
```

Provide an API similar to:

```c
esp_err_t board_ui_get_event(
    board_ui_event_t *event,
    TickType_t timeout
);
```

This allows:

```c
board_ui_get_event(&event, portMAX_DELAY);
```

and avoids application-side polling loops.

Alternatively, provide both:

```text
queue-based blocking read
non-blocking try-read
```

Do not call arbitrary application callbacks directly from GPIO ISR context.

ISR code must remain minimal.



Input task requirements

If a dedicated component task is used:

	●	keep CPU usage negligible when idle;
	●	use deterministic timing;
	●	make stack size configurable;
	●	make task priority configurable;
	●	document which core affinity is used, if any;
	●	do not busy-loop.

The display itself does not require a permanent display task unless measurements show that one is useful.



Thread safety

Document whether public display APIs are thread-safe.

Preferred behavior:

Protect SPI display operations using a mutex so multiple application tasks cannot corrupt one another’s transfers.

Input event delivery through a FreeRTOS queue is naturally safe for one or more consumers only if the consumption semantics are clearly documented.

Do not claim thread safety without implementing it.



Menuconfig / Kconfig

Provide a useful Kconfig.

At minimum make these configurable:

Display pins

```text
TFT backlight GPIO
TFT reset GPIO
TFT CS GPIO
TFT SCLK GPIO
TFT MOSI GPIO
TFT D/C GPIO
```

Default:

```text
2
4
5
12
11
15
```

Display behavior

```text
default rotation
SPI host
SPI clock frequency
SPI queue size if relevant
backlight active level
display inversion default
```

Encoder pins

```text
encoder A GPIO      = 16
encoder B GPIO      = 17
encoder push GPIO   = 6
```

Additional push button

```text
button GPIO         = 1
```

Input timings

```text
encoder short minimum = 50 ms
encoder medium        = 700 ms
encoder long          = 1500 ms
button short minimum  = 50 ms
button medium         = 700 ms
button long           = 1500 ms
```

Do not expose parameters in menuconfig that are fundamental fixed ST7789 protocol constants.



Logging

Use ESP-IDF logging.

Example tags:

```text
board_ui
board_ui_tft
board_ui_input
```

Normal operation must not spam the serial console.

Recommended logging:

```text
ERROR   hardware or API failure
WARN    unexpected encoder state / queue overflow
INFO    initialization summary
DEBUG   ST7789 initialization and input diagnostics
VERBOSE raw SPI or quadrature traces when specifically enabled
```

Provide optional diagnostic logging through menuconfig.



Error handling

Public functions that can fail should return:

```c
esp_err_t
```

Do not silently ignore:

	●	SPI initialization failure;
	●	SPI device registration failure;
	●	GPIO configuration failure;
	●	queue creation failure;
	●	mutex creation failure;
	●	invalid rotation;
	●	invalid text handle;
	●	invalid coordinates where failure is appropriate.

Drawing that falls partially outside the screen may be clipped rather than treated as an error.

Document the behavior.



Memory requirements

The component should remain lightweight.

Do not allocate a permanent full-screen framebuffer by default.

At 320 x 240 x 16-bit this would require approximately 150 KiB.

For ordinary text/UI updates this should not be necessary.

Prefer small line/chunk buffers suitable for SPI DMA.

If DMA buffers are used, allocate them using ESP-IDF-compatible DMA-capable memory.

Avoid dynamic allocation during normal repeated drawing operations when practical.



Fonts

Phase 2 only requires a useful built-in bitmap font.

Requirements:

	●	predictable character width/height;
	●	no wrapping;
	●	scalable size if practical;
	●	bounding-box calculation;
	●	clipping;
	●	efficient RGB565 rendering.

The design must make it possible to add more fonts later without changing the basic text API.

Do not make external font support a Phase 1 dependency.



Text replacement behavior

This requirement is important enough to test explicitly.

Create tests equivalent to:

```text
write "123456"
replace with "99"
```

Expected visual result:

```text
99
```

not:

```text
993456
```

Also test:

```text
"Temperature: 123.4 C"
"Temperature: 9 C"
""
```

Every replacement must clear only the affected text region.

No complete-screen erase is allowed for these operations.



Full-screen refresh semantics

The phrase “refresh full screen” in this project means an explicit application request to repaint/clear the full physical display.

The ST7789 does not need a periodic refresh initiated by the application.

Therefore the component should not continuously rewrite the framebuffer/display.

Provide an explicit full screen operation and otherwise leave already rendered pixels untouched.



Examples

The finished component must include at least three examples.

displayTest

The mandatory Phase 1 diagnostic described above.

It must remain in the repository after development and should become the primary hardware diagnostic example.

inputTest

Print events to UART.

Example:

```text
ENCODER steps=+1
ENCODER steps=+4
ENCODER steps=-2
ENCODER_BUTTON SHORT
ENCODER_BUTTON MEDIUM
ENCODER_BUTTON LONG
BUTTON SHORT
BUTTON MEDIUM
BUTTON LONG
```

The display should not be required for this example beyond optional visual feedback.

basic

Demonstrate the public component API:

	●	initialize;
	●	clear background;
	●	set rotation;
	●	set text color;
	●	set text size;
	●	create several text slots;
	●	update text repeatedly without full-screen clears;
	●	display encoder step count;
	●	display button event names.

The interactive menu must use the USB Serial/JTAG console with this exact
interaction order:

1. Print `Choice => ` before waiting for input.
2. Echo every received character immediately after the prompt.
3. Require `Enter` (`CR`, `LF`, or `CRLF`) to finish the input line.
4. Print one newline and execute the command only after Enter.
5. Return to step 1 after the command completes, including after redrawing the
   menu.

The prompt and character echo must be transmitted immediately. Use direct
console writes and flush the USB Serial/JTAG TX FIFO so the prompt is visible
while the application is waiting for input. Empty USB reads must not end the
interactive application.

The example must clearly demonstrate that changing one field does not redraw the entire screen.



Test strategy

Where hardware-independent logic exists, use unit tests where practical.

Good unit-test candidates include:

	●	RGB888 to RGB565 conversion;
	●	clipping calculations;
	●	text bounding boxes;
	●	text-slot replacement calculations;
	●	press-duration classification;
	●	quadrature transition table;
	●	encoder detent accumulation.

Hardware-dependent behavior must be tested with the actual board.



Documentation

Create:

```text
README.md
components/tft_lcd_display_ec11/README.md
```

Document:

	●	supported board revision;
	●	display controller;
	●	resolution;
	●	default GPIO map;
	●	inversion behavior;
	●	RGB565 color model;
	●	initialization;
	●	rotation;
	●	text API;
	●	no-wrap behavior;
	●	partial updates;
	●	event API;
	●	press thresholds;
	●	menuconfig options;
	●	examples;
	●	known limitations.

Include a section:

```text
Why this ST7789 is special
```

explaining that the attached panel requires inverted-display handling and that this behavior is deliberately normalized inside the component.

Do not force users of the component to rediscover this issue.



Component Registry readiness

Structure the component so it can later be published through the Espressif Component Registry.

Include an appropriate:

```text
idf_component.yml
```

Do not publish anything automatically.

Keep the public header clean and stable.

Avoid exposing private ESP-IDF driver objects or internal ST7789 state.



Coding style

Use clean, maintainable C.

General rules:

	●	descriptive names;
	●	no magic GPIO numbers in source;
	●	no unexplained ST7789 magic values;
	●	document every ST7789 initialization command;
	●	small focused functions;
	●	avoid global mutable state where unnecessary;
	●	check ESP-IDF return values;
	●	no Arduino APIs;
	●	no application-specific UI logic.

Use the project coding rules if a codingRules.md file is present in the project.



Non-goals

The first version is NOT intended to provide:

	●	LVGL integration;
	●	touch screen support;
	●	arbitrary display controller support;
	●	arbitrary board support;
	●	animation framework;
	●	menu system;
	●	Groovebox-specific UI;
	●	Ultimate Timer-specific UI;
	●	image decoder;
	●	JPEG/PNG rendering;
	●	SD card handling;
	●	Wi-Fi handling;
	●	audio handling.

Keep the component focused.



Required implementation order

Follow this order strictly:

```text
Phase 1
  low-level displayTest
  ↓
  prove inversion/colors
  ↓
  prove geometry/rotation
  ↓
  prove partial rectangle writes
Phase 2
  reusable display primitives
  ↓
  font/text support
  ↓
  no-wrap support
  ↓
  text replacement / text slots
Phase 3
  EC11 rotation
  ↓
  encoder push short/medium/long
Phase 4
  extra button short/medium/long
Phase 5
  unified event queue
  ↓
  basic example
  ↓
  documentation
  ↓
  cleanup for Component Registry
```

Do not skip ahead when Phase 1 color/inversion behavior is not yet proven.



Definition of Done

The component is finished when:

	1.	It builds with current ESP-IDF for ESP32-S3.
	2.	The displayTest example runs without Arduino dependencies.
	3.	ST7789 initialization is understood and documented.
	4.	The panel inversion behavior is correctly handled internally.
	5.	Normal RGB565 colors render correctly.
	6.	All supported rotations work.
	7.	Full screen clear is explicitly available.
	8.	Normal text operations never refresh the complete screen.
	9.	Text wrapping never occurs.
	10.	Replacing longer text with shorter text leaves no stale pixels.
	11.	Background color is configurable.
	12.	Text foreground color is configurable.
	13.	Text size is configurable.
	14.	Text can be written at deterministic X/Y positions.
	15.	EC11 rotation produces signed accumulated step counts.
	16.	EC11 push produces SHORT / MEDIUM / LONG events.
	17.	The extra push button produces SHORT / MEDIUM / LONG events.
	18.	Input events can be consumed through a clean ESP-IDF-native API.
	19.	GPIOs and timing thresholds have sensible menuconfig defaults.
	20.	The public component API contains no Groovebox- or Timer-specific concepts.
	21.	The examples and documentation are complete enough that a new ESP-IDF application can use the component without reading its private source code.



Most important engineering rule

The existing projects already contain hard-earned knowledge about this particular TFT panel.

Preserve that knowledge.

Do not replace a proven hardware-specific solution with a theoretically “standard” ST7789 implementation unless the replacement has been verified on the actual board with the Phase 1 displayTest.

The purpose of this component is precisely to make sure this hardware-specific knowledge only has to be solved once.