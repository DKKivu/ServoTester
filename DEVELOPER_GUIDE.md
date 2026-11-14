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
800 ms. `_throttle01` starts at 0.5 for PWM and 0.0 for non-PWM protocols.
When the selected protocol changes to a non-PWM protocol, the app resets the
normalized output to 0.0 before rebuilding the adapter.

The Arduino loop calls `ServoTesterApp::loop()`. Each iteration reads encoder
movement, polls the button, advances Sweep if active, renders the current
screen, and delays for 20 ms. Sweep uses elapsed `millis()` time, so update
timing may be delayed by loop and display work.

## Input and menus

`RotaryEncoder::readDelta()` returns encoder movement since its previous
read. While the main screen is active, the current mode decides how that
delta is used:

- **Manual:** change normalized output by `delta * stepNormalized()`.
- **Sweep:** change the interval by `delta` milliseconds.
- **Center:** ignore encoder movement.
- **Step:** change `step` by `delta` for PWM, or `stepPercent` for non-PWM.

A click cycles operating mode, except in Step mode where it advances the
normalized output by one step. Selecting Center commands 0.5 (50%) for every
protocol. Selecting Sweep resets its direction to increasing.

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
| `stepPercent` | 5 | 1–20% |
| `sweepSpeed` | 20 | 5–100 ms |
| `minAngle` | 0 | 0–180 |
| `maxAngle` | 120 | 90–360 |
| `protocol` | PWM | PWM, OneShot125, OneShot42, Multishot, DShot150/300/600/1200 |

Menu pulse limits are edited in 10-unit increments; other numeric values use
1-unit increments. `step` is used by PWM in microseconds, while
`stepPercent` is used for non-PWM protocols. Pulse limits and angle endpoints
are clamped independently. Avoid setting `minAngle` equal to or above
`maxAngle`.

`NvsSettingsStorage` uses ESP32 Preferences namespace `servo`. It stores each
setting by key, including the integer value of the protocol enum. Missing keys
use the in-code defaults. Invalid protocol values are reset to PWM by
`Settings::clampAll()`.

## Output protocols and current limits

`ServoTesterApp::rebuildEscOutput()` constructs the selected adapter and
passes the configured pulse limits to `begin()`; non-PWM adapters use their
own fixed native ranges.

`ServoTesterApp` controls adapters through `setNormalized()` and
`getNormalized()`, using a normalized range of 0.0–1.0 regardless of protocol.
The adapters map this to their native values:

| Adapter | Native range |
| --- | --- |
| PWM | Configured min/max microseconds |
| OneShot125 | 125–250 µs |
| OneShot42 | 42–84 µs |
| Multishot | 5–25 µs |
| DShot150/300/600/1200 | Command values 48–2047 |

`setRaw()` writes a protocol-native value; the app instead controls output
through normalized values. `getRaw()` supplies the native value for display.
`setThrottle()` remains as a compatibility method and is not used by the app.
The OLED shows PWM pulse width and angle; for non-PWM it shows throttle
percentage and raw value.

The DShot adapter requests arming on the first initialization in a runtime,
then sends command 48. Rebuilding the adapter does not request arming again
unless `DShotEscOutput::requestRearm()` is called. The app does not currently
call that method; ESC-specific startup behavior must be validated on hardware.

## Sweep and displayed angle

Sweep increments or decrements `_throttle01` by `stepNormalized()` at the
configured interval and reverses at 0.0/1.0. PWM's normalized increment is
`step / (maxPulse - minPulse)`; non-PWM uses `stepPercent / 100`. There is no
acceleration or ramping.

`angleFromThrottle()` maps `_throttle01` to `minAngle..maxAngle` with rounding.
This is a display scale, not position feedback.

When `maxPulse` changes, `recalculateFromMaxPulse()` computes a paired
`maxAngle` that preserves the previous degrees-per-microsecond ratio; when
`maxAngle` changes, `recalculateFromMaxAngle()` computes a paired `maxPulse`.
Each operation rejects the whole change if the new paired value is outside
its allowed range or the angle span is invalid. Editing `minPulse` or
`minAngle` does not trigger a paired recalculation.

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
6. Update app behavior if it affects output control.
7. Update the manual and README, including the setting's units and limits.

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
