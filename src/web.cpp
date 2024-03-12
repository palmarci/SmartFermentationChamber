#include <Arduino.h>
#include <ESPUI.h>

#include "config.h"
#include "web.h"
#include "utils.h"
#include "control.h"
#include "sensors.h"

uint16_t humidifier_control;
uint16_t heater_control;
uint16_t target_temp_control;
uint16_t status_control;
uint16_t target_hum_control;
uint16_t autopilot_control;
uint16_t reboot_control;
uint16_t wifi_ssid_control;
uint16_t wifi_pass_control;
uint16_t ferm_tab;
uint16_t settings_tab;

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

	if (sender->id == target_hum_control)
	{
		// TODO target temp & hum range validation in server side
		float new_hum = sender->value.toFloat();
		set_target_hum(new_hum);
	}

	if (sender->id == target_temp_control)
	{
		float new_temp = sender->value.toFloat();
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

	web_update();
}

void web_update()
{
	ESPUI.setEnabled(humidifier_control, !get_autopilot_state());
	ESPUI.setEnabled(heater_control, !get_autopilot_state());
	ESPUI.updateLabel(status_control, get_sensor_status_text());
	ESPUI.updateSwitcher(heater_control, get_heater_state());
	ESPUI.updateSwitcher(humidifier_control, get_humidifer_state());
}

// TODO save everything in nvm
void web_init()
{
	ESPUI.setVerbosity(Verbosity::Quiet);
	ESPUI.sliderContinuous = true;

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
	wifi_ssid_control = ESPUI.addControl(Text, "SSID", "", Alizarin, settings_tab, general_callback);
	reboot_control = ESPUI.addControl(Button, "Restart", "Restart", Peterriver, settings_tab, general_callback);
	wifi_pass_control = ESPUI.addControl(Text, "Password", "", Alizarin, settings_tab, general_callback);

	//TODO order
	//TODO add mqtt settings
	ESPUI.addControl(Max, "", "64", None, wifi_pass_control);
	ESPUI.addControl(Button, "Save Wifi", "Save Wifi", Peterriver, settings_tab, general_callback);
	ESPUI.addControl(Max, "", "32", None, wifi_ssid_control);
	ESPUI.addControl(Label, "Note", "Please press enter in the textboxes before clicking on save!", Emerald, settings_tab, general_callback);

	ESPUI.begin(HOSTNAME); // TODO i cant seem to set this dynamically (or at least with numbers in it)

	web_update();
}