#include <Arduino.h>
#include <math.h>
#include <ESP32Encoder.h>

#define TABLE_SIZE 1024
#define AMPLITUDE 100
#define MAX_SKEW 100

float wave[TABLE_SIZE] = {0.0};

ESP32Encoder encoder;


int skew = 50;
int t_start_index = 0;
int t_peak_index = (0 * skew + (TABLE_SIZE / 2) * (MAX_SKEW - skew)) / MAX_SKEW;
int t_trough_index = TABLE_SIZE - t_peak_index;
int t_end_index = TABLE_SIZE;
float rise_delta = (float)AMPLITUDE / t_peak_index;
float fall_delta = (float)AMPLITUDE / ((t_trough_index - t_peak_index) / 2);

int enc_val = 0;
int old_enc = 0;

int i = 0;


void setup(void) {
    encoder.attachSingleEdge(36, 39);
    encoder.setFilter(1023);
    encoder.clearCount();
}

void loop(void) {
    enc_val = encoder.getCount();
    if (enc_val > 30) enc_val = 30;
    if (enc_val < 0) enc_val = 0;
    encoder.setCount(enc_val);

    if (enc_val != old_enc) {
        old_enc = enc_val;
        printf("Encoder: %d ", enc_val);
        printf("skew: %d\n", skew = (31 - (enc_val + 1)) * 10);
    }

    //sine
    if (enc_val < 10) {
        wave[i] = (float)(AMPLITUDE * sin(2.0 * M_PI * (1.0 / TABLE_SIZE) * i));
    }

    //square
    else if (enc_val < 20) {
        if (i < TABLE_SIZE / 2) {
            wave[i] = AMPLITUDE;
            //square[i] = (int)AMPLITUDE;
        }
        else {
            wave[i] = (-AMPLITUDE);
            //wave[i] = (int)-AMPLITUDE;
        }
    }

    //triangle
    else if (enc_val < 31) {
        skew = (31 - (enc_val + 1)) * 10;
        t_start_index = 0;
        t_peak_index = (0 * skew + (TABLE_SIZE / 2) * (MAX_SKEW - skew)) / MAX_SKEW;
        t_trough_index = TABLE_SIZE - t_peak_index;
        t_end_index = TABLE_SIZE;
        rise_delta = (float)AMPLITUDE / t_peak_index;
        fall_delta = (float)AMPLITUDE / ((t_trough_index - t_peak_index) / 2);

        if (i < t_peak_index) {
            wave[i] = (rise_delta * i);
        }
        else if (i < t_trough_index) {
            wave[i] = (AMPLITUDE - fall_delta * (i - t_peak_index));
        }
        else {
            wave[i] = (-AMPLITUDE + rise_delta * (i - t_trough_index));
        }
    }
    printf("%f\n", wave[i]);

    i++;
    if (i >= TABLE_SIZE) i = 0;
}