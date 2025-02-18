#ifndef UTILS
#define UTILS

#include <Arduino.h>
#include "config.h"

void logprint(String text, int level = LOG_DEBUG);
String bool_to_str(bool val);
void halt(String reason, int delay_between, int delay_after);
void reboot(String error_message);
String remove_leading_slash(String input);
void do_blink(int delay_between, int delay_after);

#endif /* UTILS */