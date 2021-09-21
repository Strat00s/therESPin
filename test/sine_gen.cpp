#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define TABLE_SIZE 100
#define AMPLITUDE 8

int main() {
    int sine[TABLE_SIZE] = {0};
    int square[TABLE_SIZE] = {0};
    int triangle[TABLE_SIZE] = {0};
    int sawtooth[TABLE_SIZE] = {0};
    float f_sine[TABLE_SIZE] = {0.0};
    float f_square[TABLE_SIZE] = {0.0};
    float f_triangle[TABLE_SIZE] = {0.0};
    float f_sawtooth[TABLE_SIZE] = {0.0};

    printf("dT,category,format,val\n");
    //printf("dT,val\n");
    float delta = (float)AMPLITUDE / (TABLE_SIZE / 2.0);
    for (int i = 0; i < TABLE_SIZE; i++) {
        //sine wave
        f_sine[i] = (AMPLITUDE * sin(2.0 * M_PI * (1.0 / TABLE_SIZE) * i));
        sine[i] = (int)f_sine[i];

        //square wave and sawtooth
        if (i < TABLE_SIZE / 2) {
            f_square[i] = AMPLITUDE;
            square[i] = (int)AMPLITUDE;
            f_sawtooth[i] = (delta * i);
            sawtooth[i] = (int)f_sawtooth[i];
        }
        else {
            f_square[i] = -AMPLITUDE;
            square[i] = (int)-AMPLITUDE;
            f_sawtooth[i] = (-AMPLITUDE * 2 + delta * i);
            sawtooth[i] = (int)f_sawtooth[i];
        }

        //triangle wave
        if (i < TABLE_SIZE / 4) {
            f_triangle[i] = (delta * i * 2);
            triangle[i] = (int)f_triangle[i];
        }
        else if (i < (TABLE_SIZE / 2)) {
            f_triangle[i] = (AMPLITUDE * 2 - delta * i * 2);
            triangle[i] = (int)f_triangle[i];
        }
        else if (i < (TABLE_SIZE / 4) * 3) {
            f_triangle[i] = (AMPLITUDE * 2 - delta * i * 2);
            triangle[i] = (int)f_triangle[i];
        }
        else {
            f_triangle[i] = (-AMPLITUDE * 4 + delta * i * 2);
            triangle[i] = (int)f_triangle[i];
        }

        printf("%d,sine,red,%d\n", i, sine[i]);
        printf("%d,f_sine,violet,%f\n", i, f_sine[i]);
        printf("%d,square,green,%d\n", i, square[i]);
        printf("%d,f_square,yellow,%f\n", i, f_square[i]);
        printf("%d,triangle,blue,%d\n", i, triangle[i]);
        printf("%d,f_triangle,pink,%f\n", i, f_triangle[i]);
        printf("%d,sawtooth,lime,%d\n", i, sawtooth[i]);
        printf("%d,f_sawtooth,orange,%f\n", i, f_sawtooth[i]);
    }
}