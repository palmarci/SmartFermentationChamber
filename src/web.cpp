#include <Arduino.h>
#include <ESPUI.h>

#include "config.h"
#include "web.h"
#include "utils.h"

void general_callback(Control *sender, int type)
{
	String debug = "[callback] id=" + String(sender->id) + " type=" + String(type) + " label=" + String(sender->label) + " ->" + String(sender->value);
	logprint(debug);

	if (sender->id == humidifier_control)
	{
		humidifier_enabled = sender->value == "1";
		logprint("[ui callback] set humidifier_enabled to " + String(humidifier_enabled));
	}

	if (sender->id == heater_control)
	{
		heater_enabled = sender->value == "1";
		logprint("[ui callback] set heater_enabled to " + String(heater_enabled));
	}
	// TODO target temp & hum range validation in server side
	if (sender->id == target_hum_control)
	{
		logprint("[ui callback] setting target_humidity to " + String(sender->value));
		target_humidity = sender->value.toFloat();
	}

	if (sender->id == target_temp_control)
	{
		logprint("[ui callback] setting target_temp to " + String(sender->value));
		target_temp = sender->value.toFloat();
	}

	if (sender->id == automatic_mode_control)
	{
		automatic_mode = sender->value == "1";
		logprint("[ui callback] set automatic_mode to " + String(automatic_mode));
		ESPUI.setEnabled(humidifier_control, !automatic_mode);
		ESPUI.setEnabled(heater_control, !automatic_mode);
	}

	if (sender->id == reboot_control) {
		logprint("[ui callback] reboot called");
		reboot();
	}

	if (sender->id == test_telegram_msg_control) {
		alert("This is a test message from " + String(HOSTNAME));
		//reboot();
	}
}

// main ui setup
void setUpUI()
{
	ESPUI.setVerbosity(Verbosity::Quiet);
	ESPUI.sliderContinuous = true;

	// settings tab
	auto settings_tab = ESPUI.addControl(Tab, "config", "Configuration");
	automatic_mode_control = ESPUI.addControl(Switcher, "Automatic mode", bool_to_str(automatic_mode), Alizarin, settings_tab, general_callback);
	heater_control = ESPUI.addControl(Switcher, "Heater enabled", bool_to_str(heater_enabled), Alizarin, settings_tab, general_callback);
	humidifier_control = ESPUI.addControl(Switcher, "Humidifier enabled", bool_to_str(humidifier_enabled), Alizarin, settings_tab, general_callback);

	status_control = ESPUI.addControl(Label, "Status", "...", Sunflower, settings_tab, general_callback);
	target_temp_control = ESPUI.addControl(Number, "Target temperature (in C)", String(target_temp), Emerald, settings_tab, general_callback);
	target_hum_control = ESPUI.addControl(Number, "Target humidity (in %)", String(target_humidity), Emerald, settings_tab, general_callback);

	ESPUI.setEnabled(humidifier_control, !automatic_mode);
	ESPUI.setEnabled(heater_control, !automatic_mode);


	//TODO error handling = send alert (telegram?), turn off everything, relay default NC
	ESPUI.addControl(Min, "", String(target_temp_min), None, target_temp_control);
	ESPUI.addControl(Max, "", String(target_temp_max), None, target_temp_control);
	ESPUI.addControl(Min, "", String(hum_min), None, target_hum_control);
	ESPUI.addControl(Max, "", String(hum_max), None, target_hum_control);

	reboot_control = ESPUI.addControl(Button, "Restart", "Restart", Peterriver, settings_tab, general_callback);

	// wifi settings tab
	auto wifitab = ESPUI.addControl(Tab, "", "WiFi Settings");
	ESPUI.addControl(Label, "Note", "Please press enter in the textboxes before clicking on save!", Emerald, wifitab, general_callback);
	wifi_ssid_text = ESPUI.addControl(Text, "SSID", "", Alizarin, wifitab, general_callback);
	ESPUI.addControl(Max, "", "32", None, wifi_ssid_text);
	wifi_pass_text = ESPUI.addControl(Text, "Password", "", Alizarin, wifitab, general_callback);
	ESPUI.addControl(Max, "", "64", None, wifi_pass_text);
	ESPUI.addControl(Button, "Save Wifi", "Save Wifi", Peterriver, wifitab, set_wifi_details_callback);
	test_telegram_msg_control = ESPUI.addControl(Button, "Test Message", "Send Test Message", Peterriver, wifitab, general_callback);

	String display_name = String(HOSTNAME) + "v" + String(VERSION);
	ESPUI.begin(display_name.c_str());
}

