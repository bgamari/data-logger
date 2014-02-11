#pragma once
#include "sensor.h"

extern struct sensor *sensors[];
void config_pins();

/*
 * This disables entering Very Low-Power Run (VLPR) mode in low-power
 * mode due to erratum e4590.
 * 
 * This erratum appears to apply to all masks up to and including
 * 1N86B.
 * 
 * Really this isn't a terribly big loss as we are in a STOP state
 * most of the time anyways.
 */
#define NO_VLPR
