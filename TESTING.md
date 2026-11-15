# Hardware feedback checklist

Build `examples/basic/testProgram`, open the 115200-baud monitor, enter a letter, then Enter.

- Corner test A-D: report which labeled corner block is visible, clipped, displaced, or missing.
- Vertical stripes E: report `V1..V8` from left to right and the observed color for each.
- Horizontal stripes F: report `H1..H8` from top to bottom and the observed color for each.
- Color/inversion tests: report the label and observed color, not only the raw hex value.
- Rotation test: report orientation number and which edge/corner is incorrect.
- Partial-update test: confirm the surrounding checkerboard remains unchanged.
- Text overwrite test: confirm `123456789` becomes only `99` (no old characters).
- Font faces test: in the `J` text-settings submenu, press `9` Enter repeatedly to cycle
  SMALL (6x8 cell) -> MEDIUM (8x12) -> LARGE (9x16). Each step must redraw the four full-ASCII
  sample lines (32..126) at the bottom of the screen in the selected face: 1 px strokes, no doubled
  or ragged lines, `g j p q y` and `, ; _` descending below the text baseline in MEDIUM and LARGE,
  and unknown bytes as hollow boxes. Then use `6` to draw your own text in the selected face.
  Glyph coverage is also provable without hardware via the host check
  `tools/font_check/font_check.c` (prints every glyph of every face; must report
  `FONT CHECK OK`).
- Input tests: compare monitor event type, signed step count, and measured press duration.

The program logs display dimensions and each operation result. Press `M` Enter to print the menu again.
