#include <Arduino.h>
#include <math.h>
#include "driver/i2s.h"

#define NUM_FRAMES_TO_SEND 128

#define FREQUENCY 440
#define SAMPLE_RATE 44100
#define MAGNITUDE 0.25

i2s_port_t m_i2sPort = I2S_NUM_1;

//I2SOutput *output;
//SampleSource *sampleSource;

TaskHandle_t m_i2sWriterTaskHandle;
QueueHandle_t m_i2sQueue;


float m_current_position = 0;

typedef struct {
    int16_t left;
    int16_t right;
} Frame_t;

void getFrames(Frame_t *frames, int number_frames) {
    float full_wave_samples = SAMPLE_RATE / FREQUENCY;
    float step_per_sample = M_TWOPI / full_wave_samples;
    // fill the buffer with data from the file wrapping around if necessary
    for (int i = 0; i < number_frames; i++) {
        frames[i].left = frames[i].right = 16384 * MAGNITUDE * sin(m_current_position);
        m_current_position += step_per_sample;
        // wrap around to maintain numerical stability
        if (m_current_position > M_TWOPI) {
            m_current_position -= M_TWOPI;
        }
    }
}

void i2sWriterTask(void *param) {
    //I2SOutput *output = (I2SOutput *)param;
    int availableBytes = 0;
    int buffer_position = 0;
    Frame_t frames;
    int32_t sample;
    size_t bytesWritten = 0;
    while (true) {
        //if (availableBytes == 0) {
        // get some frames from the wave file - a frame consists of a 16 bit left and right sample
        //getFrames(frames, 1);
        float full_wave_samples = SAMPLE_RATE / FREQUENCY;
        float step_per_sample = M_TWOPI / full_wave_samples;
        // fill the buffer with data from the file wrapping around if necessary
        sample = 16384 * MAGNITUDE * sin(m_current_position);
        m_current_position += step_per_sample;
        // wrap around to maintain numerical stability
        if (m_current_position > M_TWOPI) {
            m_current_position -= M_TWOPI;
        }

        i2s_write(m_i2sPort, &sample, sizeof(sample), &bytesWritten, portMAX_DELAY); //stereo -> we need to send both channels or set it for mono (somehow)
    }
}

void setup() {
    Serial.begin(115200);

    Serial.println("Starting up");

    Serial.println("Created sample source");

    //sampleSource = new SinWaveGenerator(44100, 440, 0.25);

    //sampleSource = new WAVFileReader("/sample.wav");

    Serial.println("Starting I2S Output");
    //output = new I2SOutput();
    //output->start(I2S_NUM_1, i2sPins, sampleSource);

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
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 64};

    //install and start i2s driver
    i2s_driver_install(I2S_NUM_1, &i2sConfig, 0, NULL);
    // set up the i2s pins
    i2s_set_pin(I2S_NUM_1, &i2sPins);
    // clear the DMA buffers
    i2s_zero_dma_buffer(I2S_NUM_1);
    // start a task to write samples to the i2s peripheral
    TaskHandle_t writerTaskHandle;
    xTaskCreate(i2sWriterTask, "i2s Writer Task", 4096, NULL, 1, &writerTaskHandle);
}

void loop()
{
  // nothing to do here - everything is taken care of by tasks
}