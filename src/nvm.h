#ifndef NVM
#define NVM

String nvm_read_string(String name);
void nvm_write_string(String name, String data);
void nvm_init();
bool nvm_validate_stored_config();
void nvm_set_defaults();

#endif /* NVM */
