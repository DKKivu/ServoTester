# Servo Tester User Manual

## Overview

Servo Tester runs on an ESP32-C3 DevKitM-1 and sends servo pulses through GPIO
4. Use the rotary encoder and its push button to select a mode, control the
pulse, and edit settings. The 72×40 OLED displays the current mode, commanded
pulse width, and a calculated angle.

The angle is calculated from the command; it is not measured position
feedback. Actual servo travel depends on the servo and its mechanical setup.

## Wiring

| Component signal | ESP32-C3 GPIO |
| --- | ---: |
| Servo signal | 4 |
| Encoder A / CLK | 0 |
| Encoder B / DT | 1 |
| Encoder push button | 3 |
| OLED SDA | 5 |
| OLED SCL | 6 |

The display driver is U8g2 `SSD1306_72X40_ER` over hardware I2C at 400 kHz.
The button input uses the ESP32's internal pull-up and is active-low.

The firmware does not supply component power. Check the requirements of your
board, OLED, and servo. Use a supply suitable for the servo; if it is powered
separately, connect its ground to ESP32 ground. Keep the servo linkage clear,
particularly when testing unfamiliar pulse limits.

## Startup and loop

At startup the firmware:

1. Loads the saved settings from ESP32 NVS, using the built-in defaults for
   missing values, then clamps them to the storage/startup limits.
2. Initializes the encoder, button, servo, and OLED.
3. Commands a 1500 µs pulse.
4. Shows the boot screen for 800 ms and starts in Manual mode.

Each loop reads encoder movement, polls the button, updates Sweep mode if
active, renders the current screen, and delays for 20 ms. The sweep interval is
checked using elapsed milliseconds; actual updates can be delayed by the loop
and display work.

## Running screen and modes

The OLED shows the mode name, `P` (pulse width in microseconds), and `A`
(calculated angle in degrees).

| Mode | Turn encoder | Short-press button |
| --- | --- | --- |
| Manual | Change pulse by `step` µs per encoder count | Cycle to the next mode |
| Sweep | Change sweep interval by 1 ms per count | Cycle to the next mode |
| Center | No effect | Cycle to the next mode |
| Step | Change `step` by 1 µs per count | Advance pulse by `step` |

The mode cycle is **Manual → Sweep → Center → Step → Manual**. Manual and Step
pulse changes are clamped to the current pulse limits. Selecting Center moves
the servo to the arithmetic midpoint of the two pulse limits. Sweep advances
by `step` at each interval, clamps at the endpoints, then reverses direction.
Selecting Sweep starts it in the increasing direction.

The displayed angle maps `minPulse` to 0° and `maxPulse` to `maxAngle`; it
does not use `minAngle` in the displayed mapping. The settings menu exposes
`minAngle` and the firmware runs recalculation logic when pulse or angle
settings are edited, but the OLED angle calculation itself uses only
`maxAngle`. This display value is not a servo calibration or feedback reading.

## Button and menu navigation

The button is active-low. A release in less than 500 ms produces a click. A
press held for more than 700 ms produces a long press. Releases from 500 ms
through 700 ms produce neither event.

### Main menu

- Long-press on the running screen to open the main menu.
- Turn the encoder to select **Manual**, **Sweep**, **Center**, **Step**, or
  **Settings**. Selection wraps at either end.
- Short-press a mode to activate it and return to the running screen.
- Short-press **Settings** to open the settings menu.
- Long-press in the main menu to return to the running screen.

### Settings menu

The settings list is **MinPulse**, **MaxPulse**, **Step**, **SweepSpd**,
**MinAngle**, **MaxAngle**, **Save**, and **Back**. Three rows are displayed at
a time, and the selected numeric setting is shown with its current value.

- Turn the encoder on a numeric setting to edit it; short-press to advance to
  the next item.
- **Save** writes all settings to NVS and returns to the main menu.
- **Back** returns to the main menu without saving.
- Long-press also returns to the main menu without saving.

Edits take effect immediately and remain active for the current session even
after leaving Settings. They are lost after reset or power-off unless saved.

### Settings defaults and limits

| Setting | Default | Startup/storage clamp | Settings-menu edit range |
| --- | ---: | ---: | ---: |
| Minimum pulse | 500 µs | 0–1500 µs | 500–1500 µs, in 10 µs increments |
| Maximum pulse | 2500 µs | 1500–3500 µs | 1500–2500 µs, in 10 µs increments |
| Step | 10 µs | 1–50 µs | 1–50 µs, in 1 µs increments |
| Sweep interval | 20 ms | 5–100 ms | 5–100 ms, in 1 ms increments |
| Minimum angle | 0° | 0–180° | 0–180°, in 1° increments |
| Maximum angle | 180° | 90–360° | 90–360°, in 1° increments |

The pulse limits are clamped independently. Their allowed ranges meet at
1500 µs, so both can be set to the same value; keep a nonzero pulse span
because the displayed angle maps across that span.

The angle limits are also clamped independently, and the firmware does not
enforce `maxAngle > minAngle`. Keep `maxAngle` greater than `minAngle` when
editing angle settings because the settings recalculation uses their
difference.

## Firmware build and upload

Install PlatformIO and run these commands from the project directory:

```sh
pio run
pio run --target upload
pio device monitor
```

The configured PlatformIO environment is `esp32-c3-devkitm-1`; the serial
monitor baud rate is 115200.
