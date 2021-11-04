#include <Arduino.h>
#include <math.h>
#include <ESP32Encoder.h>
#include <driver/i2s.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <driver/dac.h>
#include <Wire.h>
#include <VL53L0X.h>
#include <SPI.h>
#include <Wire.h>
#include <iostream>

#include "MenuLib/menu.hpp"

using namespace std;

//TODO fix display crap

#define TABLE_SIZE 65536 //table size for calculations (wave samples) -> how many waves can be created
#define AMPLITUDE 0.1 * 16384
#define MAX_SKEW 100   //max skew for triangle wave

#define SAMPLE_RATE 44100 //audio sample rate
#define MIN_FREQ 170.0
#define MAX_FREQ 440.0

ESP32Encoder encoder;   //encoder
VL53L0X sensor1;
VL53L0X sensor2;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
//U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, 7, 6, U8X8_PIN_NONE);

Menu menu(&u8g2);


//int skew = 50;
//int t_start_index = 0;
//int t_peak_index = (0 * skew + (TABLE_SIZE / 2) * (MAX_SKEW - skew)) / MAX_SKEW;
//int t_trough_index = TABLE_SIZE - t_peak_index;
//int t_end_index = TABLE_SIZE;
//float rise_delta = (float)AMPLITUDE / t_peak_index;
//float fall_delta = (float)AMPLITUDE / ((t_trough_index - t_peak_index) / 2);

float target_frequency = MIN_FREQ;
float target_amplitude = AMPLITUDE;
int target_wave_type = 0;
int target_skew = 50;
int target_duty_cycle = 50;

//wave type switching
//int old_type = 0;
//int wave_type = 0;


//sensor configuration rutine
void registerSensor(int enable_pin, uint8_t address, VL53L0X *sensor) {
    pinMode(enable_pin, OUTPUT);
    digitalWrite(enable_pin, HIGH); //enable sensor on enable_pinn
    sensor->init(true);             //initialize the sensor
    sensor->setAddress(address);    //set new address (required as all sensors have the same default address)
    pinMode(enable_pin, INPUT);
    sensor->setMeasurementTimingBudget(20000);  //change timing for faster refresh
}

void menuTask(void *params) {
    menu.addByName("root", "Wave", picker, &target_wave_type, {"sine", "square", "triangle"});
    menu.addByName("root", "Skew", counter, 0, 100, &target_skew);
    menu.addByName("root", "Duty cycle", counter, 0, 100, &target_duty_cycle);
    int position = 0;
    bool select = false;

    while(true) {
        if (!digitalRead(23)) {
        while(!digitalRead(23));
            select = true;
        }
        position = encoder.getCount();
        menu.render(&position, &select);
        encoder.setCount(position);
    }
}


//main task for "playing" waves
void playTask(void *params) {
    //triangle config
    //int skew = 0;
    //int t_start_index = 0;
    int t_peak_index = (0 * target_skew + (TABLE_SIZE / 2) * (MAX_SKEW - target_skew)) / MAX_SKEW;
    int t_trough_index = TABLE_SIZE - t_peak_index;
    //int t_end_index = TABLE_SIZE;
    float rise_delta = (float)AMPLITUDE / t_peak_index;
    float fall_delta = (float)AMPLITUDE / ((t_trough_index - t_peak_index) / 2);

    //int i = 0;  //indexing for calculations

    //int type = 0;   //current wave type
    //float target_frequency = MIN_FREQ; //current frequency
    float frequency = MIN_FREQ;
    //float target_amplitude = AMPLITUDE;
    float amplitude = AMPLITUDE;
    float delta;

    uint16_t pos = 0;
    int16_t int_sample;

    printf("playTask default frequency: %f\n", frequency);
    printf("playTask default amplitude: %f\n", AMPLITUDE);


    while (true) {
        frequency += 0.05 * (target_frequency - frequency);
        delta = (frequency * TABLE_SIZE) / SAMPLE_RATE;
        pos += delta;

        //sine
        if (target_wave_type == 0) {
            int_sample = (int16_t)(amplitude * sin(2.0 * M_PI * (1.0 / TABLE_SIZE) * pos));   //calculate sine * amplitude
        }

        //square
        else if (target_wave_type == 1) {
            if (pos < (TABLE_SIZE / 100 * target_duty_cycle))
                int_sample = (int16_t)amplitude;
            else
                int_sample = (int16_t)(-amplitude);
        }

        //TODO fix triangle amplitude (starts in the middle)
        //triangle
        else if (target_wave_type == 2) {
            t_peak_index = (0 * target_skew + (TABLE_SIZE / 2) * (MAX_SKEW - target_skew)) / MAX_SKEW;
            t_trough_index = TABLE_SIZE - t_peak_index;
           //t_end_index = TABLE_SIZE;
            rise_delta = (float)amplitude / t_peak_index;
            fall_delta = (float)amplitude / ((t_trough_index - t_peak_index) / 2);
            if (pos < t_peak_index) 
                int_sample =  (int16_t)(rise_delta * pos);
            else if (pos < t_trough_index)
                int_sample = (int16_t)(amplitude - fall_delta * (pos - t_peak_index));
            else
                int_sample = (int16_t)(-amplitude + rise_delta * (pos - t_trough_index));
        }


        //INFO temporary amplitude fix???
        if (int_sample == 0)
                amplitude = target_amplitude;
        
        
        size_t i2s_bytes_write;
        i2s_write(I2S_NUM_1, &int_sample, sizeof(uint16_t), &i2s_bytes_write, portMAX_DELAY);

        //INFO fix for other wave types (and generaly for overflow)
        if (pos >= TABLE_SIZE)
            pos = pos - TABLE_SIZE;
    }
}

void setup(void) {
    menu.addByName("root", "Wave", picker, &target_wave_type, {"sine", "square", "triangle"});
    menu.addByName("root", "Skew", counter, 0, 100, &target_skew);
    menu.addByName("root", "Duty cycle", counter, 0, 100, &target_duty_cycle);

    Wire.begin(SDA, SCL, 100000);
    //Wire1.begin(19, 18, 100000);

    //sensor1.setBus(&Wire1);
    //sensor2.setBus(&Wire1);
    registerSensor(4, 4, &sensor1);
    registerSensor(0, 0, &sensor2);
    
    sensor1.startContinuous();
    sensor2.startContinuous();

    u8g2.begin();
    u8g2.setFont(u8g2_font_6x10_mf);   //font dimensions: 11x11
    u8g2.clearBuffer();

    ESP32Encoder::useInternalWeakPullResistors=UP;
    encoder.attachSingleEdge(36, 39);
    encoder.setFilter(1023);
    encoder.clearCount();

    menu.begin();   //start menu

    //
    pinMode(23, INPUT_PULLUP);
    //pinMode(9 , INPUT_PULLUP);
    //pinMode(10, INPUT_PULLUP);
    //pinMode(11, INPUT_PULLUP); 

    // i2s pins
    i2s_pin_config_t i2sPins = {
        .bck_io_num = 16,
        .ws_io_num = 17,
        .data_out_num = 25,
        .data_in_num = -1};
    
    // i2s config for writing both channels of I2S
    i2s_config_t i2sConfig = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 64,
        .use_apll = true};

    //install and start i2s driver
    i2s_driver_install(I2S_NUM_1, &i2sConfig, 0, NULL);
    // set up the i2s pins
    i2s_set_pin(I2S_NUM_1, &i2sPins);
    // clear the DMA buffers
    i2s_zero_dma_buffer(I2S_NUM_1);
    //i2s_set_clk(I2S_NUM_1, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);

    TaskHandle_t task;
    xTaskCreate(playTask, "play_task", 100000, NULL, 1, &task);   //create task

    TaskHandle_t menu_task;
    xTaskCreate(menuTask, "menu_task", 100000, NULL, 1, &menu_task);   //create task
}

int old_len1 = 0;
int old_len2 = 0;
int position = 0;
    bool select = false;

//TODO input smoothing
void loop(void) {
    int len1 = sensor1.readRangeContinuousMillimeters();
    int len2 = sensor2.readRangeContinuousMillimeters();
    if (len1 < 1000) {
        if (len1 < 60) len1 = 60;
        if (len1 > 800) len1 = 800;
        if (old_len1 - len1) {
            old_len1 = len1;
            target_frequency = (float)(map(len1, 60, 800, MIN_FREQ * 100, MAX_FREQ * 100)) / 100.0;
            printf("Distance: %d freq: %f\n", len1, target_frequency);
        }
    }

    if (len2 < 1000) {
        if (len2 < 60) len2 = 60;
        if (len2 > 800) len2 = 800;
        if (old_len2 != len2) {
            old_len2 = len2;

            target_amplitude = ((float)(map(len2, 60, 800, 5, 20)) / 100.0) * 16384;
            printf("Distance: %d ampl: %f\n", len2, target_amplitude);
        }
    }

    if (!digitalRead(23)) {
        while(!digitalRead(23));
            select = true;
        }
    position = encoder.getCount();
    menu.render(&position, &select);
    encoder.setCount(position);
    
}