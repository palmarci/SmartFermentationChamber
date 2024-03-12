#include "nvm.h"
#include "config.h"
#include "utils.h"

String nvm_read_string(String name) {
	nvm_preferences.getString(name.c_str(), "");
}

void nvm_init() {
	nvm_preferences.begin(HOSTNAME.c_str(), false);
}

void nvm_clear() {
	nvm_preferences.clear();
}

String nvm_write_string(String name, String data) {
	nvm_preferences.putString(name.c_str(), data);
}

bool nvm_validate_stored_config() {
	String stored_ssid = nvm_read_string(NVM_WIFI_SSID);
	String stored_pw = nvm_read_string(NVM_WIFI_PW);
	String stored_ip = nvm_read_string(NVM_MQTT_IP);
	String stored_port = nvm_read_string(NVM_MQTT_PORT);
	
	if (stored_ssid == "" || stored_pw == "" || \
		stored_ip == "" || stored_port == "") {
			log("cannot read config from nvm");
			return false;

		} 
	return true;
}

void nvm_set_defaults() {
	nvm_clear();
	nvm_write_string(String(NVM_WIFI_PW).c_str(), String(WIFI_DEFAULT_PW).c_str());
	nvm_write_string(String(NVM_WIFI_SSID).c_str(), String(WIFI_DEFAULT_SSID).c_str());
	nvm_write_string(String(NVM_MQTT_IP).c_str(), String(MQTT_DEFAULT_IP).c_str());
	nvm_write_string(String(NVM_MQTT_PORT).c_str(), String(MQTT_DEFAULT_PORT).c_str());
}