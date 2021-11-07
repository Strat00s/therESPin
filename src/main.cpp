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

//TODO general refactoring
//TODO clean it

//TODO wait for i2s to ask for data
//TODO make a class for i2s?


#define TABLE_SIZE 65536 //table size for calculations (wave samples) -> how many waves can be created
#define AMPLITUDE 0.1 * 16384   //TODO proper amplitude
#define MAX_SKEW 100   //max skew for triangle wave

#define SAMPLE_RATE 44100 //audio sample rate
#define MIN_FREQ 65.41
#define MAX_FREQ 3951.07
//#define MAX_FREQ 22050


enum waves {
    sine,
    square,
    triangle
};

//classes
ESP32Encoder encoder;   //encoder
VL53L0X sensor1;
VL53L0X sensor2;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
Menu menu(&u8g2);

//task handles
TaskHandle_t menu_handle;
TaskHandle_t sensor_handle;
TaskHandle_t play_handle;

//global variables for data exchange
float target_frequency = MIN_FREQ;
float target_amplitude = AMPLITUDE;
int target_wave_type = 0;
int target_skew = 50;
int target_duty_cycle = 50;
int target_min_freq = MIN_FREQ;
int target_max_freq = MAX_FREQ;
int target_sample_rate = SAMPLE_RATE;
int target_table_size = TABLE_SIZE;
bool update_menu = false;
bool started = false;


//sensor configuration rutine
void registerSensor(int enable_pin, uint8_t address, VL53L0X *sensor) {
    pinMode(enable_pin, OUTPUT);
    digitalWrite(enable_pin, HIGH); //enable sensor on enable_pinn
    sensor->init(true);             //initialize the sensor
    sensor->setAddress(address);    //set new address (required as all sensors have the same default address)
    pinMode(enable_pin, INPUT);
}

//TODO refactor
Entry *startFunction(int *position, bool *select, Entry *entry) {
    if (!started) {
        started = true;
        update_menu = true;
        i2s_start(I2S_NUM_1);
    }

    u8g2.clearBuffer();
    u8g2.setCursor(10, 20);
    u8g2.printf("%f", target_frequency);
    u8g2.sendBuffer();
    if (*select) {
        *select = false;
        update_menu = false;
        started = false;
        i2s_stop(I2S_NUM_1);
        entry->resetToParent(position);
        return entry->parent();
    }
    return entry;
}

//TODO store data in flash???
/*----(Tasks)----*/
//menu task for menu setup and execution
//TODO refactor
void menuTask(void *params) {
    u8g2.begin();
    u8g2.setFont(u8g2_font_6x10_mf);   //font dimensions: 11x11
    u8g2.clearBuffer();

    int start_data = 0;
    int settings_mod = 1;
    int settings_mod_data = 2;

    menu.begin();   //start menu
    menu.addByName("root", "Status", toggle, &start_data, {"stopped", "running"});
    menu.addByName("root", "Wave", picker, &target_wave_type, {"sine", "square", "triangle"});
    menu.addByName("root", "Skew", counter, 0, 100, &target_skew);
    menu.addByName("root", "Duty cycle", counter, 0, 100, &target_duty_cycle);
    menu.addByName("root", "Settings", "Settings");
    menu.addByName("Settings", "Steps", picker, &settings_mod_data, {"1", "10", "100", "1000"});
    menu.addByName("Settings", "Table size", counter, 32, 65536, &target_table_size, &settings_mod);
    menu.addByName("Settings", "Sample rate", counter, 44100, 96000, &target_sample_rate, &settings_mod);
    menu.addByName("Settings", "Frequency range", "F. range");
    menu.addByName("Frequency range", "Minimum", counter, 0, 22049, &target_min_freq, &settings_mod);
    menu.addByName("Frequency range", "Maximum", counter, 1, 22050, &target_max_freq, &settings_mod);

    int position = 0;
    bool select = false;

    bool running = false;

    while(true) {
        switch (settings_mod_data) {
            case 0: settings_mod = 1;    break;
            case 1: settings_mod = 10;   break;
            case 2: settings_mod = 100;  break;
            case 3: settings_mod = 1000; break;
        }

        if (start_data && !running) {
            i2s_start(I2S_NUM_1);
            running = true;
        }
        if (!start_data && running) {
            i2s_stop(I2S_NUM_1);
            running = false;
        }

        if (!digitalRead(23)) {
        while(!digitalRead(23));
            select = true;
        }
        position = encoder.getCount();
        menu.render(&position, &select, update_menu);
        encoder.setCount(position);
        delay(1);
    }
}

//TODO refactor
void sensorTask(void *params) {
    //configure sensors
    sensor1.setBus(&Wire1);
    sensor2.setBus(&Wire1);
    registerSensor(4, 18, &sensor1);
    registerSensor(0, 19, &sensor2);
    //sensor->setMeasurementTimingBudget(20000);
    //sensor->setMeasurementTimingBudget(20000);
    sensor1.startContinuous();
    sensor2.startContinuous();

    int len1, len2;
    int old_len1 = 0;
    int old_len2 = 0;

    //smoothing vectors
    //TODO add changable smoothing size???
    deque<int> sens1_array = {0, 0, 0};
    deque<int> sens2_array = {0, 0, 0};

    while(true) {
        len1 = sensor1.readRangeContinuousMillimeters();
        sens1_array.pop_front();
        sens1_array.push_back(len1);
        len1 = 0;
        for (int i = 0; i < sens1_array.size(); i++) {
            len1 += sens1_array[i];
        }
        len1 /= 3;
        if (len1 < 1000) {
            if (len1 < 60) len1 = 60;
            if (len1 > 800) len1 = 800;
            if (old_len1 != len1) {
                old_len1 = len1;
                target_frequency = (float)(map(len1, 60, 800, target_min_freq * 100, target_max_freq * 100)) / 100.0;
            }
        }

        len2 = sensor2.readRangeContinuousMillimeters();
        sens2_array.pop_front();
        sens2_array.push_back(len2);
        len2 = 0;
        for (int i = 0; i < sens2_array.size(); i++) {
            len2 += sens2_array[i];
        }
        len2 /= 3;
        if (len2 < 1000) {
            if (len2 < 60) len2 = 60;
            if (len2 > 800) len2 = 800;
            if (old_len2 != len2) {
                old_len2 = len2;
                target_amplitude = ((float)(map(len2, 60, 800, 5, 20)) / 100.0) * 16384;
            }
        }
    }
}

//main task for "playing" waves
void playTask(void *params) {
    //triangle config
    int t_peak_index, t_trough_index;
    float rise_delta, fall_delta;

    float frequency = MIN_FREQ;
    float amplitude = AMPLITUDE;
    float delta;

    uint16_t pos = 0;   //starting position
    int16_t int_sample; //final sample

    //this is somehow required?????????????
    //printf("playTask default frequency: %f\n", frequency);
    //printf("playTask default amplitude: %f\n", AMPLITUDE);
    delay(100);

    //TODO replacing SAMPLE_RATE and TABLE_SIZE makes this not report to watchdog

    while (true) {
        //smooth frequency "stitching"
        frequency += 0.01 * (target_frequency - frequency); //"slowly" aproach our desired frequency
        delta = (frequency * TABLE_SIZE) / SAMPLE_RATE;     //calculate delta - change in our non-existatn lookup table
        pos += delta;                                       //move to next position

        //generate apropriate wave type
        switch (target_wave_type) {
            case sine:
                int_sample = (int16_t)(amplitude * sin(2.0 * M_PI * (1.0 / TABLE_SIZE) * pos));   //calculate sine * amplitudebreak;
                break;

            case square:
                if (pos < (TABLE_SIZE / 100 * target_duty_cycle))
                    int_sample = (int16_t)amplitude;
                else
                    int_sample = (int16_t)(-amplitude);
                break;

            case triangle:
                t_peak_index = (0 * target_skew + (TABLE_SIZE / 2) * (MAX_SKEW - target_skew)) / MAX_SKEW;
                t_trough_index = TABLE_SIZE - t_peak_index;
                rise_delta = (float)amplitude / t_peak_index;
                fall_delta = (float)amplitude / ((t_trough_index - t_peak_index) / 2);
                if (pos < t_peak_index) 
                    int_sample =  (int16_t)(rise_delta * pos);
                else if (pos < t_trough_index)
                    int_sample = (int16_t)(amplitude - fall_delta * (pos - t_peak_index));
                else
                    int_sample = (int16_t)(-amplitude + rise_delta * (pos - t_trough_index));
                break;
        }

        //change amplitude only when going through zero
        if (int_sample == 0)
            amplitude = target_amplitude;

        //write sample to driver
        size_t i2s_bytes_write;
        i2s_write(I2S_NUM_1, &int_sample, sizeof(uint16_t), &i2s_bytes_write, portMAX_DELAY);

        //overflow fix
        if (pos >= TABLE_SIZE)
            pos = pos - TABLE_SIZE;
    }
}



void setup(void) {
    //i2c configuration
    Wire.begin(SDA, SCL, 100000);   //display wire
    Wire1.begin(18, 19);            //sensors wire


    //encoder configuration (used only inside menu)
    encoder.attachSingleEdge(36, 39);
    encoder.setFilter(1023);
    encoder.clearCount();
    pinMode(23, INPUT_PULLUP);


    //i2s configuration
    //i2s pins
    i2s_pin_config_t i2sPins = {
        .bck_io_num = 16,
        .ws_io_num = 17,
        .data_out_num = 25,
        .data_in_num = -1};
    
    //i2s config for writing both channels of I2S
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

    i2s_driver_install(I2S_NUM_1, &i2sConfig, 0, NULL); //install and start i2s driver
    i2s_stop(I2S_NUM_1);                                //instantly stop the driver
    i2s_set_pin(I2S_NUM_1, &i2sPins);                   //set up the i2s pins
    i2s_zero_dma_buffer(I2S_NUM_1);                     //clear the DMA buffers


    //create tasks
    xTaskCreatePinnedToCore(menuTask, "menu_task", 20000, NULL, 1, &menu_handle, 1);    //menu task needs to be pinned to a core, to avoid garbage
    xTaskCreate(sensorTask, "sensor_task", 10000, NULL, 1, &sensor_handle);
    xTaskCreate(playTask, "play_task", 10000, NULL, 1, &play_handle);
}


void loop(void) {
    //nothing here as everything has it's own task
}