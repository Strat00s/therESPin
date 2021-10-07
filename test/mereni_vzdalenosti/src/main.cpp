#include <Wire.h>
#include <VL53L0X.h>

VL53L0X sensor1;
VL53L0X sensor2;

void sensorSetup(int enable_pin, uint8_t address, VL53L0X *sensor) {
    pinMode(enable_pin, OUTPUT);
    digitalWrite(enable_pin, HIGH); //enable sensor on enable_pinn
    sensor->init(true);             //initialize the sensor
    sensor->setAddress(address);    //set new address (required as all sensors have the same default address)
    pinMode(enable_pin, INPUT);
    sensor->setMeasurementTimingBudget(20000);  //change timing for faster refresh
}

void setup() {
    Wire.begin();

    sensorSetup(18, 18, &sensor1);
    sensorSetup(19, 19, &sensor2);
    sensor1.startContinuous();
    sensor2.startContinuous();
}

int old_len1 = 0;
int old_len2 = 0;

void loop() {
    int len1 = sensor1.readRangeContinuousMillimeters();
    int len2 = sensor2.readRangeContinuousMillimeters();
    if (old_len1 != len1) {
        printf("New range(1): %d\n", len1);
        old_len1 = len1;
    }
    if (old_len2 != len2) {
        printf("New range(2): %d\n", len2);
        old_len2 = len2;
    }
}