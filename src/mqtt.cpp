#include "mqtt.h"
#include "config.h"
#include "nvm.h"
#include "utils.h"

void mqtt_init() {
    String stored_ip = nvm_read_string(NVM_MQTT_IP);
    String stored_port = nvm_read_string(NVM_MQTT_PORT);
    IPAddress ip;

    if (!ip.fromString(stored_ip)) {
        halt("could not parse ip address from nvm: " + stored_ip);
    }

   	mqtt_client.setServer(ip, stored_port.toInt());
}

bool mqtt_is_connected() {
    bool state = mqtt_client.connected();
    log("mqtt is connected? " + String(false));
    return state;
}

void mqtt_connect() {
    log("connecting to mqtt...");
    mqtt_client.connect(HOSTNAME);
}

void mqtt_send(String topic, String msg) {
	if (!mqtt_is_connected)
	{
		mqtt_connect();
	} 

    if (!mqtt_is_connected)
	{
		log("failed to connect to mqtt!", LOG_WARNING);
        return;
	} 

	log("[mqtt] sending data: " + topic + " -> " + msg);
	mqtt_client.publish(topic.c_str(), msg.c_str());
}