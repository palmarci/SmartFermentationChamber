#ifndef UTILS
#define UTILS

#include <Arduino.h>
#include "config.h"

void logprint(String text, int level = LOG_DEBUG);
String bool_to_str(bool val);
void halt(String reason);
void reboot(String error_message);

#endif /* UTILS */