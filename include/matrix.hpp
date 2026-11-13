namespace Matrix
{
    constexpr uint8_t DATA = 2; // data is loaded on clocks rising edge
    constexpr uint8_t LOAD = 7; // data is latched on loads rising edge
    constexpr uint8_t CLOCK = 8; // max of 10Mhz
    // define MAX7219 register addresses (for convenience)
    constexpr uint8_t MAX7219_MODE_NOP = 0b00000000;
    constexpr uint8_t MAX7219_MODE_DECODE = 0b00001001;
    constexpr uint8_t MAX7219_MODE_INTENSITY = 0b00001010;
    constexpr uint8_t MAX7219_MODE_SCANLIMIT = 0b00001011;
    constexpr uint8_t MAX7219_MODE_SHUTDOWN = 0b00001100;
    constexpr uint8_t MAX7219_MODE_TEST = 0b00001111;

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
        std::vector<uint16_t> screen = {
            0b0000000000000000,
            0b0000000000000000,
            0b0000000000000000,
            0b0000000000000000,
            0b0000000000000000,
            0b0000000000000000,
            0b0000000000000000,
            0b0000000000000000,
            0b0000000000000000,
            0b0000000000000000,
            0b0000000000000000,
            0b0000000000000000,
            0b0000000000000000,
            0b0000000000000000,
            0b0000000000000000,
            0b0000000000000000,
        };
        uint8_t dirty_row = 0b11111111;

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

        // rightmost bit refers to row 0 and 8
        void set_dirty_bits(int8_t row, bool dirty = true) {
            if (row > 15) return;
            if (row > 7) row -= 8;
            if (dirty)
                this->dirty_row |= (1 << row);
            else
                this->dirty_row ^= (1 << row);
        }

    public:
        Matrix() = default;

        void update() {
            for (int i = 0; i < 8; i++) {
                // only update dirty rows
                if ((this->dirty_row >> i) & 1) {
                    const int i2 = i + 8; // double
                    write2(8 - i, screen[i2] & 0xFF);
                    write2(8 - i, screen[i2] >> 8);
                    write2(8 - i, screen[i] & 0xFF);
                    write2(8 - i, screen[i] >> 8);
                    latch_load();
                    // mark as clean
                    set_dirty_bits(i, false);
                }
            }
        }

        int8_t get_pixel(uint8_t x, uint8_t y) {
            if (x > 15 || y > 15) return -1;
            return (this->screen[y] & (1 << (15 - x))) != 0;
        }

        void set_pixel(uint8_t x, uint8_t y, bool on) {
            if (x > 15 || y > 15) return;

            const uint16_t mask = 1 << (15 - x);
            if (on) {
                this->screen[y] |= mask;
            } else {
                this->screen[y] &= ~mask;
            }
            this->set_dirty_bits(y);
        }

        void clear() {
            for (int i = 0; i < 16; i++) {
                this->screen[i] = 0;
            }
            this->dirty_row = 0b11111111;
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

        void draw_rect(int8_t x1, int8_t y1, int8_t x2, int8_t y2, bool on = true, bool fill = false) {
            for (int y = y1; y <= y2; y++) {
                if (y < 0 || y > 15) continue;
                uint16_t row = this->screen[y];

                for (int x = x1; x <= x2; x++) {
                    if (x < 0 || x > 15) continue;
                    if (!fill && (x != x1 && x != x2 && y != y1 && y != y2)) continue;

                    const uint16_t mask = (1 << (15 - x));
                    if (on)
                        row |= mask;
                    else
                        row &= ~mask;
                }
                this->screen[y] = row;
                this->set_dirty_bits(y);
            }
        }

        void printBinary(unsigned long value, int bits) {
            for (int i = bits - 1; i >= 0; i--) {
                Serial.print(bitRead(value, i));
            }
        }

        void draw_map(int8_t x0, int8_t y0, uint8_t w, uint8_t h, std::vector<uint16_t> map, bool invert = false) {
            if (w > 16 || h > 16) return;

            uint8_t i = 0;
            const int8_t x_shift = 16 - (x0 + w);
            for (int y = y0; y < y0 + h; y++) {
                if (y < 0 || y > 15) {
                    i++;
                    continue;
                }
                uint16_t row = this->screen[y];
                uint16_t mask = 0;
                if (x_shift > 0) {
                    mask = (map[i] << x_shift) & 0xFFFF;
                } else if (x_shift < 0) {
                    mask = (map[i] >> abs(x_shift));
                } else {
                    mask = map[i];
                }
                if (invert) {
                    row &= ~mask;
                } else {
                    row |= mask;
                }

                this->screen[y] = row;
                this->set_dirty_bits(y);
                i++;
            }
        }
    };
}