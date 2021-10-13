#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>

using namespace std;

#define SAMPLES 4096
#define SPLIT_LEN 512
#define FREQUENCY 10
#define JUMP 100

#define TABLE_SIZE 1024
#define SAMPLE_RATE 44100

//check and convert string to number
int StringToNumber(string s) {
    //check if all chars are numbers
    for (int i = 0; i < s.length(); i++) {
        if (!isdigit(s[i]))
            return -1;
    }
    return stoi(s);
}

int main(int argc, char *argv[]) {
    
    int samples = SAMPLES;
    int split = SPLIT_LEN;
    float frequency = FREQUENCY;
    int jump = JUMP;
    if (argc > 1)
        samples =   StringToNumber(string(argv[1]));
    if (argc > 2)
        split =     StringToNumber(string(argv[2]));
    if (argc > 3)
        frequency = StringToNumber(string(argv[3]));
    if (argc > 4)
        jump =      StringToNumber(string(argv[4]));
    if (samples == -1)   samples   = SAMPLES;
    if (split == -1)     split     = SPLIT_LEN;
    if (frequency == -1) frequency = FREQUENCY;
    if (jump == -1)      jump      = JUMP;
    cout << samples << " | " << split << " | " << frequency << " | " << jump << endl;

    ofstream file;
    file.open("test.txt");
    file << "dT,category,format,val" << endl;
    float delta, s_delta, pos, s_pos, old_sample = 0;
    float s_freq = frequency;
    for (int i = 0; i < samples; i++) {
        if (i -  old_sample >= split) {
            old_sample = i;
            frequency += jump;
        }
        //s_freq += 0.1 * (frequency - s_freq);
        //s_delta = (s_freq * TABLE_SIZE) / SAMPLE_RATE;
        delta = (frequency * TABLE_SIZE) / SAMPLE_RATE;
        //s_pos = (int)(i * s_delta) % TABLE_SIZE;  
        pos += delta;//(int)(i * delta) % TABLE_SIZE;    //skip
        //float s_val = 100 * sin(2.0 * M_PI * (1.0 / TABLE_SIZE) * s_pos);
        float val = 100 * sin(2.0 * M_PI * (1.0 / TABLE_SIZE) * pos);
        cout << i << " | " << frequency << "Hz | Pos: "   << pos   << " | val: "   << val << endl;
        //cout << i << " | " << s_freq    << "Hz | s_pos: " << s_pos << " | s_val: " << s_val << endl;
        file << i << ",normal,red," << val << endl;
        //file << i << ",smoothed,blue," << s_val << endl;
    }
    file.close();
}