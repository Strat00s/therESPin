#include <Arduino.h>
#include <math.h>
#include <ESP32Encoder.h>
#include <driver/i2s.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <driver/dac.h>
#include <Wire.h>
#include <VL53L0X.h>

#define TABLE_SIZE 65536 //table size for calculations (wave samples) -> how many waves can be created
#define AMPLITUDE 0.1 * 16384
#define MAX_SKEW 100   //max skew for triangle wave

#define SAMPLE_RATE 44100 //audio sample rate
#define MIN_FREQ 170.0
#define MAX_FREQ 440.0

ESP32Encoder encoder;   //encoder
//queues for sending data between tasks

//fix the queues (sine waves gets broken for some reason)
QueueHandle_t type_q;
QueueHandle_t freq_q;
QueueHandle_t ampl_q;
QueueHandle_t skew_q;
VL53L0X sensor1;
VL53L0X sensor2;


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
int target_skew = 0;
int target_duty_cycle = 0;

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


    //int old_type = 0;
    while (true) {
        //do "math" if there are items in queue
        //frequency
        //if (uxQueueMessagesWaiting(freq_q)) {
        //    xQueueReceive(freq_q, &target_frequency, 0);
        //    //delta = (frequency * TABLE_SIZE) / (float)SAMPLE_RATE;
        //}

        //amplitude
        //if (uxQueueMessagesWaiting(ampl_q)) {
        //    xQueueReceive(ampl_q, &target_amplitude, 0);
        //}

        //wave type
        //if (uxQueueMessagesWaiting(type_q)) {
        //    xQueueReceive(type_q, &type, 0);
        //}
        //skew
        //if (uxQueueMessagesWaiting(skew_q)) {
        //    xQueueReceive(skew_q, &skew, 0);
        //    //t_start_index = 0;
        //    t_peak_index = (0 * skew + (TABLE_SIZE / 2) * (MAX_SKEW - skew)) / MAX_SKEW;
        //    t_trough_index = TABLE_SIZE - t_peak_index;
        //    //t_end_index = TABLE_SIZE;
        //    rise_delta = (float)amplitude / t_peak_index;
        //    fall_delta = (float)amplitude / ((t_trough_index - t_peak_index) / 2);
        //}


        //uint16_t pos = uint32_t(i * delta) % TABLE_SIZE;    //skip
        frequency += 0.05 * (target_frequency - frequency);
        delta = (frequency * TABLE_SIZE) / SAMPLE_RATE;
        pos += delta;

        //INFO sine wave works best alone only with amplitude and frequency (queues)
        //sine
        if (target_wave_type == 0) {
            //if (old_type != type) {
            //    frequency = target_frequency;
            //    pos = 0;
            //}
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
        //old_type = type;


        //INFO temporary amplitude fix???
        if (int_sample == 0)
                amplitude = target_amplitude;
        
        
        size_t i2s_bytes_write;
        i2s_write(I2S_NUM_1, &int_sample, sizeof(uint16_t), &i2s_bytes_write, portMAX_DELAY);
        //printf("%d,%d\n", int_sample, pos);

        //INFO fix for other wave types (and generaly for overflow)
        if (pos >= TABLE_SIZE)
            pos = pos - TABLE_SIZE;
    }
}

void setup(void) {
    Wire.begin();

    registerSensor(18, 18, &sensor1);
    registerSensor(19, 19, &sensor2);
    sensor1.startContinuous();
    sensor2.startContinuous();
    freq_q = xQueueCreate(1, sizeof(float));
    ampl_q = xQueueCreate(1, sizeof(float));
    
    //create queues
    type_q = xQueueCreate(1, sizeof(int));
    skew_q = xQueueCreate(1, sizeof(int));

    //encoder setup
    ESP32Encoder::useInternalWeakPullResistors=UP;
    encoder.attachSingleEdge(36, 39);
    encoder.setFilter(1023);
    encoder.clearCount();

    //buttons setup
    pinMode(9 , INPUT_PULLUP);
    pinMode(10, INPUT_PULLUP);
    pinMode(11, INPUT_PULLUP); 

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
}

int old_len1 = 0;
int old_len2 = 0;
int old_enc = 0;
int old_type = 0;
int wave_type = 0;

//TODO input smoothing

//TODO //WARNING //FIX find out why using multiple queues at the same time sometimes causes sine choppines 
void loop(void) {
    int len1 = sensor1.readRangeContinuousMillimeters();
    int len2 = sensor2.readRangeContinuousMillimeters();
    int enc = encoder.getCount();
    if (len1 < 1000) {
        if (len1 < 60) len1 = 60;
        if (len1 > 800) len1 = 800;
        if (old_len1 - len1) {
            old_len1 = len1;
            //float frequency = (float)(map(len1, 60, 800, MIN_FREQ * 100, MAX_FREQ * 100)) / 100.0;
            target_frequency = (float)(map(len1, 60, 800, MIN_FREQ * 100, MAX_FREQ * 100)) / 100.0;
            printf("Distance: %d freq: %f\n", len1, target_frequency);
            //if (!uxQueueMessagesWaiting(freq_q))
            //    xQueueSend(freq_q, &frequency, portMAX_DELAY);
        }
    }

    if (len2 < 1000) {
        if (len2 < 60) len2 = 60;
        if (len2 > 800) len2 = 800;
        if (old_len2 != len2) {
            old_len2 = len2;
            //float amplitude = (float)(map(len2, 60, 800, 0, 100)) / 100.0;
            //amplitude *= 16384;
            
            target_amplitude = ((float)(map(len2, 60, 800, 5, 20)) / 100.0) * 16384;
            printf("Distance: %d ampl: %f\n", len2, target_amplitude);
            
            //if (!uxQueueMessagesWaiting(ampl_q))
            //    xQueueSend(ampl_q, &amplitude, portMAX_DELAY);
        }
    }

    //TODO fix this - sine wave is somehow choppy
    if (enc < 0)
        encoder.setCount(0);
    if (enc > 100)
        encoder.setCount(100);
    if (old_enc != enc) {
        old_enc = enc;
        target_skew = enc;
        target_duty_cycle = enc;
        printf("Target skew: %d\n", target_skew);
        printf("Target dc: %d\n", target_duty_cycle);
        //if (!uxQueueMessagesWaiting(skew_q))
        //    xQueueSend(skew_q, &enc, portMAX_DELAY);
    }

    //read buttons and queue desired wave type
    if (!digitalRead(9))  wave_type = 0;
    if (!digitalRead(10)) wave_type = 1;
    if (!digitalRead(11)) wave_type = 2;
    if (old_type != wave_type) {
        old_type = wave_type;
        printf("Wave type: ");
        switch (wave_type) {
            case 0:  printf("sine\n");     break;
            case 1:  printf("square\n");   break;
            case 2:  printf("triangle\n"); break;
            default: printf("unknown\n");  break;
        }
        target_wave_type = wave_type;
        //if (!uxQueueMessagesWaiting(type_q))
        //    xQueueSend(type_q, &wave_type, portMAX_DELAY);
    }
    delay(10);
}