# Servo Tester

Servo Tester is firmware for an ESP32-C3 DevKitM-1. It drives a hobby servo
with configurable pulse limits and provides Manual, Sweep, Center, and Step
modes. A rotary encoder and its push button control the firmware; an I2C OLED
shows the active mode, pulse width, estimated angle, and menus.

The angle is calculated from the configured pulse range and angle limit. It is
an estimate based on the output command, not position feedback from the servo.

## Hardware and signal pins

| Connection | ESP32-C3 GPIO |
| --- | ---: |
| Servo signal | 4 |
| Encoder A / CLK | 0 |
| Encoder B / DT | 1 |
| Encoder push button | 3 |
| OLED SDA | 5 |
| OLED SCL | 6 |

The firmware configures the encoder button as `INPUT_PULLUP`. The OLED driver
is U8g2 `SSD1306_72X40_ER` over hardware I2C. Power and ground wiring depend on
the exact modules used and are not configured by firmware. Use a suitable
servo supply; when it is separate from the ESP32 supply, connect the grounds.
Check the OLED's voltage requirements before wiring power.

See [DIAGRAMS.md](DIAGRAMS.md) for a wiring diagram, class diagram, and
application workflow. See [USER_MANUAL.md](USER_MANUAL.md) for operating
instructions.

## Features

- **Manual:** turn the encoder to adjust pulse width by the configured step.
- **Sweep:** automatically sweeps between the minimum and maximum pulse values;
  encoder rotation adjusts the interval between sweep steps.
- **Center:** moves the servo to the midpoint of the configured pulse range.
- **Step:** turn the encoder to change step size; each button click advances
  the pulse by one step.
- Main and settings menus navigated with the encoder and button.
- Configurable minimum/maximum pulse, step, sweep interval, and displayed
  maximum angle.
- Settings loaded from and saved to ESP32 NVS using the `servo` namespace.

### Factory defaults and limits

| Setting | Default | Firmware limits |
| --- | ---: | ---: |
| Minimum pulse | 500 µs | 50–1500 µs |
| Maximum pulse | 2500 µs | 1500–2500 µs |
| Step | 10 µs | 1–50 µs |
| Sweep interval | 20 ms | 5–100 ms |
| Display maximum angle | 240° | 90–360° |

Minimum and maximum pulse are constrained independently; the firmware does not
require the minimum to be less than the maximum. The angle display maps the
minimum pulse to 0° and maximum pulse to the configured maximum angle.

## Build, upload, and monitor

The project uses PlatformIO with the Arduino framework. Open the project in VS
Code with PlatformIO, or run these commands from the project directory:

```sh
pio run
pio run --target upload
pio device monitor
```

The PlatformIO environment is `esp32-c3-devkitm-1`; serial is configured for
115200 baud. Dependencies are U8g2, ESP32Servo, and ESP32_Knob.

## Source layout

- `src/main.cpp` — GPIO assignments and dependency composition
- `src/ServoTesterApp.cpp` — startup, input handling, mode logic, sweep, and
  rendering dispatch
- `src/OledDisplay.cpp` — OLED rendering
- `src/RotaryEncoder.h` — ESP32_Knob adapter
- `src/Button.h` — push-button click/long-press detection
- `src/ServoOutput.h` — servo pulse output
- `src/SettingsStorage.h` — NVS persistence
- `platformio.ini` — board, framework, dependencies, and build configuration

## License

This project is released under the [MIT License](LICENSE).
