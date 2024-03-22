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

void disable_brownout()
{
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
}

void setup()
{
	if (DISABLE_BROWNOUT)
	{
		disable_brownout();
	}
	Serial.begin(115200);
	nvm_init();
	control_init();
	wifi_init();
	mqtt_init();
	sensors_init();
	web_init();
	tasks_init();

}

void loop()
{
	// Empty, because we're using FreeRTOS tasks
}