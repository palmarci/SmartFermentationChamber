#include "network.h"
#include "ota.h"
#include "utils.h"

#include <WiFi.h>
#include <ArduinoOTA.h>

static IPAddress startup_ip;
static bool ota_running = false;


void ota_stop()
{
	ArduinoOTA.end();
	ota_running = false;
	startup_ip = IPAddress().fromString("0.0.0.0");
	logprint("ota stopped!");
}

void ota_init()
{
	if (!OTA_ENABLED) {
		return;
	}
	logprint("*** ota_init ***");
	ota_stop();

	if (wifi_is_connected())
	{
		startup_ip = WiFi.localIP();

		ArduinoOTA.begin(startup_ip, OTA_USER, OTA_PW, InternalStorage);

	//	ArduinoOTA.setPort(8266);
	//	ArduinoOTA.setHostname(HOSTNAME);
		ota_running = true;
	}
}


void ota_task(void *parameter)
{

	int delay = 2 * 1000;
	while (true)
	{
		vTaskDelay(pdMS_TO_TICKS(delay));
		if (wifi_is_connected())
		{
			auto curr_ip = WiFi.localIP();

			if (curr_ip == startup_ip)
			{
				if (ota_running) {
					ArduinoOTA.handle();
				}
			}
			else
			{
				ota_init();
			}
		}
		else
		{
			ota_init();
		}
	}
}