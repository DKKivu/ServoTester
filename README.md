# ServoTester

**ServoTester** is a professional-grade firmware application for the ESP32-C3 DevKitM-1 microcontroller designed to test and calibrate hobby servo motors. It provides intuitive manual and automated control modes, persistent configuration, and a real-time display interface.

### Quick Start
- **Users**: See [USER_MANUAL.md](USER_MANUAL.md) for operating instructions
- **Developers**: See [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md) for architecture and code organization
- **Architecture**: See [DIAGRAMS.md](DIAGRAMS.md) for wiring, class, and workflow diagrams

## Overview

The firmware drives a hobby servo with configurable pulse limits across four control modes:
- **Manual**: Direct encoder control of pulse width
- **Sweep**: Automated triangular wave between min/max pulse
- **Center**: Move to midpoint of configured range
- **Step**: Increment pulse by fixed steps

A rotary encoder and push button provide input control; a 72×40 pixel OLED displays the current mode, pulse width in microseconds, and estimated servo angle. All settings are persisted to the ESP32's non-volatile storage.

**Important**: The displayed angle is a linear mapping from pulse width and is **not** based on servo position feedback. It's a display convenience for calibration.

## Hardware Pins

| Connection | ESP32-C3 GPIO | Notes |
| --- | ---: | --- |
| Servo signal | 4 | PWM output; connect to servo signal pin |
| Encoder A (CLK) | 0 | Phase A from rotary encoder |
| Encoder B (DT) | 1 | Phase B from rotary encoder |
| Encoder button | 3 | Active-low with internal pull-up |
| OLED SDA | 5 | I2C data; check OLED voltage requirements |
| OLED SCL | 6 | I2C clock; 400 kHz hardware I2C |

**Power Wiring**: The firmware does not configure power. Use a servo supply suitable for your motor; if separate from ESP32 power, connect all grounds together. Verify OLED power requirements (typically 3.3V or 5V) before connecting.

**Button Behavior**: The encoder push button is read as active-low with the ESP32's internal pull-up enabled. Button debouncing is handled by the firmware.

## Operating Modes

ServoTester provides four distinct control modes, selected via the rotary encoder and activated by short-press:

| Mode | Encoder Behavior | Button Behavior |
| --- | --- | --- |
| **Manual** | Adjust pulse width by step size (default 10 µs) | Cycle to next mode |
| **Sweep** | Adjust sweep interval (5–100 ms) | Cycle to next mode |
| **Center** | No effect | Cycle to next mode |
| **Step** | Adjust step size (1–50 µs) | Advance pulse by one step |

Mode cycle: **Manual → Sweep → Center → Step → Manual**

For detailed mode descriptions and usage, see [USER_MANUAL.md](USER_MANUAL.md).

## Configurable Settings

The firmware provides six user-adjustable settings, all stored in ESP32 non-volatile storage (NVS):

| Setting | Default | Range | Unit | Description |
| --- | ---: | ---: | --- | --- |
| Minimum pulse | 500 | 50–1500 | µs | Servo pulse width at 0° |
| Maximum pulse | 2500 | 1500–2500 | µs | Servo pulse width at max angle |
| Step size | 10 | 1–50 | µs | Pulse increment per encoder detent |
| Sweep interval | 20 | 5–100 | ms | Time between sweep steps |
| Display min angle | 0 | 0–180 | ° | Lower bound of angle display scale |
| Display max angle | 180 | 90–360 | ° | Upper bound of angle display scale |

**Calibration**: The pulse and angle ranges are independent; you can configure the display scale separately from actual servo pulse limits. This is useful for testing non-standard servo ranges or mapping custom servo linkages.

**Persistence**: Changes are live immediately but are only written to NVS when you select **Save** in the settings menu. Selecting **Back** or long-pressing returns to the main menu without saving, allowing safe experimentation.

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
- **Libraries**: U8g2, ESP32Servo, ESP32_Knob

To customize GPIO pins, see `src/main.cpp` and modify the pin constants.

## Project Structure

### Source Files

| File | Purpose |
| --- | --- |
| `src/main.cpp` | **Composition root**: GPIO pin definitions, dependency instantiation, Arduino setup/loop |
| `src/ServoTesterApp.{h,cpp}` | **Core application**: State machines, input handling, mode logic, sweep control, render dispatch |
| `src/OledDisplay.{h,cpp}` | **UI rendering**: OLED display screens using U8g2 library |
| `src/RotaryEncoder.h` | **Input adapter**: Wrapper for ESP32_Knob library; detects encoder rotation |
| `src/Button.h` | **Input handler**: Debouncing and click/long-press detection for active-low button |
| `src/ServoOutput.h` | **Output driver**: PWM servo control via ESP32Servo library |
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

### Angle Calculation

The displayed servo angle is calculated from the pulse width using a **linear mapping**:

```
angle = (pulse - minPulse) / (maxPulse - minPulse) × maxAngle
```

**Important**: This is a display convenience and **not** based on actual servo position feedback. The firmware sends pulse commands but does not read servo position. Actual servo travel depends on:
- Servo linkage geometry
- Servo mechanical limits
- Physical load on the servo
- Servo calibration and center point

Use the angle display as a reference during calibration, but verify actual servo behavior independently.

### Sweep Algorithm

The Sweep mode implements a simple **triangle wave oscillator**:
1. Starts in the "up" direction
2. Increments pulse by the step size at each interval
3. Reverses direction at min/max pulse limits
4. Continues indefinitely

Sweep does not include velocity ramping or acceleration—it's pure rectangular steps. This is intentional for testing servo response to discrete pulse commands.

### Settings Persistence

All user settings are stored in the ESP32's **non-volatile storage (NVS)** under the namespace `servo`. This means:
- Settings survive power cycles and firmware updates
- Multiple applications on the same device can use different NVS namespaces
- Settings are only written when user selects **Save** in the settings menu
- Unsaved changes are live but are lost on reset

### Hardware Design Notes

- **Servo Supply**: Use a supply capable of delivering the servo's peak current. Do not share with ESP32 USB power if current exceeds ~500 mA.
- **Ground Connection**: All grounds (ESP32, OLED, servo) must be common.
- **Button Polarity**: The button pin (GPIO 3) is configured as active-low (pulled up internally). The button must pull GPIO 3 to ground when pressed.
- **Encoder Polarity**: Encoder phases A and B (GPIO 0/1) should follow standard quadrature encoding. If rotation direction is reversed, swap the GPIO assignments in `src/main.cpp`.
- **OLED Communication**: I2C runs at 400 kHz on GPIO 5/6. Some OLED modules may require external pull-ups if I2C communication is unreliable.

### Dependencies

| Library | Version | License | Purpose |
| --- | --- | --- | --- |
| **U8g2** | ^2.35.30 | BSD | Monochrome OLED graphics and rendering |
| **ESP32Servo** | ^3.0.5 | LGPL | PWM servo signal generation and control |
| **ESP32_Knob** | (git) | MIT | Rotary encoder reading and debouncing |

All dependencies are managed by PlatformIO and are automatically fetched during build.

## Contributing

Contributions are welcome! See [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md) for:
- Architecture and design patterns
- Code style and conventions
- How to add features or fix bugs
- Testing recommendations

## License

This project is released under the [MIT License](LICENSE).
