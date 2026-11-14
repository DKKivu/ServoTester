# ServoTester Developer Guide

This guide provides an in-depth explanation of the firmware architecture, code organization, and development practices for contributors and maintainers.

## Architecture Overview

ServoTester follows a **clean architecture** with clear separation of concerns. The main application logic (`ServoTesterApp`) is decoupled from hardware specifics through abstraction layers and dependency injection.

### Dependency Injection Pattern

All major components are instantiated in `src/main.cpp` and injected into `ServoTesterApp`:

```cpp
static NvsSettingsStorage storage;  // Loads/saves settings from NVS
static OledDisplay        display;   // Renders UI to OLED
static RotaryEncoder      encoder;   // Reads encoder rotations
static Button             button;    // Detects clicks/long-presses
static ServoOutput        servo;     // Controls servo PWM signal

static ServoTesterApp app(storage, display, encoder, button, servo);
```

This approach enables:
- **Testability**: Components can be mocked for unit testing
- **Flexibility**: Easy to swap implementations (e.g., different display types)
- **Modularity**: Each component has a single responsibility

### Component Responsibilities

| Component | File | Responsibility |
|-----------|------|---|
| **ServoTesterApp** | `ServoTesterApp.{h,cpp}` | Central state machine; coordinates input → logic → output |
| **OledDisplay** | `OledDisplay.{h,cpp}` | Renders UI to 72×40 OLED; screen transitions |
| **RotaryEncoder** | `RotaryEncoder.h` | Wrapper around ESP32_Knob; detects rotation direction/speed |
| **Button** | `Button.h` | Debounces active-low button; distinguishes clicks from long-presses |
| **ServoOutput** | `ServoOutput.h` | PWM signal generation; servo pulse control |
| **SettingsStorage** | `SettingsStorage.h` | Interface for persistent settings (NVS implementation provided) |
| **Settings** | `Settings.h` | Data structure with range validation |
| **AppTypes** | `AppTypes.h` | Enumerations for mode, menu state, and button events |

## State Management

ServoTesterApp maintains three primary state variables:

### 1. **Current Mode** (`_mode`)
```cpp
enum class Mode { Manual, Sweep, Center, Step }
```
Controls how encoder input is interpreted:
- **Manual**: Adjust pulse width
- **Sweep**: Adjust sweep interval
- **Center**: No encoder input (moves to midpoint)
- **Step**: Adjust step size

### 2. **Menu State** (`_menuState`)
```cpp
enum class MenuState { Off, Main, Settings }
```
Determines which input handler processes encoder/button:
- **Off**: Running mode active (not in menu)
- **Main**: Mode selection menu
- **Settings**: Settings edit menu

### 3. **Settings** (`_settings`)
Live configuration that affects servo behavior. Persisted to NVS when user selects "Save".

### 4. **Sweep State** (`_sweepUp`, `_lastSweep`)
Tracks sweep direction and timing for automatic pulse iteration.

## Input Handling

Input processing follows a pipeline: encoder → button → logic → servo update.

### Encoder Reading
The `RotaryEncoder` adapter calls the underlying ESP32_Knob library:
```cpp
int delta = _encoder.readDelta();  // Returns cumulative rotation since last read
```
Deltas are typically ±1 per detent and are multiplied by the configured step size when in Manual mode.

### Button Processing
The `Button` class performs debouncing and detects two event types:

```cpp
enum class ButtonEvent { None, Click, LongPress }
```

- **Click** (press < 500 ms): Cycles mode or advances pulse (context-dependent)
- **LongPress** (hold ≥ 700 ms): Enters/exits menu

The button is configured as active-low with internal pull-up on GPIO 3.

### Input Dispatch
`ServoTesterApp::handleEncoder()` routes to either:
- `handleRunningEncoder()` when menu is off (affects pulse, interval, step, or does nothing)
- `handleMenuEncoder()` when menu is open (moves selection or edits numeric setting)

## Rendering Pipeline

The application renders the current state at each 20 ms loop iteration via `render()`:

```cpp
void ServoTesterApp::render() {
    if (_menuState == MenuState::Off) {
        int angleDeg = angleFromPulse();
        _display.showRunning(_mode, _servo.getPulse(), angleDeg);
    } else if (_menuState == MenuState::Main) {
        _display.showMainMenu(_menuIndex);
    } else if (_menuState == MenuState::Settings) {
        _display.showSettingsMenu(_settings, _menuIndex);
    }
}
```

### Display Interface
The abstract `Display` interface defines what screens the UI must support:

```cpp
class Display {
    virtual void showBoot() = 0;
    virtual void showRunning(Mode mode, int pulseUs, int angleDeg) = 0;
    virtual void showMainMenu(int selected) = 0;
    virtual void showSettingsMenu(const Settings& s, int selected) = 0;
};
```

`OledDisplay` implements this interface using U8g2 graphics library.

## Settings & Persistence

### Settings Structure
Located in `Settings.h`, with validation ranges:

```cpp
struct Settings {
    int minPulse   = 500;      // 50–1500 µs
    int maxPulse   = 2500;     // 1500–2500 µs
    int step       = 10;       // 1–50 µs
    int sweepSpeed = 20;       // 5–100 ms
    int minAngle   = 0;        // 0–180°
    int maxAngle   = 180;      // 90–360°
    
    void clampAll() { /* validates and constrains all ranges */ }
};
```

### NVS Persistence
`NvsSettingsStorage` uses ESP32's Preferences library to store/load from the "servo" namespace:

```cpp
class NvsSettingsStorage : public SettingsStorage {
    void load(Settings& s) override {
        Preferences prefs;
        prefs.begin("servo");
        s.minPulse = prefs.getInt("minPulse", 500);
        // ... other fields
        prefs.end();
    }
    
    void save(const Settings& s) override {
        Preferences prefs;
        prefs.begin("servo");
        prefs.putInt("minPulse", s.minPulse);
        // ... other fields
        prefs.end();
    }
};
```

### Save Behavior
- Settings are loaded at startup via `app.begin()`
- User changes in the settings menu are **live** but NOT persisted
- Only selecting **Save** in settings menu writes to NVS
- **Back** or long-press returns to menu without saving
- This allows users to experiment without permanent changes

## Sweep Logic

The `updateSweep()` function implements automatic pulse iteration:

```cpp
void ServoTesterApp::updateSweep(unsigned long now) {
    if (_mode != Mode::Sweep || _menuState != MenuState::Off) return;
    
    if (now - _lastSweep >= (unsigned)_settings.sweepSpeed) {
        int pulse = _servo.getPulse();
        if (_sweepUp) {
            pulse += _settings.step;
            if (pulse >= _settings.maxPulse) {
                pulse = _settings.maxPulse;
                _sweepUp = false;
            }
        } else {
            pulse -= _settings.step;
            if (pulse <= _settings.minPulse) {
                pulse = _settings.minPulse;
                _sweepUp = true;
            }
        }
        _servo.setPulse(pulse);
        _lastSweep = now;
    }
}
```

Key points:
- Implemented as a **triangle wave** oscillator
- Reverses direction instantly at endpoints (no deceleration)
- Interval is configurable via encoder in Sweep mode
- Only runs when menu is off and mode is Sweep

## Angle Calculation

The displayed angle is a **linear mapping** from pulse width, not actual servo position feedback:

```cpp
int ServoTesterApp::angleFromPulse() const {
    int pulse = _servo.getPulse();
    if (_settings.maxPulse == _settings.minPulse) return 0;
    
    int numerator = (pulse - _settings.minPulse) * _settings.maxAngle;
    return numerator / (_settings.maxPulse - _settings.minPulse);
}
```

Formula:
```
angle = (pulse - minPulse) / (maxPulse - minPulse) * maxAngle
```

This is a **display convenience** and does not represent actual servo position. The firmware sends the pulse command but does not read feedback.

## Synchronization Logic

When the user edits MinPulse, MaxPulse, MinAngle, or MaxAngle, the firmware maintains consistency between pulse-space and angle-space:

- **`recalculateFromMaxPulse()`**: Called when pulse limits change; recomputes angle display
- **`recalculateFromMaxAngle()`**: Called when angle limits change; optionally adjusts pulse range

(Full implementation in `ServoTesterApp.cpp`)

## Loop Timing

The main loop runs at a fixed 20 ms tick:

```cpp
void ServoTesterApp::loop() {
    const unsigned long now = millis();
    
    const int delta = _encoder.readDelta();
    if (delta != 0) handleEncoder(delta);
    
    const ButtonEvent ev = _button.poll(now);
    if (ev != ButtonEvent::None) handleButton(ev);
    
    updateSweep(now);
    render();
    
    delay(20);  // Fixed 20 ms tick
}
```

- No jitter compensation (acceptable for this application)
- Input processing is non-blocking
- Display updates occur every tick
- Sweep timing is based on elapsed time, not loop count (resilient to timing jitter)

## GPIO Pin Assignment

All pins are defined as compile-time constants in `main.cpp`:

```cpp
constexpr uint8_t PIN_SERVO    = 4;   // PWM servo output
constexpr uint8_t PIN_ENC_CLK  = 0;   // Encoder phase A
constexpr uint8_t PIN_ENC_DT   = 1;   // Encoder phase B
constexpr uint8_t PIN_ENC_SW   = 3;   // Encoder push button (active-low)
constexpr uint8_t PIN_OLED_SDA = 5;   // I2C data
constexpr uint8_t PIN_OLED_SCL = 6;   // I2C clock
```

To reconfigure pins, modify these constants and recompile.

## Building & Debugging

### Building
```bash
pio run
```

### Uploading
```bash
pio run --target upload
```

### Serial Monitoring
```bash
pio device monitor
```

Serial output is configured for 115200 baud via the Arduino Serial API. Currently, no debug output is generated; you can add Serial.print() calls for troubleshooting.

### Build Configuration
`platformio.ini` specifies:
- Board: `esp32-c3-devkitm-1`
- Framework: Arduino
- Libraries: U8g2, ESP32Servo, ESP32_Knob
- Compiler flags: `-std=gnu++17` (C++17 enabled)

## Adding Features

### Adding a New Mode
1. Add enum value to `Mode` in `AppTypes.h`
2. Add menu entry to `MainMenuItem` if desired
3. Handle encoder input in `ServoTesterApp::handleRunningEncoder()`
4. Handle button input in `ServoTesterApp::handleMainMenuClick()` (if mode has special behavior)
5. Add display logic in `ServoTesterApp::render()`

### Adding a New Setting
1. Add field to `Settings` struct with default and range
2. Add `clampAll()` constraint
3. Add `SettingsItem` enum value in `AppTypes.h`
4. Handle encoder in `ServoTesterApp::handleMenuEncoder()`
5. Add menu display entry in `OledDisplay::showSettingsMenu()`
6. Add NVS load/save in `NvsSettingsStorage`

### Adding a New Display
1. Create a new class implementing the `Display` interface
2. Instantiate in `main.cpp` and pass to `ServoTesterApp` constructor
3. Implement required methods: `begin()`, `showBoot()`, `showRunning()`, `showMainMenu()`, `showSettingsMenu()`

## Testing Recommendations

### Unit Tests
- Test `Settings::clampAll()` with edge cases
- Test `angleFromPulse()` with various pulse ranges
- Mock `Display`, `SettingsStorage`, `ServoOutput` to test state transitions

### Integration Tests
- Verify mode cycle: Manual → Sweep → Center → Step → Manual
- Verify menu navigation and persistence
- Verify sweep timing accuracy
- Verify angle calculation across different pulse ranges

### Hardware Tests
- Verify I2C communication with OLED on startup
- Verify servo PWM output on GPIO 4
- Verify encoder A/B phase relationship
- Verify button debounce behavior

## Common Issues & Troubleshooting

| Issue | Likely Cause | Solution |
|-------|------|------|
| OLED blank on startup | I2C address mismatch | Verify SDA/SCL GPIO; check U8g2 constructor in OledDisplay |
| Servo doesn't move | GPIO 4 not configured for PWM | Verify ServoOutput initialization |
| Encoder unresponsive | GPIO 0/1 not detected | Check ESP32_Knob setup; verify encoder polarity |
| Button doesn't work | GPIO 3 pull-up not enabled | Verify Button::begin() configures INPUT_PULLUP |
| Settings not persisting | NVS partition too small | Check ESP32 partition table; verify Preferences namespace |
| Jittery servo motion | Loop timing not stable | Add telemetry output; profile loop timing with Serial |

## Future Improvements

- [ ] Add Serial debug output for troubleshooting
- [ ] Implement adaptive loop timing with jitter compensation
- [ ] Add unit tests for core logic
- [ ] Support servo position feedback (requires hardware changes)
- [ ] Add calibration mode for servo limits
- [ ] Implement preset profiles (save multiple configurations)
- [ ] Add external control interface (e.g., Bluetooth LE)
- [ ] Improve error handling for I2C/initialization failures

## References

- [ESP32-C3 DevKitM-1 Datasheet](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/)
- [U8g2 Library Documentation](https://github.com/olikraus/u8g2/wiki)
- [ESP32Servo Library](https://github.com/madhephaestus/ESP32Servo)
- [ESP32_Knob Library](https://github.com/esp-arduino-libs/ESP32_Knob)
