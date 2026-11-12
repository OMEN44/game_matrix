#include <Arduino.h>
#include <vector>
#include <string>

#include "counter.hpp"

namespace Matrix
{
    constexpr uint8_t DATA = 2; // data is loaded on clocks rising edge
    constexpr uint8_t LOAD = 7; // data is latched on loads rising edge
    constexpr uint8_t CLOCK = 8; // max of 10Mhz

    void latch_load() {
        digitalWriteFast(LOAD, HIGH);
        delayMicroseconds(10);
        digitalWriteFast(LOAD, LOW);
    }

    void clock_pulse() {
        digitalWriteFast(CLOCK, HIGH);
        delayMicroseconds(10);
        digitalWriteFast(CLOCK, LOW);
    }

    // define MAX7219 register addresses (for convenience)
#define MAX7219_MODE_NOP       0b00000000
#define MAX7219_MODE_DECODE    0b00001001
#define MAX7219_MODE_INTENSITY 0b00001010
#define MAX7219_MODE_SCANLIMIT 0b00001011
#define MAX7219_MODE_SHUTDOWN  0b00001100
#define MAX7219_MODE_TEST      0b00001111

    void write(uint8_t address, uint8_t data) {
        // fill internal shift reg MSB first
        for (int i = 7; i >= 0; i--) {
            digitalWriteFast(DATA, (address >> i) & 1);
            clock_pulse();
        }
        for (int i = 7; i >= 0; i--) {
            digitalWriteFast(DATA, (data >> i) & 1);
            clock_pulse();
        }
    }

    // adjust for the mistake in the pcb
    void write2(uint8_t address, uint8_t data) {
        uint8_t b = data;
        // reverse the byte for conveinience
        b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
        b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
        b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
        // 0b11000000 prints as 0b00000011 so rotate by 2
        b = (b >> 2) | (b << 6);
        write(address, b);
    }

    void set_brightness(uint8_t value) {
        if (value > 15) return;

        write(MAX7219_MODE_INTENSITY, value);
        write(MAX7219_MODE_INTENSITY, value);
        write(MAX7219_MODE_INTENSITY, value);
        write(MAX7219_MODE_INTENSITY, value);
        latch_load();
    }

    void setup() {
        pinMode(DATA, OUTPUT);
        pinMode(LOAD, OUTPUT);
        pinMode(CLOCK, OUTPUT);

        digitalWrite(LOAD, LOW);
        digitalWrite(CLOCK, LOW);

        // setup MAX7219
        write(MAX7219_MODE_SCANLIMIT, 0x07); // scan all digits
        write(MAX7219_MODE_SCANLIMIT, 0x07);
        write(MAX7219_MODE_SCANLIMIT, 0x07);
        write(MAX7219_MODE_SCANLIMIT, 0x07);
        latch_load();
        write(MAX7219_MODE_DECODE, 0x00); // no decode
        write(MAX7219_MODE_DECODE, 0x00);
        write(MAX7219_MODE_DECODE, 0x00);
        write(MAX7219_MODE_DECODE, 0x00);
        latch_load();
        write(MAX7219_MODE_INTENSITY, 0x02);
        write(MAX7219_MODE_INTENSITY, 0x02);
        write(MAX7219_MODE_INTENSITY, 0x02);
        write(MAX7219_MODE_INTENSITY, 0x02);
        latch_load();
        write(MAX7219_MODE_SHUTDOWN, 0x01); // normal operation
        write(MAX7219_MODE_SHUTDOWN, 0x01);
        write(MAX7219_MODE_SHUTDOWN, 0x01);
        write(MAX7219_MODE_SHUTDOWN, 0x01);
        latch_load();

        // clear display
        for (int i = 1; i <= 8; i++) {
            write(i, 0x00);
            write(i, 0x00);
            write(i, 0x00);
            write(i, 0x00);
            latch_load();
        }
    }

    class Matrix {
    private:
        std::vector<uint8_t> screen = {
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,

            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
            0b00000000, 0b00000000,
        };
        uint8_t dirty_row = 0b11111111;

    public:
        Matrix() = default;

        void update() {
            for (int i = 0; i < 8; i++) {
                // only update dirty rows
                if ((this->dirty_row >> i) & 1) {
                    const int i2 = i << 1; // double
                    write2(8 - i, screen[i2 + 17]);
                    write2(8 - i, screen[i2 + 16]);
                    write2(8 - i, screen[i2 + 1]);
                    write2(8 - i, screen[i2]);
                    latch_load();
                    // mark as clean
                    this->dirty_row ^= (1 << i);
                }
            }
        }

        int8_t get_pixel(uint8_t col, uint8_t row) {
            if (col > 15 || row > 15) return -1;

            const int i = row << 1; // double
            const int mask = 0b10000000 >> (col % 7);

            return (this->screen[i] & mask) != 0;
        }

        void set_pixel(uint8_t col, uint8_t row, bool on) {
            if (col > 15 || row > 15) return;

            const int i = (row << 1) + (col > 7);
            const int mask = 0b10000000 >> (col % 8);

            if (on) {
                this->screen[i] |= mask;
            } else {
                this->screen[i] &= ~mask;
            }
            this->dirty_row |= 1 << (row % 8);
        }

        void clear() {
            for (int i = 0; i < 16; i++) {
                this->screen[i << 1] = 0;
                this->screen[(i << 1) + 1] = 0;
            }
            this->dirty_row = 0b11111111;
        }

        void draw_line_low(int8_t x1, int8_t y1, int8_t x2, int8_t y2, bool on = true) {
            int8_t y = y1;
            float dx = static_cast<float>(x2 - x1);
            float dy = static_cast<float>(y2 - y1);
            float yi = 1.0;

            if (dy < 0) {
                yi = -1;
                dy = -dy;
            }

            float D = (2 * dy) - dx;
            for (int x = x1; x <= x2; x++) {
                set_pixel(x, y, on);
                if (D > 0) {
                    y = y + yi;
                    D = D + (2 * (dy - dx));
                } else {
                    D = D + 2 * dy;
                }
            }
        }

        void draw_line_high(int8_t x1, int8_t y1, int8_t x2, int8_t y2, bool on = true) {
            int8_t x = x1;
            float dx = static_cast<float>(x2 - x1);
            float dy = static_cast<float>(y2 - y1);
            float xi = 1.0;

            if (dx < 0) {
                xi = -1;
                dx = -dx;
            }

            float D = (2 * dx) - dy;
            for (int y = y1; y <= y2; y++) {
                set_pixel(x, y, on);
                if (D > 0) {
                    x = x + xi;
                    D = D + (2 * (dx - dy));
                } else {
                    D = D + 2 * dx;
                }
            }
        }

        void draw_line(int8_t x1, int8_t y1, int8_t x2, int8_t y2, bool on = true) {

            // fix coordinate order
            static int count = 0;
            if (abs(y2 - y1) < abs(x2 - x1)) {
                if (x1 > x2)
                    draw_line_low(x2, y2, x1, y1, on);
                else draw_line_low(x1, y1, x2, y2, on);
            } else {
                if (y1 > y2)
                    draw_line_high(x2, y2, x1, y1, on);
                else draw_line_high(x1, y1, x2, y2, on);
            }
        }


    };
}

Matrix::Matrix matrix;

void setup() {
    pinMode(25, OUTPUT);
    Serial.begin(115200);
    Counter::setup();
    Matrix::setup();
    Matrix::set_brightness(2);
}

int v = 0;
long last_update = millis();
int x = 0;
int y = 0;
int x_dir = 0;
int y_dir = 0;

bool on = true;

void loop() {
    // matrix.clear();
    // Serial.printf("%d, %d\n", x, y);
    matrix.draw_line(8, 8, x, y, on);

    matrix.update();
    if (millis() - last_update > 25) {

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
        if (v > 31) {
            v = 0;
        }
        last_update = millis();
    }
}