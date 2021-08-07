#define THING_INDEX_MINE 0
#define THING_INDEX_SARAH 1

// change this for each device
#define THING_INDEX THING_INDEX_MINE

#define ENABLE_FASTLED
// #define ENABLE_SERIAL

#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
// #include <WiFiClient.h>
#include <FastLED.h>

#include <DNSServer.h>
#include <WiFiManager.h>

// #include <EEPROM.h>
// #define EEPROM_ADDR_SOFT_POT_MIN 0
// #define EEPROM_ADDR_SOFT_POT_MAX (EEPROM_ADDR_SOFT_POT_MIN + sizeof(uint16_t))
// #define EEPROM_SIZE (sizeof(uint16_t) * 2)

#define LEDTYPE WS2812B

#define NUMLEDS 7

// PIN CONNECTIONS
#define LEDPIN 23
#define BRIGHTNESS_POT_PIN A4
#define POTPIN A2
// #define CALIBRATE_BTN_PIN 0
#define ADC_MAXVAL 4095

// STATUS CODES
#define STATUS_REQUEST_COLOR 0

// include the correct secrets file
#if THING_INDEX == 0
#include "secrets/secrets_thing0.h"
#else
#include "secrets/secrets_thing1.h"
#endif

extern const uint8_t gammaVals[];

CRGB leds[NUMLEDS];

// TaskHandle_t core0LoopTask;
// TaskHandle_t core1LoopTask;

uint8_t currentSoftPotHue = 0;
uint8_t currentLedHue = 0;
uint8_t targetHue = 0;

uint8_t lastSentLedHue = 0;

uint8_t lastBrightness = 0;

BlynkTimer softPotTimer;
BlynkTimer updateToIotTimer;
BlynkTimer ledOutputTimer;
WidgetBridge bridge(VPIN_SEND);

// this assumes soft pot is pulled HIGH when not pressed
// (ADC_MAXVAL * 2/3) = 4095 * 2/3 = 2730
#define POT_HIGH_THRESH 2730
// uint16_t SOFT_POT_MIN = ADC_MAXVAL;
// uint16_t SOFT_POT_MAX = 0;
#define SOFT_POT_MIN 1400
#define SOFT_POT_MAX 2400

#if THING_INDEX == THING_INDEX_MINE
    #define POT_LOW_THRESH 1600
    #define SOFT_POT_MIN 1630
    #define SOFT_POT_MAX 2100
#else
    #define POT_LOW_THRESH 0
#endif

#if THING_INDEX == THING_INDEX_MINE
uint8_t lastPotValue = 0; 
#endif

void setup() {
    pinMode(13, OUTPUT);

    #ifdef ENABLE_FASTLED
    FastLED.addLeds<LEDTYPE, LEDPIN, GRB>(leds, NUMLEDS);
    FastLED.setDither(0);
    #endif
    #ifdef ENABLE_SERIAL
    Serial.begin(115200);
    #endif

    showStatusIndicator(CRGB(128, 0, 0));

    // set builtin led on until connected
    digitalWrite(13, HIGH);

    WiFiManager wifiManager;

    // wifiManager.resetSettings();

    // tries to connect with configured ssid/password
    // else starts AP with these ssid/password and awaits configuration
    if (!wifiManager.autoConnect(AP_SSID, AP_PASSWORD)) {
        delay(3000);
        // reset and try again
        ESP.restart();
        delay(5000);
    }

    // connected to wifi

    // configure blynk
    delay(2000);
    Blynk.config(BLYNK_AUTH_THIS);
    bool success = Blynk.connect(180); // timeout param
    if (!success) {
        ESP.restart();
        delay(5000);
    }

    showStatusIndicator(CRGB(64, 64, 64));

    // EEPROM.begin(EEPROM_SIZE);

    // Blynk.begin(BLYNK_AUTH_THIS, WIFI_SSID, WIFI_PASSWORD);
    // delay(2000);

    // pinMode(CALIBRATE_BTN_PIN, INPUT_PULLUP);

    // FastLED.addLeds<LEDTYPE, LEDPIN, GRB>(leds, NUMLEDS);
    // FastLED.setDither(0);
    // Serial.begin(115200);

    // fill_solid(&leds[0], NUMLEDS, CRGB(0, 0, 0));
    // leds[0] = CHSV(0, 255, 255);
    // FastLED.show();

    // uint16_t softPotCalibrateBuffer[8];
    // uint8_t i = 0;
    // uint16_t rawVal = ADC_MAXVAL;
    // bool newCalibration = false;
    // while (digitalRead(CALIBRATE_BTN_PIN) == LOW) {
    //     newCalibration = true;
    //     rawVal = analogRead(POTPIN);
    //     if (rawVal < POT_HIGH_THRESH) {
    //         // softPotCalibrateBuffer[i] = analogRead(POTPIN);
    //         // i++;
    //         // if (i >= 8) {
    //         //     i = 0;
    //         // }

    //         // uint16_t avg = 0;
    //         // for (uint8_t j = 0; j < 8; j++) {
    //         //     avg += softPotCalibrateBuffer[i];
    //         // }
    //         // avg = avg >> 3;

    //         if (rawVal < SOFT_POT_MIN) {
    //             SOFT_POT_MIN = rawVal;
    //         }
    //         if (rawVal > SOFT_POT_MAX) {
    //             SOFT_POT_MAX = rawVal;
    //         }
    //     }
    // }
    // if (newCalibration) {
    //     SOFT_POT_MAX -= 200;
    //     SOFT_POT_MIN += 200;
    //     // store new calibration values in eeprom
    //     EEPROM.put(EEPROM_ADDR_SOFT_POT_MIN, SOFT_POT_MIN);
    //     EEPROM.put(EEPROM_ADDR_SOFT_POT_MAX, SOFT_POT_MAX);
    // } else {
    //     // get last calibration values from eeprom
    //     SOFT_POT_MIN = EEPROM.get(EEPROM_ADDR_SOFT_POT_MIN, SOFT_POT_MIN);
    //     SOFT_POT_MAX = EEPROM.get(EEPROM_ADDR_SOFT_POT_MAX, SOFT_POT_MAX);
    // }

    // leds[0] = CHSV(200, 255, 255);
    // FastLED.show();

    // fill_solid(&leds[0], NUMLEDS, CRGB(0, 0, 0));
    // FastLED.show();

    // run core0Loop pinned to cpu core 0
    // params: task function, task name, task stack size, task parameter, task handle to keep track of it, core
    // xTaskCreatePinnedToCore(core0Loop, "Core0Loop", 10000, NULL, 1, &core0LoopTask, 0);

    // run core1Loop pinned to cpu core 1
    // xTaskCreatePinnedToCore(core1Loop, "Core1Loop", 10000, NULL, 1, &core1LoopTask, 1);

    softPotTimer.setInterval(10, readSoftPotTimerEvent);
    updateToIotTimer.setInterval(1000, sendValueToIot);
    ledOutputTimer.setInterval(20, updateLedsOutput);

    // get other device to send color
    bridge.virtualWrite(VPIN_STATUS_SEND, STATUS_REQUEST_COLOR);

    #if THING_INDEX == THING_INDEX_MINE
    FastLED.setBrightness(255);
    // so pot doesn't register as changed right at the start regardless of what position it's in
    lastPotValue = map(analogRead(BRIGHTNESS_POT_PIN), 0, ADC_MAXVAL, 0, 255);
    #endif
}

void loop() {
    Blynk.run();
    softPotTimer.run();
    updateToIotTimer.run();
    ledOutputTimer.run();
}

void showStatusIndicator(CRGB color) {
    #ifdef ENABLE_FASTLED
    fill_solid(&leds[0], NUMLEDS, CRGB(0, 0, 0));
    #endif

    leds[NUMLEDS - 1] = color;
    leds[NUMLEDS - 2] = color;
    #ifdef ENABLE_FASTLED
    FastLED.show();
    #endif
}

// this assumes soft pot is pulled HIGH when not pressed
// (ADC_MAXVAL * 2/3) = 4095 * 2/3 = 2730
// #define POT_HIGH_THRESH 2730

// #define POT_MIN 1320
// #define POT_MAX 2490
// #define POT_MIN 1400
// #define POT_MAX 2400

// #define INPUT_BUFFER_SIZE 64
// uint8_t inputBuffer[INPUT_BUFFER_SIZE];
// uint16_t inputBufferPointer = 0;
uint8_t rollingAvg = 0;

// uint8_t readSoftPot() {
//     uint16_t rawValue = analogRead(POTPIN);
//     if (rawValue < POT_HIGH_THRESH) {
//         if (rawValue < POT_MIN) {
//             rawValue = POT_MIN;
//         } else if (rawValue > POT_MAX) {
//             rawValue = POT_MAX;
//         }
//         inputBuffer[inputBufferPointer] = map(rawValue, POT_MIN, POT_MAX, 0, 255);
//         inputBufferPointer++;
//         if (inputBufferPointer >= INPUT_BUFFER_SIZE) {
//             inputBufferPointer = 0;
//         }
//     }
//     return inputBuffer[inputBufferPointer];
// }

void setRollingAvg(uint8_t value) {
    // for (int i = 0; i < INPUT_BUFFER_SIZE; i++) {
    //     inputBuffer[i] = value;
    // }
    rollingAvg = value;
}

void readSoftPotTimerEvent() {
    #if THING_INDEX == THING_INDEX_SARAH

    // currentSoftPotHue = readSoftPot();
    uint16_t rawValue = analogRead(POTPIN);
    #ifdef ENABLE_SERIAL
    Serial.println(rawValue);
    #endif
    if (rawValue < POT_HIGH_THRESH && rawValue > POT_LOW_THRESH) {
        // if (rawValue < SOFT_POT_MIN) {
        //     rawValue = SOFT_POT_MIN;
        // } else if (rawValue > SOFT_POT_MAX) {
        //     rawValue = SOFT_POT_MAX;
        // }
        rawValue = constrain(rawValue, SOFT_POT_MIN, SOFT_POT_MAX);
        // Serial.println(rawValue);
        // inputBuffer[inputBufferPointer] = map(rawValue, SOFT_POT_MIN, SOFT_POT_MAX, 0, 255);
        // inputBufferPointer++;
        // if (inputBufferPointer >= INPUT_BUFFER_SIZE) {
        //     inputBufferPointer = 0;
        // }

        uint8_t mappedValue = map(rawValue, SOFT_POT_MIN, SOFT_POT_MAX, 0, 255);

        rollingAvg = ((rollingAvg * 7) + mappedValue) >> 3;

        if (/*inputBuffer[inputBufferPointer] != targetHue*/ rollingAvg != targetHue) {
            // currentLedHue = inputBuffer[inputBufferPointer];
            currentLedHue = rollingAvg;
            targetHue = currentLedHue;
            #ifdef ENABLE_FASTLED
            fill_solid(&leds[0], NUMLEDS, CHSV(currentLedHue, 255, 255));
            FastLED.show();
            #endif
        }
    }

    updateBrightnessFromPot();

    #else

    // dodgy softpot so use brightness pot instead
    uint16_t rawValue = analogRead(BRIGHTNESS_POT_PIN);
    uint8_t mappedValue = map(rawValue, 0, ADC_MAXVAL, 0, 255);

    if ((uint8_t) (mappedValue - lastPotValue) > 2) {
        int8_t diff = lastPotValue - mappedValue;
        currentLedHue += diff;
        targetHue = currentLedHue;
        #ifdef ENABLE_FASTLED
        fill_solid(&leds[0], NUMLEDS, CHSV(currentLedHue, 255, 255));
        FastLED.show();
        #endif
        lastPotValue = mappedValue;
    }

    #endif
}

#define MIN_BRIGHTNESS 25
void updateBrightnessFromPot() {
    uint16_t rawValue = analogRead(BRIGHTNESS_POT_PIN);
    uint8_t newBrightness = map(rawValue, 0, ADC_MAXVAL, MIN_BRIGHTNESS, 255);
    if (newBrightness != lastBrightness) {
    #ifdef ENABLE_FASTLED
        FastLED.setBrightness(pgm_read_byte(&gammaVals[newBrightness]));
        lastBrightness = newBrightness;
        FastLED.show();
    #endif
    }
}

void sendValueToIot() {
    if (targetHue != lastSentLedHue) {
        _writeValueToVPin();
    }
}

void _writeValueToVPin() {
    bridge.virtualWrite(VPIN_SEND, targetHue);
    // Blynk.virtualWrite(V2, currentLedHue);
    lastSentLedHue = targetHue;
}

void updateLedsOutput() {
    if (currentLedHue != targetHue) {
        if ((uint8_t)(currentLedHue - targetHue) < (uint8_t)(targetHue - currentLedHue)) {
            currentLedHue--;
        } else {
            currentLedHue++;
        }
        #ifdef ENABLE_FASTLED
        fill_solid(&leds[0], NUMLEDS, CHSV(currentLedHue, 255, 255));
        FastLED.show();
        #endif
    }
}

BLYNK_CONNECTED() {
    // Blynk.syncAll();
    bridge.setAuthToken(BLYNK_AUTH_OTHER);
}

BLYNK_WRITE(VPIN_READ) {
    targetHue = param.asInt();
    lastSentLedHue = targetHue;
    setRollingAvg(targetHue);
    // fill_solid(&leds[0], NUMLEDS, CHSV(currentLedHue, 255, 255));
    // FastLED.show();
}

BLYNK_WRITE(VPIN_STATUS_READ) {
    uint8_t code = param.asInt();
    if (code == STATUS_REQUEST_COLOR) {
        _writeValueToVPin();
    }
}

const PROGMEM uint8_t gammaVals[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
    2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5,
    5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10,
    10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
    17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
    25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
    37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
    51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
    69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
    90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
    115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
    144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
    177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
    215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255};