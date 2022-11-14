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
const double VARIENCE_EPLSION = 1e-4; // Threshold to determine if weight on scale is not changing.
const int TOLERANCE = 10;

RunningNumbers<float> weights(10);

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

  // need to actually check if weights have stabilized be within a certain amount of tolerance (light should blink druing this process)
  weights.add(loadcell.get_units(10));
  while (weights.get_varience() > VARIENCE_EPLSION) {
    // need to still add LED blinking code

    delay(200);
    weights.add(loadcell.get_units(10));
  }

  // take mean weight (bc median and mean are already pretty close since var is very tiny)
  if (!weights.is_within_tolerance(EXPECTED_WEIGHT, TOLERANCE)) {
    // WRONG WEIGHT!
    // TODO: Change LED to red, pause then return
    return;
  }

  // CORRECT WEIGHT!
  loadcell.power_down();
  // TODO: Fire spring solenoid release (note must fire for less than 100ms)
  // TODO: Turn LED green

  delay(30000);
  exit(0);
}
