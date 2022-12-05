#pragma once

class DigitalButton {
    private:
        int pin;
    public:
        DigitalButton(int pin_in);

        /**
         * Waits and pins until input to pin changes to signal
         *
         * @param signal is for HIGH or LOW
         */
        void wait_til(bool signal);
};
