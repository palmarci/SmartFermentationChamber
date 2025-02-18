#include "utils.h"
#include "control.h"
#include "nvm.h"
#include "network.h"
#include "sensors.h"
#include "web.h"
#include "config.h"
#include "tasks.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include <LittleFS.h>

void disable_brownout()
{
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
}

void pin_init() {
	logprint("*** pin_init ***");
	pinMode(LED_PIN, OUTPUT);
	pinMode(PIN_HEATER, OUTPUT);
	pinMode(PIN_HUMIDIFIER, OUTPUT);
	set_heater(false);
	set_humidifer(false);
}

void serial_init() {
	logprint("*** serial_init ***");
	Serial.begin(SERIAL_SPEED);
}

void setup()
{
	if (DISABLE_BROWNOUT)
	{
		disable_brownout();
	}
	serial_init();
	pin_init();
	nvm_init();
	control_init();
	wifi_init();
	mqtt_init();
	sensors_init();
	web_init();
	tasks_init();
	set_autopilot(AUTOPILOT_ENABLED_AT_STARTUP); //only do this after everything is alive
}

void loop()
{
	// Empty, because we're using FreeRTOS tasks
}