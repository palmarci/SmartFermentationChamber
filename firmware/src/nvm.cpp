#include <Preferences.h>

#include "nvm.h"
#include "config.h"
#include "utils.h"

#include "nvs_flash.h"
#include "nvs.h"

Preferences prefs;

String nvm_read_string(String name)
{
	String res = prefs.getString(name.c_str(), "");
	logprint("reading nvm: " + name + "=" + res);
	return res;
}

bool nvm_begin()
{
	return prefs.begin(String(NVM_KEY).c_str(), false);
}

void nvm_init()
{
	logprint("*** nvm_init ***");
	auto nvm_status = nvm_begin();
	
	if (!nvm_status) {
		logprint("nvm_begin() = " + String(nvm_status));
		reboot("nvm failure");
	}

	if (!nvm_validate_stored_config())
	{
		nvm_set_defaults();
	}
}

bool nvm_clear()
{
	return prefs.clear();
}

void nvm_write_string(String name, String data)
{
	logprint("writing nvm: key=" + name + ", data=" + data);
	prefs.putString(name.c_str(), data);
}

bool nvm_validate_stored_config() {
    const char* keys[] = {
        NVM_WIFI_SSID, NVM_WIFI_PW, NVM_MQTT_IP, NVM_MQTT_PORT,
        NVM_TARGET_TEMP, NVM_TARGET_HUM
    };
    const int num_keys = sizeof(keys) / sizeof(keys[0]);

    for (int i = 0; i < num_keys; i++) {
        if (nvm_read_string(keys[i]).isEmpty()) {
            logprint("Invalid NVM config found!");
            return false;
        }
    }
    
    logprint("NVM config is valid");
    return true;
}

void nvm_set_defaults()
{
//	bool b_stat = nvm_begin();
	bool c_stat = nvm_clear();
	//nvm_begin();

	if (!c_stat) {
		logprint("nvm clearing FAILED");
		reboot("nvm failure");
	}

	nvm_write_string(NVM_WIFI_PW, WIFI_DEFAULT_PW);
	nvm_write_string(NVM_WIFI_SSID, WIFI_DEFAULT_SSID);
	nvm_write_string(NVM_MQTT_IP, MQTT_DEFAULT_IP);
	nvm_write_string(NVM_MQTT_PORT, String(MQTT_DEFAULT_PORT));
	nvm_write_string(NVM_TARGET_TEMP, String(DEFAULT_TARGET_TEMP));
	nvm_write_string(NVM_TARGET_HUM, String(DEFAULT_TARGET_HUMIDITY_REL));

	logprint("wrote default config");
}