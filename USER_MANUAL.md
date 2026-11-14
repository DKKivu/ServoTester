# ServoTester User Manual

## What it does

ServoTester runs on an ESP32-C3 DevKitM-1 and sends a control signal from
GPIO 4 to a compatible hobby servo or ESC. Use the rotary encoder and its
push button to choose an operating mode and edit settings. The 72×40 OLED
shows the active mode, a raw output value, and a calculated angle.

The angle is calculated from the output command; it is not measured servo or
motor position feedback. The display's `P` value is not necessarily a pulse
width when a digital or non-PWM protocol is selected.

## Important safety information

- Disconnect power before changing wiring.
- GPIO 4 is a signal output only. It does not power the servo, ESC, or motor.
- Follow the device manufacturer's voltage, signal, and grounding requirements.
- Use a suitable supply for the servo/ESC. Join grounds when required by the
  device interface.
- Remove all propellers and disconnect mechanical loads before testing an ESC.
- Start with PWM unless you understand the selected ESC's protocol and range.
  The current app does not normalize Manual/Sweep values across all protocols;
  OneShot and Multishot can clamp to their upper output endpoints.
- Verify output signals with suitable equipment before connecting a motor.

## Wiring

| Component signal | ESP32-C3 GPIO |
| --- | ---: |
| Servo/ESC signal input | 4 |
| Encoder A / CLK | 0 |
| Encoder B / DT | 1 |
| Encoder push button | 3 |
| OLED SDA | 5 |
| OLED SCL | 6 |

The OLED driver is U8g2 `SSD1306_72X40_ER` over hardware I2C at 400 kHz. The
button input uses the ESP32's internal pull-up and is active-low.

## Startup

On startup, the firmware loads saved settings from ESP32 NVS, applies default
values for missing keys, and clamps settings to allowed ranges. It initializes
the selected output protocol, encoder, button, and OLED, then displays the
boot screen for 800 ms. The default protocol is PWM; the default limits are
1000 and 2000, and the initial PWM output is their midpoint (1500).

The main loop reads encoder movement, polls the button, updates Sweep mode,
redraws the display, and delays for 20 ms. Sweep checks elapsed time, so
actual update timing may be delayed by other loop work.

## Output protocols

Select the protocol in **Settings → Proto**. The output adapter is rebuilt
immediately; select **Save** to persist the selection.

| Display name | Signal implementation | Adapter output range |
| --- | --- | --- |
| PWM | 50 Hz PWM | Configured min/max pulse values |
| O125 | OneShot125, 2 kHz update | 125–250 µs |
| O42 | OneShot42, 2 kHz update | 42–84 µs |
| Multi | Multishot, 32 kHz update | 5–25 µs |
| D150 | DShot150 | Command values 48–2047 |
| D300 | DShot300 | Command values 48–2047 |
| D600 | DShot600 | Command values 48–2047 |
| D1200 | DShot1200 | Command values 48–2047 |

**Current limitation:** Manual and Sweep use the configured min/max settings
as raw output numbers for every protocol. These values are not translated
into each protocol's native range. OneShot and Multishot clamp incoming
values to their own much smaller pulse ranges, which can result in maximum
output. DShot receives only the app-limited command range. The displayed `P`
and angle may therefore not represent pulse width or physical position.
Use non-PWM protocols only for controlled signal testing with propellers and
loads removed.

## Running modes

The mode cycle is **Manual → Sweep → Center → Step → Manual**. Short-press the
button on the running screen to change modes, except in Step mode where the
click advances the output by one step.

| Mode | Turn encoder | Short-press |
| --- | --- | --- |
| Manual | Change raw output by `step` | Cycle mode |
| Sweep | Change interval by 1 ms | Cycle mode |
| Center | No effect | Cycle mode |
| Step | Change step by 1 | Advance output by one step |

Manual and Step changes are limited by configured min/max values. Center
commands their arithmetic midpoint. Sweep advances by `step` at each
interval, clamps at the configured endpoints, then reverses direction.
Selecting Sweep restarts it in the increasing direction.

For PWM, output values are pulse widths and the settings behave as pulse
limits. For other protocols, see the limitation above; the shared mode
controls currently do not translate units between protocols.

## Button and menu navigation

The active-low button produces a click when released less than 500 ms after
pressing. Holding it for more than 700 ms produces a long press. Durations
from 500 through 700 ms produce no event.

### Main menu

- Long-press on the running screen to open the main menu.
- Turn the encoder to select **Manual**, **Sweep**, **Center**, **Step**, or
  **Settings**. Selection wraps at either end.
- Short-press a mode to activate it and return to the running screen.
- Short-press **Settings** to open the settings menu.
- Long-press in the main menu to return to the running screen.

### Settings menu

The list is **MinPulse**, **MaxPulse**, **Step**, **SweepSpd**, **MinAngle**,
**MaxAngle**, **Proto**, **Save**, and **Back**. Three rows are visible at a
time. The selected protocol is shown with a compact value: PWM, O125, O42,
Multi, D150, D300, D600, or D1200.

- Turn the encoder on a numeric setting to edit it.
- Turn the encoder on **Proto** to cycle protocols; the output is rebuilt
  immediately.
- Short-press to advance to the next item.
- **Save** writes all settings to NVS and returns to the main menu.
- **Back** returns to the main menu without saving.
- Long-press also returns to the main menu without saving.

Unsaved edits are active for the current session, even after leaving Settings,
but are lost after reset or power-off.

## Settings defaults and limits

| Setting | Default | Startup/storage clamp | Settings-menu edit range |
| --- | ---: | ---: | ---: |
| Minimum pulse | 1000 | 500–1500 | 500–1500, increments of 10 |
| Maximum pulse | 2000 | 1500–2500 | 1500–2500, increments of 10 |
| Step | 10 | 1–50 | 1–50, increments of 1 |
| Sweep interval | 20 ms | 5–100 ms | 5–100 ms, increments of 1 |
| Minimum angle | 0° | 0–180° | 0–180°, increments of 1 |
| Maximum angle | 120° | 90–360° | 90–360°, increments of 1 |
| Protocol | PWM | 8 supported choices | Cycle with encoder |

The min/max pulse and angle values are clamped independently. Keep a
nonzero pulse span and keep `maxAngle` greater than `minAngle`; the display
mapping and settings recalculation assume sensible ranges. The displayed
angle maps the raw output from configured min/max to `0..maxAngle`; it does
not use `minAngle`.

## Build and upload

Install PlatformIO, open the project, and run these commands from its
directory:

```sh
pio run
pio run --target upload
pio device monitor
```

The PlatformIO environment is `esp32-c3-devkitm-1`; the serial monitor uses
115200 baud.
