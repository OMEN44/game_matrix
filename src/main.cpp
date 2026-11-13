#include <Arduino.h>
#include <vector>
#include <string>

#include "counter.hpp"
#include "matrix.hpp"

Matrix::Matrix matrix;

void setup() {
    pinMode(25, OUTPUT);
    Serial.begin(115200);
    Counter::setup();
    Matrix::setup();
    Matrix::set_brightness(0);
}

int v = 0;
long last_update = millis();
int x = 0;
int y = 0;
int x_dir = 0;
int y_dir = 0;

bool on = true;

void loop() {
    Counter::setValue(8008);

    matrix.clear();

    // Serial.printf("%d, %d\n", x, y);
    matrix.draw_line(8, 8, x, y, true);
    matrix.draw_rect(-2, -3, 4, 6, true, true);
    // matrix.draw_rect(0, 0, 10, 15, true);
    matrix.draw_rect(2, 3, 3, 5, false, true);
    matrix.draw_line(1, -3, 15, 15);
    matrix.set_pixel(0, 1, on);
    matrix.set_pixel(1, 0, on);

    matrix.draw_map(v - 5, v - 2, 10, 4, {
        0b0011001100,
        0b0000000000,
        0b0100000010,
        0b0011111100
        }, true);


    // matrix.draw_map(10, 8, 4, 10, {
    //     0b1000,
    //     0b0100,
    //     0b0010,
    //     0b0001,
    //     0b0010,
    //     0b0100,
    //     0b1000,
    //     0b0100,
    //     0b0010,
    //     0b0001,
    //     });

    matrix.update();
    if (millis() - last_update > 100) {

        if (x == 15 && y == 15) {
            x_dir = -1;
            y_dir = 0;
        } else if (x == 0 && y == 15) {
            x_dir = 0;
            y_dir = -1;
        } else if (x == 15 && y == 0) {
            x_dir = 0;
            y_dir = 1;
        } else if (x == 0 && y == 0) {
            x_dir = 1;
            y_dir = 0;
        }

        if (x == 0 && y == 1)
            on = !on;

        x += x_dir;
        y += y_dir;


        v++;
        if (v > 19) {
            v = 0;
        }
        last_update = millis();
    }
}