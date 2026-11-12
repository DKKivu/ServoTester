# Servo Tester User Manual

## What it does

Servo Tester drives a hobby servo from an ESP32-C3 and lets you adjust the
servo signal with a rotary encoder. An OLED shows the selected mode, pulse
width, and an estimated angle.

The angle is calculated by mapping the configured minimum and maximum pulse
widths to 0° and 180°. It is an estimate, not feedback from the servo.

## Before powering on

1. Connect the components according to the pin table below.
2. Connect the servo's signal wire to GPIO 4.
3. Power the servo from a supply appropriate for that servo. Do not power the
   servo from an ESP32 GPIO. If the servo has a separate supply, connect that
   supply's ground to ESP32 ground.
4. Check the OLED module's voltage requirements before connecting its power.
5. Keep the servo mechanism clear to move through its range.

### Connections

| Component signal | ESP32-C3 GPIO |
| --- | ---: |
| Servo signal | 4 |
| Encoder CLK | 0 |
| Encoder DT | 1 |
| Encoder push button | 3 |
| OLED SDA | 6 |
| OLED SCL | 5 |

The configured board is ESP32-C3 DevKitM-1. The display is configured as an
SSD1306 72x40 I2C OLED.

## Using the controls

- **Turn the encoder** to change the active value or move through a menu.
- **Short-press** the encoder button to select a menu item or perform the
  current mode's button action.
- **Press and hold** the button to enter or leave menus, as described below.

### Running modes

The display shows the current mode, pulse width in microseconds (`P`), and
estimated angle in degrees (`A`).

| Mode | Encoder rotation | Short-press |
| --- | --- | --- |
| Manual | Change pulse width by the configured step | Switch to the next mode |
| Sweep | Adjust sweep interval | Switch to the next mode |
| Center | No effect | Switch to the next mode |
| Step | Adjust step size | Advance the pulse by one step |

The running-mode order is **Manual → Sweep → Center → Step → Manual**. You can
switch modes with short-presses while running or choose a mode from the main
menu.

In Sweep mode, the servo moves back and forth between the minimum and maximum
pulse settings. The interval is the delay between sweep steps: a smaller
interval makes the sweep faster.

## Menus

### Open the main menu

Press and hold the encoder button while on the running screen. Turn the encoder
to highlight an item, then short-press to select it.

The main menu contains **Manual**, **Sweep**, **Center**, **Step**, and
**Settings**. Press and hold to close the main menu and return to the running
screen.

### Change settings

1. Select **Settings** from the main menu.
2. Turn the encoder to change the highlighted setting.
3. Short-press to move to the next setting.
4. Select **Save** and short-press to save, or select **Back** to return without
   saving.
5. Press and hold to return to the main menu.

Settings have these defaults and limits:

| Setting | Default | Allowed range |
| --- | ---: | ---: |
| Minimum pulse | 500 µs | 500–1500 µs |
| Maximum pulse | 2500 µs | 1500–2500 µs |
| Step | 10 µs | 1–50 µs |
| Sweep interval | 20 ms | 5–100 ms |

The minimum and maximum pulse settings are independently limited around
1500 µs; the firmware does not enforce a non-zero gap between them.

Saved settings are kept in the ESP32's non-volatile storage and loaded the next
time the device starts. At startup, the output is set to 1500 µs.

## Firmware setup

The project is configured for PlatformIO and the Arduino framework. From the
project directory:

```sh
pio run
pio run --target upload
pio device monitor
```

The serial monitor is configured for **115200 baud**.
