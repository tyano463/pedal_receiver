#ifndef __V_IPC_H__
#define __V_IPC_H__

#include <stdint.h>
#include <stdbool.h>

bool send_ipc(const uint8_t *data, uint8_t size);

#endif