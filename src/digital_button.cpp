#include "digital_button.h"
#include <Arduino.h>

DigitalButton::DigitalButton(int pin_in) : pin(pin_in) {
    pinMode(pin, INPUT);
}

void DigitalButton::wait_til(bool signal) {
    while (digitalRead(pin) != signal) {}
}
