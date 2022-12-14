#include "rgb_led.h"

#include <Arduino.h>
#include <math.h>

inline unsigned char anode_led_value_fix(unsigned char val, bool is_anode) {
    return (is_anode) ? (255 - val) : val;
}

RGBLED::RGBLED(bool is_anode, int red_pin, int green_pin, int blue_pin) :
        IS_ANODE_LED(is_anode), RED_LIGHT_PIN(red_pin), GREEN_LIGHT_PIN(green_pin),
        BLUE_LIGHT_PIN(blue_pin) {
    pinMode(RED_LIGHT_PIN, OUTPUT);
    pinMode(GREEN_LIGHT_PIN, OUTPUT);
    pinMode(BLUE_LIGHT_PIN, OUTPUT);
}

inline unsigned char RGBLED::brightness_correction(unsigned char val) {
    return val * ((double) brightness / MAX_BRIGHTNESS);
}

void RGBLED::reset() {
    r = 0;
    g = 0;
    b = 0;
    brightness = MAX_BRIGHTNESS;
}

void RGBLED::reset_and_apply() {
    reset();
    apply();
}

void RGBLED::set_color(RGBB color) {
    r = color.rgb.r;
    g = color.rgb.g;
    b = color.rgb.b;
    brightness = min(color.brightness, MAX_BRIGHTNESS);
}

void RGBLED::set_brightness(unsigned char brightness) {
    this->brightness = min(brightness, MAX_BRIGHTNESS);
}

void RGBLED::apply() {
    analogWrite(RED_LIGHT_PIN, anode_led_value_fix(brightness_correction(r), IS_ANODE_LED));
    analogWrite(GREEN_LIGHT_PIN, anode_led_value_fix(brightness_correction(g), IS_ANODE_LED));
    analogWrite(BLUE_LIGHT_PIN, anode_led_value_fix(brightness_correction(b), IS_ANODE_LED));
}

void RGBLED::fading_brightness_step(unsigned long step_num) {
    constexpr long PERIOD_LEN = 1000; // 1ms per step, so period = a second
    constexpr long YOFFSET = 10;
    brightness = min(max(0, (MAX_BRIGHTNESS + YOFFSET) * abs(sin((double)(step_num) * M_PI / PERIOD_LEN)) - YOFFSET), MAX_BRIGHTNESS);
    apply();
}
