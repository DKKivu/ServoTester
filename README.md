# Servo Tester

An ESP32-C3 servo tester with an OLED menu, rotary-encoder controls, and
persistent settings. The display shows the selected mode, servo pulse width,
and an angle estimate.

## Hardware

- ESP32-C3 DevKitM-1
- Servo connected to the signal pin below
- SSD1306 72x40 OLED display using I2C
- Rotary encoder with a push button

### Pin connections

| Component | ESP32-C3 GPIO |
| --- | ---: |
| Servo signal | 4 |
| Encoder CLK | 0 |
| Encoder DT | 1 |
| Encoder push button | 3 |
| OLED SDA | 6 |
| OLED SCL | 5 |

Power the servo appropriately for the servo in use. When using a separate
servo supply, connect its ground to the ESP32 ground.

## Controls

- Rotate the encoder to adjust the active mode or the selected menu setting.
- Short-press the encoder button to cycle modes while running. In **Step**
  mode, a short press moves the servo by one configured step instead.
- Long-press the button to open the main menu.
- In the main menu, rotate to select an item and short-press to activate it.
  Long-press to return to the running screen.
- In **Settings**, rotate to edit the selected value and short-press to move
  through the settings. Select **Save** to persist changes or **Back** to return
  to the main menu. Long-press also returns to the main menu.

### Modes

- **Manual** — encoder rotation changes the servo pulse width by the configured
  step.
- **Sweep** — automatically sweeps between the minimum and maximum pulse widths.
  Encoder rotation adjusts the sweep interval.
- **Center** — moves the servo to the midpoint of the configured pulse range.
- **Step** — encoder rotation changes the step size; button presses advance the
  servo by one step.

## Settings

| Setting | Default | Range |
| --- | ---: | ---: |
| Minimum pulse | 500 µs | 500–1500 µs |
| Maximum pulse | 2500 µs | 1500–2500 µs |
| Step | 10 µs | 1–50 µs |
| Sweep interval | 20 ms | 5–100 ms |

Settings are stored in ESP32 non-volatile storage (NVS) after selecting **Save**
in the Settings menu. The servo output is initialized to 1500 µs.

## Build and upload

This project uses PlatformIO with the Arduino framework. Open the project
directory in VS Code with the PlatformIO extension, or use the PlatformIO CLI:

```sh
pio run
pio run --target upload
pio device monitor
```

The configured environment is `esp32-c3-devkitm-1`. The serial monitor baud rate
is 115200.

## Project layout

- `src/main.cpp` — GPIO assignments and component setup
- `src/ServoTesterApp.cpp` — mode, input, menu, and display-update logic
- `src/OledDisplay.cpp` — OLED rendering
- `src/SettingsStorage.h` — settings persistence using ESP32 Preferences
- `platformio.ini` — board, framework, dependencies, and build configuration
