#ifndef __V_LOG_H__
#define __V_LOG_H__

#include "v_common.h"

#ifndef MAX_PATH
#define MAX_PATH 512
#endif

#define V_LOG_MAX 9 /*!< only 1 digit */
//#define V_LOG_DIR "/var/log/"
#define V_LOG_DIR "/tmp"
#define V_LOG_FILE "receiver.log"
#define V_LOG_SIZE (1024 * 1024) /*!< 1MB */

v_status_t v_log_init(void);
v_status_t v_log_write(const char *s, ...);
char *dump(const uint8_t *data, uint8_t size);

#endif
