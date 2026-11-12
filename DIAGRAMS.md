# Servo Tester Diagrams

These diagrams are based on the current source, especially `src/main.cpp` and
`src/ServoTesterApp.cpp`. Mermaid diagrams render on GitHub.

## Principal wiring

Signal pin assignments are taken from `src/main.cpp`. The firmware does not
define power connections; follow the requirements of the specific board,
display, and servo. If the servo uses a separate supply, its ground must be
connected to ESP32 ground.

```mermaid
flowchart LR
    subgraph MCU["ESP32-C3 DevKitM-1"]
        P4["GPIO 4"]
        P0["GPIO 0"]
        P1["GPIO 1"]
        P3["GPIO 3<br/>INPUT_PULLUP"]
        P6["GPIO 6"]
        P5["GPIO 5"]
    end

    subgraph SERVO["Hobby servo"]
        SIG["Signal"]
    end
    subgraph ENCODER["Rotary encoder"]
        CLK["CLK"]
        DT["DT"]
        SW["Push switch"]
    end
    subgraph OLED["SSD1306 72x40 OLED<br/>I2C"]
        SDA["SDA"]
        SCL["SCL"]
    end

    P4 -->|servo control signal| SIG
    P0 -->|encoder clock| CLK
    P1 -->|encoder data| DT
    P3 -->|button input| SW
    P6 -->|I2C data| SDA
    P5 -->|I2C clock| SCL
```

## UML class diagram

`main.cpp` creates the concrete hardware adapters and injects them into
`ServoTesterApp`. The diagram shows the application classes and their key
public and internal methods; external library internals are omitted.

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
        +clampAll()
    }

    class SettingsStorage {
        <<interface>>
        +load(settings)
        +save(settings)
    }

    class NvsSettingsStorage {
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
        +begin()
        +showBoot()
        +showRunning(mode, pulseUs, angleDeg)
        +showMainMenu(selected)
        +showSettingsMenu(settings, selected)
    }

    class RotaryEncoder {
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
    ServoTesterApp *-- Settings : owns current settings
    ServoTesterApp --> Mode : tracks
    ServoTesterApp --> MenuState : tracks
    ServoTesterApp ..> MainMenuItem : handles
    ServoTesterApp ..> SettingsItem : handles
    Button ..> ButtonEvent : emits
    SettingsStorage ..> Settings : loads and saves
    Display ..> Settings : displays
```

## UML-style application workflow

The startup path and recurring application loop are represented as an
activity-style flow. The loop polls controls, updates the sweep if active,
draws the screen for the current menu state, then waits 20 ms.

```mermaid
flowchart TD
    START([Power on / reset]) --> SETUP["setup(): Serial.begin(115200)"]
    SETUP --> LOAD["app.begin(): load settings from NVS"]
    LOAD --> CLAMP["Clamp settings to supported ranges"]
    CLAMP --> INIT["Initialize encoder, button, servo, and display"]
    INIT --> CENTER["Set servo pulse to 1500 us"]
    CENTER --> BOOT["Show boot screen; wait 800 ms"]
    BOOT --> LOOP["app.loop()"]

    LOOP --> READ["Read encoder delta"]
    READ --> HAS_DELTA{"Delta nonzero?"}
    HAS_DELTA -- Yes --> MENU_OFF{"Menu is Off?"}
    MENU_OFF -- Yes --> ACTIVE_MODE["Apply delta to active mode"]
    MENU_OFF -- No --> MENU_EDIT["Move selection or edit setting"]
    ACTIVE_MODE --> BUTTON_POLL
    MENU_EDIT --> BUTTON_POLL
    HAS_DELTA -- No --> BUTTON_POLL["Poll button"]

    BUTTON_POLL --> EVENT{"Click or long press?"}
    EVENT -- No --> SWEEP_CHECK
    EVENT -- Yes --> BUTTON_ACTION["Handle event for current menu state"]
    BUTTON_ACTION --> SWEEP_CHECK

    SWEEP_CHECK{"Mode is Sweep and menu is Off?"}
    SWEEP_CHECK -- Yes --> INTERVAL{"Sweep interval elapsed?"}
    INTERVAL -- Yes --> ADVANCE["Advance pulse by step; reverse at limits"]
    INTERVAL -- No --> RENDER
    ADVANCE --> RENDER["Render running view or active menu"]
    SWEEP_CHECK -- No --> RENDER

    RENDER --> WAIT["Wait 20 ms"]
    WAIT --> LOOP
```

### Input and menu behavior

- On the running screen, turning the encoder edits the current mode's value.
  In Step mode, a button click advances the servo by one step; in other modes
  it cycles to the next mode.
- A long press opens the main menu. A main-menu click selects a mode or opens
  Settings; a long press returns to the running screen.
- In Settings, turning the encoder edits the selected value. A click advances
  to the next item, saves when **Save** is selected, or returns without saving
  when **Back** is selected. A long press returns to the main menu.
