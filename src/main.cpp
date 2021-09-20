
/*#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_spi_flash.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "driver/i2s.h"
#include "driver/adc.h"
#include "audio_example_file.h"
#include "esp_adc_cal.h"
#include <driver/i2s.h>
#include <math.h>

static const char* TAG = "ad/da";
#define V_REF   1100
#define ADC1_TEST_CHANNEL (ADC1_CHANNEL_7)

#define PARTITION_NAME   "storage"

//enable record sound and save in flash
#define RECORD_IN_FLASH_EN        0
//enable replay recorded sound in flash
#define REPLAY_FROM_FLASH_EN      0

//i2s number
#define EXAMPLE_I2S_NUM           0
//i2s sample rate
#define EXAMPLE_I2S_SAMPLE_RATE   16000
//i2s data bits
#define EXAMPLE_I2S_SAMPLE_BITS   16
//enable display buffer for debug
#define EXAMPLE_I2S_BUF_DEBUG     0
//I2S read buffer length
#define EXAMPLE_I2S_READ_LEN      16 * 1024
//I2S data format
#define EXAMPLE_I2S_FORMAT        I2S_CHANNEL_FMT_RIGHT_LEFT
//I2S channel number
#define EXAMPLE_I2S_CHANNEL_NUM   ((EXAMPLE_I2S_FORMAT < I2S_CHANNEL_FMT_ONLY_RIGHT) ? (2) : (1))
//I2S built-in ADC unit
#define I2S_ADC_UNIT              ADC_UNIT_1
//I2S built-in ADC channel
#define I2S_ADC_CHANNEL           ADC1_CHANNEL_0

//flash record size, for recording 5 seconds' data
#define FLASH_RECORD_SIZE         (EXAMPLE_I2S_CHANNEL_NUM * EXAMPLE_I2S_SAMPLE_RATE * EXAMPLE_I2S_SAMPLE_BITS / 8 * 5)
#define FLASH_ERASE_SIZE          (FLASH_RECORD_SIZE % FLASH_SECTOR_SIZE == 0) ? FLASH_RECORD_SIZE : FLASH_RECORD_SIZE + (FLASH_SECTOR_SIZE - FLASH_RECORD_SIZE % FLASH_SECTOR_SIZE)
//sector size of flash
#define FLASH_SECTOR_SIZE         0x1000
//flash read / write address
#define FLASH_ADDR                0x200000


// @brief I2S ADC/DAC mode init.

void example_erase_flash(void)
{
#if RECORD_IN_FLASH_EN
    printf("Erasing flash \n");
    const esp_partition_t *data_partition = NULL;
    data_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
            ESP_PARTITION_SUBTYPE_DATA_FAT, PARTITION_NAME);
    if (data_partition != NULL) {
        printf("partiton addr: 0x%08x; size: %d; label: %s\n", data_partition->address, data_partition->size, data_partition->label);
    }
    printf("Erase size: %d Bytes\n", FLASH_ERASE_SIZE);
    ESP_ERROR_CHECK(esp_partition_erase_range(data_partition, 0, FLASH_ERASE_SIZE));
#else
    printf("Skip flash erasing...\n");
#endif
}

// @brief debug buffer data

void example_disp_buf(uint8_t* buf, int length)
{
#if EXAMPLE_I2S_BUF_DEBUG
    printf("======\n");
    for (int i = 0; i < length; i++) {
        printf("%02x ", buf[i]);
        if ((i + 1) % 8 == 0) {
            printf("\n");
        }
    }
    printf("======\n");
#endif
}


// @brief Reset i2s clock and mode

void example_reset_play_mode(void)
{
    i2s_set_clk((i2s_port_t)(EXAMPLE_I2S_NUM), EXAMPLE_I2S_SAMPLE_RATE, (i2s_bits_per_sample_t)(EXAMPLE_I2S_SAMPLE_BITS), (i2s_channel_t)(EXAMPLE_I2S_CHANNEL_NUM));
}


// @brief Set i2s clock for example audio file

void example_set_file_play_mode(void)
{
    i2s_set_clk((i2s_port_t)(EXAMPLE_I2S_NUM), 16000, (i2s_bits_per_sample_t)(EXAMPLE_I2S_SAMPLE_BITS), (i2s_channel_t)1);
}


// @brief Scale data to 16bit/32bit for I2S DMA output.
//        DAC can only output 8bit data value.
//        I2S DMA will still send 16 bit or 32bit data, the highest 8bit contains DAC data.

int example_i2s_dac_data_scale(uint8_t* d_buff, uint8_t* s_buff, uint32_t len)
{
    uint32_t j = 0;
#if (EXAMPLE_I2S_SAMPLE_BITS == 16)
    for (int i = 0; i < len; i++) {
        d_buff[j++] = 0;
        d_buff[j++] = s_buff[i];
    }
    return (len * 2);
#else
    for (int i = 0; i < len; i++) {
        d_buff[j++] = 0;
        d_buff[j++] = 0;
        d_buff[j++] = 0;
        d_buff[j++] = s_buff[i];
    }
    return (len * 4);
#endif
}


// @brief Scale data to 8bit for data from ADC.
//        Data from ADC are 12bit width by default.
//        DAC can only output 8 bit data.
//        Scale each 12bit ADC data to 8bit DAC data.

void example_i2s_adc_data_scale(uint8_t * d_buff, uint8_t* s_buff, uint32_t len)
{
    uint32_t j = 0;
    uint32_t dac_value = 0;
#if (EXAMPLE_I2S_SAMPLE_BITS == 16)
    for (int i = 0; i < len; i += 2) {
        dac_value = ((((uint16_t) (s_buff[i + 1] & 0xf) << 8) | ((s_buff[i + 0]))));
        d_buff[j++] = 0;
        d_buff[j++] = dac_value * 256 / 4096;
    }
#else
    for (int i = 0; i < len; i += 4) {
        dac_value = ((((uint16_t)(s_buff[i + 3] & 0xf) << 8) | ((s_buff[i + 2]))));
        d_buff[j++] = 0;
        d_buff[j++] = 0;
        d_buff[j++] = 0;
        d_buff[j++] = dac_value * 256 / 4096;
    }
#endif
}


// @brief I2S ADC/DAC example
////        1. Erase flash
////        2. Record audio from ADC and save in flash
////        3. Read flash and replay the sound via DAC
//        4. Play an example audio file(file format: 8bit/8khz/single channel)
//        5. Loop back to step 3

void example_i2s_adc_dac(void*arg)
{

    int i2s_read_len = EXAMPLE_I2S_READ_LEN;
    int flash_wr_size = 0;
    size_t bytes_read, bytes_written;

    uint8_t* flash_read_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));
    uint8_t* i2s_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));
    while (1) {
        //4. Play an example audio file(file format: 8bit/16khz/single channel)
        printf("Playing file example: \n");
        int offset = 0;
        int tot_size = sizeof(audio_table);
        example_set_file_play_mode();
        while (offset < tot_size) {
            int play_len = ((tot_size - offset) > (4 * 1024)) ? (4 * 1024) : (tot_size - offset);
            int i2s_wr_len = example_i2s_dac_data_scale(i2s_write_buff, (uint8_t*)(audio_table + offset), play_len);
            i2s_write((i2s_port_t)EXAMPLE_I2S_NUM, i2s_write_buff, i2s_wr_len, &bytes_written, portMAX_DELAY);
            offset += play_len;
            example_disp_buf((uint8_t*) i2s_write_buff, 32);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
        example_reset_play_mode();
    }
    free(flash_read_buff);
    free(i2s_write_buff);
    vTaskDelete(NULL);
}

void adc_read_task(void* arg)
{
    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(ADC1_TEST_CHANNEL, ADC_ATTEN_11db);
    esp_adc_cal_characteristics_t characteristics;
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, V_REF, &characteristics);
    while(1) {
        uint32_t voltage;
        esp_adc_cal_get_voltage( ADC_CHANNEL_7, &characteristics, &voltage);
        ESP_LOGI(TAG, "%d mV", voltage);
        vTaskDelay(200 / portTICK_RATE_MS);
    }
}


void setup(void) {
    int i2s_num = EXAMPLE_I2S_NUM;
    i2s_config_t i2s_config = {
       .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN | I2S_MODE_ADC_BUILT_IN),
       .sample_rate =  EXAMPLE_I2S_SAMPLE_RATE,
       .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
       .channel_format = EXAMPLE_I2S_FORMAT,
       .communication_format = I2S_COMM_FORMAT_I2S_MSB,
       .intr_alloc_flags = 0,
       .dma_buf_count = 2,
       .dma_buf_len = 1024,
       .use_apll = 1
    };
    //install and start i2s driver
    i2s_driver_install((i2s_port_t)(i2s_num), &i2s_config, 0, NULL);
    //init DAC pad
    i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
    //init ADC pad
    i2s_set_adc_mode(I2S_ADC_UNIT, I2S_ADC_CHANNEL);
    
    esp_log_level_set("I2S", ESP_LOG_INFO);
    xTaskCreate(example_i2s_adc_dac, "example_i2s_adc_dac", 1024 * 2, NULL, 5, NULL);
    //xTaskCreate(adc_read_task, "ADC read task", 2048, NULL, 5, NULL);
}

void loop(void) {
}
*/

/* I2S Example
    This example code will output 100Hz sine wave and triangle wave to 2-channel of I2S driver
    Every 5 seconds, it will change bits_per_sample [16, 24, 32] for i2s data
    This example code is in the Public Domain (or CC0 licensed, at your option.)
    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
*/

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
        i2s_write((i2s_port_t)I2S_NUM, &sample, sizeof(sample), &i2s_bytes_write, 100);
        //printf("Wanted %d vs Writen: %d\n", sizeof(sample), i2s_bytes_write);
    }
}

void setup(void) {
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

    generateSine(data.amplitude, sine, WAVE_TABLE_SIZE);
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
    }
    playWave(sine, WAVE_TABLE_SIZE, data.frequency, 0.1);
}