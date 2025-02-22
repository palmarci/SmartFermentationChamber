#ifndef CONTROL
#define CONTROL

enum PredictionState {
    GOING_UP,
    GOING_DOWN,
    UNKNOWN
};

void set_heater(bool state, bool force_mode);
void set_humidifier(bool state);
bool get_heater_state();
bool get_humidifer_state();

void control_init();
void duty_cycle_update();

void set_target_hum(float val); 
void set_target_hum_rel(float val); 
float get_target_hum();
float get_target_hum_rel();

void set_target_temp(float val);
float get_target_temp();

void set_autopilot(bool state);
bool get_autopilot_state();
void autopilot_step();

int get_heater_duty_cycle();
int get_humidifier_duty_cycle();

#endif /* CONTROL */