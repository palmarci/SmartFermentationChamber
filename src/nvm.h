#ifndef NVM
#define NVM

#include <Preferences.h>

Preferences nvm_preferences;

String nvm_read_string(String name);
String nvm_write_string(String name, String data);
void nvm_init();
void nvm_clear();
bool nvm_validate_stored_config();
void nvm_set_defaults();

#endif /* NVM */
