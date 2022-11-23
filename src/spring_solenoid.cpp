#include "spring_solenoid.h"

#include <Arduino.h>

SpringSolenoid::SpringSolenoid(int pin_out) : pin_out(pin_out) {}

void SpringSolenoid::release() {
    digitalWrite(pin_out, HIGH);
    delay(100); // The maximum delay the solenoid should be on for is 100ms
    digitalWrite(pin_out, LOW);
}
