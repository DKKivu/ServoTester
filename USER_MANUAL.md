# Servo Tester User Manual

## Overview

Servo Tester runs on an ESP32-C3 DevKitM-1 and sends a servo control pulse
through GPIO 4. Use the rotary encoder and its push button to select a mode,
move the servo, and adjust settings. The OLED displays the mode, commanded
pulse width, and an estimated angle.

The angle is calculated from the commanded pulse width. It is not measured
position feedback, and actual servo travel varies by servo.

## Wiring

| Component signal | ESP32-C3 GPIO |
| --- | ---: |
| Servo signal | 4 |
| Encoder A / CLK | 0 |
| Encoder B / DT | 1 |
| Encoder push button | 3 |
| OLED SDA | 5 |
| OLED SCL | 6 |

The configured display driver is U8g2 `SSD1306_72X40_ER`, using hardware I2C.
The button input uses the ESP32's internal pull-up.

The firmware does not configure or supply component power. Check the
requirements of your exact ESP32 board, OLED, and servo. Use a supply suitable
for the servo; if it is powered separately, connect its ground to ESP32 ground.
Keep the servo linkage clear before powering it, especially when testing
unknown pulse limits.

## Startup

At startup the firmware:

1. Loads saved settings from ESP32 non-volatile storage, or uses the defaults.
2. Clamps loaded values to the supported ranges.
3. Initializes the encoder, button, servo, and OLED.
4. Commands a 1500 µs servo pulse.
5. Shows the boot screen briefly, then enters the running screen in Manual
   mode.

The boot screen remains for 800 ms. The main loop then processes input, updates
the sweep if active, redraws the current screen, and waits 20 ms.

## Running screen and modes

The OLED shows the mode name, `P` (pulse width in microseconds), and `A`
(calculated angle in degrees).

| Mode | Turn encoder | Short-press button |
| --- | --- | --- |
| Manual | Change pulse by the configured step | Cycle to the next mode |
| Sweep | Change sweep interval | Cycle to the next mode |
| Center | No effect | Cycle to the next mode |
| Step | Change step size | Advance pulse by one step |

The mode cycle is **Manual → Sweep → Center → Step → Manual**. Encoder changes
to pulse width are clamped to the configured minimum and maximum. Center moves
to the arithmetic midpoint of those two pulse settings. Sweep moves toward
each endpoint by one configured step at each interval and reverses at an
endpoint.

The displayed angle maps the configured minimum pulse to 0° and maximum pulse
to the configured maximum angle. The default maximum angle is 240°; this is a
display scale only and does not change servo mechanics or calibrate the servo.

## Button and menu navigation

The button is read as active-low with an internal pull-up. A press released
within 500 ms is reported as a click; a held press is reported as a long press
after 700 ms. A release between those timings does not produce a click.

### Main menu

- On the running screen, long-press to open the main menu.
- Turn the encoder to highlight **Manual**, **Sweep**, **Center**, **Step**, or
  **Settings**.
- Short-press a mode to activate it and return to the running screen.
- Select **Settings** to open the settings menu.
- Long-press in the main menu to return to the running screen.

### Settings menu

The settings list is **MinPulse**, **MaxPulse**, **Step**, **SweepSpd**,
**MaxAngle**, **Save**, and **Back**. Three menu rows are shown at a time.

- On a numeric setting, turn the encoder to edit it and short-press to move to
  the next item.
- **Save** writes all current settings to NVS and returns to the main menu.
- **Back** returns to the main menu without writing settings to NVS.
- Long-press also returns to the main menu without writing settings to NVS.

Changes made before **Save** remain active in the current running session,
including after leaving the settings menu. They are lost after reset or power
off unless saved.

### Settings defaults and limits

| Setting | Default | Range |
| --- | ---: | ---: |
| Minimum pulse | 500 µs | 50–1500 µs |
| Maximum pulse | 2500 µs | 1500–2500 µs |
| Step | 10 µs | 1–50 µs |
| Sweep interval | 20 ms | 5–100 ms |
| Display maximum angle | 240° | 90–360° |

Minimum and maximum pulse are bounded independently; the firmware does not
enforce that the minimum is less than the maximum. Use compatible limits for
your servo.

## Firmware build and upload

Install PlatformIO and run these commands from the project directory:

```sh
pio run
pio run --target upload
pio device monitor
```

The configured PlatformIO environment is `esp32-c3-devkitm-1` and the serial
monitor baud rate is 115200.
