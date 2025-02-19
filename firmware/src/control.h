#ifndef CONTROL
#define CONTROL

void set_heater(bool state, bool force_mode);
void set_humidifer(bool state);
bool get_heater_state();
bool get_humidifer_state();
void control_init();
void set_autopilot(bool state);
bool get_autopilot_state();
float get_target_temp();
float get_target_hum();
void set_target_hum_abs(float val); 
void set_target_temp(float val);
void autopilot_logic();
int get_heater_duty_cycle();
int get_humidifier_duty_cycle();

#endif /* CONTROL */