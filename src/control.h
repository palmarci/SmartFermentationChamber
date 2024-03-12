#ifndef CONTROL
#define CONTROL

void set_heater(bool state);
void set_humidifer(bool state);
bool get_heater_state();
bool get_humidifer_state();
void control_init();
void set_autopilot(bool state);
bool get_autopilot_state();

#endif /* CONTROL */