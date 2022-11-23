#pragma once

/**
 * I am using a 12v cabinet spring lock to release the container with the token:
 * https://shop.escaperoomtechs.com/products/cabinet-magnetic-spring-lock-kit
 *
 * As mentioned by their documentation, the solenoid is naturally in a locked state, so without
 * power applied to it, it stays locked. The solenoid should also only be powered on for no more
 * than 100 ms, otherwise there is a risk of it heating up and it getting damaged.
 *
 * The solenoid is hooked up to a circuit as shown here:
 * https://www.dropbox.com/s/wffsfemuwiaew9q/Schematic%20R1.pdf
 */
class SpringSolenoid {
    private:
        int pin_out;
    public:
        SpringSolenoid(int pin_out);

        void release();
};
