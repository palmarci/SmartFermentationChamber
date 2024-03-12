#include <Arduino.h>
#include <EEPROM.h>
#include <ESPUI.h>
#include <WiFi.h>
#include <ESPmDNS.h>

//#include "logic.h"
#include "config.h"



// callbacks
void set_wifi_details_callback(Control *sender, int type)
{
	if (type != B_UP)
	{
		return;
	}
	log("Saving credentials to EPROM...");
	log(ESPUI.getControl(wifi_ssid_text)->value);
	log(ESPUI.getControl(wifi_pass_text)->value);
	unsigned int i;
	EEPROM.begin(100);
	for (i = 0; i < ESPUI.getControl(wifi_ssid_text)->value.length(); i++)
	{
		EEPROM.write(i, ESPUI.getControl(wifi_ssid_text)->value.charAt(i));
		if (i == 30)
			break;
	}
	EEPROM.write(i, '\0');

	for (i = 0; i < ESPUI.getControl(wifi_pass_text)->value.length(); i++)
	{
		EEPROM.write(i + 32, ESPUI.getControl(wifi_pass_text)->value.charAt(i));
		if (i == 94)
			break;
	}
	EEPROM.write(i + 32, '\0');
	EEPROM.end();
}

void general_callback(Control *sender, int type)
{
#ifdef DEBUG_LOG_CALLBACK
	String debug = "[callback] id=" + String(sender->id) + " type=" + String(type) + " label=" + String(sender->label) + " ->" + String(sender->value);
	log(debug);
#endif

	if (sender->id == humidifier_control)
	{
		humidifier_enabled = sender->value == "1";
		log("[ui callback] set humidifier_enabled to " + String(humidifier_enabled));
	}

	if (sender->id == heater_control)
	{
		heater_enabled = sender->value == "1";
		log("[ui callback] set heater_enabled to " + String(heater_enabled));
	}
	// TODO target temp & hum range validation in server side
	if (sender->id == target_hum_control)
	{
		log("[ui callback] setting target_humidity to " + String(sender->value));
		target_humidity = sender->value.toFloat();
	}

	if (sender->id == target_temp_control)
	{
		log("[ui callback] setting target_temp to " + String(sender->value));
		target_temp = sender->value.toFloat();
	}

	if (sender->id == automatic_mode_control)
	{
		automatic_mode = sender->value == "1";
		log("[ui callback] set automatic_mode to " + String(automatic_mode));
		ESPUI.setEnabled(humidifier_control, !automatic_mode);
		ESPUI.setEnabled(heater_control, !automatic_mode);
	}

	if (sender->id == reboot_control) {
		log("[ui callback] reboot called");
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

	// start the ui
#ifdef UI_USE_FS
	log("[setupui] starting to prepare fs");
	ESPUI.prepareFileSystem();
	log("[setupui] prepareFileSystem finished");
	ESPUI.list();
#endif

	ESPUI.begin(HOSTNAME);
}

void read_string_from_EEPROM(String &buf, int baseaddress, int size)
{
	buf.reserve(size);
	for (int i = baseaddress; i < baseaddress + size; i++)
	{
		char c = EEPROM.read(i);
		buf += c;
		if (!c)
			break;
	}
}

void wifi_connect()
{
	int connect_timeout;

	WiFi.setHostname(HOSTNAME.c_str());

	log("[wifi] starting...");

	if (!FORCE_USE_HOTSPOT)
	{
		yield();
		EEPROM.begin(100);
		String stored_ssid, stored_pass;
		read_string_from_EEPROM(stored_ssid, 0, 32);
		read_string_from_EEPROM(stored_pass, 32, 96);
		EEPROM.end();

		log("[wifi] got stored ssid=" + stored_ssid);
		log("[wifi] got stored password=" + stored_pass);
		WiFi.begin(stored_ssid.c_str(), stored_pass.c_str());

		connect_timeout = 28;
		log("[wifi] trying to connect to stored...");
		while (WiFi.status() != WL_CONNECTED && connect_timeout > 0)
		{
			delay(250);
			Serial.print(".");
			connect_timeout--;
		}
	}

	if (WiFi.status() == WL_CONNECTED)
	{
		log(WiFi.localIP().toString());
		log("[wifi] connected!");

		if (!MDNS.begin(HOSTNAME))
		{
			log("[wifi] error setting up MDNS responder!");
		}
	}
	else
	{
		log("[wifi] failed, creating access point...");
		WiFi.mode(WIFI_AP);
		IPAddress ip;
		bool ip_parse_success = ip.fromString(ACCESS_POINT_IP);
		WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0));
		WiFi.softAP(HOSTNAME);

		connect_timeout = 20;
		do
		{
			delay(250);
			Serial.print(".");
			connect_timeout--;
		} while (connect_timeout);
	}
}

void loop()
{
	static long unsigned last_update = 0;

	if (millis() > last_update + UPDATE_DELTA)
	{
		do_logic_step();
		ESPUI.updateLabel(status_control, get_status_text());
		ESPUI.updateSwitcher(heater_control, heater_enabled);
		ESPUI.updateSwitcher(humidifier_control, humidifier_enabled);
		ESPUI.updateSwitcher(automatic_mode_control, automatic_mode);
		//todo target temp & humidity update

		last_update = millis();
	}
	/*
	if (Serial.available())
	{
		switch (Serial.read())
		{
		case 'I':
			log(WiFi.localIP().toString());
			break;
		case 'W':
			wifi_connect();
			break;
		default:
			Serial.print('#');
			break;
		}
	}
	*/
}

//TODO enter & save token + chat id in wifi tab + rename wifi tab

void setup()
{
	Serial.begin(115200);
	while (!Serial);

	setup_logic();

	wifi_connect();
	WiFi.setSleep(false);
	setUpUI();
}
