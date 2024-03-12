#ifndef SENSORS
#define SENSORS

float get_humidity();
float get_air_temp();
float get_food_temp();
void sensors_init();
String get_status_text();

#endif /* SENSORS */