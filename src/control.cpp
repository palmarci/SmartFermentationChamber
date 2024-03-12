#include <Arduino.h>
#include "config.h"

void set_heater(bool state) {
    digitalWrite(RELAY_PIN_HEATER, state);

}

void set_humidifer(bool state) {
    digitalWrite(RELAY_PIN_HUMIDIFIER, state);
}