#include <Arduino.h>
#include <vector>
#include <string>

namespace Counter
{
    enum class Digit : uint8_t {
        DIGIT_0 = 22,
        DIGIT_1 = 26,
        DIGIT_2 = 27,
        DIGIT_3 = 28,
    };

    enum class Seg : uint8_t {
        A = 13,
        B = 17, // pico doesnt have this pin, pin 17 is unused
        C = 6,
        D = 10,
        E = 11,
        F = 12,
        G = 5,
        DP = 9
    };

    enum class Char : uint8_t {
        ZERO = 0,
        ONE,
        TWO,
        THREE,
        FOUR,
        FIVE,
        SIX,
        SEVEN,
        EIGHT,
        NINE,
        A,
        b,
        C,
        d,
        E,
        F,
        DASH,
        UNDER_SCORE,
        SPACE
    };

    constexpr uint8_t SEGMENTS_PER_DIGIT = 7;
    constexpr uint8_t DIGITS_COUNT = 4;
    constexpr uint8_t LAST_SEG_INDEX = SEGMENTS_PER_DIGIT - 1;
    static const std::vector<Seg> segment_pin_map = {
        Seg::A, Seg::B, Seg::C, Seg::D, Seg::E, Seg::F, Seg::G, Seg::DP
    };
    static const std::vector<Digit> digit_pin_map = {
        Digit::DIGIT_0, Digit::DIGIT_1, Digit::DIGIT_2, Digit::DIGIT_3
    };
    static const std::vector<uint8_t> character_map = {
        //abcdefg
        0b1111110, // 0
        0b0000110, // 1
        0b1101101, // 2
        0b1111001, // 3
        0b0110011, // 4
        0b1011011, // 5
        0b1011111, // 6
        0b1110000, // 7
        0b1111111, // 8
        0b1110011, // 9
        0b0000000, // blank
        0b0000001, // -
    };

    void setup() {
        // Set all segment pins as OUTPUT
        for (const auto& segment : segment_pin_map) {
            pinMode(static_cast<uint8_t>(segment), OUTPUT);
        }

        for (const auto& digit : digit_pin_map) {
            pinMode(static_cast<uint8_t>(digit), OUTPUT);
        }
    }

    void setDigit(uint8_t digit, uint8_t number, bool decimalPoint = false) {
        if (number >= character_map.size() || digit >= DIGITS_COUNT) return;
        // disable output immediatly
        for (int i = 0; i < DIGITS_COUNT; i++) {
            digitalWrite(static_cast<uint8_t>(digit_pin_map[i]), LOW);
        }
        // write to each segment
        for (int i = LAST_SEG_INDEX; i >= 0; i--) {
            uint8_t on = (character_map[number] >> i) & 1;
            digitalWrite(static_cast<uint8_t>(segment_pin_map[LAST_SEG_INDEX - i]), on ? LOW : HIGH);
        }
        // update dp
        digitalWrite(static_cast<uint8_t>(Seg::DP), decimalPoint ? LOW : HIGH);

        // enable display
        digitalWrite(static_cast<uint8_t>(digit_pin_map[digit]), HIGH);
        delayMicroseconds(500);
    }

    void clear() {
        for (const auto& segment : segment_pin_map) {
            digitalWrite(static_cast<uint8_t>(segment), HIGH); // Turn off all segments
        }
    }

    void setValue(int16_t value) {
        if (value < -999 || value > 9999) {
            // Out of range, clear the display
            clear();
            return;
        }

        // convert to a string for easy iteration
        const std::string str = std::to_string(value);
        const uint8_t size = str.size();

        for (int i = 0; i < size; i++) {
            if (str[i] != '-') {
                // align to the right
                setDigit(i + (DIGITS_COUNT - size), str[i] - '0');
            } else {
                setDigit(i + (DIGITS_COUNT - size), 11);
            }
        }
    }
}