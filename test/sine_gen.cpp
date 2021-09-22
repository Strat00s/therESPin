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
    int skew = 50;
    //triangle wave: rest -> peak -> rest -> trough -> rest; here: start -> peak -> trough -> end
    int t_start_index = 0;
    int t_peak_index = (0 * skew + (TABLE_SIZE / 2) * (MAX_SKEW - skew)) / MAX_SKEW;
    int t_trough_index = TABLE_SIZE - t_peak_index;
    int t_end_index = TABLE_SIZE;

    float rise_delta = (float)AMPLITUDE / t_peak_index;
    float fall_delta = (float)AMPLITUDE / ((t_trough_index - t_peak_index) / 2);
    
    printf("dT,category,format,val\n");
    
    for (int i = 0; i < TABLE_SIZE; i++) {
        //sine wave
        f_sine[i] = (float)(AMPLITUDE * sin(2.0 * M_PI * (1.0 / TABLE_SIZE) * i));
        sine[i] = (int)f_sine[i];

        //square
        if (i < TABLE_SIZE / 2) {
            f_square[i] = AMPLITUDE;
            square[i] = (int)AMPLITUDE;
        }
        else {
            f_square[i] = (-AMPLITUDE);
            square[i] = (int)-AMPLITUDE;
        }

        //triangle
        if (i < t_peak_index) {
            f_triangle[i] = (rise_delta * i);
        }
        else if (i < t_trough_index) {
            f_triangle[i] = (AMPLITUDE - fall_delta * (i - t_peak_index));
        }
        else {
            f_triangle[i] = (-AMPLITUDE + rise_delta * (i - t_trough_index));
        }
        printf("%d,f_sine,red,%f\n", i, f_sine[i]);
        printf("%d,f_square,green,%f\n", i, f_square[i]);
        printf("%d,f_triangle,blue,%f\n", i, f_triangle[i]);
    }

    //for (int i = t_start_index; i < t_peak_index; i++) {
    //    f_triangle[i] = (rise_delta * i);
    //}
    //for (int i = t_peak_index; i < t_trough_index; i++) {
    //    f_triangle[i] = (AMPLITUDE - fall_delta * (i - t_peak_index));
    //}
    //for (int i = t_trough_index; i < t_end_index; i++) {
    //    f_triangle[i] = (-AMPLITUDE + rise_delta * (i - t_trough_index));
    //}
}