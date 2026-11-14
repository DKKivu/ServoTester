# ServoTester User Manual

## What it does

ServoTester runs on an ESP32-C3 DevKitM-1 and sends a control signal from
GPIO 4 to a compatible hobby servo or ESC. Use the rotary encoder and its
push button to choose an operating mode and edit settings. The 72×40 OLED
shows the active mode and protocol. PWM displays pulse width and angle;
non-PWM protocols display throttle percentage and the protocol's raw value.

The displayed angle is a configured scale, not measured servo or motor
position feedback.

## Important safety information

- Disconnect power before changing wiring.
- GPIO 4 is a signal output only. It does not power the servo, ESC, or motor.
- Follow the device manufacturer's voltage, signal, and grounding requirements.
- Use a suitable supply for the servo/ESC. Join grounds when required by the
  device interface.
- Remove all propellers and disconnect mechanical loads before testing an ESC.
- The app uses a normalized output for all protocols, but selecting Center
  sends 50% output. Do not select Center with a motor or propeller connected.
- Confirm that the ESC supports the selected protocol. DShot requests arming
  once per firmware runtime; follow the ESC manufacturer's startup procedure.
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
1000 and 2000 µs, and the initial PWM output is their midpoint (1500 µs).
The normalized output starts at 50% for PWM and 0% for non-PWM protocols.

The main loop reads encoder movement, polls the button, updates Sweep mode,
redraws the display, and delays for 20 ms. Sweep checks elapsed time, so
actual update timing may be delayed by other loop work.

## Output protocols

Select the protocol in **Settings → Protocol**. The output adapter is rebuilt
immediately. Selecting a non-PWM protocol resets normalized output to 0%;
select **Save** to persist the selection.

| Display name | Signal | Native output range |
| --- | --- | --- |
| PWM | 50 Hz PWM | Configured min/max pulse values |
| OS125 | OneShot125, 2 kHz update | 125–250 µs |
| OS42 | OneShot42, 2 kHz update | 42–84 µs |
| MS | Multishot, 32 kHz update | 5–25 µs |
| DS150 | DShot150 | Command values 48–2047 |
| DS300 | DShot300 | Command values 48–2047 |
| DS600 | DShot600 | Command values 48–2047 |
| DS1200 | DShot1200 | Command values 48–2047 |

Manual, Sweep, Center, and Step use a normalized 0–100% output position;
each adapter converts it to the selected protocol's native range. The
min/max pulse settings apply to PWM only. For non-PWM protocols, the running
screen shows `T` (throttle percent) and `R` (raw value); for PWM it shows
`P` (microseconds) and `A` (calculated angle).

## Running modes

The mode cycle is **Manual → Sweep → Center → Step → Manual**. Short-press the
button on the running screen to change modes, except in Step mode where the
click advances the output by one step.

| Mode | Turn encoder | Short-press |
| --- | --- | --- |
| Manual | Change output by the configured step | Cycle mode |
| Sweep | Change interval by 1 ms | Cycle mode |
| Center | No effect | Cycle mode |
| Step | Change the active step size | Advance output by one step |

For PWM, Manual and Step use the `Step` setting in microseconds divided by the
configured pulse span. For non-PWM protocols, they use `StepPct` as a
percentage of the normalized range. Center commands 50%. Sweep traverses
from 0% to 100% and back at the configured interval; selecting Sweep starts
in the increasing direction.

PWM uses the configured pulse limits; each other adapter uses its native
range. The angle display maps normalized output to `minAngle..maxAngle` and
is not position feedback.

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

The list is **MinPulse**, **MaxPulse**, **Step**, **StepPct**, **SweepSpd**,
**MinAngle**, **MaxAngle**, **Protocol**, **Save**, and **Back**. Three rows
are visible at a time. Protocol names are abbreviated on the running screen;
the settings row shows the full protocol name.

- Turn the encoder on a numeric setting to edit it.
- Turn the encoder on **Protocol** to cycle protocols; the output is rebuilt
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
| Minimum pulse | 1000 µs | 500–1500 µs | 500–1500 µs, increments of 10 |
| Maximum pulse | 2000 µs | 1500–2500 µs | 1500–2500 µs, increments of 10 |
| PWM step | 10 µs | 1–50 | 1–50, increments of 1 |
| Non-PWM step (StepPct) | 5% | 1–20% | 1–20, increments of 1 |
| Sweep interval | 20 ms | 5–100 ms | 5–100 ms, increments of 1 |
| Minimum angle | 0° | 0–180° | 0–180°, increments of 1 |
| Maximum angle | 120° | 90–360° | 90–360°, increments of 1 |
| Protocol | PWM | 8 supported choices | Cycle with encoder |

The min/max pulse and angle values are clamped independently. Changing
MaxPulse or MaxAngle attempts to preserve their current pulse/angle ratio;
the paired change is rejected if the result would exceed the paired setting's
allowed range. Keep `maxAngle` greater than `minAngle`; angle settings are
independently clamped.

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
