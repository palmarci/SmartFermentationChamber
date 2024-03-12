#ifndef MQTT
#define MQTT

#include <PubSubClient.h>
#include <WiFiClient.h>

WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

void mqtt_init();
bool mqtt_is_connected();
void mqtt_connect();
void mqtt_send(String topic, String msg);

#endif /* MQTT */
