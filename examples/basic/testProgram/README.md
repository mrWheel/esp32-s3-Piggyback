# Interactive testProgram

Flash this project and use the serial monitor at 115200 baud. Commands are single letters A-Z,
case-insensitive, and are executed only after Enter. This makes pasted input and terminal line endings
safe and predictable.

The menu covers every public component feature. A-D each draw one exact corner. E and F use eight
numbered/color-named stripe positions. The serial output is the authoritative map to use when reporting
physical observations. P and Q intentionally toggle raw panel inversion so incorrect panel behavior can
be diagnosed. The expected production setting is Q / INV ON.

## Font faces submenu

The `J` text-settings submenu has option `9`, which cycles the face used by `tft_ec11_draw_text_font()`:
SMALL (5x7 glyph, 6x8 cell) -> MEDIUM (7x12, 8x12) -> LARGE (8x16, 9x16) -> SMALL. Every step redraws
four sample lines at the bottom of the screen that together contain the complete printable ASCII range
(32..126), so glyph coverage of the selected face can be verified on the display. Option `6` (enter text)
draws the typed text with the selected face; with SMALL it uses the legacy scale-based path. The same
coverage can be proven without hardware by compiling and running `tools/font_check/font_check.c`
on the host (see [TESTING.md](../../../TESTING.md)).

## SDcard submenu

Command M opens a numbered submenu (1-9, Enter to execute) that exercises the SD card slot on the
board over its own SPI bus (SPI3_HOST, separate from the TFT's SPI2_HOST):

```text
SD_DTCT  GPIO 7
SD_CS    GPIO 13
SD_SCLK  GPIO 14
SD_MISO  GPIO 18
SD_MOSI  GPIO 21
```

The submenu header reports whether a card is currently detected on `SD_DTCT`
(`SDcard detected` / `No SDcard detected`), independent of whether it has been initialized yet.

1. Initialize SDcard - mounts a FAT filesystem at `/sdcard`.
2. Format SDcard - formats the mounted card (destroys existing data).
3. Write Testfile - prompts for a line of text and writes `/sdcard/testfile.txt`, overwriting it.
4. Append Testfile - prompts for a line of text and appends it to `/sdcard/testfile.txt`.
5. Read TestFile - prints the contents of `/sdcard/testfile.txt`.
9. Return to main menu.

This SD card handling lives only in this test program; it is not part of the reusable
`esp32_s3_piggyback` component, which deliberately excludes SD card support (see `projectPrompt.md`).
