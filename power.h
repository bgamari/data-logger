#pragma once
#include <mchck.h>

extern volatile bool low_power_mode;

void enter_low_power_mode(void);
void exit_low_power_mode(void);

void power_notify_activity(void);

void power_init(void);
