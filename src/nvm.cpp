#include <Preferences.h>

#include "nvm.h"
#include "config.h"
#include "utils.h"

Preferences prefs;

String nvm_read_string(String name)
{
	return prefs.getString(name.c_str(), "");
}

void nvm_begin()
{
	prefs.begin(String(HOSTNAME).c_str(), false);
}

void nvm_init()
{
	nvm_begin();
	if (!nvm_validate_stored_config())
	{
		nvm_set_defaults();
	}
}

void nvm_write_string(String name, String data)
{
	logprint("writing nvm: key=" + name + ", data=" + data);
	prefs.putString(name.c_str(), data);
}

bool nvm_validate_stored_config()
{
	String stored_ssid = nvm_read_string(NVM_WIFI_SSID);
	String stored_pw = nvm_read_string(NVM_WIFI_PW);
	String stored_ip = nvm_read_string(NVM_MQTT_IP);
	String stored_port = nvm_read_string(NVM_MQTT_PORT);
	String stored_temp = nvm_read_string(NVM_TARGET_TEMP);
	String stored_hum = nvm_read_string(NVM_TARGET_HUM);

	if (stored_ssid == "" || stored_pw == "" ||
		stored_ip == "" || stored_port == "" ||
		stored_hum == "" || stored_hum == "")
	{
		logprint("cannot read config from nvm");
		return false;
	}
	return true;
}

void nvm_set_defaults()
{
	bool success = prefs.clear();
	nvm_begin();
	logprint("nvm clearing ok? " + String(success));
	nvm_write_string(NVM_WIFI_PW, WIFI_DEFAULT_PW);
	nvm_write_string(NVM_WIFI_SSID, WIFI_DEFAULT_SSID);
	nvm_write_string(NVM_MQTT_IP, MQTT_DEFAULT_IP);
	nvm_write_string(NVM_TARGET_TEMP, String(DEFAULT_TARGET_TEMP));
	nvm_write_string(NVM_TARGET_HUM, String(DEFAULT_TARGET_HUMIDITY));
	nvm_write_string(NVM_MQTT_PORT, String(MQTT_DEFAULT_PORT));
	logprint("wrote default config");
}