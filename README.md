# ServoTester

**ServoTester** is firmware for the ESP32-C3 DevKitM-1 that generates control signals for hobby servos and compatible ESCs. It provides manual and automated output modes, a rotary-encoder interface, an OLED display, and persistent configuration.

### Quick Start
- **Users**: See [USER_MANUAL.md](USER_MANUAL.md) for operating instructions
- **Developers**: See [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md) for architecture and code organization
- **Architecture**: See [DIAGRAMS.md](DIAGRAMS.md) for wiring, class, and workflow diagrams

## Overview

Four operating modes control the selected output:
- **Manual**: Adjust the output value with the encoder
- **Sweep**: Repeatedly move between configured limits
- **Center**: Move to 50% of the normalized output range
- **Step**: Increment the output by one configured step on each click

The output protocol is configurable: PWM, OneShot125, OneShot42, Multishot,
and DShot150/300/600/1200. PWM is the default.

A rotary encoder and push button provide input; a 72×40 pixel OLED shows the
current mode and protocol, plus protocol-appropriate output details. Settings,
including the selected protocol, are stored in ESP32 non-volatile storage
(NVS) when saved.

**Important:** The angle is a display scale from `minAngle` to `maxAngle`,
based on normalized output. It is not measured servo or motor position
feedback.

## Hardware Pins

| Connection | ESP32-C3 GPIO | Notes |
| --- | ---: | --- |
| Servo/ESC signal | 4 | Output signal; connect to compatible signal input |
| Encoder A (CLK) | 0 | Phase A from rotary encoder |
| Encoder B (DT) | 1 | Phase B from rotary encoder |
| Encoder button | 3 | Active-low with internal pull-up |
| OLED SDA | 5 | I2C data; check OLED voltage requirements |
| OLED SCL | 6 | I2C clock; 400 kHz hardware I2C |

**Power and safety:** GPIO 4 is a signal output, not a power supply. Follow the
servo/ESC manufacturer's wiring and voltage requirements and connect grounds
as required. Remove propellers or disconnect mechanical loads before testing
ESCs. The firmware does not measure motor speed, position, or current.

**Button Behavior**: The encoder push button is read as active-low with the ESP32's internal pull-up enabled. Button debouncing is handled by the firmware.

## Operating Modes

ServoTester provides four distinct control modes, selected via the rotary encoder and activated by short-press:

| Mode | Encoder Behavior | Button Behavior |
| --- | --- | --- |
| **Manual** | Adjust output by configured step | Cycle to next mode |
| **Sweep** | Adjust sweep interval (5–100 ms) | Cycle to next mode |
| **Center** | No effect | Cycle to next mode |
| **Step** | Adjust PWM step or non-PWM percentage step | Advance output by one step |

Mode cycle: **Manual → Sweep → Center → Step → Manual**. PWM step is
configured in microseconds; non-PWM step is configured as a percentage of the
normalized output range.

For detailed mode descriptions and usage, see [USER_MANUAL.md](USER_MANUAL.md).

## Configurable Settings

The firmware provides eight user-adjustable settings, stored in ESP32
non-volatile storage (NVS):

| Setting | Default | Range | Unit | Description |
| --- | ---: | ---: | --- | --- |
| Minimum pulse | 1000 | 500–1500 | µs | PWM lower pulse limit |
| Maximum pulse | 2000 | 1500–2500 | µs | PWM upper pulse limit |
| PWM step size | 10 | 1–50 | µs | PWM output increment |
| Non-PWM step | 5 | 1–20 | % | Normalized output increment |
| Sweep interval | 20 | 5–100 | ms | Time between sweep steps |
| Display min angle | 0 | 0–180 | ° | Lower angle setting |
| Display max angle | 120 | 90–360 | ° | Upper angle scale value |
| Output protocol | PWM | 8 choices | — | Signal-generation protocol |

Manual, Sweep, Center, and Step operate on a normalized 0–1 output position;
each adapter converts it to its protocol's native range. The PWM pulse limits
apply only to PWM. OneShot125, OneShot42, and Multishot use their fixed native
pulse ranges, while DShot maps to command values. Confirm the selected ESC
supports the chosen protocol and test with propellers and mechanical loads
removed.

The settings screen displays the protocol name. Selecting a non-PWM protocol
sets its normalized output to zero; PWM starts at its midpoint on boot.
Protocol edits take effect immediately and persist when you select **Save**.

Changes are live immediately but are written to NVS only when you select
**Save** in the settings menu. **Back** and long-press leave without saving.

## Building, Uploading & Monitoring

ServoTester uses **PlatformIO** for build automation and library management. PlatformIO integrates with VS Code, command line, and other IDEs.

### Installation
1. Install [VS Code](https://code.visualstudio.com/)
2. Install the [PlatformIO extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide)
3. Open this project in VS Code

### Build Commands

From the project directory:

```bash
# Compile the firmware
pio run

# Compile and upload to connected ESP32-C3
pio run --target upload

# Start serial monitor (115200 baud)
pio device monitor

# Clean build artifacts
pio run --target clean
```

### VS Code Integration
- **PlatformIO Home**: Click the PlatformIO icon in the activity bar to access the UI
- **Build**: Click the checkmark icon or run `pio run`
- **Upload**: Click the arrow icon or run `pio run --target upload`
- **Monitor**: Click the plug icon or run `pio device monitor`

### Configuration

All build settings are defined in `platformio.ini`:
- **Board**: `esp32-c3-devkitm-1`
- **Framework**: Arduino
- **Compiler**: GCC with C++17 support
- **Serial Monitor Speed**: 115200 baud
- **Libraries**: U8g2, ESP32Servo, ESP32_Knob, ESP32_DSHOT

To customize GPIO pins, see `src/main.cpp` and modify the pin constants.

## Project Structure

### Source Files

| File | Purpose |
| --- | --- |
| `src/main.cpp` | **Composition root**: GPIO pin definitions, dependency instantiation, Arduino setup/loop |
| `src/ServoTesterApp.{h,cpp}` | **Core application**: State machines, input handling, mode logic, sweep control, output adapter selection |
| `src/OledDisplay.{h,cpp}` | **UI rendering**: OLED display screens using U8g2 library |
| `src/RotaryEncoder.h` | **Input adapter**: Wrapper for ESP32_Knob library; detects encoder rotation |
| `src/Button.h` | **Input handler**: Debouncing and click/long-press detection for active-low button |
| `src/IEscOutput.h` | **Output interface**: Common ESC/servo output contract |
| `src/*EscOutput.h` | **Output adapters**: PWM, OneShot, Multishot, and DShot implementations |
| `src/EscProtocol.h` | **Protocol selection**: Protocol enum, display names, cycling |
| `src/SettingsStorage.h` | **Persistence interface**: Abstraction for loading/saving settings |
| `src/NvsSettingsStorage` (in SettingsStorage.h) | **Persistence implementation**: NVS-based storage using ESP32 Preferences API |
| `src/Settings.h` | **Configuration**: Settings data structure with range validation |
| `src/AppTypes.h` | **Enumerations**: Mode, MenuState, ButtonEvent, MainMenuItem, SettingsItem |
| `src/Display.h` | **Display interface**: Abstract base class defining UI contract |

### Documentation Files

| File | Audience | Content |
| --- | --- | --- |
| [README.md](README.md) | Everyone | Overview, features, quick start, build instructions |
| [USER_MANUAL.md](USER_MANUAL.md) | End users | Operating instructions, wiring, startup, menu navigation |
| [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md) | Contributors | Architecture, design patterns, code organization, testing |
| [DIAGRAMS.md](DIAGRAMS.md) | Everyone | Wiring schematic, UML class diagram, application workflow |

## Technical Notes

### Normalized output and angle display

The app stores output as a normalized value from 0.0 to 1.0. Each adapter
maps this value to its protocol-specific domain. The OLED shows PWM pulse
width and angle for PWM; for other protocols it shows throttle percentage
and raw output value.

The angle maps normalized output to `minAngle..maxAngle`. It is a display
scale, not position feedback. Changing `maxPulse` or `maxAngle` attempts to
preserve the current pulse-to-angle ratio; a change is rejected if the paired
value would exceed its allowed range.

The output adapters map normalized output to these native ranges: PWM uses
the configured pulse limits; OneShot125 uses 125–250 µs; OneShot42 uses
42–84 µs; Multishot uses 5–25 µs; DShot uses commands 48–2047.

### Dependencies

| Library | Version | License | Purpose |
| --- | --- | --- | --- |
| **U8g2** | ^2.35.30 | BSD | Monochrome OLED graphics and rendering |
| **ESP32Servo** | ^3.0.5 | LGPL | PWM servo signal generation and control |
| **ESP32_Knob** | (git) | MIT | Rotary encoder reading and debouncing |
| **ESP32_DSHOT** | (git) | — | DShot signal generation |

All dependencies are managed by PlatformIO and are automatically fetched during build.

## Contributing

Contributions are welcome! See [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md) for:
- Architecture and design patterns
- Code style and conventions
- How to add features or fix bugs
- Testing recommendations

## License

This project is released under the [MIT License](LICENSE).
