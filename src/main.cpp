#include <Arduino.h>
#include <stdlib.h>
#include "HX711.h"

#include "running_numbers.h"

HX711 loadcell;

// 1. HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 3;

// 2. Adjustment settings
const long LOADCELL_OFFSET = 50682624;
const long LOADCELL_DIVIDER = 5895655;

const int EXPECTED_WEIGHT = 1000; // TODO: Some total TBD (weight combos must sum up to a distinct value)
const int TOLERANCE = 10;

void setup() {
  loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  loadcell.set_scale(LOADCELL_DIVIDER);
  loadcell.set_offset(LOADCELL_OFFSET);
}

void loop() {
  if (!loadcell.is_ready()) {
    delay(50);
    return;
  }

  RunningNumbers<float> weights(5);

  const int CONSECUTIVE_CHECKS_TO_PASS = 10;
  int passed_checks = 0;
  // need to check if weight on scale has changed (if yes, need to blink LED as yellow slowly)
  while (passed_checks != CONSECUTIVE_CHECKS_TO_PASS) {
    delay(200);
    weights.add(loadcell.get_units(10));

    if (weights.is_within_tolerance(EXPECTED_WEIGHT, TOLERANCE)) {
      passed_checks++;
    } else {
      passed_checks = 0;
    }
  }

  loadcell.power_down();
  // Fire spring solenoid release (note must fire for less than 100ms)
  // Turn LED green

  delay(30000);
  exit(0);
}
