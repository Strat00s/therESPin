#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define TABLE_SIZE 1024
#define AMPLITUDE 100

int main() {
    int sine[TABLE_SIZE] = {0};
    int square[TABLE_SIZE] = {0};
    int triangle[TABLE_SIZE] = {0};

    //generate sine
    printf("Sine:\n");
    for (int i = 0; i < TABLE_SIZE; i++) {
        sine[i] = (int)(AMPLITUDE * sin(2.0 * M_PI * (1.0 / TABLE_SIZE) * i));
        printf("%d,", sine[i]);
    }
    //generate square
    printf("\nSquare:\n");
    for (int i = 0; i < TABLE_SIZE; i++) {
        if (i < TABLE_SIZE / 2) {
            square[i] = AMPLITUDE;
        }
        else {
            square[i] = -AMPLITUDE;
        }
        printf("%d,", square[i]);
    }

    //generate triangle
    printf("\nTirangle:\n");
    float delta = (float)AMPLITUDE / (TABLE_SIZE / 2.0);
    for (int i = 0; i < TABLE_SIZE; i++) {
        //if (i < TABLE_SIZE / 2) {
        //    triangle[i] = (int)((-AMPLITUDE / 4) + delta * i);
        //}
        //else {
        //    triangle[i] = (int)(((AMPLITUDE / 4) - delta * i) + AMPLITUDE);
        //}
        if (i < TABLE_SIZE / 4) {
            triangle[i] = delta * i;
        }
        else if (i < TABLE_SIZE / 2) {
            triangle[i] = (int)AMPLITUDE - delta * i;
        }
        else if (i < (TABLE_SIZE / 4) * 3) {
            triangle[i] = (int)(- delta * i);
        }
        else {
            triangle[i] = -AMPLITUDE + delta * i;
        }
        printf("%d,", triangle[i]);
    }
}