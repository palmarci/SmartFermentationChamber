#ifndef UTILS
#define UTILS
#include <Arduino.h>

float get_temp_bosch();
float get_temp_dallas();
void do_blink(int time_delay);
float get_humidity();
void startup_logic();
void halt(String reason);
void log(String text, int level);
void setup_logic();
String get_status_text();
void do_logic_step();
void reboot(String reason);
String bool_to_str(bool val);
//void alert(String text, bool do_reboot=false);

// used by the ui
// extern float target_temp;
// extern float target_humidity;
// extern bool heater_enabled;
// extern bool humidifier_enabled;
// extern bool automatic_mode;
// extern int target_temp_min;
// extern int target_temp_max;
// extern int hum_min;
// extern int hum_max;


#endif /* UTILS */
