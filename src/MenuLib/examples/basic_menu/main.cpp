#include <Arduino.h>
#include <ESP32Encoder.h>
#include <iostream>
#include <Wire.h>
#include "menu.hpp"

using namespace std;

int item2_data = 1;
int item13_data = 69;
int item3121_data = -8;
int item3122_data = 1;
int item3123_data = 3;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
ESP32Encoder encoder;

Entry *customFnc(int *position, bool *select, Entry *entry) {
    cout << "inside custom function" << endl;
    u8g2.clearBuffer();
    u8g2.drawStr(20, 40, "Test");
    if (*select) {
        *select = false;
        entry->resetToParent(position);
        return entry->parent();
    }
    return entry;
}

Entry *selected;

int value1 = 0;
int value2 = 0;

hw_timer_t *timer = nullptr;
int interrupt_counter = 0;

Menu menu(&u8g2);


void setup(void) {
    u8g2.begin(); 
    u8g2.setFont(u8g2_font_6x10_mf);   //font dimensions: 11x11
    u8g2.clearBuffer();

    encoder.attachSingleEdge(36, 39);
    encoder.setFilter(1023);
    encoder.clearCount();

    pinMode(23, INPUT_PULLUP);

    menu.addByName("root", "entry1", "Entry 1");
    menu.addByName("root", "item2", picker, &item2_data, {"Low", "Medium", "High", "V. High"});
    menu.addByName("root", "entry3", "");
    menu.addByName("root", "entry4", "Entry 4");
    menu.addByName("root", "entry5", "Entry 5");
    menu.addByName("root", "entry6", "Entry 6");
    menu.addByName("root", "entry7", "Entry 7");
    menu.addByName("root", "entry8", "Entry 8");
    menu.addByName("root", "entry9", "Entry 9");

    menu.addByName("entry1", "entry11", "Entry 11");
    menu.addByName("entry1", "entry12", "Entry12");
    menu.addByName("entry1", "item13", counter, 0, 100, &item13_data);
    
    menu.addByName("entry3", "entry31", "Entry 31");
    menu.addByName("entry3", "entry32", "Entry 32");
    menu.addByName("entry3", "entry33", "Entry 33");
    menu.addByName("entry3", "entry34", "Entry 34");
    menu.addByName("entry3", "entry35", "Entry 35");
    menu.addByName("entry3", "entry36", "Entry 36");
    menu.addByName("entry3", "entry37", "Entry 37");
    menu.addByName("entry3", "entry38", "Entry 38");
    
    menu.addByName("entry31", "entry311", "Entry 311");
    menu.addByName("entry31", "entry312", "Entry 312");
    menu.addByName("entry38", "entry381", "Entry 381");
    
    menu.addByName("entry37", "item371", 0, 0, &item3121_data, {}, &customFnc);
    menu.addByName("entry37", "item372", toggle, &item3122_data, {"on", "off", "auto"});
    menu.addByName("entry37", "item373", counter, -10, 10, &item3123_data);

    cout << "Begin: " << menu.begin() << endl;
}

int position = 0;
bool select = false;
void loop(void) {
    if (!digitalRead(23)) {
        while(!digitalRead(23));
        select = true;
    }
    position = encoder.getCount();
    menu.render(&position, &select);
    encoder.setCount(position);
}
