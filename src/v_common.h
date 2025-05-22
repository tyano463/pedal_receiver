#ifndef __V_COMMON_H__
#define __V_COMMON_H__

#include <string.h>
#include <stdint.h>

#define APP_NAME "pdf_viewer_lvgl"
#define WIDTH 768
#define HEIGHT 1024

#ifndef min
#define min(a, b) (((b) < (a)) ? (b) : (a))
#define max(a, b) (((b) > (a)) ? (b) : (a))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

#ifndef TAILQ_FOREACH_SAFE
#define TAILQ_FOREACH_SAFE(var, head, field, tvar)                       \
    for ((var) = TAILQ_FIRST((head)), (tvar) = TAILQ_NEXT((var), field); \
         (var) && ((var) != TAILQ_NEXT((var), field));                   \
         (var) = (tvar), (tvar) = (var ? TAILQ_NEXT((var), field) : NULL))
#endif

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define d(x, ...)                                                                          \
    do                                                                                     \
    {                                                                                      \
        v_log_write("%s(%d) %s " x "\n", __FILENAME__, __LINE__, __func__, ##__VA_ARGS__); \
    } while (0)

#define ERR_RETn(c)            \
    do                         \
    {                          \
        if (c)                 \
            goto error_return; \
    } while (0)

#define ERR_RET(c, s, ...)                                                                                \
    do                                                                                                    \
    {                                                                                                     \
        if (c)                                                                                            \
        {                                                                                                 \
            v_log_write("### ERROR: %s(%d) %s " s "\n", __FILENAME__, __LINE__, __func__, ##__VA_ARGS__); \
            goto error_return;                                                                            \
        }                                                                                                 \
    } while (0)

#define DEFINE_ENUM_WITH_STRINGS(name, LIST)               \
    typedef enum                                           \
    {                                                      \
        LIST(DEFINE_ENUM_ELEMENT)                          \
    } name;                                                \
                                                           \
    static inline const char *name##_to_string(name value) \
    {                                                      \
        switch (value)                                     \
        {                                                  \
            LIST(DEFINE_ENUM_CASE)                         \
        default:                                           \
            return "UNKNOWN";                              \
        }                                                  \
    }

#define DEFINE_ENUM_ELEMENT(e) e,
#define DEFINE_ENUM_CASE(e) \
    case e:                 \
        return #e;

#define MODE_ITEMS(X)     \
    X(V_USER_MODE_NORMAL) \
    X(V_USER_MODE_PEN)    \
    X(V_USER_MODE_SELECT) \
    X(V_USER_MODE_MAX)

DEFINE_ENUM_WITH_STRINGS(v_user_mode_t, MODE_ITEMS);

#define SHOW_MODE_ITEMS(X)         \
    X(V_SHOW_MODE_ANNOT_WITH_MENU) \
    X(V_SHOW_MODE_ANNOT_NO_MENU)   \
    X(V_SHOW_MODE_SCORE_ONLY)      \
    X(V_SHOW_MODE_MAX)

DEFINE_ENUM_WITH_STRINGS(v_show_mode_t, SHOW_MODE_ITEMS);

typedef uint8_t v_annot_display_t;
enum
{
    V_ANNOT_SHOW,
    V_ANNOT_HIDE,
};

typedef uint8_t v_format_t;
enum
{
    V_FORMAT_PDF,
    V_FORMAT_SVG,
    V_FORMAT_PNG,
    V_FORMAT_JPEG,
    V_FORMAT_MUSICXML,
    V_FORMAT_MIDI,
    V_FORMAT_MAX,
};

typedef struct
{
    float sx;
    float sy;
} v_scale_t;

typedef struct
{
    int16_t left;
    int16_t top;
    int16_t right;
    int16_t bottom;
} v_rect_t;

typedef struct
{
    float elm[2][3];
} v_matrix_t;

typedef enum
{
    ST_SUCCESS,
    ST_SETTING_LOAD_FAILED,
    ST_SETTING_SAVE_FAILED,
    ST_SETTING_PARSE_FAILED,
    ST_SETTING_SERIALIZE_FAILED,
    ST_CREATE_CANVAS_FAILED,
    ST_LOG_INIT_FAILED,
    ST_LOG_WRITE_FAILED,
    ST_LOG_ROTATE_FAILED,
    ST_DISPLAY_INIT_FAILED,
    ST_PDF_OPEN_FAILED,
    ST_PDF_CONTEXT_CREAT_FAILED,
    ST_PDF_DOCUMENT_OPEN_FAILED,
    ST_PDF_ANNOTATION_FAILED,
    ST_PNG_OPEN_FAILED,
    ST_MXL_OPEN_FAILED,
    ST_MIDI_OPEN_FAILED,
    ST_SVG_OPEN_FAILED,
    ST_JPEG_OPEN_FAILED,
    ST_MENU_OPEN_FAIL,
    ST_EXT_RECEIVER_INIT_FAILED,
    ST_EXT_NO_RECEIVE,
} v_status_t;

#include "v_log.h"

#endif