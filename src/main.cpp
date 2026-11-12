#include <Arduino.h>
#include "ServoTesterApp.h"
#include "SettingsStorage.h"
#include "OledDisplay.h"
#include "Button.h"
#include "RotaryEncoder.h"
#include "ServoOutput.h"

// --- пины ---
static constexpr uint8_t PIN_SERVO    = 4;
static constexpr uint8_t PIN_ENC_CLK  = 0;
static constexpr uint8_t PIN_ENC_DT   = 1;
static constexpr uint8_t PIN_ENC_SW   = 3;
static constexpr uint8_t PIN_OLED_SDA = 5;
static constexpr uint8_t PIN_OLED_SCL = 6;

// --- зависимости (composition root) ---
static NvsSettingsStorage storage;
static OledDisplay        display(PIN_OLED_SDA, PIN_OLED_SCL);
static RotaryEncoder      encoder(PIN_ENC_CLK, PIN_ENC_DT);
static Button             button(PIN_ENC_SW);
static ServoOutput        servo(PIN_SERVO);

static ServoTesterApp     app(storage, display, encoder, button, servo);

void setup() {
    Serial.begin(115200);
    app.begin();
}

void loop() {
    app.loop();
}