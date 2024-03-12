#include <PubSubClient.h>
#include <WiFiClient.h>
#include <WiFi.h>

#include "network.h"
#include "config.h"
#include "nvm.h"
#include "utils.h"

WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

void mqtt_init()
{
	String stored_ip = nvm_read_string(NVM_MQTT_IP);
	String stored_port = nvm_read_string(NVM_MQTT_PORT);
	mqtt_connect(stored_ip, stored_port.toInt());
}

bool mqtt_connected()
{
	bool state = mqtt_client.connected();
	// cant use logprint here due to infinite loop
	//String text = "mqtt currently connected?" + String(state); 
	//Serial.println(text);
	return state;
}

void mqtt_connect(String ip_string, int port)
{
	IPAddress ip;
	if (!ip.fromString(ip_string))
	{
		logprint("could not parse ip address from nvm: " + ip_string, LOG_WARNING);
		return;
	}
	logprint("connecting to mqtt...");
	mqtt_client.setServer(ip, port);
	mqtt_client.connect(HOSTNAME);
}

void mqtt_send(String topic, String msg)
{
	if (!mqtt_connected)
	{
		mqtt_init();
	}

	if (!mqtt_connected)
	{
		logprint("failed to connect to mqtt!", LOG_WARNING);
		return;
	}
	topic.replace("/", "");
	topic = String(HOSTNAME) + "/" + topic;
	mqtt_client.publish(topic.c_str(), msg.c_str());
}

bool wifi_connected()
{
	// TODO whats the status in AP mode?
	bool status = (WiFi.status() == WL_CONNECTED);
	logprint("wifi is connected? " + String(status));
	return status;
}

void wifi_connect(String ssid, String pw)
{
	int connect_timeout = WIFI_CONNECT_TIMEOUT;
	logprint("using wifi credentials: " + ssid + " - " + pw);
	WiFi.begin(ssid.c_str(), pw.c_str());
	while (!wifi_connected() && connect_timeout > 0)
	{
		delay(1000);
		logprint("connecting to wifi...");
		connect_timeout--;
	}
}

void wifi_create_ap()
{
	logprint("creating wifi in AP mode...");
	WiFi.mode(WIFI_AP);
	IPAddress ip;
	ip.fromString(WIFI_AP_DEFAULT_IP);
	WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0));
	WiFi.softAP(String(HOSTNAME).c_str());
}

void wifi_init()
{
	WiFi.setHostname(String(HOSTNAME).c_str());
	if (!WIFI_AP_MODE_FORCE)
	{
		String ssid = nvm_read_string(NVM_WIFI_SSID);
		String pw = nvm_read_string(NVM_WIFI_PW);
		wifi_connect(ssid, pw);
		if (wifi_connected())
		{
			return;
		}
	}
	logprint("failed to connect to wifi!", LOG_WARNING);
	wifi_create_ap();
	WiFi.setSleep(false);
}