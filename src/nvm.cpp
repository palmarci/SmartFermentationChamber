#include <Preferences.h>

#include "nvm.h"
#include "config.h"
#include "utils.h"

Preferences prefs;

String nvm_read_string(String name) {
	prefs.getString(name.c_str(), "");
}

void nvm_init() {
	prefs.begin(String(HOSTNAME).c_str(), false);
	if (!nvm_validate_stored_config()) {
		nvm_set_defaults();
	}
}

String nvm_write_string(String name, String data) {
	prefs.putString(name.c_str(), data);
}

bool nvm_validate_stored_config() {
	String stored_ssid = nvm_read_string(NVM_WIFI_SSID);
	String stored_pw = nvm_read_string(NVM_WIFI_PW);
	String stored_ip = nvm_read_string(NVM_MQTT_IP);
	String stored_port = nvm_read_string(NVM_MQTT_PORT);
	
	if (stored_ssid == "" || stored_pw == "" || \
		stored_ip == "" || stored_port == "") {
			logprint("cannot read config from nvm");
			return false;

		} 
	return true;
}

void nvm_set_defaults() {
	prefs.clear();
	nvm_write_string(String(NVM_WIFI_PW).c_str(), String(WIFI_DEFAULT_PW).c_str());
	nvm_write_string(String(NVM_WIFI_SSID).c_str(), String(WIFI_DEFAULT_SSID).c_str());
	nvm_write_string(String(NVM_MQTT_IP).c_str(), String(MQTT_DEFAULT_IP).c_str());
	nvm_write_string(String(NVM_MQTT_PORT).c_str(), String(MQTT_DEFAULT_PORT).c_str());
}