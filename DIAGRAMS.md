# ServoTester Diagrams

These diagrams describe the current firmware in `src/`. Mermaid diagrams
render on GitHub.

## Principal signal wiring

GPIO assignments are defined in `src/main.cpp`; the OLED bus clock is set in
`src/OledDisplay.cpp`. GPIO 4 is a signal output only. Power wiring depends
on the specific ESP32 board, display, servo, and ESC; observe their manuals
and connect signal grounds as required. Remove propellers or mechanical loads
when testing ESC outputs.

```mermaid
flowchart LR
    subgraph MCU["ESP32-C3 DevKitM-1"]
        P4["GPIO 4<br/>selected servo/ESC signal"]
        P0["GPIO 0"]
        P1["GPIO 1"]
        P3["GPIO 3<br/>INPUT_PULLUP"]
        P5["GPIO 5"]
        P6["GPIO 6"]
    end

    subgraph OUTPUT["Compatible servo or ESC"]
        SIG["Signal input"]
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

    P4 -->|PWM / OneShot / Multishot / DShot| SIG
    P0 -->|encoder A| A
    P1 -->|encoder B| B
    P3 -->|active-low button| SW
    P5 -->|I2C data| SDA
    P6 -->|I2C clock| SCL
```

## Output adapter selection

```mermaid
flowchart LR
    APP["ServoTesterApp"] -->|select by saved EscProtocol| FACTORY["rebuildEscOutput()"]
    FACTORY --> PWM["PwmEscOutput<br/>50 Hz"]
    FACTORY --> OS["OneShotEscOutput<br/>OneShot125 / OneShot42"]
    FACTORY --> MS["MultishotEscOutput<br/>32 kHz"]
    FACTORY --> DS["DShotEscOutput<br/>DShot150 / 300 / 600 / 1200"]
    PWM --> IFACE["IEscOutput"]
    OS --> IFACE
    MS --> IFACE
    DS --> IFACE
    IFACE --> PIN["GPIO 4"]
```

The app controls every adapter through a normalized 0–1 output position;
each adapter converts it to its native range. PWM min/max settings apply only
to PWM.

## UML class diagram

`main.cpp` constructs storage, display, encoder, and button dependencies.
`ServoTesterApp` constructs the selected output adapter.

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
        -handleSettingsClick()
        -rebuildEscOutput()
        -stepNormalized()
        -updateSweep(now)
        -render()
        -angleFromThrottle()
        -_throttle01: float
    }

    class Settings {
        +minPulse: int
        +maxPulse: int
        +step: int
        +stepPercent: int
        +sweepSpeed: int
        +minAngle: int
        +maxAngle: int
        +protocol: EscProtocol
        +clampAll()
    }

    class SettingsStorage {
        <<interface>>
        +load(settings)
        +save(settings)
    }
    class NvsSettingsStorage
    class Display {
        <<interface>>
        +begin()
        +showBoot()
        +showRunning(mode, protocol, rawValue, normalized, angleDeg)
        +showMainMenu(selected)
        +showSettingsMenu(settings, selected)
    }
    class OledDisplay
    class RotaryEncoder
    class Button
    class IEscOutput {
        <<interface>>
        +begin(minPulseUs, maxPulseUs)
        +setRaw(value)
        +setThrottle(percent)
        +setNormalized(value01)
        +getNormalized()
        +getRaw()
        +name()
    }
    class PwmEscOutput
    class OneShotEscOutput
    class MultishotEscOutput
    class DShotEscOutput
    class EscProtocol {
        <<enumeration>>
        PWM
        OneShot125
        OneShot42
        Multishot
        DShot150
        DShot300
        DShot600
        DShot1200
    }

    NvsSettingsStorage ..|> SettingsStorage
    OledDisplay ..|> Display
    PwmEscOutput ..|> IEscOutput
    OneShotEscOutput ..|> IEscOutput
    MultishotEscOutput ..|> IEscOutput
    DShotEscOutput ..|> IEscOutput
    ServoTesterApp --> SettingsStorage
    ServoTesterApp --> Display
    ServoTesterApp --> RotaryEncoder
    ServoTesterApp --> Button
    ServoTesterApp --> IEscOutput : selected implementation
    ServoTesterApp *-- Settings
    Settings --> EscProtocol
    SettingsStorage ..> Settings
    Display ..> Settings
```

## Application workflow

```mermaid
flowchart TD
    START([Power on or reset]) --> SETUP["setup(): Serial at 115200"]
    SETUP --> LOAD["Load settings from NVS; use defaults for missing keys"]
    LOAD --> CLAMP["Clamp values and validate protocol"]
    CLAMP --> NORMALIZED["Set output to 50% for PWM, 0% for non-PWM"]
    NORMALIZED --> INPUTS["Initialize encoder and active-low button"]
    INPUTS --> SELECT["Construct and initialize selected output adapter"]
    SELECT --> DISPLAY["Initialize OLED and show boot screen for 800 ms"]
    DISPLAY --> LOOP["app.loop()"]

    LOOP --> READ["Read encoder delta"]
    READ --> EDIT{"Encoder moved?"}
    EDIT -- "running screen" --> MODE["Apply active mode behavior"]
    EDIT -- "in menu" --> MENU["Move selection or edit setting"]
    EDIT -- "no" --> POLL
    MODE --> POLL["Poll button"]
    MENU --> POLL
    POLL --> EVENT{"Click or long press?"}
    EVENT -- Yes --> HANDLE["Handle event for current mode/menu"]
    EVENT -- No --> SWEEP_CHECK
    HANDLE --> SWEEP_CHECK{"Sweep active outside menus?"}
    SWEEP_CHECK -- Yes --> ELAPSED{"Sweep interval elapsed?"}
    ELAPSED -- Yes --> SWEEP["Adjust normalized output; reverse at 0% / 100%"]
    ELAPSED -- No --> RENDER
    SWEEP --> RENDER["Render running view or menu"]
    SWEEP_CHECK -- No --> RENDER
    RENDER --> WAIT["delay(20 ms)"]
    WAIT --> LOOP
```

## Input and persistence notes

- Running mode cycle: **Manual → Sweep → Center → Step → Manual**.
- Manual adjusts normalized output using the active step; Sweep adjusts
  interval; Step adjusts the PWM or percentage step size; Center ignores
  encoder rotation.
- A click cycles mode except in Step mode, where it advances output once.
  Selecting Center commands 50% normalized output.
- A long press opens the main menu. In Settings, a click advances to the next
  item; **Save** persists settings, while **Back** and long press leave
  without saving.
- Protocol is shown as `Proto` with a compact value in the OLED Settings menu.
  Protocol changes take effect immediately; Save persists them.
- Settings defaults are `minPulse=1000`, `maxPulse=2000`, `step=10`,
  `stepPercent=5`, `sweepSpeed=20`, `minAngle=0`, `maxAngle=120`, and
  `protocol=PWM`.
- NVS namespace is `servo`. Protocol values are stored as enum integers;
  invalid values are clamped back to PWM.
- The displayed angle maps normalized output to `minAngle..maxAngle`; it is
  not position feedback. PWM displays pulse width and angle; non-PWM displays
  throttle percent and raw protocol value.
- PWM step is `step / (maxPulse - minPulse)`; non-PWM step is
  `stepPercent / 100`.
- `maxPulse` and `maxAngle` edits attempt to preserve the pulse/angle ratio;
  the paired update is rejected if its resulting value is outside its allowed
  range.
