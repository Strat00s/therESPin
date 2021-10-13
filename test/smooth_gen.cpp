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
    float e_delta, e_pos, e_freq = frequency;
    float delta, o_pos, old_sample = 0;
    float s_pos;
    for (int i = 0; i < samples; i++) {
        if (i -  old_sample >= split) {
            old_sample = i;
            frequency += jump;
        }

        e_freq += 0.05 * (frequency - e_freq);
        e_delta = (e_freq * TABLE_SIZE) / SAMPLE_RATE;
        delta = (frequency * TABLE_SIZE) / SAMPLE_RATE;
        e_pos += e_delta;   //(int)(i * e_delta) % TABLE_SIZE;  
        s_pos += delta;//(int)(i * delta) % TABLE_SIZE;    //skip
        o_pos = (int)(i * delta) % TABLE_SIZE;
        float e_val = 100 * sin(2.0 * M_PI * (1.0 / TABLE_SIZE) * e_pos);
        float s_val = 100 * sin(2.0 * M_PI * (1.0 / TABLE_SIZE) * s_pos);
        float o_val = 100 * sin(2.0 * M_PI * (1.0 / TABLE_SIZE) * o_pos);
        //cout << i << " | " << frequency << "Hz | Pos: "   << pos   << " | val: "   << val << endl;
        //cout << i << " | " << e_freq    << "Hz | e_pos: " << e_pos << " | e_val: " << e_val << endl;
        file << i << ",old,red," << o_val << endl;
        file << i << ",new_smoothed,blue," << s_val << endl;
        file << i << ",new_extraS,green," << e_val << endl;
    }
    file.close();
}