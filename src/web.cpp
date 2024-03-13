#include <Arduino.h>
#include <ESPUI.h>

#include "config.h"
#include "web.h"
#include "utils.h"
#include "control.h"
#include "sensors.h"
#include "nvm.h"
#include "network.h"

uint16_t humidifier_control;
uint16_t heater_control;
uint16_t target_temp_control;
uint16_t status_control;
uint16_t target_hum_control;
uint16_t autopilot_control;
uint16_t reboot_control;
uint16_t wifi_ssid_control;
uint16_t wifi_pw_control;
uint16_t ferm_tab;
uint16_t settings_tab;
uint16_t mqtt_address_control;
uint16_t mqtt_port_control;
uint16_t save_config_control;

String mqtt_address_buffer = "";
String mqtt_port_buffer = "";
String wifi_ssid_buffer = "";
String wifi_pw_buffer = "";

void general_callback(Control *sender, int type)
{
	String debug = "CALLBACK: id=" + String(sender->id) + " type=" + String(type) + " label=" +
				   String(sender->label) + " -> " + String(sender->value);
	logprint(debug);

	if (sender->id == humidifier_control)
	{
		bool new_state = sender->value == "1";
		set_humidifer(new_state);
	}

	if (sender->id == heater_control)
	{
		bool new_state = sender->value == "1";
		set_heater(new_state);
	}
	// TODO better user input handling (also for mqtt & wifi settings)
	if (sender->id == target_hum_control)
	{
		float new_hum = sender->value.toFloat();
		if (!validate_hum_range(new_hum))
		{
			reboot("out of range humity was read from user: " + String(new_hum));
		}
		set_target_hum(new_hum);
	}

	if (sender->id == target_temp_control)
	{
		float new_temp = sender->value.toFloat();
		if (!validate_temp_range(new_temp))
		{
			reboot("out of range temperature was read from user: " + String(new_temp));
		}
		set_target_temp(new_temp);
	}

	if (sender->id == autopilot_control)
	{
		bool new_state = sender->value == "1";
		set_autopilot(new_state);
	}

	if (sender->id == reboot_control)
	{
		reboot("reboot called from web ui");
	}

	if (sender->id == mqtt_address_control || sender->id == mqtt_port_control ||
		sender->id == wifi_ssid_control || sender->id == wifi_pw_control)
	{

		mqtt_address_buffer = ESPUI.getControl(mqtt_address_control)->value;
		mqtt_port_buffer = ESPUI.getControl(mqtt_port_control)->value;
		wifi_ssid_buffer = ESPUI.getControl(wifi_ssid_control)->value;
		wifi_pw_buffer = ESPUI.getControl(wifi_pw_control)->value;
	}

	if (sender->id == save_config_control)
	{
		nvm_write_string(NVM_MQTT_IP, mqtt_address_buffer);
		nvm_write_string(NVM_MQTT_PORT, mqtt_port_buffer);
		nvm_write_string(NVM_WIFI_SSID, wifi_ssid_buffer);
		nvm_write_string(NVM_WIFI_PW, wifi_pw_buffer);
		nvm_write_string(NVM_TARGET_TEMP, String(get_target_temp()));
		nvm_write_string(NVM_TARGET_HUM, String(get_target_hum()));
		wifi_init();
		mqtt_init();
	}

	web_update();
}

void web_update()
{
	ESPUI.setEnabled(humidifier_control, !get_autopilot_state());
	ESPUI.setEnabled(heater_control, !get_autopilot_state());
	ESPUI.updateLabel(status_control, get_sensor_status_text());
	ESPUI.updateSwitcher(heater_control, get_heater_state());
	ESPUI.updateSwitcher(humidifier_control, get_humidifer_state());
	ESPUI.updateNumber(target_temp_control, get_target_temp());
	ESPUI.updateNumber(target_hum_control, get_target_hum());
	ESPUI.updateSwitcher(autopilot_control, get_autopilot_state());
}

void web_init()
{
	ESPUI.setVerbosity(Verbosity::Quiet);
	ESPUI.sliderContinuous = true;

	wifi_ssid_buffer = nvm_read_string(NVM_WIFI_SSID);
	wifi_pw_buffer = nvm_read_string(NVM_WIFI_PW);
	mqtt_address_buffer = nvm_read_string(NVM_MQTT_IP);
	mqtt_port_buffer = nvm_read_string(NVM_MQTT_PORT);

	// fermentation tab
	ferm_tab = ESPUI.addControl(Tab, "ferm_tab", "Fermentation settings");
	autopilot_control = ESPUI.addControl(Switcher, "Autopilot mode", bool_to_str(get_autopilot_state()), Alizarin, ferm_tab, general_callback);
	heater_control = ESPUI.addControl(Switcher, "Heater", bool_to_str(get_heater_state()), Alizarin, ferm_tab, general_callback);
	humidifier_control = ESPUI.addControl(Switcher, "Humidifier", bool_to_str(get_humidifer_state()), Alizarin, ferm_tab, general_callback);
	status_control = ESPUI.addControl(Label, "Status", "...", Sunflower, ferm_tab, general_callback);
	target_temp_control = ESPUI.addControl(Number, "Target temperature (in C)", String(get_target_temp()), Emerald, ferm_tab, general_callback);
	target_hum_control = ESPUI.addControl(Number, "Target humidity (in %)", String(get_target_hum()), Emerald, ferm_tab, general_callback);
	ESPUI.addControl(Min, "", String(INVALID_MINIMUM_TEMP), None, target_temp_control);
	ESPUI.addControl(Max, "", String(INVALID_MAX_TEMP), None, target_temp_control);
	ESPUI.addControl(Min, "", String(0), None, target_hum_control);
	ESPUI.addControl(Max, "", String(100), None, target_hum_control);

	// config tab
	settings_tab = ESPUI.addControl(Tab, "settings_tab", "Settings");
	wifi_ssid_control = ESPUI.addControl(Text, "Wifi SSID", wifi_ssid_buffer, Alizarin, settings_tab, general_callback);
	wifi_pw_control = ESPUI.addControl(Text, "Wifi Password", wifi_pw_buffer, Alizarin, settings_tab, general_callback);
	mqtt_address_control = ESPUI.addControl(Text, "MQTT IP", mqtt_address_buffer, Alizarin, settings_tab, general_callback);
	mqtt_port_control = ESPUI.addControl(Number, "MQTT Port", mqtt_port_buffer, Alizarin, settings_tab, general_callback);
	reboot_control = ESPUI.addControl(Button, "Restart", "Restart", Peterriver, settings_tab, general_callback);
	ESPUI.addControl(Label, "Version", VERSION, Emerald, settings_tab, general_callback);
	save_config_control = ESPUI.addControl(Button, "Save", "Save", Peterriver, settings_tab, general_callback);

	ESPUI.begin(HOSTNAME);
	web_update();
}