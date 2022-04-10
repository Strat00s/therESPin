#include <math.h>
#include <iostream>
#include <Arduino.h>
//multitasking
#include <freertos/task.h>
#include <freertos/queue.h>
//IO
#include <SPI.h>
#include <Wire.h>
#include <VL53L0X.h>
#include <ESP32Encoder.h>
//audio
#include <driver/i2s.h>
#include <driver/dac.h>
//custom
#include "MenuLib/menu.hpp"

//TODO: Changeable working table size
//TODO: (if time) move to whole numbers instead of floats for speed (and maybe better theremin like wave)
//TODO: finish final menu

using namespace std;

#define TABLE_SIZE 65536    //table size for calculations (wave samples) -> how granual the frequency is
#define AMPLITUDE 512       //kinda arbitrary value
#define MAX_SKEW 100        //max skew for triangle wave

#define SAMPLE_RATE 44100   //audio sample rate
#define MIN_FREQ 65.41
#define MAX_FREQ 3951.07
//#define MAX_FREQ 22050

//sensors
#define SENSOR_L_PIN 4      //first sensor pin
#define SENSOR_R_PIN 0      //second sensor pin
#define SENSOR_L_ADDR 18    //first sensor I2C address
#define SENSOR_R_ADDR 19    //second sensor I2C address

//detection ranges (in mm)
#define DISABLE_RANGE 1000  //range where sound generation will be disabled (amplitude will be lowered to 0)
#define MAX_RANGE 500       //maximum detection range
#define MIN_RANGE 50        //minimum detection range

#define CALIBRATION_OFFSET (-56)


enum waves {
    sine,
    square,
    triangle,
    custom_wave
};

//classes
ESP32Encoder encoder;                                               //encoder
VL53L0X sensor_L;                                                   //sensors
VL53L0X sensor_R;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);   //display
Menu menu(&u8g2);                                                   //menu

//global variables for data exchange and settings
int target_frequency = MIN_FREQ;
int target_amplitude = AMPLITUDE;
int target_wave_type = 3;
int target_skew = 50;
int target_duty_cycle = 50;
int target_min_freq = MIN_FREQ;
int target_max_freq = MAX_FREQ;
int target_sample_rate = SAMPLE_RATE;
int target_table_size = TABLE_SIZE;
bool update_menu = false;
bool started = false;
int start_data = 0;


int global_range = 0;


/*----(IO)----*/
//sensor configuration rutine
void registerSensor(int enable_pin, uint8_t address, VL53L0X *sensor) {
    pinMode(enable_pin, OUTPUT);
    digitalWrite(enable_pin, HIGH); //enable sensor on enable_pinn
    sensor->init(true);             //initialize the sensor
    sensor->setAddress(address);    //set new address (required as all sensors have the same default address)
    pinMode(enable_pin, INPUT);
}

//sensor task for reading, averaging and writing data do queues
void sensorTask(void *params) {
    //configure sensors
    sensor_L.setBus(&Wire1);
    sensor_R.setBus(&Wire1);
    registerSensor(SENSOR_L_PIN, SENSOR_L_ADDR, &sensor_L);
    registerSensor(SENSOR_R_PIN, SENSOR_R_ADDR, &sensor_R);
    sensor_L.setMeasurementTimingBudget(20000); //20ms timing budger (lowest possible)
    sensor_R.setMeasurementTimingBudget(20000);
    sensor_L.startContinuous();
    sensor_R.startContinuous();

    int distance_L, distance_R;
    int old_distance_L = 0;
    int old_distance_R = 0;

    //smoothing vectors
    int array_len = 3;
    deque<int> smoothing_array_L = {0, 0, 0};
    deque<int> smoothing_array_R = {0, 0, 0};

    while(true) {
        distance_L = sensor_L.readRangeContinuousMillimeters() + CALIBRATION_OFFSET;
        distance_R = sensor_R.readRangeContinuousMillimeters() + CALIBRATION_OFFSET;

        //Smooth input using moving average
        smoothing_array_L.pop_front();
        smoothing_array_R.pop_front();
        for (int i = 0; i < array_len - 1; i++) {
            distance_L += smoothing_array_L[i];
            distance_R += smoothing_array_R[i];
        }
        distance_L /= array_len;
        distance_R /= array_len;

        smoothing_array_L.push_back(distance_L);
        smoothing_array_R.push_back(distance_R);

        //Do not play anything, while at least one hand is out of range
        if (distance_L > DISABLE_RANGE || distance_R > DISABLE_RANGE) {
            target_amplitude -= 100;
            if (target_amplitude < 0)
                target_amplitude = 0;
        }
        else {
            if (distance_L < MIN_RANGE) distance_L = MIN_RANGE;
            if (distance_L > MAX_RANGE) distance_L = MAX_RANGE;
            if (old_distance_L != distance_L) {
                old_distance_L = distance_L;
                global_range = distance_L;
                target_frequency = map(distance_L, MIN_RANGE, MAX_RANGE, target_min_freq, target_max_freq);
            }
            if (distance_R < MIN_RANGE) distance_R = MIN_RANGE;
            if (distance_R > MAX_RANGE) distance_R = MAX_RANGE;
            if (old_distance_R != distance_R) {
                old_distance_R = distance_R;
                target_amplitude = map(distance_R, MIN_RANGE, MAX_RANGE, 0, 2048);
            }
        }
    }
}

//not currently used
Entry *startFunction(int *position, bool *select, Entry *entry) {
    //running
    if (start_data) {
        u8g2.clearBuffer();
        u8g2.setCursor(10, 20);
        u8g2.printf("%f", target_frequency);
        u8g2.sendBuffer();
    }
    else {
        u8g2.clearBuffer();
        u8g2.setCursor(10, 20);
        u8g2.printf("not runnign");
        u8g2.sendBuffer();
    }
    if (*select) {
        *select = false;
        update_menu = false;
        started = false;
        //i2s_stop(I2S_NUM_1);
        entry->resetToParent(position);
        return entry->parent();
    }
    return entry;
}

//menu task for menu setup and execution
void menuTask(void *params) {
    printf("u8g2 begin: %d\n", u8g2.begin());
    u8g2.setFont(u8g2_font_6x10_mf);   //font dimensions: 11x11
    u8g2.clearBuffer();

    //int start_data = 0;
    int settings_mod = 1;
    int settings_mod_data = 2;

    //Create menu
    menu.begin();   //start menu
    //menu.addByName("root",            "Status screen",  "Header", startFunction);
    menu.addByName("root",            "Start",           toggle, &start_data, {"idle", "running"});
    menu.addByName("root",            "Wave",            picker, &target_wave_type, {"sine", "square", "triangle", "custom"});
    menu.addByName("root",            "Skew",            counter, 0, 100, &target_skew);
    menu.addByName("root",            "Duty cycle",      counter, 0, 100, &target_duty_cycle);
    menu.addByName("root",            "Settings",        "Settings");
    menu.addByName("Settings",        "Steps",           picker, &settings_mod_data, {"1", "10", "100", "1000"});
    menu.addByName("Settings",        "Table size X",    counter, 32, 65536, &target_table_size, &settings_mod);    //Cannot edit as of now
    menu.addByName("Settings",        "Frequency range", "F. range");
    menu.addByName("Frequency range", "Minimum",         counter, 0, 22049, &target_min_freq, &settings_mod);
    menu.addByName("Frequency range", "Maximum",         counter, 1, 22050, &target_max_freq, &settings_mod);
    printf("Menus added\n");

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
            menu.getEntryByName("Start")->name = "Stop";
            update_menu = true;
            i2s_start(I2S_NUM_1);
            running = true;
        }
        if (!start_data && running) {
            menu.getEntryByName("Stop")->name = "Start";
            update_menu = true;
            i2s_stop(I2S_NUM_1);
            running = false;
        }

        if (!digitalRead(15)) {
            while(!digitalRead(15));
                select = true;
        }
        position = encoder.getCount();
        menu.render(&position, &select, &update_menu);
        encoder.setCount(position);
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

    delay(100);

    //TODO fix variable table_size causing freezes

    while (true) {
        //smooth frequency "stitching"
        frequency += 0.01 * (target_frequency - frequency); //"slowly" aproach our desired frequency
        delta = (frequency * target_table_size) / SAMPLE_RATE;     //calculate delta - change in our non-existant lookup table
        pos += delta;                                       //move to next position

        //change amplitude only when going through zero
        if (int_sample == 0)
            amplitude = target_amplitude;

        //generate apropriate wave type
        switch (target_wave_type) {
            case sine:
                int_sample = static_cast<int16_t>(sin(2.0 * M_PI * (1.0 / target_table_size) * pos) * amplitude);   //calculate sine * amplitudebreak;
                break;

            case square:
                if (pos < (target_table_size / 100 * target_duty_cycle))
                    int_sample = static_cast<int16_t>(target_amplitude);
                else
                    int_sample = static_cast<int16_t>(-target_amplitude);
                break;

            case triangle:
                t_peak_index = (0 * target_skew + (target_table_size / 2) * (MAX_SKEW - target_skew)) / MAX_SKEW;
                t_trough_index = target_table_size - t_peak_index;
                rise_delta = static_cast<float>(amplitude / t_peak_index);
                fall_delta = static_cast<float>(amplitude / ((t_trough_index - t_peak_index) / 2));
                if (pos < t_peak_index) 
                    int_sample =  static_cast<int16_t>(rise_delta * pos);
                else if (pos < t_trough_index)
                    int_sample = static_cast<int16_t>(amplitude - fall_delta * (pos - t_peak_index));
                else
                    int_sample = static_cast<int16_t>(-amplitude + rise_delta * (pos - t_trough_index));
                break;
            
            case custom_wave:
                //int_sample = static_cast<int16_t>(amplitude * sin(2.0 * M_PI * (1.0 / target_table_size) * pos));
                float x = 2.0 * M_PI * (1.0 / target_table_size) * pos;
                float modifier = static_cast<float>(map(global_range, MIN_RANGE, MAX_RANGE, 1000, 0) / 1000.0); //TODO fix
                int_sample = static_cast<int16_t>(amplitude * sin(x + sin(x) + modifier));
                break;
        }

        //write sample to driver
        size_t i2s_bytes_write;
        i2s_write(I2S_NUM_1, &int_sample, sizeof(int16_t), &i2s_bytes_write, portMAX_DELAY);

        //overflow fix
        if (pos >= target_table_size)
            pos = pos - target_table_size;
    }
}



void setup(void) {
    //i2c configuration
    Wire.begin (SDA, SCL, 400000);  //display wire
    Wire1.begin(18,  19,  400000);  //sensors wire


    //encoder configuration (used only for menu)
    encoder.attachSingleEdge(36, 39);
    encoder.setFilter(1023);
    encoder.clearCount();
    pinMode(15, INPUT_PULLUP);


    //i2s configuration
    //i2s pins
    i2s_pin_config_t i2sPins = {
        .bck_io_num   = 16,
        .ws_io_num    = 17,
        .data_out_num = 25,
        .data_in_num  = -1
    };
    
    //i2s config for writing both channels of I2S
    i2s_config_t i2sConfig = {
        .mode                 = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate          = SAMPLE_RATE,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = 4,
        .dma_buf_len          = 64,
        .use_apll             = true
    };

    i2s_driver_install(I2S_NUM_1, &i2sConfig, 0, NULL); //install and start i2s driver
    i2s_stop(I2S_NUM_1);                                //instantly stop the driver
    i2s_set_pin(I2S_NUM_1, &i2sPins);                   //set up the i2s pins
    i2s_zero_dma_buffer(I2S_NUM_1);                     //clear the DMA buffers
    setCpuFrequencyMhz(240);

    //create tasks
    xTaskCreatePinnedToCore(menuTask,   "menu_task",   5000, NULL, 10, NULL, 1);    //menu task needs to be pinned to a signle core, to avoid garbage
    xTaskCreate            (sensorTask, "sensor_task", 2000, NULL, 10, NULL   );
    xTaskCreate            (playTask,   "play_task",   5000, NULL, 10, NULL   );
}


void loop(void) {
}