#ifndef WIRE_H_
#define WIRE_H_

#include "process.h"
#include <stdint.h>

void handle_data(uint8_t* full_data_in, size_t* read_ptr, uint8_t* data_out, jade_msg_source_t source);

#endif /* WIRE_H_ */
