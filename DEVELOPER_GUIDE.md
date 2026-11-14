# ServoTester Developer Guide

This guide describes the current ESP32-C3 firmware architecture and the
implementation details that matter when extending or maintaining it.

## Architecture

`src/main.cpp` is the composition root. It defines GPIO assignments and
constructs the settings storage, OLED display, encoder, button, and
`ServoTesterApp`. The app receives the ESC signal pin and constructs the
selected output adapter itself.

| Component | File | Responsibility |
| --- | --- | --- |
| `ServoTesterApp` | `ServoTesterApp.h/.cpp` | Input dispatch, operating modes, menus, sweep logic, output selection, render dispatch |
| `IEscOutput` | `IEscOutput.h` | Common interface for output protocol implementations |
| `PwmEscOutput` | `PwmEscOutput.h` | PWM signal generation using ESP32Servo |
| `OneShotEscOutput` | `OneShotEscOutput.h` | OneShot125 and OneShot42 generation using LEDC |
| `MultishotEscOutput` | `MultishotEscOutput.h` | Multishot generation using LEDC |
| `DShotEscOutput` | `DShotEscOutput.h` | DShot generation using ESP32_DSHOT |
| `EscProtocol` | `EscProtocol.h` | Protocol enum, names, and protocol-cycle helper |
| `Settings` | `Settings.h` | Defaults and validation/clamping |
| `NvsSettingsStorage` | `SettingsStorage.h` | Load/save settings in ESP32 Preferences NVS |
| `OledDisplay` | `OledDisplay.h/.cpp` | OLED screens using U8g2 |
| `RotaryEncoder` | `RotaryEncoder.h` | ESP32_Knob input adapter |
| `Button` | `Button.h` | Active-low button event detection |
| `AppTypes` | `AppTypes.h` | Operating mode and menu item enums |
| `Display` | `Display.h` | UI abstraction |

The output abstraction allows protocol drivers to be changed independently
from the UI and menu state. In the current implementation, however, the app
uses `setRaw()` directly; `setThrottle()` exists on every adapter but is not
called by `ServoTesterApp`.

## Startup and main loop

At startup, `ServoTesterApp::begin()` loads settings from the `servo` NVS
namespace, clamps them, initializes the encoder and button, constructs the
selected output adapter, initializes the OLED, and shows the boot screen for
800 ms.

The Arduino loop calls `ServoTesterApp::loop()`. Each iteration reads encoder
movement, polls the button, advances Sweep if active, renders the current
screen, and delays for 20 ms. Sweep uses elapsed `millis()` time, so update
timing may be delayed by loop and display work.

## Input and menus

`RotaryEncoder::readDelta()` returns encoder movement since its previous
read. While the main screen is active, the current mode decides how that
delta is used:

- **Manual:** change the output by `delta * step`.
- **Sweep:** change the interval by `delta` milliseconds.
- **Center:** ignore encoder movement.
- **Step:** change the configured step by `delta`.

A click cycles operating mode, except in Step mode where it advances the
output by one step. Selecting Center commands the midpoint of the configured
min/max values. Selecting Sweep resets its direction to increasing.

The button is active-low on GPIO 3. `Button::poll()` reports a click on release
after a press shorter than 500 ms, and a long press after the held duration
exceeds 700 ms. Durations from 500 through 700 ms produce no event.

A long press on the running screen opens the main menu. In Settings, encoder
rotation edits the selected item; a click advances to the next item. Protocol
edits immediately replace and initialize the output adapter. **Save** writes
all values to NVS and returns to the main menu; **Back** and long press leave
without saving.

The OLED settings row labels Protocol as `Proto` and shows a compact protocol
name. Keep the label array and the `SettingsItem` ordering aligned when
adding or reordering settings.

## Settings and persistence

The `Settings` defaults and startup/storage clamps are:

| Setting | Default | Clamp |
| --- | ---: | ---: |
| `minPulse` | 1000 | 500–1500 |
| `maxPulse` | 2000 | 1500–2500 |
| `step` | 10 | 1–50 |
| `sweepSpeed` | 20 | 5–100 ms |
| `minAngle` | 0 | 0–180 |
| `maxAngle` | 120 | 90–360 |
| `protocol` | PWM | PWM, OneShot125, OneShot42, Multishot, DShot150/300/600/1200 |

Menu pulse limits are edited in 10-unit increments; other numeric values use
1-unit increments. `clampAll()` clamps pulse and angle endpoints
independently, and does not ensure the min/max values differ. Angle
recalculation assumes a nonzero angle span, so avoid setting `maxAngle` equal
to `minAngle`.

`NvsSettingsStorage` uses ESP32 Preferences namespace `servo`. It stores each
setting by key, including the integer value of the protocol enum. Missing keys
use the in-code defaults. Invalid protocol values are reset to PWM by
`Settings::clampAll()`.

## Output protocols and current limits

The selected adapter is built in `ServoTesterApp::rebuildEscOutput()` and
receives the configured pulse limits in `begin()`. The adapter implementations
currently use these native domains:

| Protocol | Adapter native range | Implementation |
| --- | --- | --- |
| PWM | Configured min/max microseconds | ESP32Servo at 50 Hz |
| OneShot125 | 125–250 µs | LEDC at 2 kHz |
| OneShot42 | 42–84 µs | LEDC at 2 kHz |
| Multishot | 5–25 µs | LEDC at 32 kHz |
| DShot150/300/600/1200 | Command values 48–2047 | ESP32_DSHOT |

**Known limitation:** The app clamps values to `minPulse..maxPulse` before
calling `setRaw()`. It does not convert those configured values to the
selected adapter's native domain. Consequently, OneShot and Multishot values
are clamped by their adapters, and DShot gets the app-limited command range
rather than a percentage mapped across its full range. The OLED labels the
displayed number `P` even when the value is not a pulse width. Treat
non-PWM operation as experimental; remove propellers and mechanical loads
while validating signal behavior.

The DShot adapter calls the library's `arm()` during initialization and then
sends the minimum command value. Hardware compatibility and arming behavior
depend on the ESC and library.

## Sweep and displayed angle

Sweep increments or decrements the current adapter raw value by `step`, at
the configured interval, and reverses at the configured min/max endpoints.
It has no acceleration or ramping. Since the app works on raw values, these
limits and step values should not be assumed to have equivalent meaning
across protocols.

`angleFromPulse()` uses Arduino `map()` to map the adapter's `getRaw()` value
from configured `minPulse..maxPulse` to `0..maxAngle`. `minAngle` is not used
in this display calculation. The result is a command-derived display value,
not measured position feedback.

## Extending the firmware

### Add an output protocol

1. Add the protocol enum entry and full display name in `EscProtocol.h`.
2. Implement `IEscOutput` in a new adapter under `src/`.
3. Add a compact choice label to the settings rendering in `OledDisplay.cpp`.
4. Construct the adapter in `ServoTesterApp::rebuildEscOutput()`.
5. Define and test how app-level limits, steps, and displayed units map to
   the adapter's native domain. Do not rely on independent adapter clamping
   as a substitute for this mapping.
6. Add the required library to `platformio.ini` and update user-facing docs.

### Add a setting

1. Add a default and clamp in `Settings.h`.
2. Add a `SettingsItem` in `AppTypes.h`.
3. Add encoder edit handling in `ServoTesterApp::handleMenuEncoder()`.
4. Add its label and display value in `OledDisplay.cpp`.
5. Add NVS load/save in `SettingsStorage.h`.
6. Update the manual and README, including the setting's units and limits.

### Add an operating mode

1. Add a value to `Mode` in `AppTypes.h`.
2. Update the mode cycle and running-mode input behavior in
   `ServoTesterApp.cpp`.
3. Update menu selection/rendering and the OLED running label if needed.
4. Update the user manual and diagrams.

## Build and verification

Build with PlatformIO:

```sh
pio run
```

Upload and monitor:

```sh
pio run --target upload
pio device monitor
```

The configured environment is `esp32-c3-devkitm-1`; serial is 115200 baud.
The project currently has no automated unit-test suite. Verify changes with a
build and hardware checks appropriate to the selected signal protocol.

## Hardware checks

- Verify GPIO assignments and the selected protocol against the target servo
  or ESC documentation.
- Use a suitable supply and common ground where required; GPIO 4 is signal
  only.
- Remove propellers or disconnect mechanical loads during ESC testing.
- Check OLED I2C wiring on GPIO 5/6 at 400 kHz.
- If encoder direction is reversed, check the GPIO 0/1 phase assignments.

## Dependencies

PlatformIO manages U8g2, ESP32Servo, ESP32_Knob, and ESP32_DSHOT. Dependency
versions and URLs are declared in `platformio.ini`.
