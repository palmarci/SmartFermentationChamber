#ifndef SENSORS
#define SENSORS

void sensors_init();
void measure_sensors();

bool validate_temp_range(float input);
bool validate_hum_range(float input);

float relative_to_abs_humidity(float temp, float hum_percentage);
float abs_to_relative_humidity(float temp, float absH);

String get_sensor_status_text();

extern float last_hum;
extern float last_food_temp;
extern float last_air_temp;

#endif /* SENSORS */