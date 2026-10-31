// ═══════════════════════════════════════════════════════════════════════════
// Домашка 2.5 · витяжний вентилятор на апаратному таймері
// Плата: YD-ESP32-S3 (ESP32-S3-WROOM-1-N16R8) · PlatformIO · Arduino framework
// Карта: map.svg
//
// Умова: кожну годину вмикається на 15 хвилин. Для налагодження час стиснутий:
// година = 12 с, 15 хвилин = 3 с, пропорція та сама (1 : 4).
// Вмикає й вимикає лише ISR таймера, loop() тільки друкує лог.
//
// Клавіші в моніторі порту:
//   b — заблокувати loop() на 5 с: лог мовчить, а вентилятор працює далі
//   ? — підказка
// ═══════════════════════════════════════════════════════════════════════════
#include <Arduino.h>

// Для справжньої роботи: 3600 і 900
#define FAN_PERIOD_S 12
#define FAN_ON_S 3

namespace {
    struct Config {
        static constexpr uint8_t FAN_PIN = 13; // база 2N2222A через 1 кОм: мотор K-04 = вентилятор
        static constexpr uint8_t LED_PIN = 18; // дублює стан вентилятора

        static constexpr uint8_t TIMER_ID = 0;
        static constexpr uint16_t TIMER_DIVIDER = 80; // 80 МГц ÷ 80 = 1 МГц, один тік = 1 мкс
        static constexpr uint64_t TICK_US = 1000000; // переривання раз на секунду

        static constexpr uint32_t BLOCK_MS = 5000;
    };

    hw_timer_t *timer = nullptr;

    // Спільне з ISR, тому volatile; змінює лише ISR, loop() лише читає
    volatile bool fanOn = true;
    volatile uint32_t secondsInPeriod = 0;

    void ARDUINO_ISR_ATTR onSecond() {
        const uint32_t seconds = secondsInPeriod + 1;
        if (seconds >= FAN_PERIOD_S) {
            secondsInPeriod = 0;
            fanOn = true;
        } else {
            secondsInPeriod = seconds;
            if (seconds == FAN_ON_S) fanOn = false;
        }
        digitalWrite(Config::FAN_PIN, fanOn ? HIGH : LOW);
        digitalWrite(Config::LED_PIN, fanOn ? HIGH : LOW);
    }

    void printHelp() {
        Serial.println("b — заблокувати loop() на 5 с · ? — підказка");
    }

    void printState(const bool on) {
        Serial.printf("%7.2f с | вентилятор %s\n", millis() / 1000.0f, on ? "УВІМК" : "вимк");
    }

    void handleKeys() {
        while (Serial.available() > 0) {
            const char key = static_cast<char>(Serial.read());
            if (key == 'b') {
                Serial.println("loop() заблокований на 5 с");
                delay(Config::BLOCK_MS); // навмисно: таймер не залежить від цього циклу
                Serial.println("loop() знову працює");
            } else if (key == '?') {
                printHelp();
            }
        }
    }
} // namespace

void setup() {
    digitalWrite(Config::FAN_PIN, HIGH);
    pinMode(Config::FAN_PIN, OUTPUT);
    digitalWrite(Config::LED_PIN, HIGH);
    pinMode(Config::LED_PIN, OUTPUT);

    Serial.begin(115200);
    delay(1500); // час на підключення монітора (рідний USB)
    Serial.printf("Домашка 2.5 · період %d с, увімкнено %d с\n", FAN_PERIOD_S, FAN_ON_S);
    printHelp();

    timer = timerBegin(Config::TIMER_ID, Config::TIMER_DIVIDER, true);
    timerAttachInterrupt(timer, &onSecond, true);
    timerAlarmWrite(timer, Config::TICK_US, true); // true = повторювати без участі loop()
    timerAlarmEnable(timer);
}

void loop() {
    static bool printed = false;
    static bool lastState = false;

    handleKeys();

    const bool state = fanOn;
    if (!printed || state != lastState) {
        printed = true;
        lastState = state;
        printState(state);
    }
}
