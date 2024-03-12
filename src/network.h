#ifndef NETWORK
#define NETWORK

#include <Arduino.h>

void mqtt_init();
bool mqtt_connected();
void mqtt_connect(String ip_string, int port);
void mqtt_send(String topic, String msg);
bool wifi_connected();
void wifi_connect(String ssid, String pw);
void wifi_create_ap();
void wifi_init();

#endif /* NETWORK */
