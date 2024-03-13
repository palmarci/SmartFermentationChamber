#ifndef SENSORS
#define SENSORS

float get_humidity();
float get_air_temp();
float get_food_temp();
void sensors_init();
String get_sensor_status_text();
bool validate_hum_range(float in);
bool validate_temp_range(float input);

#endif /* SENSORS */