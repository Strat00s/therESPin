#include <Arduino.h>
#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2s.h"
#include "driver/gpio.h"
#include "ESP32Encoder.h"


#define SAMPLE_RATE     44100
#define I2S_NUM         0
#define SAMPLE_PER_CYCLE(x) (SAMPLE_RATE/(int)(x))

#define WAVE_TABLE_SIZE 1024

#define MIN_FREQ 65.41      //C2
#define MAX_FREQ 3951.07    //B7
#define MIN_VOL 0
#define MAX_VOL 100

int32_t sine[WAVE_TABLE_SIZE]     = {0};
int32_t sawtooth[WAVE_TABLE_SIZE] = {0};
int32_t triangle[WAVE_TABLE_SIZE] = {0};
int32_t square[WAVE_TABLE_SIZE]   = {0};

ESP32Encoder encoder;
QueueHandle_t queue1;
QueueHandle_t queue2;

struct dataStruct{
    float frequency;
    int amplitude;
};

dataStruct data;

void generateSine(int32_t amplitude, int32_t* buffer, uint16_t length) {
  // Generate a sine wave signal with the provided amplitude and store it in
  // the provided buffer of size length.
  for (int i = 0; i < length; i++) {
    buffer[i] = int32_t((float)(amplitude) * sin(TWO_PI * (1.0 / length) * i));
    printf("%d\n", buffer[i]);
  }
}

void generateTriangle(int32_t amplitude, int32_t* buffer, uint16_t length) {
  // Generate a triangle wave signal with the provided amplitude and store it in
  // the provided buffer of size length.
  float delta = float(amplitude)/float(length);
  for (int i=0; i<length/2; ++i) {
    buffer[i] = -(amplitude/2)+delta*i;
    printf("%d\n", buffer[i]);
  }
    for (int i=length/2; i<length; ++i) {
    buffer[i] = (amplitude/2)-delta*(i-length/2);
    printf("%d\n", buffer[i]);
  }
}

void generateSquare(int32_t amplitude, int32_t* buffer, uint16_t length) {
  // Generate a square wave signal with the provided amplitude and store it in
  // the provided buffer of size length.
  for (int i=0; i<length/2; ++i) {
    buffer[i] = -(amplitude/2);
    printf("%d\n", buffer[i]);
  }
    for (int i=length/2; i<length; ++i) {
    buffer[i] = (amplitude/2);
    printf("%d\n", buffer[i]);
  }
}

void waveTask(void *params) {
    float frequency = MIN_FREQ;
    uint32_t iterations = 0.1 * SAMPLE_RATE;
    float delta = (frequency * WAVE_TABLE_SIZE) / (float)SAMPLE_RATE;
    i2s_set_clk((i2s_port_t)I2S_NUM, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, (i2s_channel_t)2);
    
    uint32_t i = 0;
    int32_t wave[WAVE_TABLE_SIZE];
    memcpy(wave, sine, WAVE_TABLE_SIZE);
    int wave_selector;
    int old_selector = 0;

    while(true) {
        if (uxQueueMessagesWaiting(queue1)) {
            printf("got data on 1\n");
            xQueueReceive(queue1, &frequency, 0);
            delta = (frequency * WAVE_TABLE_SIZE) / (float)SAMPLE_RATE;
        }
        if (uxQueueMessagesWaiting(queue2)) {
            xQueueReceive(queue2, &wave_selector, 0);
            if (old_selector != wave_selector) {
                old_selector = wave_selector;
                printf("got data on 2\n");
                switch (wave_selector) {
                    case 1: printf("sine\n"); memcpy(wave, sine, WAVE_TABLE_SIZE);break;
                    case 2: printf("square\n"); memcpy(wave, square, WAVE_TABLE_SIZE);break;
                    case 3: printf("triangle\n"); memcpy(wave, triangle, WAVE_TABLE_SIZE);break;
                }
            }
        }
        uint16_t pos = uint32_t(i * delta) % WAVE_TABLE_SIZE;
        int32_t sample = wave[pos];
        // Duplicate the sample so it's sent to both the left and right channel.
        // It appears the order is right channel, left channel if you want to write
        // stereo sound.
        size_t i2s_bytes_write;
        //i2s_write((i2s_port_t)I2S_NUM, &sample, sizeof(sample), &i2s_bytes_write, portMAX_DELAY);
        printf("pos: %d %d\n", pos, wave[pos]);
        i++;
        if (i >= iterations) i = 0;
    }
}

void playWave(int32_t* buffer, uint16_t length, float frequency, float seconds) {
    uint32_t iterations = seconds * SAMPLE_RATE;
    float delta = (frequency * length) / (float)SAMPLE_RATE;
    i2s_set_clk((i2s_port_t)I2S_NUM, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, (i2s_channel_t)2);
    for (uint32_t i = 0; i < iterations; i++) {
        uint16_t pos = uint32_t(i * delta) % length;
        int32_t sample = buffer[pos];
        // Duplicate the sample so it's sent to both the left and right channel.
        // It appears the order is right channel, left channel if you want to write
        // stereo sound.
        size_t i2s_bytes_write;
        i2s_write((i2s_port_t)I2S_NUM, &sample, sizeof(sample), &i2s_bytes_write, 10);
        //printf("Wanted %d vs Writen: %d\n", sizeof(sample), i2s_bytes_write);
    }
}

void setup(void) {
    queue1 = xQueueCreate(1, sizeof(float));
    queue2 = xQueueCreate(1, sizeof(int));
    pinMode(5, INPUT_PULLUP);
    pinMode(17, INPUT_PULLUP);
    pinMode(16, INPUT_PULLUP);

    encoder.attachSingleEdge(36, 39);
    encoder.setFilter(1023);
    encoder.clearCount();
    encoder.setCount(1);

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,                                //Interrupt level 1
        .dma_buf_count = 10,
        .dma_buf_len = 70,
        .use_apll = false,
    };
    i2s_driver_install((i2s_port_t)I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);

    data.frequency = MIN_FREQ;
    data.amplitude = 50;
    //data.seconds = 0.25;
    //memcpy(data.wave, sine, WAVE_TABLE_SIZE);

    generateSine(data.amplitude, sine, WAVE_TABLE_SIZE);
    generateTriangle(data.amplitude, triangle, WAVE_TABLE_SIZE);
    generateSquare(data.amplitude, square, WAVE_TABLE_SIZE);
    xTaskCreate(waveTask, "wave_task", 10000, NULL, 1, NULL);
}

int test_bits = 16;
int enc_old = 0;

void loop(void) {
    int enc_val = encoder.getCount();
    if (enc_val < 1) enc_val = 1;
    if (enc_val > 84) enc_val = 84;
    encoder.setCount(enc_val);

    if (enc_old != enc_val) {
        data.frequency = (float)(map(enc_val, 1, 84, MIN_FREQ * 100, MAX_FREQ * 100)) / 100.0;
        enc_old = enc_val;
        printf("Frequency: %f | Samples per cycle: %d\n", data.frequency, SAMPLE_RATE/(int)data.frequency);
        xQueueSend(queue1, &data.frequency, portMAX_DELAY);
        printf("Btn1: %d Btn2: %d Btn3 %d\n", digitalRead(5), digitalRead(17), digitalRead(16));
    }
    if (!digitalRead(5)) {
        int tmp = 1;
        xQueueSend(queue2, &tmp, portMAX_DELAY);
    }
    if (!digitalRead(17)) {
        int tmp = 2;
        xQueueSend(queue2, &tmp, portMAX_DELAY);
    }
    if (!digitalRead(16)) {
        int tmp = 3;
        xQueueSend(queue2, &tmp, portMAX_DELAY);
    }
    //playWave(sine, WAVE_TABLE_SIZE, data.frequency, 0.1);
}