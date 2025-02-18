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
float get_target_hum_rel();
void set_target_hum_rel(float val); 
float get_target_hum_abs();
void set_target_hum_abs(float val); 
void set_target_temp(float val);
void autopilot_logic();

//extern bool humidifer_button_fire;

#endif /* CONTROL */