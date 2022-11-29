#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#include <Arduino.h>
#include <stdlib.h>
#include "HX711.h"
#include "protothreads.h"

#include "rgb_led.h"
#include "running_numbers.h"
#include "spring_solenoid.h"

const int RED_LIGHT_PIN = 9;
const int GREEN_LIGHT_PIN = 10;
const int BLUE_LIGHT_PIN = 11;

RGBLED led(true, RED_LIGHT_PIN, GREEN_LIGHT_PIN, BLUE_LIGHT_PIN);

const int LOCK_RELEASE_PIN = 7;

SpringSolenoid lock(LOCK_RELEASE_PIN);

HX711 loadcell;

// 1. HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 3;

// 2. Adjustment settings
const long LOADCELL_OFFSET = 50682624;
const long LOADCELL_DIVIDER = 5895655;

const double VARIENCE_EPLSION = 1e-4; // Threshold to determine if weight on scale is not changing.
const float TOLERANCE = 10;

RunningNumbers<float> weights(10);

static bool weighing_completed  = false;

static PT_THREAD(led_gradual_pulse (struct pt *pt)) {
    PT_BEGIN(pt);

    #ifdef DEBUG
    if (!weighing_completed)
        Serial.println("Began fade step");
    #endif

    led.set_color(RGBB{255, 150, 0, 0}); // Orange color

    while (!weighing_completed) {
        #ifdef DEBUG
        Serial.println("LED Step");
        #endif
        led.fading_brightness_step();
        PT_SLEEP(pt, 10);
    }

    led.reset_and_apply();
    PT_END(pt);
}

// Based off from HX711::read_average()
inline long read_average_pt(struct pt *led_pt, byte times = 1U) {
    long sum = 0;
    for (byte i = 0; i < times; i++) {
        #ifdef DEBUG
        Serial.println("Reading loadcell");
        delayMicroseconds(random(10, 5000));
        #endif
        sum += loadcell.read();
        delay(0);
        PT_SCHEDULE(led_gradual_pulse(led_pt));
    }
    return sum / times;
}

static struct pt measure_tare_pt;

static PT_THREAD(measure_tare (struct pt *pt)) {
    static struct pt led_pulse_pt;

    PT_BEGIN(pt);

    weighing_completed = false;

    Serial.println("Calibrating tare weight ...");

    PT_INIT(&led_pulse_pt);
    PT_SCHEDULE(led_gradual_pulse(&led_pulse_pt));

    long avg = read_average_pt(&led_pulse_pt, 100);
    loadcell.set_offset(avg);

    weighing_completed = true;
    PT_WAIT_THREAD(pt, led_gradual_pulse(&led_pulse_pt));

    PT_END(pt);
}

static PT_THREAD(measure_weight (struct pt *pt)) {
    static struct pt led_pulse_pt;

    PT_BEGIN(pt);

    weighing_completed = false;
    PT_INIT(&led_pulse_pt);

    for (size_t i = 0; i < weights.get_max_elements(); i++) {
        weights.add(loadcell.get_units(10));
    }

    while (true) {
        if (weights.get_varience() <= VARIENCE_EPLSION && !weighing_completed) {
        weighing_completed = true;
        PT_WAIT_THREAD(pt, led_gradual_pulse(&led_pulse_pt));

        // TODO: check if weight is within tolerance to 0
        if (abs(weights.get_avg()) <= TOLERANCE) {
            // CORRECT!
            #ifdef DEBUG
            Serial.println("Correct!");
            #endif
            led.set_color(RGBB{0, 255, 0, 100});
            led.apply();
            lock.release();
            delay(30000);
            break;
        } else {
            // INCORRECT!
            #ifdef DEBUG
            Serial.println("Incorrect!");
            #endif
            for (int i = 0; i < 3; i++) {
            led.set_color(RGBB{255, 0, 0, 100});
            led.apply();
            delay(1000);
            led.reset_and_apply();
            delay(1000);
            }
        }
        }

        long avg = read_average_pt(&led_pulse_pt, 10) - loadcell.get_offset();
        weights.add(avg / loadcell.get_scale());

        if (weights.get_varience() > VARIENCE_EPLSION) {
        weighing_completed = false;
        }
    }

    led.reset_and_apply();
    PT_END(pt);
}

void setup() {
    Serial.begin(9600);
    PT_INIT(&measure_tare_pt);

    loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    loadcell.set_scale(LOADCELL_DIVIDER);

    // Change LED to indicate the scale is powered on
    led.set_color(RGBB{255, 255, 255, 100}); // turn LED white
    led.apply();
    delay(1000);

    // pulse LED blue in 1-second cycles for 3 seconds
    constexpr int PRE_TARE_SECS = 3;
    for (int i = 0; i < PRE_TARE_SECS; i++) {
        led.set_color(RGBB{0, 0, 255, 100});
        led.apply();
        delay(500);
        led.set_brightness(0);
        led.apply();
    }

    Serial.println("Calibrating ...");

    // Call the protothread routine to measure the tare weight
    // This should also make the LED gradually pulse orange every 1 second or so
    PT_SCHEDULE(measure_tare(&measure_tare_pt));

    // Set the LED color to green to signal it is done calibraing
    led.set_color(RGBB{0, 255, 0, 100});
    led.apply();
    Serial.println("Done calibrating!");
    delay(5000); // Wait 5 seconds to allow for weights to be taken off the scale
    led.reset_and_apply();
    Serial.println("Ready and actively waiting for correct weights!");
    // Once LED is off, the scale is operational
}

void loop() {
    static struct pt measure_pt;
    PT_INIT(&measure_pt);
    PT_SCHEDULE(measure_weight(&measure_pt));
    exit(0);
}
