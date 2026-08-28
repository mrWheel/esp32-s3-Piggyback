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

Every menu choice is a letter A-Z (case-insensitive) followed by Enter. `M` redraws the menu.
The visual tests print stable labels in the monitor, so observations can be reported without ambiguity.

## Other examples

- `examples/displayTest`: phase-1 low-level display diagnostics
- `examples/inputTest`: encoder and both push-button event diagnostics
- `examples/basic/testProgram`: complete menu covering every public component operation

See [projectPrompt.md](projectPrompt.md), [TESTING.md](TESTING.md), and the public API in
`components/esp32_s3_piggyback/include/esp32_s3_piggyback.h`.
