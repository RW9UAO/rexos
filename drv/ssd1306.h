#ifndef MT_H
#define MT_H

#include <stdbool.h>
#include <stdint.h>
#include "../userspace/process.h"
#include "../userspace/sys.h"
#include "../userspace/canvas.h"
#include "../userspace/graphics.h"
#include "oled_config.h"
#include "m051_gpio.h"

/* System 5x8 */
#define font_u8Width 5
#define font_u8Height 8
#define font_u8FirstChar 32
#define font_u8LastChar 128
extern const unsigned char au8FontSystem5x8[];


#define MT_MODE_IGNORE                     0x0
#define MT_MODE_OR                         0x1
#define MT_MODE_XOR                        0x2
#define MT_MODE_AND                        0x3
#define MT_MODE_FILL                       0x4

#if (MT_DRIVER)
typedef enum {
    MT_RESET = HAL_IPC(HAL_POWER),
    MT_SHOW,
    MT_BACKLIGHT,
    MT_CLS,
    MT_CLEAR_RECT,
    MT_READ_CANVAS,
    MT_WRITE_RECT,
    MT_WRITE_CANVAS,
    MT_PIXEL_TEST
} MT_IPCS;

extern const REX __OLED;

typedef struct {
    RECT rect;
    HANDLE block;
} MT_REQUEST;

#else

void oled_set_backlight(bool on);
void oled_show(bool on);
bool oled_is_on();
void oled_cls();
void oled_reset();
void oled_init();
#if (MT_TEST)
void oled_clks_test();
void oled_pixel_test();
#endif
void oled_clear_rect(RECT* rect);
void oled_write_rect(RECT* rect, const uint8_t *data);
void oled_read_canvas(CANVAS* canvas, POINT* point);
void oled_write_canvas(CANVAS* canvas, POINT* point);
void ssd1306DrawString(unsigned char x, unsigned char y, char* text);
void ssd1306DrawChar(unsigned char x, unsigned char y, unsigned char c);


#endif //MT_DRIVER

#endif // MT_H
