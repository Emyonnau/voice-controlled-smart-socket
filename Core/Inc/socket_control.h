#ifndef SOCKET_CONTROL_H
#define SOCKET_CONTROL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void socket_control_init(void);
void toggle_socket(uint8_t state);
uint8_t socket_control_get_state(void);

#ifdef __cplusplus
}
#endif

#endif