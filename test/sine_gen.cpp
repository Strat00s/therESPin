#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define TABLE_SIZE 64
#define AMPLITUDE 100
#define MAX_SKEW 100

int main() {
    int sine[TABLE_SIZE] = {0};
    int square[TABLE_SIZE] = {0};
    //int triangle[TABLE_SIZE] = {0};
    //int sawtooth[TABLE_SIZE] = {0};
    float f_sine[TABLE_SIZE] = {0.0};
    float f_square[TABLE_SIZE] = {0.0};
    //float f_triangle[TABLE_SIZE] = {0.0};
    //float f_sawtooth[TABLE_SIZE] = {0.0};

    int sawtooth_skew[5][TABLE_SIZE] = {0};
    float f_sawtooth_skew[5][TABLE_SIZE] = {0.0};


    int skew = 100;
    int skew_index = (0 * skew + (TABLE_SIZE / 2) * (MAX_SKEW - skew)) / MAX_SKEW;
    //printf("Skew: %d Skew index: %f\n", skew, skew_index);
    float delta = (float)AMPLITUDE / (TABLE_SIZE / 4.0);
    float sdelta = (float)AMPLITUDE / (TABLE_SIZE / 2.0);
    float skew_delta = (float)AMPLITUDE / skew_index;
    float sskew_delta = (float)AMPLITUDE / ((TABLE_SIZE - skew_index * 2) / 2);
    
    printf("dT,category,format,val\n");
    for (int j = 0; j < 5; j++) {
        int skew = j * 25;
        int skew_index = (0 * skew + (TABLE_SIZE / 2) * (MAX_SKEW - skew)) / MAX_SKEW;
        //printf("Skew: %d Skew index: %f\n", skew, skew_index);
        float delta = (float)AMPLITUDE / (TABLE_SIZE / 4.0);
        float sdelta = (float)AMPLITUDE / (TABLE_SIZE / 2.0);
        float skew_delta = (float)AMPLITUDE / skew_index;
        float sskew_delta = (float)AMPLITUDE / ((TABLE_SIZE - skew_index * 2) / 2);
        for (int i = 0; i < skew_index; i++) {
            f_sawtooth_skew[j][i] = (skew_delta * i);
        }
        for (int i = skew_index; i < TABLE_SIZE - skew_index; i++) {
            f_sawtooth_skew[j][i] = (AMPLITUDE - sskew_delta * (i - skew_index));
        }
        for (int i = TABLE_SIZE - skew_index; i < TABLE_SIZE; i++) {
            f_sawtooth_skew[j][i] = (-AMPLITUDE + skew_delta * (i - (TABLE_SIZE - skew_index)));
        }
        for (int i = 0; i < TABLE_SIZE; i++) {
            printf("%d,f_sawtooth_skew_%d,blue,%f\n", i, j, f_sawtooth_skew[j][i]);
        }
    }


    for (int i = 0; i < TABLE_SIZE; i++) {
        //sine wave
        f_sine[i] = (float)(AMPLITUDE * sin(2.0 * M_PI * (1.0 / TABLE_SIZE) * i));
        sine[i] = (int)f_sine[i];

        //square wave and sawtooth
        if (i < TABLE_SIZE / 2) {
            f_square[i] = AMPLITUDE;
            square[i] = (int)AMPLITUDE;
            //f_sawtooth[i] = (sdelta * i);
            //sawtooth[i] = (int)f_sawtooth[i];
        }
        else {
            f_square[i] = (-AMPLITUDE);
            square[i] = (int)-AMPLITUDE;
            //f_sawtooth[i] = (-AMPLITUDE * 2 + sdelta * i);
            //sawtooth[i] = (int)f_sawtooth[i];
        }

        //triangle wave
        //if (i < TABLE_SIZE / 4) {
        //    f_triangle[i] = (delta * i);
        //    triangle[i] = (int)f_triangle[i];
        //}
        //else if (i < (TABLE_SIZE / 2)) {
        //    f_triangle[i] = (AMPLITUDE - delta * i);
        //    triangle[i] = (int)f_triangle[i];
        //}
        //else if (i < (TABLE_SIZE / 4) * 3) {
        //    f_triangle[i] = (AMPLITUDE * 2 - delta * i );
        //    triangle[i] = (int)f_triangle[i];
        //}
        //else {
        //    f_triangle[i] = (-AMPLITUDE * 4 + delta * i);
        //    triangle[i] = (int)f_triangle[i];
        //}

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

    //float tmp;
    //int i = 0;
    //for (; skew_delta * i < AMPLITUDE; i++) {
    //    f_sawtooth_skew[i] = (skew_delta * i);
    //    f_sawtooth_skew[TABLE_SIZE - i] = -(skew_delta * i);
    //}
    //tmp = f_sawtooth_skew[i];
    //for (int i = 0; i < TABLE_SIZE; i++) {
    //    printf("%d,f_sawtooth_skew,9,%f\n", i, f_sawtooth_skew[i]);
    //}
}