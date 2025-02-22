#ifndef NETWORK
#define NETWORK

#include <Arduino.h>

void mqtt_init();
bool mqtt_connected();
void mqtt_connect(String ip_string, int port);
void mqtt_send(String topic, String msg);

void wifi_create_ap();
void wifi_init();
bool wifi_is_in_ap_mode();
bool wifi_is_connected();
void wifi_connect(String ssid, String pw);

#endif /* NETWORK */
