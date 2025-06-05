#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

#include "v_common.h"
#include "v_ipc.h"

#define FIFO_PATH "/tmp/ipc_fifo"

bool send_ipc(uint8_t *data, uint8_t size)
{
    bool ret = false;
    int fd = open(FIFO_PATH, O_WRONLY);
    ERR_RET(fd < 0, "no fifo");

    write(fd, data, size);

    close(fd);
    ret = true;
error_return:
    return ret;
}