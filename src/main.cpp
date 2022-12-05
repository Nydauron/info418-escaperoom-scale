#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#include <Arduino.h>
#include <stdlib.h>
#include "HX711.h"
#include "protothreads.h"

#include "digital_button.h"
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
constexpr float LOADCELL_MAX_KG = 20; // 20kg
constexpr float LOADCELL_DIVIDER = 2010.0F / LOADCELL_MAX_KG;

const double VARIENCE_EQUILIBRIUM_REACHED = 2e-2; // Threshold to determine if weight on scale is not changing.
const double VARIENCE_CURRENTLY_WEIGHING = 5e-1;
const float TOLERANCE = 1.0F;

const int BUTTON_TARE_PIN = 12;
float expected_weight = 0.0;

DigitalButton button(BUTTON_TARE_PIN);

constexpr RGB RED{255, 0, 0};       // Used when the submitted weight is wrong
constexpr RGB GREEN{0, 255, 0};     // Used when the submitted weight is correct and when the scale is ready after setup
constexpr RGB BLUE{0, 0, 255};      // Used when taring occurs
constexpr RGB ORANGE{255, 100, 0};  // Used when measuring a submitted weight value
constexpr RGB VIOLET{238, 0, 238};  // Used when button is pressed (and turns off when released) (exists to show responsiveness)
constexpr RGB WHITE{255, 255, 255}; // Used for idicating the machine is on, when the machine is waiting for a weight submission, and when the scale is measuring the calibrated weight

RunningNumbers<float> weights(5);

#ifdef DEBUG
RunningNumbers<float> calibration_weights(100);
#endif

static bool weighing_completed  = false;

static PT_THREAD(led_gradual_pulse (struct pt *pt, RGBB color = RGBB{ORANGE, 0})) {
    PT_BEGIN(pt);

    #ifdef DEBUG_FADE
    if (!weighing_completed)
        Serial.println("Starting fade");
    #endif
    led.set_color(color);
    static unsigned long start_time = millis();

    while (!weighing_completed) {
        #ifdef DEBUG_FADE
        Serial.println("Step");
        #endif
        constexpr unsigned long cycles = 10; // in ms
        unsigned long current_time = millis();
        unsigned long current_frame = current_time - start_time;
        led.fading_brightness_step(current_frame);
        static unsigned long time_to_wait = (((millis() + cycles) / cycles) * cycles) - current_time;
        PT_SLEEP(pt, time_to_wait);
    }

    #ifdef DEBUG_FADE
    Serial.println("Fading ended");
    #endif
    led.reset_and_apply();
    PT_END(pt);
}

// Based off from HX711::read_average()
inline long read_average_pt(struct pt *led_pt, unsigned int times = 1U, RGBB color = RGBB{ORANGE, 0}) {
    #ifdef DEBUG
    static int c = 0;
    #endif
    long sum = 0;
    for (unsigned int i = 0; i < times; i++) {
        sum += loadcell.read();
        delay(0);
        PT_SCHEDULE(led_gradual_pulse(led_pt, color));
    }
    #ifdef DEBUG
    if (c % 10 == 0) {
        calibration_weights.add(sum / times);
    }
    c++;
    #endif
    return sum / times;
}

static struct pt measure_tare_pt;

static PT_THREAD(measure_tare (struct pt *pt)) {
    constexpr RGBB BLUE_HALF_BRIGHT = RGBB{BLUE, 50};
    static struct pt led_pulse_pt;

    PT_BEGIN(pt);

    weighing_completed = false;

    Serial.println("Calibrating tare weight ...");

    PT_INIT(&led_pulse_pt);
    PT_SCHEDULE(led_gradual_pulse(&led_pulse_pt, BLUE_HALF_BRIGHT));
    #ifdef DEBUG
    static unsigned long start = millis();
    #endif

    Serial.println("Sampling next 50 units ...");

    long avg = read_average_pt(&led_pulse_pt, 50, BLUE_HALF_BRIGHT);
    loadcell.set_offset(avg);

    #ifdef DEBUG
    static unsigned long end = millis();
    Serial.print(avg);
    Serial.print("\t");
    Serial.println(expected_weight);

    Serial.print("Time: ");
    Serial.println(end - start);
    Serial.print("Average tare: ");
    Serial.println(calibration_weights.get_avg());
    Serial.print("Variation: ");
    Serial.println(calibration_weights.get_varience());
    Serial.print("Tare set to: ");
    Serial.println(loadcell.get_offset());
    #endif
    weighing_completed = true;
    PT_WAIT_THREAD(pt, led_gradual_pulse(&led_pulse_pt, BLUE_HALF_BRIGHT));

    PT_END(pt);
}

static PT_THREAD(measure_weight (struct pt *pt)) {
    static struct pt led_pulse_pt;
    static struct pt tare_pt;

    PT_BEGIN(pt);

    button.wait_til(HIGH);

    led.set_color(RGBB{BLUE, 50});
    led.apply();

    button.wait_til(LOW);

    delay(10);

    loadcell.power_up();

    PT_INIT(&tare_pt);
    PT_SCHEDULE(measure_tare(&tare_pt));

    led.set_color(RGBB{WHITE, 50});
    led.apply();

    button.wait_til(HIGH);

    led.set_color(RGBB{ORANGE, 50});
    led.apply();

    button.wait_til(LOW);

    weighing_completed = false;
    PT_INIT(&led_pulse_pt);

    while (true) {
        long avg = read_average_pt(&led_pulse_pt, 10) - loadcell.get_offset();
        weights.add(avg / loadcell.get_scale());

        #ifdef DEBUG
        Serial.print("Average running weight: ");
        Serial.println(weights.get_avg());
        Serial.print("Weight varience: ");
        Serial.println(weights.get_varience());
        #endif

        if (weights.get_varience() > VARIENCE_CURRENTLY_WEIGHING) {
            weighing_completed = false;
        } else if (weights.get_varience() <= VARIENCE_EQUILIBRIUM_REACHED && !weighing_completed) {
            weighing_completed = true;
            PT_SCHEDULE(led_gradual_pulse(&led_pulse_pt)); // clean up LED runtime
            loadcell.power_down();
            #ifdef DEBUG
            Serial.print("Average weight for check: ");
            Serial.println(weights.get_avg());
            #endif

            if (abs(weights.get_avg() - expected_weight) <= TOLERANCE) {
                // CORRECT!
                #ifdef DEBUG
                Serial.println("Correct!");
                #endif
                led.set_color(RGBB{GREEN, 100});
                led.apply();
                lock.release();
                delay(3000);
                while (true) {
                    // Allow for the button to release the payload once the box has been unlocked once
                    button.wait_til(HIGH);
                    lock.release();
                    button.wait_til(LOW);
                    delay(3000); // Rate limit it to prevent solenoid from burning out :)
                }
            } else {
                // INCORRECT!
                #ifdef DEBUG
                Serial.println("Incorrect!");
                #endif
                for (int i = 0; i < 3; i++) {
                    led.set_color(RGBB{RED, 100});
                    led.apply();
                    delay(1000);
                    led.reset_and_apply();
                    delay(1000);
                }
                PT_EXIT(pt);
            }
        }
    }

    led.reset_and_apply();
    PT_END(pt);
}

void setup() {
    #ifdef DEBUG
    Serial.begin(9600);
    #endif
    PT_INIT(&measure_tare_pt);

    loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    loadcell.set_scale(LOADCELL_DIVIDER);

    // Change LED to indicate the scale is powered on
    led.set_color(RGBB{WHITE, 50}); // turn LED white
    led.apply();
    delay(1000);

    led.set_color(RGBB{BLUE, 50});
    led.apply();

    #ifdef DEBUG
    Serial.println("Waiting for signal to tare ...");
    #endif

    button.wait_til(HIGH);

    led.set_color(RGBB{VIOLET, 50});
    led.apply();

    button.wait_til(LOW); // must release the button

    PT_SCHEDULE(measure_tare(&measure_tare_pt));

    #ifdef DEBUG
    Serial.println("Taring done!");
    #endif

    led.set_color(RGBB{ORANGE, 50});
    led.apply();

    #ifdef DEBUG
    Serial.println("Waiting for signal to measure correct weight value ...");
    #endif

    button.wait_til(HIGH);

    led.set_color(RGBB{VIOLET, 50});
    led.apply();

    button.wait_til(LOW); // must release the button

    #ifdef DEBUG
    Serial.println("Measuring ...");
    #endif

    // Call the protothread routine to measure the tare weight
    // This should also make the LED gradually pulse blue every 1 second or so
    static struct pt led_pt;
    PT_INIT(&led_pt);
    constexpr RGBB WHITE_FADE = RGBB{WHITE, 0};

    weighing_completed = false;
    long weight_raw = read_average_pt(&led_pt, 50, WHITE_FADE);

    weighing_completed = true;
    PT_SCHEDULE(led_gradual_pulse(&led_pt, WHITE_FADE));

    expected_weight = (weight_raw - loadcell.get_offset()) / loadcell.get_scale();
    // Set the LED color to green to signal it is done calibraing
    led.set_color(RGBB{GREEN, 50});
    led.apply();

    #ifdef DEBUG
    Serial.println("Done calibrating!");
    Serial.print("Expected weight:\t");
    Serial.println(expected_weight);
    #endif
    delay(3000); // Wait 3 seconds to allow for weights to be taken off the scale

    // Fill up running average
    for (size_t i = 0; i < weights.get_max_elements(); i++) {
        long avg = loadcell.get_units(10);
        weights.add(avg);
    }
    // Once LED is off, the scale is operational
    led.reset_and_apply();
    loadcell.power_down();
    #ifdef DEBUG
    Serial.println("Ready and actively waiting for correct weights!");
    #endif
}

void loop() {
    static struct pt measure_pt;
    PT_INIT(&measure_pt);
    measure_weight(&measure_pt);
}
