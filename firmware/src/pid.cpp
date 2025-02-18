#include <Arduino.h>
#include "utils.h"
#include "pid.h"

// tuning
static float kp = 0;
static float ki = 0;
static float kd = 0;
static float integral_limit_max = 10.f;
static float integral_limit_min = -10.f;

// state variables
static float integral = 0.0f;
static float prev_error = 0.0f;

bool pid_control(float setpoint, float temperature, float dt)
{
	if (kp == 0 || ki == 0 | kd == 0) {
		logprint("PID values were not initialized!", LOG_PANIC);
		return false;
	}

	float ki_internal = kp / ki;
	float kd_internal = kp / kd;

	float error = setpoint - temperature;
	float P = kp * error;
	integral += ki_internal * error * dt;

	if (integral > integral_limit_max)
		integral = integral_limit_max;
	if (integral < integral_limit_min)
		integral = integral_limit_min;

	float derivative = (error - prev_error) / dt;
	float D = kd_internal * derivative;

	float control_signal = P + integral + D;
	bool toret = (control_signal > 0) ? true : false;
	logprint(String("PID output = ") + String(control_signal) + String("; P=") + String(P) + String(", I=") + String(integral) + String(", D=") + String(D), LOG_DEBUG);

	prev_error = error;
	return toret;
}

void pid_init(float newkp, float newki, float newkd)
{
	kp = newkp;
	ki = newki;
	kd = newkd;
	integral = 0.0f;
	logprint("pid values set!");
	// TODO: reset prev_error too?
}

float pid_get_kp() {
	return kp;
}

float pid_get_ki() {
	return ki;
}

float pid_get_kd() {
	return kd;
}