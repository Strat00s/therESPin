#include <Arduino.h>
#include <math.h>
#include <ESP32Encoder.h>
#include <driver/i2s.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#define TABLE_SIZE 256
#define AMPLITUDE 128
#define MAX_SKEW 100

#define SAMPLE_RATE 44100
#define MIN_FREQ 65.41
#define MAX_FREQ 3951.07

float wave[TABLE_SIZE] = {0.0};

ESP32Encoder encoder;
QueueHandle_t type_q;
QueueHandle_t freq_q;

//int skew = 50;
//int t_start_index = 0;
//int t_peak_index = (0 * skew + (TABLE_SIZE / 2) * (MAX_SKEW - skew)) / MAX_SKEW;
//int t_trough_index = TABLE_SIZE - t_peak_index;
//int t_end_index = TABLE_SIZE;
//float rise_delta = (float)AMPLITUDE / t_peak_index;
//float fall_delta = (float)AMPLITUDE / ((t_trough_index - t_peak_index) / 2);

float frequency = MIN_FREQ;
int old_type = 0;
int wave_type = 0;

int enc_val = 0;
int old_enc = 0;

void playTask(void *params) {
    int skew = 0;
    int t_start_index = 0;
    int t_peak_index = (0 * skew + (TABLE_SIZE / 2) * (MAX_SKEW - skew)) / MAX_SKEW;
    int t_trough_index = TABLE_SIZE - t_peak_index;
    int t_end_index = TABLE_SIZE;
    float rise_delta = (float)AMPLITUDE / t_peak_index;
    float fall_delta = (float)AMPLITUDE / ((t_trough_index - t_peak_index) / 2);

    int i = 0;
    int type = 0;
    float frequency = MIN_FREQ;

    uint32_t iterations = 0.25 * SAMPLE_RATE;
    float delta = (frequency * TABLE_SIZE) / (float)SAMPLE_RATE;

    i2s_set_clk(I2S_NUM_0, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, (i2s_channel_t)2);

    float sample = 0;

    while (true) {
        if (uxQueueMessagesWaiting(freq_q)) {
            xQueueReceive(freq_q, &frequency, 0);
            delta = (frequency * TABLE_SIZE) / (float)SAMPLE_RATE;
        }
        if (uxQueueMessagesWaiting(type_q)) {
            xQueueReceive(type_q, &type, 0);
        }

        uint16_t pos = uint32_t(i * delta) % TABLE_SIZE;

        //sine
        if (type == 0) {
            sample = (float)(AMPLITUDE * sin(2.0 * M_PI * (1.0 / TABLE_SIZE) * pos));
        }

        //square
        else if (type == 1) {
            if (pos < TABLE_SIZE / 2) {
                sample = AMPLITUDE;
                //square[i] = (int)AMPLITUDE;
            }
            else {
                sample = (-AMPLITUDE);
                //wave[i] = (int)-AMPLITUDE;
            }
        }

        //triangle
        else if (type == 2) {
            if (pos < t_peak_index) {
                sample = (rise_delta * pos);
            }
            else if (pos < t_trough_index) {
                sample = (AMPLITUDE - fall_delta * (pos - t_peak_index));
            }
            else {
                sample = (-AMPLITUDE + rise_delta * (pos - t_trough_index));
            }
        }
        size_t i2s_bytes_write;
        int32_t int_sample = (int)sample;
        i2s_write(I2S_NUM_0, &int_sample, sizeof(int_sample), &i2s_bytes_write, 100);

        i++;
        if (i >= iterations) i = 0;
    }
}

void setup(void) {
    type_q = xQueueCreate(1, sizeof(int));
    freq_q = xQueueCreate(1, sizeof(float));

    encoder.attachSingleEdge(36, 39);
    encoder.setFilter(1023);
    encoder.clearCount();

    pinMode(16, INPUT_PULLUP);
    pinMode(5, INPUT_PULLUP);
    pinMode(17, INPUT_PULLUP);

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,                                //Interrupt level 1
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = false,
    };
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);

    xTaskCreate(playTask, "play_task", 10000, NULL, 1, NULL);
}

void loop(void) {
    enc_val = encoder.getCount();
    if (enc_val > 84) enc_val = 84;
    if (enc_val < 1) enc_val = 1;
    encoder.setCount(enc_val);

    if (enc_val != old_enc) {
        old_enc = enc_val;
        frequency = (float)(map(enc_val, 1, 84, MIN_FREQ * 100, MAX_FREQ * 100)) / 100.0;
        printf("Encoder: %d freq: %f\n", enc_val, frequency);
        xQueueSend(freq_q, &frequency, portMAX_DELAY);
    }

    if (!digitalRead(16)) wave_type = 0;
    if (!digitalRead(17)) wave_type = 1;
    if (!digitalRead(5))  wave_type = 2;
    if (old_type != wave_type) {
        old_type = wave_type;
        printf("Wave type: ");
        switch (wave_type) {
        case 0: printf("sine\n"); break;
        case 1: printf("square\n"); break;
        case 2: printf("triangle\n"); break;
        default: printf("unknown\n"); break;
        }
        xQueueSend(type_q, &wave_type, portMAX_DELAY);
    }
}