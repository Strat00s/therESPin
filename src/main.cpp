#include <Arduino.h>
#include <math.h>
#include <ESP32Encoder.h>
#include <driver/i2s.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <driver/dac.h>

#define TABLE_SIZE 256 //table size for calculations
#define AMPLITUDE 3.3 / 2.0 //3.3 is DAC max voltage
#define MAX_SKEW 100   //max skew for triangle wave

#define SAMPLE_RATE 44100 //audio sample rate
#define MIN_FREQ 65.41    //C2
#define MAX_FREQ 3951.07  //B7

ESP32Encoder encoder;   //encoder
//queues for sending data between tasks
QueueHandle_t type_q;
QueueHandle_t freq_q;
QueueHandle_t skew_q;


//int skew = 50;
//int t_start_index = 0;
//int t_peak_index = (0 * skew + (TABLE_SIZE / 2) * (MAX_SKEW - skew)) / MAX_SKEW;
//int t_trough_index = TABLE_SIZE - t_peak_index;
//int t_end_index = TABLE_SIZE;
//float rise_delta = (float)AMPLITUDE / t_peak_index;
//float fall_delta = (float)AMPLITUDE / ((t_trough_index - t_peak_index) / 2);

float frequency = MIN_FREQ;

//wave type switching
int old_type = 0;
int wave_type = 0;

//skew changing
int skew_val = 0;
int old_skew = 0;

//encoder (frequency) changing
int enc_val = 0;
int old_enc = 0;

//main task for "playing" waves
void playTask(void *params) {
    //triangle config
    int skew = 0;
    int t_start_index = 0;
    int t_peak_index = (0 * skew + (TABLE_SIZE / 2) * (MAX_SKEW - skew)) / MAX_SKEW;
    int t_trough_index = TABLE_SIZE - t_peak_index;
    int t_end_index = TABLE_SIZE;
    float rise_delta = (float)AMPLITUDE / t_peak_index;
    float fall_delta = (float)AMPLITUDE / ((t_trough_index - t_peak_index) / 2);

    int i = 0;  //indexing for calculations

    int type = 0;   //current wave type
    float frequency = MIN_FREQ; //current frequency

    uint32_t iterations = 0.25 * SAMPLE_RATE;                       //iterations (depending on "segment" length). Here it is 0.25s
    float delta = (frequency * TABLE_SIZE) / (float)SAMPLE_RATE;    //how much should we "skip" to get desired frequency

    float sample = 0;   //sample which should be writen to the I2S

    while (true) {
        //do "math" if there are items in queue
        //frequency
        if (uxQueueMessagesWaiting(freq_q)) {
            xQueueReceive(freq_q, &frequency, 0);
            delta = (frequency * TABLE_SIZE) / (float)SAMPLE_RATE;
        }
        //wave type
        if (uxQueueMessagesWaiting(type_q)) {
            xQueueReceive(type_q, &type, 0);
        }
        //skew
        if (uxQueueMessagesWaiting(skew_q)) {
            xQueueReceive(skew_q, &skew, 0);
            //t_start_index = 0;
            t_peak_index = (0 * skew + (TABLE_SIZE / 2) * (MAX_SKEW - skew)) / MAX_SKEW;
            t_trough_index = TABLE_SIZE - t_peak_index;
            //t_end_index = TABLE_SIZE;
            rise_delta = (float)AMPLITUDE / t_peak_index;
            fall_delta = (float)AMPLITUDE / ((t_trough_index - t_peak_index) / 2);
        }


        uint16_t pos = uint32_t(i * delta) % TABLE_SIZE;    //skip

        //sine
        if (type == 0) {
            sample = (float)(AMPLITUDE * sin(2.0 * M_PI * (1.0 / TABLE_SIZE) * pos));   //calculate sine * amplitude
        }

        //square
        else if (type == 1) {
            if (pos < TABLE_SIZE / 2) {
                sample = AMPLITUDE;
            }
            else {
                sample = (-AMPLITUDE);
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
        i2s_write(I2S_NUM_0, &int_sample, sizeof(int_sample), &i2s_bytes_write, 100);   //write data
        //printf("%f\n", sample);

        i++;
        if (i >= iterations) i = 0; //iterate
    }
}

void setup(void) {
    //create queues
    type_q = xQueueCreate(1, sizeof(int));
    freq_q = xQueueCreate(1, sizeof(float));
    skew_q = xQueueCreate(1, sizeof(int));

    //encoder setup
    encoder.attachSingleEdge(36, 39);
    encoder.setFilter(1023);
    encoder.clearCount();

    //buttons setup
    pinMode(16, INPUT_PULLUP);
    pinMode(17, INPUT_PULLUP);
    pinMode(5, INPUT_PULLUP);
    pinMode(18, INPUT_PULLUP);

    //i2s setup
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = false,
    };
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
    i2s_set_clk(I2S_NUM_0, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, (i2s_channel_t)2);

    xTaskCreate(playTask, "play_task", 10000, NULL, 1, NULL);   //create task
}

void loop(void) {
    //switch between frequency and skew adjustment
    if (!digitalRead(18)) {
        skew_val = encoder.getCount();
        if (skew_val > 100) skew_val = 100;
        if (skew_val < 0)   skew_val = 0;
        encoder.setCount(skew_val);
    }
    else {
        enc_val = encoder.getCount();
        if (enc_val > 84) enc_val = 84;
        if (enc_val < 1)  enc_val = 1;
        encoder.setCount(enc_val);
    }

    //queue skew
    if (skew_val != old_skew) {
        old_skew = skew_val;
        printf("Encoder: %d skew: %d\n", enc_val, skew_val);
        xQueueSend(skew_q, &skew_val, portMAX_DELAY);
    }

    //queue frequency
    if (enc_val != old_enc) {
        old_enc = enc_val;
        frequency = (float)(map(enc_val, 1, 84, MIN_FREQ * 100, MAX_FREQ * 100)) / 100.0;
        printf("Encoder: %d freq: %f\n", enc_val, frequency);
        xQueueSend(freq_q, &frequency, portMAX_DELAY);
    }

    //read buttons and queue desired wave type
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