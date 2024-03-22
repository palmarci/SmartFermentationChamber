#ifndef CONTROL
#define CONTROL

void set_heater(bool state);
void set_humidifer(bool state);
bool get_heater_state();
bool get_humidifer_state();
void control_init();
void set_autopilot(bool state);
bool get_autopilot_state();
float get_target_temp();
float get_target_hum();
void set_target_hum(float val); 
void set_target_temp(float val);
void autopilot_logic();

extern bool humidifer_helper_do;

#endif /* CONTROL */