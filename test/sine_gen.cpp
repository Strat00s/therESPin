#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define TABLE_SIZE 1024
#define AMPLITUDE 100
#define MAX_SKEW 100

int main() {
    int sine[TABLE_SIZE] = {0};
    float f_sine[TABLE_SIZE] = {0.0};
    int square[TABLE_SIZE] = {0};
    float f_square[TABLE_SIZE] = {0.0};
    int triangle[TABLE_SIZE] = {0};
    float f_triangle[TABLE_SIZE] = {0.0};

    //50% skew = triangle wave; 0% = sawtooth; 100% = inverted sawtooth
    int skew = 0;
    //triangle wave: rest -> peak -> rest -> trough -> rest; here: start -> peak -> trough -> end
    int t_start_index = 0;
    int t_peak_index = (0 * skew + (TABLE_SIZE / 2) * (MAX_SKEW - skew)) / MAX_SKEW;
    int t_trough_index = TABLE_SIZE - t_peak_index;
    int t_end_index = TABLE_SIZE;

    float rise_delta = (float)AMPLITUDE / t_peak_index;
    float fall_delta = (float)AMPLITUDE / ((t_trough_index - t_peak_index) / 2);
    
    printf("dT,category,format,val\n");
    
    for (int i = t_start_index; i < t_start_index; i++) {
        f_triangle[i] = (rise_delta * i);
    }
    for (int i = t_peak_index; i < t_trough_index; i++) {
        f_triangle[i] = (AMPLITUDE - fall_delta * (i - t_peak_index));
    }
    for (int i = t_trough_index; i < t_end_index; i++) {
        f_triangle[i] = (-AMPLITUDE + rise_delta * (i - t_trough_index));
    }

    for (int i = 0; i < TABLE_SIZE; i++) {
        //sine wave
        f_sine[i] = (float)(AMPLITUDE * sin(2.0 * M_PI * (1.0 / TABLE_SIZE) * i));
        sine[i] = (int)f_sine[i];

        //square wave and sawtooth
        if (i < TABLE_SIZE / 2) {
            f_square[i] = AMPLITUDE;
            square[i] = (int)AMPLITUDE;
        }
        else {
            f_square[i] = (-AMPLITUDE);
            square[i] = (int)-AMPLITUDE;
        }

        //printf("%d,sine,0,%d\n", i, sine[i]);
        //printf("%d,f_sine,1,%f\n", i, f_sine[i]);
        //printf("%d,square,2,%d\n", i, square[i]);
        //printf("%d,f_square,3,%f\n", i, f_square[i]);
        //printf("%d,triangle,4,%d\n", i, triangle[i]);
        //printf("%d,f_triangle,red,%f\n", i, f_triangle[i]);
        //printf("%d,sawtooth,6,%d\n", i, sawtooth[i]);
        //printf("%d,f_sawtooth,green,%f\n", i, f_sawtooth[i]);
        //printf("%d,sawtooth_skew,8,%d\n", i, sawtooth_skew[i]);
        //printf("%d,f_sawtooth_skew,blue,%f\n", i, f_sawtooth_skew[i]);
    }
}