#include <Arduino.h>
#include <U8g2lib.h>
#include <U8x8lib.h>
#include <SPI.h>
#include <esp32-hal-cpu.h>
#include <math.h>
#include <Tone32.h>
#include "analogWrite.h"

#define ECHO 36
#define TRIG 39
#define SPEED_OF_SOUND 343.0 //m/s

#define LCD_HEIGHT 64
#define LCD_WIDTH 168

#define BUZZER_PIN 16

U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE);

unsigned long getTravelTime(int trig, int echo) {
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);

    return pulseIn(echo, HIGH);
}

void setup(void) {
    Serial.begin(115200);
    Serial.printf("\nStarting\n");
    Serial.printf("Frequency: %d\n", getCpuFrequencyMhz());
    
    u8x8.begin();
    u8x8.setPowerSave(0);
    u8x8.setFont(u8x8_font_chroma48medium8_r);

    pinMode(TRIG, OUTPUT);
    pinMode(ECHO, INPUT);
    digitalWrite(TRIG, LOW);
    analogWriteResolution(LED_BUILTIN, 1);
}


void loop(void) {
    unsigned long travel_time = getTravelTime(TRIG, ECHO);
    float distance = travel_time / 2 * (SPEED_OF_SOUND / 10000);
    u8x8.setCursor(0, 1);
    u8x8.printf("Travel time:\n");
    u8x8.printf("%lu\n", travel_time);
    u8x8.printf("Distance:\n");
    u8x8.printf("%.2fcm  \n", distance);
    u8x8.refreshDisplay();

    if (distance > 5.0 && distance < 20.0) {
        long position = (distance - 5) * 100;
        long frequency = map(position, 500, 2000, 500, 1000);
        u8x8.printf("Freq: %ld\n", frequency);

        Serial.println(analogWrite(16, 50, (float)frequency));
    }
    else {
        u8x8.clearLine(5);
    }
}

/*
#include "analogWrite.h"
#define LED_BUILTIN 2

int brightness = 0;
int step = 1;

void setup() {
  Serial.begin(115200);
  analogWriteResolution(LED_BUILTIN, 10);
}

void loop() {
  analogWrite(LED_BUILTIN, brightness);
  brightness += step;
  if ( brightness >= 1023) step = -1;
  if ( brightness <= 0) step = 1;
  delay(1)
}
*/