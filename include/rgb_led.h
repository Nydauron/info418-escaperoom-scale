#pragma once

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char brightness;
} RGBB;

constexpr unsigned char MAX_BRIGHTNESS = 100;

/**
 * RGB driver to control brightness and color.
 *
 * Will be updated in cycles and will can follow a function.
 *
 * Will be used in proto-threads to allow for simutaneous processing.
*/
class RGBLED {
    private:
        const bool IS_ANODE_LED;
        const int RED_LIGHT_PIN;
        const int GREEN_LIGHT_PIN;
        const int BLUE_LIGHT_PIN;

        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char brightness; // [0-100] and is multiplied to the corresponding (r,g,b) values

        unsigned long cycle;

        inline unsigned char brightness_correction(unsigned char);
    public:
        RGBLED(bool is_anode, int red_pin, int green_pin, int blue_pin);

        // Resets the color and brightness of the LED to 0
        void reset();

        // Resets the color and brightness and updates the LED
        void reset_and_apply();

        // Updates the color and brightness
        void set_color(RGBB color);

        // Updates only the brightness
        void set_brightness(unsigned char brightness);

        // Changes the brightness based off a sinusoidal graph. Updates the LED after each step.
        // Each step is equivalent to a "frame" on what is displayed on the LED
        void fading_brightness_step(unsigned long steps);

        // The update function to update the LED based off the current color and brightness values
        void apply();
};
