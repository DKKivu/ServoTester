# Servo Tester Diagrams

These diagrams describe the current firmware in `src/`. Mermaid diagrams
render on GitHub.

## Principal signal wiring

Signal pin assignments and the OLED bus clock are defined in `src/main.cpp`
and `src/OledDisplay.cpp`. Power pins are intentionally not specified: supply
requirements depend on the exact board, OLED, and servo. Use a suitable servo
supply; if it is separate, connect its ground to ESP32 ground.

```mermaid
flowchart LR
    subgraph MCU["ESP32-C3 DevKitM-1"]
        P4["GPIO 4"]
        P0["GPIO 0"]
        P1["GPIO 1"]
        P3["GPIO 3<br/>INPUT_PULLUP"]
        P5["GPIO 5"]
        P6["GPIO 6"]
    end

    subgraph SERVO["Hobby servo"]
        SIG["Signal"]
    end
    subgraph ENCODER["Rotary encoder"]
        A["A / CLK"]
        B["B / DT"]
        SW["Push switch"]
    end
    subgraph OLED["SSD1306 72x40 ER OLED<br/>Hardware I2C, 400 kHz"]
        SDA["SDA"]
        SCL["SCL"]
    end

    P4 -->|servo control pulse| SIG
    P0 -->|encoder A| A
    P1 -->|encoder B| B
    P3 -->|active-low button| SW
    P5 -->|I2C data| SDA
    P6 -->|I2C clock| SCL
```

## UML class diagram

`main.cpp` constructs the concrete adapters and injects them into
`ServoTesterApp`. External Arduino and library classes are shown only where
they clarify an adapter's implementation.

```mermaid
classDiagram
    direction LR

    class ServoTesterApp {
        +begin()
        +loop()
        -handleEncoder(delta)
        -handleButton(event)
        -handleRunningEncoder(delta)
        -handleMenuEncoder(delta)
        -handleMainMenuClick()
        -handleSettingsClick()
        -recalculateFromMaxAngle()
        -recalculateFromMaxPulse()
        -updateSweep(now)
        -render()
        -enterMenu()
        -exitMenu()
        -cycleMode()
        -setMode(mode)
        -angleFromPulse()
    }

    class Settings {
        +minPulse: int
        +maxPulse: int
        +step: int
        +sweepSpeed: int
        +minAngle: int
        +maxAngle: int
        +clampAll()
    }

    class SettingsStorage {
        <<interface>>
        +load(settings)
        +save(settings)
    }

    class NvsSettingsStorage {
        -Preferences _prefs
        +load(settings)
        +save(settings)
    }

    class Display {
        <<interface>>
        +begin()
        +showBoot()
        +showRunning(mode, pulseUs, angleDeg)
        +showMainMenu(selected)
        +showSettingsMenu(settings, selected)
    }

    class OledDisplay {
        -U8G2_SSD1306_72X40_ER_F_HW_I2C _u8g2
        +begin()
        +showBoot()
        +showRunning(mode, pulseUs, angleDeg)
        +showMainMenu(selected)
        +showSettingsMenu(settings, selected)
    }

    class RotaryEncoder {
        -ESP_Knob* _knob
        +begin()
        +readDelta()
    }

    class Button {
        +begin()
        +poll(now)
    }

    class ServoOutput {
        +begin(minUs, maxUs)
        +setPulse(us)
        +setPulseClamped(us, minUs, maxUs)
        +getPulse()
    }

    class Mode {
        <<enumeration>>
        Manual
        Sweep
        Center
        Step
    }

    class MenuState {
        <<enumeration>>
        Off
        Main
        Settings
    }

    class ButtonEvent {
        <<enumeration>>
        None
        Click
        LongPress
    }

    class MainMenuItem {
        <<enumeration>>
        Manual
        Sweep
        Center
        Step
        Settings
    }

    class SettingsItem {
        <<enumeration>>
        MinPulse
        MaxPulse
        Step
        SweepSpeed
        MinAngle
        MaxAngle
        Save
        Back
    }

    NvsSettingsStorage ..|> SettingsStorage : implements
    OledDisplay ..|> Display : implements
    ServoTesterApp --> SettingsStorage : uses
    ServoTesterApp --> Display : uses
    ServoTesterApp --> RotaryEncoder : uses
    ServoTesterApp --> Button : uses
    ServoTesterApp --> ServoOutput : uses
    ServoTesterApp *-- Settings : owns runtime settings
    ServoTesterApp --> Mode : current mode
    ServoTesterApp --> MenuState : current menu
    ServoTesterApp ..> MainMenuItem : handles
    ServoTesterApp ..> SettingsItem : handles
    Button ..> ButtonEvent : emits
    SettingsStorage ..> Settings : loads and saves
    Display ..> Settings : displays
```

## Application workflow

```mermaid
flowchart TD
    START([Power on or reset]) --> SETUP["setup(): start Serial at 115200"]
    SETUP --> LOAD["app.begin(): load settings from NVS"]
    LOAD --> CLAMP["Clamp all loaded/default settings"]
    CLAMP --> INIT["Initialize encoder, button, servo, and OLED"]
    INIT --> INITIAL_PULSE["Command servo to 1500 us"]
    INITIAL_PULSE --> BOOT["Show boot screen for 800 ms"]
    BOOT --> LOOP["app.loop()"]

    LOOP --> READ["Read encoder count delta"]
    READ --> HAS_DELTA{"Delta != 0?"}
    HAS_DELTA -- Yes --> MENU_OFF{"Menu state is Off?"}
    MENU_OFF -- Yes --> RUN_EDIT["Apply active mode adjustment"]
    MENU_OFF -- No --> MENU_EDIT["Move selection or edit setting"]
    RUN_EDIT --> POLL["Poll active-low button"]
    MENU_EDIT --> POLL
    HAS_DELTA -- No --> POLL

    POLL --> BUTTON_EVENT{"Click or long press?"}
    BUTTON_EVENT -- Yes --> HANDLE["Handle event for current mode/menu"]
    BUTTON_EVENT -- No --> SWEEP_CHECK
    HANDLE --> SWEEP_CHECK{"Mode is Sweep and menu is Off?"}

    SWEEP_CHECK -- Yes --> ELAPSED{"Sweep interval elapsed?"}
    ELAPSED -- Yes --> SWEEP["Advance pulse by step;<br/>reverse at pulse limits"]
    ELAPSED -- No --> RENDER
    SWEEP --> RENDER["Render running view or active menu"]
    SWEEP_CHECK -- No --> RENDER

    RENDER --> WAIT["delay(20 ms)"]
    WAIT --> LOOP
```

### Input and persistence details

- In Manual mode, encoder rotation changes pulse width by `step`; in Sweep
  mode it changes the interval by 1 ms; in Step mode it changes `step` by 1
  µs; Center ignores encoder rotation.
- A short press cycles mode except in Step mode, where it advances the pulse
  by `step`. Selecting Center commands the pulse-range midpoint. A long press
  opens the main menu from the running screen.
- The main-menu selection wraps. In Settings, a click on a numeric item
  advances to the next item; **Save** persists to NVS, while **Back** and
  long-press return to the main menu without saving.
- Settings defaults are `minPulse=500`, `maxPulse=2500`, `step=10`,
  `sweepSpeed=20`, `minAngle=0`, and `maxAngle=180`.
- Startup/storage clamps are `minPulse=0–1500`, `maxPulse=1500–3500`,
  `step=1–50`, `sweepSpeed=5–100`, `minAngle=0–180`, and `maxAngle=90–360`.
  The interactive pulse-edit ranges are narrower: `minPulse=500–1500` and
  `maxPulse=1500–2500`; all menu adjustments use the increments described in
  [USER_MANUAL.md](USER_MANUAL.md).
- The angle display uses Arduino `map()` to map `minPulse..maxPulse` to
  `0..maxAngle`; `minAngle` is not used by that displayed-angle calculation.
  The pulse/angle settings handlers also call recalculation methods when those
  values are edited. The display is command-derived, not position feedback.
