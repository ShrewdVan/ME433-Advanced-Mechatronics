#ifndef MOTOR_h
#define MOTOR_h

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <math.h>
#include <stdlib.h>

// =========================== Define Blocking ========================
#define PWM_RIGHT 16
#define PWM_LEFT 17
#define DIR_LEFT 18
#define DIR_RIGHT 19

// =========================== Global Variable ========================
static int duty_cycle_right = 0, duty_cycle_left = 0;

// =========================== Function Blocking ========================

void pin_init_all();
void wheel_update(bool, bool, int);


#endif