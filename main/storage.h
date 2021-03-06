#ifndef STORAGE_H_
#define STORAGE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum { NONE, MAIN, TEST } network_type_t;

#define BLE_ENABLED 0x1

bool storage_init();
bool storage_erase();

bool storage_set_pin_privatekey(const unsigned char* privatekey, size_t key_len);
bool storage_get_pin_privatekey(unsigned char* privatekey, size_t key_len);
bool storage_erase_pin_privatekey();

bool storage_set_encrypted_blob(const unsigned char* encrypted, size_t encrypted_len);
bool storage_get_encrypted_blob(unsigned char* encrypted, size_t encrypted_len);
bool storage_decrement_counter();
bool storage_restore_counter();
uint8_t storage_get_counter();
bool storage_erase_encrypted_blob();

bool storage_set_network_type_restriction(network_type_t networktype);
network_type_t storage_get_network_type_restriction();

bool storage_set_idle_timeout(uint16_t timeout);
uint16_t storage_get_idle_timeout();

bool storage_set_click_event(uint8_t event);
uint8_t storage_get_click_event();

bool storage_set_ble_flags(uint8_t flags);
uint8_t storage_get_ble_flags();

#endif /* STORAGE_H_ */
